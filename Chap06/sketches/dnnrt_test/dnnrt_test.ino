/*
 *  dnnrt_test.ino - Test sketch for DNNRT(Deep Neural Network RunTime)
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

#include <DNNRT.h>
#include <SDHCI.h>
#include <BmpImage.h>

#define DNN_WIDTH (28)
#define DNN_HEIGHT (28)

DNNRT dnnrt;
SDClass SD;
BmpImage BMP;
DNNVariable input(DNN_WIDTH * DNN_HEIGHT);

const char label[12] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', NULL};
const char filename[16] = "test4.bmp";

void setup() {
  Serial.begin(115200);
  while(!SD.begin()) { Serial.println("Insert SD Card"); }

  File nnbfile = SD.open("model.nnb");  // 学習済モデルをオープン
  if (!nnbfile) {
    Serial.println("model.nnb is not found");
    while(1);
  }
  int ret = dnnrt.begin(nnbfile);  // DNNRTを初期化
  if (ret < 0) {
    Serial.println("DNNRT begin fail: " + String(ret));
    while(1);
  }

  // テスト用モノクロ画像をオープン
  File testImg = SD.open(filename); 
  if (!testImg) {
    Serial.println("Test Image not found");
    while(1);
  }
  
  BMP.begin(testImg); // BMP画像の読み込み
  uint8_t* imgbuf = BMP.getImgBuff(); // 画像データを取得
  
  float* buf = input.data();  // DNNRT変数用内部バッファ取得
  for (int n = 0; n < DNN_WIDTH*DNN_HEIGHT; ++n) {
    buf[n] = imgbuf[n] / 255.0; // 画像データを0.0~1.0に正規化
  }
  BMP.end(); // メモリの解放

  dnnrt.inputVariable(input, 0);  // 入力データを設定
  dnnrt.forward();  // 推論を実行
  DNNVariable output = dnnrt.outputVariable(0);  // 出力を取得

  // 確からしさがもっとも高いインデックス値を取得
  int index = output.maxIndex();
  Serial.println("Image: " + String(filename));
  Serial.println("Result: " + String(label[index]));
  Serial.println("Probability: " + String(output[index]));
}

void loop() {}
