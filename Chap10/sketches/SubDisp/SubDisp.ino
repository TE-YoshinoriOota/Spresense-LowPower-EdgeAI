/*
 *  SubDisp.ino - Spectrogram display sample
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
  setupLcd();
  MP.begin();
}

void loop() {
  int ret;
  int8_t msgid;
  float *buff;

  ret = MP.Recv(&msgid, &buff);
  if (ret < 0)  return;

  // 描画処理開始前にMutexを確保
  do { ret = mutex.Trylock(); } while (ret != 0);
  
  displayLcd(buff);  // FFT演算結果をディスプレイに反映
  
  // 描画処理終了時にMutexを開放
  mutex.Unlock();
}
