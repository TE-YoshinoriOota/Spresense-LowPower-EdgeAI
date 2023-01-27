/*
 *  dispUtil.ino - Graphic procedure for voice command recognition
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


#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#define TFT_RST 8
#define TFT_DC  9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

// テキスト表示位置
#define TX (35)
#define TY (210)

// グラフの幅（周波数）と高さ（時間）：縦向きなのに注意）
#define SPECTRO_GRAPH_FREQ (96)
#define SPECTRO_GRAPH_TIME (320)

// グラフの描画位置
#define GRAPH_OFFSET (35)

// フレームバッファを確保
uint16_t frameBuffer[SPECTRO_GRAPH_TIME*SPECTRO_GRAPH_FREQ];
// メインコアから渡されるFFTサンプル数
const int disp_samples = 96;  

void setupLcd() {
  tft.begin();
  tft.setRotation(3); // ディスプレイ向きを横(3)に設定
  tft.fillScreen(ILI9341_BLACK); // 黒に塗りつぶし
  tft.setCursor(TX, TY); // テキスト描画開始位置を設定
  tft.setTextColor(ILI9341_WHITE); // テキストを白色に設定
  tft.setTextSize(2); // テキストの大きさを設定
  tft.println("FFT Spectrogram Viewer"); // テキスト描画
  tft.setRotation(2); // ディスプレイ向きを縦(2)に設定
  memset(frameBuffer, 0
       , SPECTRO_GRAPH_FREQ*SPECTRO_GRAPH_TIME*sizeof(uint16_t));
}

// スペクトログラムを描画
void displayLcd(float *data) {

  // グラフ全体をシフト
  uint16_t* top = frameBuffer;
  for (int t = 1; t < SPECTRO_GRAPH_TIME; ++t) {
    uint16_t* bf0 = top+(t-1)*SPECTRO_GRAPH_FREQ;
    uint16_t* bf1 = top+(t)  *SPECTRO_GRAPH_FREQ;
    memcpy(bf0, bf1, SPECTRO_GRAPH_FREQ*sizeof(uint16_t));
  }

  // フレームバッファに最新のFFT演算結果を反映
  uint16_t val_6, val_5, val16;  
  for (int f = 0; f < disp_samples; ++f) {
    float val = data[f] < 0 ? 0 : data[f];
    val_6 = (uint16_t)(val*64) >= 64 ? 64 : (uint16_t)(val*64);
    val_5 = (uint16_t)(val*32) >= 32 ? 32 : (uint16_t)(val*32);
    val16 = val_5 << 11 | val_6 << 5 | val_5;     
    frameBuffer[(SPECTRO_GRAPH_TIME-1)*SPECTRO_GRAPH_FREQ + f] = val16;
  }
  
  // ディスプレイに転送
  tft.drawRGBBitmap(GRAPH_OFFSET, 0, frameBuffer
                  , SPECTRO_GRAPH_FREQ, SPECTRO_GRAPH_TIME);

  // 認識領域のボックスを描画
  const int recog_time_area = 32;
  tft.drawRect(GRAPH_OFFSET, SPECTRO_GRAPH_TIME-recog_time_area
              ,SPECTRO_GRAPH_FREQ, recog_time_area, ILI9341_RED);
}

//　認識結果の表示
void displayResult(int index, float value) {
  const int text_x = 80;
  const int text_y = 70;
  const int text_w = 150;
  const int text_h = 30;
  const char label[3][10] = {"end:   ", "next:  ", "start: "};
  tft.setRotation(3); // ディスプレイの描画方向を変更
  tft.fillRect(text_x, text_y, text_w, text_h, ILI9341_BLACK);
  tft.setCursor(text_x, text_y);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.print(String(label[index]) + String(value));
  tft.setRotation(2);
}
