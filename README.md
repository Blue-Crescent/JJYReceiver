# JJYReceiver

JJY Receiver Library for arduino

# 機能

JJYの日本標準時刻データを受信します。C言語標準のtime_t型のUTC基準で時刻を返します。受信後は、タイマにより時刻を刻み維持することもできます。

arduinoで再利用しやすいJJY受信ライブラリ的なものがWebに見つけられなかったので作ってみました。

# ハードウェア要件

- 10msecのタイマ
- 端子変化割り込み

を使用します。
10msecのタイマーはJJY受信時のサンプリング周期、及び時刻を刻む場合に使用します。
RTCなどを使用して時刻を維持し、マイコン側で時を刻まない場合、受信完了後タイマー動作は不要です。

端子変化割り込みは秒の基準信号とエッジ同期をはかるために使用します。
端子変化により、サンプリングデータの格納インデックスをリセットします。

最低限の端子要件は、JJY受信モジュールのデータ出力を一本、マイコンのピン変化割り込み入力端子につなげてください。その場合は受信モジュールの他の端子は適切な値に固定してください。

## 確認しているハードウェア

### マイコン

- lgt8f328p

できる限りアーキテクチャに依存しないように作っているつもりですが、uint64_tが16bitしか使えなかったり、発見したものを潰している感じですので、ハードウェア(コンパイラ)により対応できない場合があるかもしれません。esp32も所持しているので時間があるときに確認するかもしれません。

### JJY受信IC

写真には黄色のコンデンサがついていますが不要です。JJY受信モジュールはおおむねどれも似たようなインタフェースかと思いますが、負論理と正論理のものがあり手元にあるものは負論理モジュールでしたので、負論理と正論理の出力がある場合は負論理出力をつないでください。

利用したモジュールの参考：

- 製品番号: JJY-1060N-MAS
- 基板表記: MASD-S-R1 2020/05/27
- 受信IC: MAS6181B

![](img/IMG_5775.jpeg)
![](img/IMG_5777.jpeg)
![](img/IMG_5776.jpeg)



| 基板シルク | 機能                                                                                                              |
| ----- | --------------------------------------------------------------------------------------------------------------- |
| SEL   | L: 40kHz<br/>H: 60kHz                                                                                           |
| OUT   | JJYデータ出力 PWM<br/>P,M : 0.2sec Low - 0.8sec High<br/>H: 0.5sec Low - 0.5sec High<br/>L: 0.8sec Low - 0.2sec High |
| PON   | L: 動作<br/> H: 停止                                                                                                |
| GND   | 基準電位                                                                                                            |
| VDD   | 1.1~3.3 v                                                                                                       |



負論理出力でした。JJY信号の波形の立下りが1秒幅になります。

![](img/wave%20(2).jpeg)

JJY波形



![](img/wave%20(3).jpeg)

H



![](img/wave%20(5).jpeg)
L



![](img/wave%20(6).jpeg)

P,M





補足
lgt8f328pを使用する場合は、書き込み時にVccは5Vが出力されます。この受信モジュールは3.6Vが絶対最大定格ですので、書き込み時は受信モジュールを外すか、電圧レギュレーターをライタとの間に設けて保護してください。3.3Vでも書き込めました。

![](img/IMG_5779.jpeg)

入手元:
https://ja.aliexpress.com/item/1005005254051736.html

# ソフトウェア

## 関数

### begin(ピン番号)

JJYデータの受信を開始します。ピン番号にはJJY受信モジュール出力を接続したマイコンの入力端子番号を設定します。

### power(bool)

引数はtrueがモジュール動作状態、falseがモジュール停止状態です。

### freq()

受信周波数を設定します。

- 40kHzの場合40

- 60kHzの場合60

### monitor(ピン番号);

JJYモジュールの信号状態を(立ち上がりでH、立下りエッジでL)出力します。任意の出力端子にLEDなどを接続し電波状態を確認する目的で設けています。 デバッグ用です。不要の場合は呼び出さないか-1を設定してください。

JJYデータはLを含むデータが多いので、短めの点灯時間が多い場合は負論理出力、長めの点灯時間が多い場合は正論理出力です。

### delta_tick()

10msecおきに呼び出してください。JJYデータのサンプリング、時刻を刻む2つの用途で使用します。時刻を刻む場合はできる限り精度の高いクロックを利用したタイマーで呼び出すことを推奨します。

受信後RTCなどに時刻設定し、マイコン内部で時刻を維持する必要がない場合は、受信後呼び出す必要はありません。

### jjy_receive()

JJY受信モジュールのデータ出力をマイコンの端子変化割り込み対応の端子に接続し、その端子の変化割り込みルーチンで呼び出してください。

### getTime()

時刻を取得します。時刻が受信できていない場合は-1を返します。

delta_tick()が受信後も供給されている場合は、マイコンのクロック精度で刻んで維持している現在時刻が返されます。供給されていない場合は最後に受信した時刻を返します。現在時刻が確定した時点で内部受信動作は停止します。再び受信動作を実行する場合はbegin()を呼び出します。

getTime()が戻り値を返すには最低2つの内部の時刻受信データが一致する必要があります。JJYは1分かけて時刻情報を送信していますので、受信には最低2分かかります。良好な状態で3分程度、ノイズがある環境だと数分～受信不可となります。

マイコンのクロック精度は、100ppmの発振器で2時間で1秒弱程度ずれますので、適宜受信してください。

## サンプルスケッチ

サンプルスケッチの出力結果は以下の通りです。 受信中(time_tが-1の時)は2136年の時刻を指すようです。

受信した段階で時刻がインクリメントし変化していきます。(10msecタイマx100カウント)

![](img/examplesketch.png)

# デバッグモード

SoftwareSerialなどのシリアル通信ライブラリを有効にすることで、文字出力されます。
ヘッダファイル内の#define DEBUG_BUILD有効にしてください。

左端に表示されている16進数の数値はサンプリングデータです。
データ判定結果が:の後にP,L,Hで表示され、それぞれマーカ、L、Hとなります。
そのあとにマーカの区間、受信状態を表示します。

![](img/Debug1.png)

受信中の中間データはlocaltime[0],localtime[1],localtime[2]のデータに格納されます。[0]のデータは途中から受信し、[1]のデータは最初から受信処理がされるためlocaltime[1]に最も早く時刻データが入ります。

# アルゴリズム

## ハミング距離によるデータ判定

JJYのビットデータを10msec毎にサンプリングします。サンプリングの開始インデックスはJJYの信号変化でリセットされます。(負論理出力のモジュールの場合は立下りエッジ)
lgt8f328の場合は+-60msec程度揺らぐので、100サンプリングではなく80サンプリング程度取得段階でハミング距離を計算しH,L,マーカのいずれかを判定します。

信号幅を測定したりする方法も実装してみましたが、ノイズが多く受信が困難だったためこの方式にしています。

https://www.nict.go.jp/sts/jjy_signal.html

## 時刻データのデコード

受信は3回分の受信データを保持しており、同一の時刻を二つ観測した段階で正式時刻として採用します。
二つデータが揃わない場合は二つ揃うまでデータを巡回して上書きしていきます。
時刻データはtime.hのtimeinfo構造体を利用して、JJYデータからUTC時刻に変換しtime_t型で管理します

40KHzでの動作確認をしています。

# TODOメモ

いつかやるかもしれない

- 受信高速化
  　マーカービットを待つことなく途中から受信したデータの再利用する
- リファクタリング
- 40kHz/60kHz自動選択
- 負論理・正論理出力確認補助

未定

- 立ち上がりエッジ動作受信モジュール対応。未所持
- 本家Arduinoボード動作確認。未所持 Uno R4欲しいな～

# その他

ハードウェアのカンパ等歓迎します。開発に利用した場合、どのようなものに利用したか、感想なども歓迎します。(要望の実現・返信は必ずしも行いません)

バグ修正・エンハンスを歓迎します。パッチは受け付けていませんので、プルリクエストをお送りください。

GitHubの利用は初めてですので、お作法等ご容赦ください。
