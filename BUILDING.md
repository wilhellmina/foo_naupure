# ビルドとリリース

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
