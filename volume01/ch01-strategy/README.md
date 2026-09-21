# 第1章（Strategy）

割引ルールが増えるたびに、既存の割引判定まで書き換えることになる問題を扱います。

| フォルダ | 本での位置 | 内容 |
|---|---|---|
| [`1-before/`](1-before/) | 1-4 実装コード（現状） | 変更が届く前のコード |
| [`2-trial/`](2-trial/) | 3-1 変更を試みる | 構造を変えずに変更を当てた仮実装（捨てる前提） |
| [`3-after/`](3-after/) | 7-1 解決後のコード（全体） | 構造を変えたあとのコード |
| [`4-after-split/`](4-after-split/) | 7-1 実務でファイルを分けるなら | 完成コードのファイル分割 |

## 動かす

Windows では、一つ上のフォルダの `run.bat` をダブルクリックし、番号で選びます。

macOS・Linux では次のとおりです。

```sh
cd 1-before
make run      # 実行
make verify   # 本に載っている実行結果と一致するか検査
```

## 変更前と変更後を見比べる

```sh
diff -u 1-before/main.cpp 2-trial/main.cpp   # 変更を当てた痛み
diff -u 1-before/main.cpp 3-after/main.cpp   # 構造を変えたあと
```
