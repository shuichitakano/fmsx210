# fMSX210
fMSX を Sipeed の Maix シリーズ向けに移植したものです。
付属の LCD での映像出力のほか、コンポジットビデオでの映像出力に対応しています。

[Download](https://github.com/shuichitakano/fmsx210/releases/download/v0.2/fmsx210_v0.2.zip)

## 動作環境
Maix Dock, Maix Go, Maixduino で動作します。
コンポジットビデオ出力を使用するには、LCD のポートがヘッダに出ているボード(Maix Dock/Go を使用するのがおすすめです。
コンポジットビデオと PS/2 キーボードの動作確認は Maix Dock のみで行なっています。

v0.2で Maix Amigo に対応しました。
Maix Amigo にはTFT版とIPS版の２種類があり、settings.jsonでの指定が必要です。
基板に2960と書いてあるのがTFT版、2970と書いてあるのがIPS版です。

## 回路
Maix Dock での回路例を示します。
![回路図](maix_dock_video.png)

- コンポジットビデオの DAC 部には金属皮膜抵抗を使用してください
- 液晶で使用する場合は PS/2 ポート部分だけ制作してください

## PS コントローラー
PSコントローラーに対応しています。
AMIGO の右側の SP-MOD 端子に接続する想定です。

|PS|MSX|
|:--|:--|
|×|Bボタン|
|○|Aボタン|
|select|SELECTキー|
|start|STOPキー|
|L1|F1キー|
|R1|F5キー|

![回路図](pspad.png)

## CardKB
M5Stack用のCardKeyboardに対応しています。
Maix Amigo の真ん中のGrove Portに接続して使用します。
Maix Amigo TFT版では i2c_num に 0 を指定してください。
Maix Amigo IPS版では i2c_num に 1 を指定し、i2c1 を enable に設定する必要があります。

シリアルキーボードの特性上、キー入力後5フレーム押される状態になる、みたいな微妙な対応です。
fn+1-5がF1-F5に対応しています。

## 実行
以下の ROM を Micro SD のルートディレクトリに置いて起動してください。

- MSX2.ROM
- MSX2EXT.ROM
- DISK.ROM
- KANJI.ROM

### settings.json 

ルートディレクトリに置いた settings.json で起動設定が可能です。

|キー|意味|
|:--|:--|
|default_rom0|スロット1の ROM イメージを指定します|
|default_rom1|スロット2の ROM イメージを指定します|
|default_disk0|ドライブ A のディスクイメージを指定します|
|default_disk1|ドライブ B のディスクイメージを指定します|
|ram_pages|搭載 RAM のページ数を指定します|
|vram_pages|搭載 VRAM のページ数を指定します|
|board|MAIX_DOCK, AMIGO_TFT, AMIGO_IPS のいずれかを指定します|
|video|ビデオ出力方式を指定します<br>LCD: 液晶<br>NTSC: コンポジットビデオ|
|flip_lcd|液晶出力時、true で 180度反転します|
|te_pin|tearing effectピンを指定するとVSync同期が有効になります。AMIGO TFTのみで使用できます。|
|audio_enable_pin|オーディオを有効にする IO ポートを指定します<br>Maix Go: 32<br>Maixduino: 2|
|volume|オーディオ出力の音量を指定します(0-255)|
|ps2|PS2キーボードのI/Oポートを指定します|
|pspad|PSコントローラーのI/Oポートを指定します|
|cardkb|Card KBのI2Cデバイス番号を指定します|
|i2c1|I2Cデバイス1の設定を行います。cardkbのi2c_numを1にした時に使用されます|


## 操作方法

|キー|機能|
|:--|:--|
|F10|メニューを開く|
|ENTER|決定|
|ESC|メニューを閉じる|
|Up (Maix go)|音量上げ/メニュー上|
|Down (Maix go)|音量下げ/メニュー下|
|Center (Maix go)|メニュー開く/決定|


## ビルド方法
ライセンスの関係上 fMSX のソースは含まれていません。
third_party/fMSX54 以下に fMSX のソースを展開してビルドを行なってください。

```
mkdir build
cd build
cmake -DTOOL_CHAIN=/usr/local/opt/kendryte-toolchain/bin -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

## 書き込み
kflash 等を使用してボードに書き込んでください。
```
例)
kflash -b 1500000 -p /dev/cu.usbserial-14101 -t fmsx.bin
```





