# 通常表示
[![atomicclock](image.png)](https://www.youtube.com/shorts/GZbrPVhcVWU)

Clickでyoutube

# 初回電源投入時 感度表示
![alt text](IMG_6464.jpeg)

- 非受信時は:点滅点灯
- 受信中は:持続点灯

# 裏面
![alt text](IMG_6463.jpeg)

# 回路図
![](rtcclock_lgt8f328.png)

## ノイズに関する注意

DS1307のRTCのクロック出力を行うとノイズが発生するので、停止しておく

LGT8F328Pの場合はMONITOR_LEDを動作させるとノイズが出るので、基板上のLEDは使用しない

JJY受信機への電源はなるべくクリーンなものを供給する。手持ちのLDOがAMS1117だったのでLC回路を付けてノイズを低減させている


