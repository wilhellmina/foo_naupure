// ==UserScript==
// @name         foobar2000 nowplaying artwork
// @namespace    https://github.com/wilhellmina/foo_naupure
// @version      1.0
// @description  Attaches the album art served by foo_naupure (foobar2000) to the X post intent page.
// @match        https://x.com/intent/*
// @match        https://x.com/compose/*
// @match        https://twitter.com/intent/*
// @grant        GM_xmlhttpRequest
// @connect      127.0.0.1
// @run-at       document-idle
// @noframes
// ==/UserScript==
(() => {
  'use strict';

  // Must match "Local server port" in foobar2000 > Preferences > Tools > foo_naupure.
  const PORT = 18080;

  // X's DOM changes now and then; this is the place to fix when attaching stops working.
  const FILE_INPUT_SELECTORS = ['input[data-testid="fileInput"]', 'input[type="file"]'];

  GM_xmlhttpRequest({
    method: 'GET',
    url: `http://127.0.0.1:${PORT}/art`,
    responseType: 'blob',
    timeout: 5000,
    onload: r => {
      // Not 200: no art for this post, or it was already handed out / expired.
      if (r.status !== 200 || !r.response) return;
      const type = r.response.type || contentType(r.responseHeaders) || 'image/jpeg';
      const ext = type === 'image/png' ? 'png' : 'jpg';
      waitFor(findFileInput, input => {
        const dt = new DataTransfer();
        dt.items.add(new File([r.response], `cover.${ext}`, { type }));
        input.files = dt.files;
        input.dispatchEvent(new Event('change', { bubbles: true }));
      });
    },
    // foobar2000 isn't serving anything (option off, no art, or not a post from foobar2000).
    onerror: () => {},
    ontimeout: () => {},
  });

  function findFileInput() {
    for (const sel of FILE_INPUT_SELECTORS) {
      const el = document.querySelector(sel);
      if (el) return el;
    }
    return null;
  }

  function contentType(headers) {
    const m = /^content-type:\s*([^;\r\n]+)/im.exec(headers || '');
    return m ? m[1].trim() : '';
  }

  function waitFor(find, cb, limitMs = 15000) {
    const start = Date.now();
    const t = setInterval(() => {
      const el = find();
      if (el) { clearInterval(t); cb(el); }
      else if (Date.now() - start > limitMs) clearInterval(t);
    }, 200);
  }
})();
