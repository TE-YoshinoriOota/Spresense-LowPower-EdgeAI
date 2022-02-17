/*
 *  Maincore.ino - Maincore sketch for Multi Processor Sample Program using MPMutex  
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
#include <MPMutex.h>
MPMutex mutex(MP_MUTEX_ID0); 

const int subcore=1;
const int mem_size = 128;
int data[mem_size+1]; // 末尾にチェックサムを付加

void setup() {
  int ret = MP.begin(subcore);
  if (ret < 0) {
    MPLog("MP.begin error: %d\n", ret);
  }

  // 未接続ピンのノイズを利用して乱数初期値を設定
  randomSeed(analogRead(0));  
}

void loop() {
  int ret;
  int8_t sndid = 100;  // 送信ID
  
  // mutex が確保できるまで待つ
  do { ret = mutex.Trylock(); } while (ret != 0);

  for (int n = 0; n < mem_size; ++n) {
   data[n] = random(0, 256); // 0～255の乱数生成
  }

  // チェックサムを計算
  int sum = 0;
  for (int n = 0; n < mem_size; ++n) {
    sum += data[n];
  }
  int checksum = ~sum;
  data[mem_size] = checksum;  // チェックサムを追加

  mutex.Unlock(); // mutex を開放

  // サブコア1にメモリポインターを渡す
  ret = MP.Send(sndid, &data, subcore); 
  if (ret < 0) {
    MPLog("MP.Send error: %d\n", ret);
  }

  MPLog("Checksum = 0x%2X\n", checksum);
}
