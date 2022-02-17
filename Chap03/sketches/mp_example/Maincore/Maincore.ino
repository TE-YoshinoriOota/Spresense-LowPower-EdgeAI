/*
 *  Maincore.ino - Maincore sketch for Multi Processor Sample Program 
 *  Copyright 2022 Sony Semiconductor Solutions Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifdef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <MP.h>
int subcore = 1;   // サブコア1を使用

void setup() {
  MP.begin(subcore);  // サブコア1を起動
  
  // 未接続ピンのノイズを利用して乱数初期値を設定
  randomSeed(analogRead(0));  
}

void loop() {
  int ret;
  uint32_t snddata;  // サブコアへ送信するデータ
  uint32_t rcvdata;  // サブコアからの受信データ
  int8_t sndid = 100;  // 送信ID (100)
  int8_t rcvid;  // 受信ID
 
  snddata = random(32767);  // 乱数を生成
  MPLog("Send: data= %d\n", snddata);

  // 生成した乱数をサブコア1へ送信
  ret = MP.Send(sndid, snddata, subcore);
  if (ret < 0) {
    MPLog("MP.Send error = %d¥n", ret);
  }

  // サブコアからのデータが到着するまで待つ
  MP.RecvTimeout(MP_RECV_BLOCKING); 
  ret = MP.Recv(&rcvid, &rcvdata, subcore);
  if (ret < 0) {
    MPLog("MP.Recv error = %d\n", ret);
  }

  MPLog("Recv data= %d\n", rcvdata);
  delay(1000);  // 1秒待つ
}
