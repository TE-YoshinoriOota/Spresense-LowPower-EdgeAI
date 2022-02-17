/*
 *  Subcore1.ino - Subcore sketch for Multi Processor Sample Program 
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
#if (SUBCORE != 1)
#error "Core selection is wrong!!"
#endif

#include <MP.h>

void setup() {
  MP.begin();  // メインコアに起動通知

  // データが来るまで待つように設定
  MP.RecvTimeout(MP_RECV_BLOCKING); 
}

void loop() {
  int ret;
  int8_t msgid;  // メインコアの送信ID
  uint32_t msgdata;  // 受信データ

  // メインコアからのデータを待つ
  ret = MP.Recv(&msgid, &msgdata);
  if (ret < 0) {
    MPLog("MP.Recv Error=%d\n", ret);
  }

  // メインコアの送信IDで受信データを返す
  ret = MP.Send(msgid, msgdata);
  if (ret < 0) {
    MPLog("MP.Send Error=%d\n", ret);
  }
}
