/*
 *  image_collection.ino - Image Capture sketch for making an image dataset
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
#include <Camera.h>
#include <Adafruit_ILI9341.h>
#include <SDHCI.h>
#include <BmpImage.h>

#define TFT_DC  9
#define TFT_CS  10
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define OFFSET_X  (104)
#define OFFSET_Y  (0)
#define CLIP_WIDTH (112)
#define CLIP_HEIGHT (224)
#define DNN_WIDTH  (28)
#define DNN_HEIGHT  (28)

SDClass SD;
BmpImage bmp;
char fname[16];


// ボタン用ピンの定義
// 学習キットを使用している場合はボタン番号を取り扱い説明書で確認してください
#define BUTTON 0

// ボタン押下時に呼ばれる割り込み関数
bool bButtonPressed = false;
void changeState() {
  bButtonPressed = true;
}

// 画像データの保存用関数 
void saveGrayBmpImage(int width, int height, uint8_t* grayImg) 
{
  static int g_counter = 0;  // ファイル名につける追番
  sprintf(fname, "%03d.bmp", g_counter);

  // すでに画像ファイルがあったら削除
  if (SD.exists(fname)) SD.remove(fname);
  
  // ファイルを書き込みモードでオープン
  File bmpFile = SD.open(fname, FILE_WRITE);
  if (!bmpFile) {
    Serial.println("Fail to create file: " + String(fname));
    while(1);
  }
  
  // ビットマップ画像を生成
  bmp.begin(BmpImage::BMP_IMAGE_GRAY8, DNN_WIDTH, DNN_HEIGHT, grayImg); 
  // ビットマップを書き込み
  bmpFile.write(bmp.getBmpBuff(), bmp.getBmpSize());
  bmpFile.close();
  bmp.end();
  ++g_counter;
  // ファイル名を表示
  Serial.println("Saved an image as " + String(fname));
}


void CamCB(CamImage img) {

  if (!img.isAvailable()) {
    Serial.println("Image is not available. Try again");
    return;
  }

  // カメラ画像の切り抜きと縮小
  CamImage small;
  CamErr err = img.clipAndResizeImageByHW(small
                     , OFFSET_X, OFFSET_Y
                     , OFFSET_X + CLIP_WIDTH -1
                     , OFFSET_Y + CLIP_HEIGHT -1
                     , DNN_WIDTH, DNN_HEIGHT);
  if (!small.isAvailable()){
    putStringOnLcd("Clip and Reize Error:" + String(err), ILI9341_RED);
    return;
  }

  // 推論処理に変えて学習データ記録ルーチンに置き換え
  // 学習用データのモノクロ画像を生成
  uint16_t* imgbuf = (uint16_t*)small.getImgBuff();
  uint8_t grayImg[DNN_WIDTH*DNN_HEIGHT];
  for (int n = 0; n < DNN_WIDTH*DNN_HEIGHT; ++n) {
    grayImg[n] = (uint8_t)(((imgbuf[n] & 0xf000) >> 8) 
                         | ((imgbuf[n] & 0x00f0) >> 4));
  }

  // ボタンが押されたら画像を保存
  if (bButtonPressed) {
    Serial.println("Button Pressed");
    // 学習データを保存
    saveGrayBmpImage(DNN_WIDTH, DNN_HEIGHT, grayImg);
    // ファイル名をディスプレイ表示
    putStringOnLcd(String(fname), ILI9341_GREEN);
    bButtonPressed = false;   
  }

  // 処理結果のディスプレイ表示
  img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);
  uint16_t* imgBuf = (uint16_t*)img.getImgBuff(); 
  drawBox(imgBuf, OFFSET_X, OFFSET_Y, CLIP_WIDTH, CLIP_HEIGHT, 5, ILI9341_RED); 
  drawGrayImg(imgBuf, grayImg);
  display.drawRGBBitmap(0, 0, (uint16_t *)img.getImgBuff(), 320, 224);
}


void setup() {   
  Serial.begin(115200);
  display.begin();
  theCamera.begin();
  display.setRotation(3);
  theCamera.startStreaming(true, CamCB);

  attachInterrupt(digitalPinToInterrupt(BUTTON), changeState, FALLING);
  while (!SD.begin()) { 
    putStringOnLcd("Insert SD card", ILI9341_RED); 
  }
  putStringOnLcd("ready to save", ILI9341_GREEN);
}

void loop() { }
