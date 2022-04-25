/*
 *  semaseg_camera.ino - Binary Sematic Segmentation sample
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
#include <SDHCI.h>
#include <DNNRT.h>
#define TFT_DC  9
#define TFT_CS  10
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define OFFSET_X  (48)
#define OFFSET_Y  (6)
#define CLIP_WIDTH  (224)
#define CLIP_HEIGHT  (224)
#define DNN_WIDTH  (28)
#define DNN_HEIGHT  (28)

DNNRT dnnrt;
SDClass SD;
// RGBの画像を入力
DNNVariable input(DNN_WIDTH*DNN_HEIGHT*3);  

void CamCB(CamImage img) {
  if (!img.isAvailable()) return;

  // 画像の切り出しと縮小
  CamImage small; 
  CamErr camErr = img.clipAndResizeImageByHW(small
            ,OFFSET_X ,OFFSET_Y 
            ,OFFSET_X+CLIP_WIDTH-1 ,OFFSET_Y+CLIP_HEIGHT-1 
            ,DNN_WIDTH ,DNN_HEIGHT);
  if (!small.isAvailable()) return;

  // 画像をYUVからRGB565に変換
  small.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565); 
  uint16_t* sbuf = (uint16_t*)small.getImgBuff();

  // RGBのピクセルをフレームに分割
  float* fbuf_r = input.data();
  float* fbuf_g = fbuf_r + DNN_WIDTH*DNN_HEIGHT;
  float* fbuf_b = fbuf_g + DNN_WIDTH*DNN_HEIGHT;
  for (int i = 0; i < DNN_WIDTH*DNN_HEIGHT; ++i) {
   fbuf_r[i] = (float)((sbuf[i] >> 11) & 0x1F)/31.0; // 0x1F = 31
   fbuf_g[i] = (float)((sbuf[i] >>  5) & 0x3F)/63.0; // 0x3F = 64
   fbuf_b[i] = (float)((sbuf[i])       & 0x1F)/31.0; // 0x1F = 31
  }
  
  // 推論を実行
  dnnrt.inputVariable(input, 0);
  dnnrt.forward();
  DNNVariable output = dnnrt.outputVariable(0); 
 
  // DNNRTの結果をLCDに出力するために画像化
  static uint16_t result_buf[DNN_WIDTH*DNN_HEIGHT];
  for (int i = 0; i < DNN_WIDTH * DNN_HEIGHT; ++i) {
    uint16_t value = output[i] * 0x3F; // 6bit
    if (value > 0x3F) value = 0x3F;
    result_buf[i] = (value << 5);  // Only Green
  }
  
  // 認識対象の横幅と横方向座標を取得
  bool err;
  int16_t s_sx, s_width;
  err = get_sx_and_width_of_region(output, DNN_WIDTH, DNN_HEIGHT, &s_sx, &s_width);
  
  // 認識対象の縦幅と縦方向座標を取得
  int16_t s_sy, s_height;
  int sx, width, sy, height;
  sx = width = sy = height = 0;
  err = get_sy_and_height_of_region(output, DNN_WIDTH, DNN_HEIGHT, &s_sy, &s_height);
  if (!err) {
    Serial.println("detection error");
    goto disp;
  }
  
  // 何も検出できなかった
  if (s_width == 0 || s_height == 0) {
    Serial.println("no detection");
    goto disp;
  }
  
  // 認証対象のボックスと座標をカメラ画像にあわせて拡大
  sx = s_sx * (CLIP_WIDTH/DNN_WIDTH) + OFFSET_X;
  width = s_width * (CLIP_WIDTH/DNN_WIDTH);
  sy = s_sy * (CLIP_HEIGHT/DNN_HEIGHT) + OFFSET_Y;
  height = s_height * (CLIP_HEIGHT/DNN_HEIGHT);
    
disp:
  img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);  
  uint16_t* buf = (uint16_t*)img.getImgBuff();  

  // 認識対象のボックスをカメラ画像に描画
  draw_box(buf, sx, sy, width, height); 
  
  // サイドバンドをフレームバッファに描画
  draw_sideband(buf, OFFSET_X, ILI9341_BLACK);
    
  // DNNRTへの入力画像をLCDの左上に表示
  display.drawRGBBitmap(0, 0, (uint16_t*)sbuf, DNN_HEIGHT, DNN_WIDTH);  
  // DNNRTの出力画像をLCDの右上に表示
  display.drawRGBBitmap(320-DNN_WIDTH, 0, result_buf, DNN_WIDTH, DNN_HEIGHT);  
  // ボックス描画されたカメラ画像を表示
  display.drawRGBBitmap(0, DNN_HEIGHT, buf, 320, 240-DNN_HEIGHT);
}


void setup() {

  Serial.begin(115200);
  display.begin();
  display.setRotation(3);
  display.fillRect(0, 0, 320, 240, ILI9341_BLUE);
  
  File nnbfile = SD.open("model.nnb");
  if (!nnbfile) {
    Serial.print("nnb not found");
    return;
  }

  Serial.println("DNN initialize");
  int ret = dnnrt.begin(nnbfile);
  if (ret < 0) {
    Serial.print("Runtime initialization failure. ");
    Serial.println(ret);
    return;
  }
  theCamera.begin();
  theCamera.startStreaming(true, CamCB);
}

void loop() {
  // put your main code here, to run repeatedly:

}
