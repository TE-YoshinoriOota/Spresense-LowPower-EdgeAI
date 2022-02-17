/*
 *  Subcore1.ino - Subcore sketch for Multi Processor Sample Program using MPMutex  
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
#include <MPMutex.h>
MPMutex mutex(MP_MUTEX_ID0);

const int mem_size = 128;
int* data;  // メインコアが確保したメモリポインター

void setup() {
  int ret = MP.begin();
  if (ret < 0) {
    MPLog("MP.begin error: %d\n", ret);
  }
  // データ受信をポーリングモードで監視
  MP.RecvTimeout(MP_RECV_POLLING);
}

void loop() {
  int ret;
  int8_t rcvid;

  // メモリエリアのアドレスを受信
  ret = MP.Recv(&rcvid, &data);
  if (ret < 0) return;  // データがない場合は負の値
 
  // mutex が確保できるまで待つ
  do { ret = mutex.Trylock(); } while (ret != 0);

  // 受信データのチェックサムを計算
  int sum = 0;
  for (int n = 0; n < mem_size; ++n) {
    sum += data[n];
  }
  int checksum = ~sum;

  // mutex を開放
  mutex.Unlock(); 

  // チェックサムを比較
  if (checksum != data[mem_size]) {
    MPLog("Error = 0x%2X\n", checksum);
  } else {
    MPLog("  Ok  = 0x%2X\n", checksum);
  }
}
