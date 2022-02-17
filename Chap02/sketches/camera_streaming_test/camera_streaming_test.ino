/*
 *  camera_streaming_test.ino - Simple camera preview example application
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
#include "Adafruit_ILI9341.h"

#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);

void CamCB(CamImage img) {
  if (img.isAvailable()) {
    img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);
    display.drawRGBBitmap(0, 0 /* 開始座標 */
        , (uint16_t*)img.getImgBuff() /* 画像データ */ 
        , 320, 240); /* 横幅、縦幅  */
  }
}
void setup() {
  display.begin(); //　液晶ディスプレイの開始
  theCamera.begin(); // カメラの開始
  display.setRotation(3); // ディスプレイの向きを設定
  theCamera.startStreaming(true, CamCB); // カメラのストリーミングを開始
}
void loop() {}
