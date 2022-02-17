/*
 *  displayUtil.ino - Graphic procedure for FFT data display
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

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#define TFT_DC  9
#define TFT_CS  10
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);

// テキストの定義とテキストの表示位置
#define TX 35
#define TY 210
#define APP_TITLE "FFT Spectrum Analyzer"

// グラフの幅と高さ（縦向きなのに注意）
#define GRAPH_WIDTH  (200) 
#define GRAPH_HEIGHT (320) 

// グラフの描画開始位置
#define GX (40)
#define GY (0)

// NGマークの描画開始位置と四角の幅
#define NG_X (GRAPH_WIDTH -30)
#define NG_Y (GRAPH_HEIGHT-30)
#define NG_W (20)

// 描画するデータの長さ
#define SAMPLES (128)

void setupLcd() {
  display.begin();
  display.setRotation(3);  // ディスプレイ向きを横(3)に設定
  display.fillScreen(ILI9341_BLACK); // 黒に塗りつぶし
  display.setCursor(TX, TY); // テキスト描画開始位置を設定
  display.setTextColor(ILI9341_DARKGREY); // テキストを灰色に設定
  display.setTextSize(2);  // テキストの大きさを設定
  display.println(APP_TITLE); // テキスト描画
  display.setRotation(2); // ディスプレイ向きを縦(2)に設定
}


void showSpectrum(float *data, bool bNG) {
  static uint16_t frameBuf[GRAPH_HEIGHT][GRAPH_WIDTH];
  int val;
  for (int i = 0; i < GRAPH_HEIGHT; ++i) {
    // サンプル数までしか描画しない
    i < SAMPLES ? val = data[i]*GRAPH_WIDTH+1 : val = 0;
    val = (val > GRAPH_WIDTH) ? GRAPH_WIDTH: val;
    for (int j = 0; j < GRAPH_WIDTH; ++j) {
      // 値以下は白を描画、値より上はグレイ（背景）を描画
      frameBuf[i][j] = (j > val) ? 
          ILI9341_DARKGREY : ILI9341_WHITE;

      // 異常判定時に赤い四角を描画
      if (bNG) {
        if ((i > NG_Y) && (i < (NG_Y + NG_W))
         && (j > NG_X) && (j < (NG_X + NG_W)))
           frameBuf[i][j] = ILI9341_RED;
      }
    }
  }
  // ディスプレイにグラフを転送
  display.drawRGBBitmap(GX, GY, 
    (uint16_t*)frameBuf, GRAPH_WIDTH, GRAPH_HEIGHT);
  return;
}
