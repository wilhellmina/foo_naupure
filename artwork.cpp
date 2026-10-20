#include "stdafx.h"
#include "artwork.h"
#include "art_server.h"
#include "config.h"

#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>

using Microsoft::WRL::ComPtr;

namespace
{
	// X rejects images larger than this.
	const size_t max_upload_bytes = 5 * 1024 * 1024;
	// The art server closes after this long if nobody asks for the image.
	const unsigned serve_timeout_ms = 30 * 1000;

	struct image
	{
		std::string data;
		std::string mime;
	};

	void log(const char* msg)
	{
		FB2K_console_formatter() << "foo_twitter: " << msg;
	}

	void check(HRESULT hr, const char* what)
	{
		if (FAILED(hr)) {
			char msg[160];
			snprintf(msg, sizeof msg, "%s failed (0x%08lX)", what, (unsigned long)hr);
			throw std::runtime_error(msg);
		}
	}

	bool has_alpha(IWICImagingFactory* factory, IWICBitmapSource* source)
	{
		WICPixelFormatGUID format;
		ComPtr<IWICComponentInfo> info;
		ComPtr<IWICPixelFormatInfo2> format_info;
		BOOL alpha = FALSE;
		if (SUCCEEDED(source->GetPixelFormat(&format)) && SUCCEEDED(factory->CreateComponentInfo(format, &info)) && SUCCEEDED(info.As(&format_info))) {
			format_info->SupportsTransparency(&alpha);
		}
		return alpha != FALSE;
	}

	std::string encode(IWICImagingFactory* factory, IWICBitmapSource* source, bool png)
	{
		const WICPixelFormatGUID format = png ? GUID_WICPixelFormat32bppBGRA : GUID_WICPixelFormat24bppBGR;
		ComPtr<IWICFormatConverter> converted;
		check(factory->CreateFormatConverter(&converted), "IWICImagingFactory::CreateFormatConverter");
		check(converted->Initialize(source, format, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom), "IWICFormatConverter::Initialize");
		UINT width, height;
		check(converted->GetSize(&width, &height), "IWICFormatConverter::GetSize");

		ComPtr<IStream> stream;
		check(CreateStreamOnHGlobal(nullptr, TRUE, &stream), "CreateStreamOnHGlobal");
		ComPtr<IWICBitmapEncoder> encoder;
		check(factory->CreateEncoder(png ? GUID_ContainerFormatPng : GUID_ContainerFormatJpeg, nullptr, &encoder), "IWICImagingFactory::CreateEncoder");
		check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache), "IWICBitmapEncoder::Initialize");

		ComPtr<IWICBitmapFrameEncode> frame;
		ComPtr<IPropertyBag2> options;
		check(encoder->CreateNewFrame(&frame, &options), "IWICBitmapEncoder::CreateNewFrame");
		if (!png) {
			PROPBAG2 option = {};
			option.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
			VARIANT value = {};
			value.vt = VT_R4;
			value.fltVal = 0.9f;
			check(options->Write(1, &option, &value), "IPropertyBag2::Write");
		}
		check(frame->Initialize(options.Get()), "IWICBitmapFrameEncode::Initialize");
		check(frame->SetSize(width, height), "IWICBitmapFrameEncode::SetSize");
		WICPixelFormatGUID accepted = format;
		check(frame->SetPixelFormat(&accepted), "IWICBitmapFrameEncode::SetPixelFormat");
		if (accepted != format) throw std::runtime_error("the encoder does not accept the pixel format");
		check(frame->WriteSource(converted.Get(), nullptr), "IWICBitmapFrameEncode::WriteSource");
		check(frame->Commit(), "IWICBitmapFrameEncode::Commit");
		check(encoder->Commit(), "IWICBitmapEncoder::Commit");

		STATSTG stat = {};
		check(stream->Stat(&stat, STATFLAG_NONAME), "IStream::Stat");
		std::string out((size_t)stat.cbSize.QuadPart, '\0');
		LARGE_INTEGER zero = {};
		check(stream->Seek(zero, STREAM_SEEK_SET, nullptr), "IStream::Seek");
		ULONG read = 0;
		check(stream->Read(out.data(), (ULONG)out.size(), &read), "IStream::Read");
		out.resize(read);
		return out;
	}

	// Shrinks the image so its long edge is at most `max_edge` and re-encodes it.
	// JPEG/PNG that are already small enough are passed through untouched.
	image convert(const void* data, size_t size, unsigned max_edge)
	{
		ComPtr<IWICImagingFactory> factory;
		check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)), "creating the WIC factory");
		ComPtr<IWICStream> stream;
		check(factory->CreateStream(&stream), "IWICImagingFactory::CreateStream");
		check(stream->InitializeFromMemory((BYTE*)data, (DWORD)size), "IWICStream::InitializeFromMemory");
		ComPtr<IWICBitmapDecoder> decoder;
		check(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder), "decoding the album art");
		ComPtr<IWICBitmapFrameDecode> frame;
		check(decoder->GetFrame(0, &frame), "IWICBitmapDecoder::GetFrame");
		UINT width = 0, height = 0;
		check(frame->GetSize(&width, &height), "IWICBitmapFrameDecode::GetSize");

		GUID container = {};
		decoder->GetContainerFormat(&container);
		const bool jpeg = container == GUID_ContainerFormatJpeg, png = container == GUID_ContainerFormatPng;
		const UINT long_edge = (std::max)(width, height);
		if ((jpeg || png) && long_edge <= max_edge && size <= max_upload_bytes) {
			return { std::string((const char*)data, size), jpeg ? "image/jpeg" : "image/png" };
		}

		ComPtr<IWICBitmapSource> source = frame;
		if (long_edge > max_edge) {
			const double scale = (double)max_edge / long_edge;
			const UINT new_width = (std::max)(1u, (UINT)(width * scale + 0.5)), new_height = (std::max)(1u, (UINT)(height * scale + 0.5));
			ComPtr<IWICBitmapScaler> scaler;
			check(factory->CreateBitmapScaler(&scaler), "IWICImagingFactory::CreateBitmapScaler");
			// HighQualityCubic needs Windows 10.
			HRESULT hr = scaler->Initialize(frame.Get(), new_width, new_height, WICBitmapInterpolationModeHighQualityCubic);
			if (FAILED(hr)) hr = scaler->Initialize(frame.Get(), new_width, new_height, WICBitmapInterpolationModeFant);
			check(hr, "IWICBitmapScaler::Initialize");
			source = scaler;
		}

		// Keep transparency as PNG unless that gets too big; everything else goes out as JPEG.
		bool as_png = has_alpha(factory.Get(), frame.Get());
		std::string out = encode(factory.Get(), source.Get(), as_png);
		if (as_png && out.size() > max_upload_bytes) {
			as_png = false;
			out = encode(factory.Get(), source.Get(), false);
		}
		if (out.size() > max_upload_bytes) throw std::runtime_error("the converted image is still larger than 5MB");
		return { std::move(out), as_png ? "image/png" : "image/jpeg" };
	}

	// Loads the front cover the way foobar2000 itself would (embedded or external file, per its album art settings).
	album_art_data_ptr load_front_cover(metadb_handle_ptr const& track, abort_callback& abort)
	{
		pfc::list_single_ref_t<metadb_handle_ptr> items(track);
		pfc::list_single_ref_t<GUID> ids(album_art_ids::cover_front);
		try {
			auto extractor = album_art_manager_v2::get()->open(items, ids, abort);
			return extractor->query(album_art_ids::cover_front, abort);
		}
		catch (exception_album_art_not_found const&) {
			return nullptr;
		}
	}

	struct com_scope
	{
		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		~com_scope() { if (SUCCEEDED(hr)) CoUninitialize(); }
	};

	std::atomic<unsigned> g_latest_request{ 0 };
	std::mutex g_start_lock;

	void serve(unsigned request, bool enabled, metadb_handle_ptr const& track, unsigned short port, unsigned max_edge)
	{
		{
			std::lock_guard<std::mutex> guard(g_start_lock);
			// A newer post command took over.
			if (request != g_latest_request) return;
			// Whatever happens below, art left over from a previous post must not end up in this one.
			art_server::stop();
		}
		if (!enabled) return;

		image art;
		try {
			com_scope com;
			album_art_data_ptr cover = load_front_cover(track, fb2k::mainAborter());
			if (cover.is_empty() || cover->get_size() == 0) {
				log("no front cover for this track, posting text only");
				return;
			}
			art = convert(cover->get_ptr(), cover->get_size(), max_edge);
		}
		catch (exception_aborted const&) {
			return;
		}
		catch (std::exception const& e) {
			FB2K_console_formatter() << "foo_twitter: could not prepare the album art, posting text only: " << e.what();
			return;
		}

		std::lock_guard<std::mutex> guard(g_start_lock);
		if (request != g_latest_request) return;
		std::string error;
		if (!art_server::start(std::move(art.data), art.mime, port, serve_timeout_ms, log, error)) {
			FB2K_console_formatter() << "foo_twitter: could not start the art server on port " << port << ", posting text only: " << error.c_str();
		}
	}
}

void artwork::attach_then(metadb_handle_ptr track, std::function<void()> open_intent)
{
	const unsigned request = ++g_latest_request;
	const bool enabled = cfg_attach_art;
	unsigned port = (unsigned)cfg_art_port.get(), max_edge = (unsigned)cfg_art_max_edge.get();
	if (port < 1 || port > 65535) port = default_art_port;
	max_edge = std::clamp(max_edge, min_art_max_edge, max_art_max_edge);

	// Load and serve the art off the main thread; the intent page is opened only once the server listens.
	fb2k::splitTask([=] {
		serve(request, enabled, track, (unsigned short)port, max_edge);
		fb2k::inMainThread([open_intent] {
			if (!core_api::is_shutting_down()) open_intent();
		});
	});
}
