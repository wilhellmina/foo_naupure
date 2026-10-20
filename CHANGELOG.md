# Changelog

リリースごとの変更点。`tools/package.ps1` がバージョンの節をそのまま GitHub Releases のリリースノートにする。

## 1.2.0

- 再生中の曲のアルバムアートを投稿画面に自動で添付できるようにした（Tampermonkey のユーザースクリプトが必要。README 参照）
- Preferences → Tools → foo_naupure に設定ページを追加（アート添付のオン/オフ、ポート、画像の最大サイズ）
- 投稿の文面を設定ページで変えられるようにした（foobar2000 のタイトルフォーマットで書く。プレビュー付き）
- コンポーネント名を foo_twitter から foo_naupure に変えた。別のコンポーネント扱いになるので、foo_twitter を入れている場合は外す（メニューの Tweet が 2 つになる）
