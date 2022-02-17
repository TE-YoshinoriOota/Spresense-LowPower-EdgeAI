/*
 *  Subcore.ino - Autoencoder sample for a pipe anomaly detection
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

#ifndef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <MP.h>
#include <MPMutex.h>
MPMutex mutex(MP_MUTEX_ID0);

void setup() {
  setupLcd();  // 液晶ディスプレイの設定
  MP.begin();  // メインコアに起動を通知
}

void loop() {
  int ret;
  int8_t msgid;
  float *buff; // メインコアから渡されたデータへのポインター
  bool bNG = false;

  ret = MP.Recv(&msgid, &buff); // データを受信
  if (ret < 0)  return; // データがない場合は何もしない

  do {
    ret = mutex.Trylock();  // MPMutexを取得
  } while (ret != 0);  // メインコアの処理が終わるまで末
  static const int msg_ng = 101;
  msgid == msg_ng ? bNG = true : bNG = false;
  showSpectrum(buff, bNG); // スペクトルを表示
  mutex.Unlock();  // MPMutexをメインコアに渡す
}
