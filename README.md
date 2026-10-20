# ツイッターにブラウザからなうぷれするやつ

行儀の悪すぎるやばコード

PlaybackのPullDownにあるはず

https://wakiga.nl/2021/04/24/foobar2000%e3%81%aenowplaying%e3%82%92%e3%83%84%e3%82%a4%e3%83%83%e3%82%bf%e3%83%bc%e3%81%ab%e6%8a%95%e7%a8%bf%e3%81%97%e3%81%9f%e3%81%84/

## アルバムアートを添付する（任意）

再生中の曲のアルバムアート（front cover）を、投稿画面に自動で添付できます。
X の API は使わず、ブラウザの Tampermonkey で動くユーザースクリプトが foobar2000 から画像を受け取って添付します。
スクリプトを入れていなければ、これまでどおりテキストだけの投稿になります。

### 準備

1. ブラウザに [Tampermonkey](https://www.tampermonkey.net/) を入れる
2. Chrome / Edge では拡張機能の管理画面で Tampermonkey の「詳細」を開き、**「ユーザー スクリプトを許可する」をオン**にする
   （古い Chrome では代わりに右上の「デベロッパー モード」をオン）。これがオフだとスクリプトは動きません
3. [`userscript/foo_naupure_artwork.user.js`](https://github.com/wilhellmina/foo_naupure/raw/master/userscript/foo_naupure_artwork.user.js) を開いてインストールする
   （Tampermonkey のダッシュボードで新規スクリプトに中身を貼り付けてもよい）

あとは今までどおり Playback → Tweet を実行すると、アート付きの投稿画面が開きます。内容を確認して投稿ボタンを押すだけです。

### 設定

Preferences → Tools → **foo_naupure**

| 項目 | 既定値 | 説明 |
| --- | --- | --- |
| Attach album art to the post | オン | アートを添付するかどうか |
| Local server port | 18080 | 画像を受け渡すポート。**変えたらスクリプト冒頭の `PORT` も同じ値にする** |
| Max image size | 1600 | 長辺の上限（px）。これより大きいアートは縮小して JPEG にする（透過ありは PNG） |

### しくみ

- Tweet を実行すると、プラグインが再生中の曲のアートを読み込み（埋め込み / `folder.jpg` などの外部ファイルは foobar2000 のアルバムアート設定どおり）、
  `http://127.0.0.1:<port>/art` で受け取れるようにしてから投稿画面を開く
- ユーザースクリプトが投稿画面でそこから画像を取得し、添付する
- サーバーは画像を 1 回渡すか、30 秒たつと閉じる。あとで x.com を普通に開いても古い画像が添付されることはない
- 127.0.0.1 でしか待ち受けず、Host ヘッダーが `127.0.0.1:<port>` / `localhost:<port>` 以外のリクエストは拒否する

### うまくいかないとき

View → Console に `foo_naupure:` で始まるメッセージが出ます。

- `no request for the artwork in time` … スクリプトが動いていない。Tampermonkey でスクリプトが有効か、「ユーザー スクリプトを許可する」、`PORT` を確認
- `could not start the art server on port ...` … ほかのアプリがそのポートを使っている。設定でポートを変え、スクリプトの `PORT` も合わせる
- `no front cover for this track` … foobar2000 がその曲のアートを見つけられない
- 画像は届いている（`artwork sent`）のに添付されない … X の画面構成が変わった可能性。スクリプトの `FILE_INPUT_SELECTORS` を調整する

## ビルド

1. [foobar2000 SDK](https://www.foobar2000.org/SDK)（動作確認: `SDK-2026-09-17`）をリポジトリ直下の `sdk/` に展開する
   （`sdk/pfc`, `sdk/foobar2000/SDK` … となる配置）
2. `foo_naupure.sln` を Visual Studio 2022 以降で開いてビルド（x64 / x86）
3. `bin/<Platform>/Release/foo_naupure.dll` ができる

SDK 付属プロジェクトは v143 固定だが、v143 が無い環境（VS2026 など）では
`Directory.Build.props` によりインストール済みの既定ツールセットで代用される。

## リリース

バージョンは `version.h` の `NAUPURE_VERSION_MAJOR` / `MINOR` / `PATCH` だけで管理している。
foobar2000 のコンポーネント一覧、DLL のプロパティ、パッケージ名はすべてここから決まる。タグは `v1.2.0` の形。

1. `version.h` の番号を上げる。ユーザースクリプトを変えたときは、スクリプトの `@version` も上げる（上げないとインストール済みのスクリプトが更新されない）
2. `CHANGELOG.md` に `## 1.2.1` のような節を足して変更点を書く。これがそのままリリースノートになる
3. コミットして push する
4. `powershell -ExecutionPolicy Bypass -File tools\package.ps1` を実行する
   - Release の x86 / x64 をビルドし直し、`dist\foo_naupure-<version>.fb2k-component` を作る（x86 はパッケージ直下、x64 は `x64\` に入る）
   - CHANGELOG の該当節を `dist\release-notes-<version>.md` に書き出す
   - タグがまだなければ、公開用のコマンド（`git tag` / `git push` / `gh release create`）を表示する
5. 表示されたコマンドでタグを打ち、GitHub Releases にパッケージを上げる

`.fb2k-component` はダブルクリック、または Preferences → Components → Install... でインストールできる。
