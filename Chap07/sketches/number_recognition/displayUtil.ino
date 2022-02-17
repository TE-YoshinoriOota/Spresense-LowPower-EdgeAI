/*
 *  displayUtil.ino - Graphic procedure for number_recognition.ino
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

// ディスプレイの縦横の大きさ
#define DISPLAY_WIDTH  (320)
#define DISPLAY_HEIGHT  (240)

// 液晶ディスプレイの下部に文字列を表示する
void putStringOnLcd(String str, int color) {
  int len = str.length();
  display.fillRect(0,224, 320, 240, ILI9341_BLACK);
  display.setTextSize(2);
  int sx = 160 - len/2*12;
  if (sx < 0) sx = 0;
  display.setCursor(sx, 225);
  display.setTextColor(color);
  display.println(str);
}

// 液晶ディスプレイに四角形を描画する
void drawBox(uint16_t* imgBuf, int offset_x, int offset_y, int width, int height, int thickness, int color) {
  /* Draw target line */
  for (int x = offset_x; x < offset_x+width; ++x) {
    for (int n = 0; n < thickness; ++n) {
      *(imgBuf + DISPLAY_WIDTH*(offset_y+n) + x)          = color;
      *(imgBuf + DISPLAY_WIDTH*(offset_y+height-1-n) + x) = color;
    }
  }
  for (int y = offset_y; y < offset_y+height; ++y) {
    for (int n = 0; n < thickness; ++n) {
      *(imgBuf + DISPLAY_WIDTH*y + offset_x+n)           = color;
      *(imgBuf + DISPLAY_WIDTH*y + offset_x + width-1-n) = color;
    }
  } 
}
