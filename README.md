# ツイッターにブラウザからなうぷれするやつ

行儀の悪すぎるやばコード

https://wakiga.nl/2021/04/24/foobar2000%e3%81%aenowplaying%e3%82%92%e3%83%84%e3%82%a4%e3%83%83%e3%82%bf%e3%83%bc%e3%81%ab%e6%8a%95%e7%a8%bf%e3%81%97%e3%81%9f%e3%81%84/

## インストール

[Releases](https://github.com/wilhellmina/foo_naupure/releases) から `foo_naupure-<バージョン>.fb2k-component` をダウンロードして、ダブルクリックで foobar2000 に入れる。

## 使い方

曲を再生中に Playback → **Tweet** を実行すると、ブラウザで投稿画面が開く。

設定は Preferences → Tools → **foo_naupure**。

## 投稿する文字列を変える

設定の「Post text」に foobar2000 のタイトルフォーマットで書く。1 行が投稿の 1 行になり、下の Preview で結果を確認できる。

既定値:

```
#NowPlaying
%title%['('%artist%')'][ / %album%]
```

- 例: `%title% / %album%` → 曲名 / アルバム名、`%artist% - %title%` → アーティスト名 - 曲名
- `[ ]` で囲んだ部分は、タグがない曲では消える
- `(` `)` などの記号を文字として出すときは `'('` のように `'` で囲む

## アルバムアートを添付する（任意）

ユーザースクリプトを入れると、再生中の曲のアルバムアートが投稿画面に自動で添付される。入れなければテキストだけの投稿になる。

1. ブラウザに [Tampermonkey](https://www.tampermonkey.net/) を入れる
2. Chrome / Edge では拡張機能の管理画面で Tampermonkey の「詳細」を開き、「ユーザー スクリプトを許可する」をオンにする
3. [foo_naupure_artwork.user.js](https://github.com/wilhellmina/foo_naupure/raw/master/userscript/foo_naupure_artwork.user.js) を開いてインストールする

設定の「Local server port」を変えたときは、スクリプト冒頭の `PORT` も同じ値にする。

うまくいかないときは、View → Console に出る `foo_naupure:` で始まるメッセージを見る。

- `no request for the artwork in time` … スクリプトが動いていない。Tampermonkey の設定と `PORT` を確認する
- `could not start the art server` … ポートがほかのアプリと重なっている。設定でポートを変える
- `no front cover for this track` … foobar2000 がその曲のアートを見つけられない

---

ビルドとリリースの手順は [BUILDING.md](BUILDING.md)。
