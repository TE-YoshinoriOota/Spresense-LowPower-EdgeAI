/*
 *  displayUtil.ino - Graphic procedure for binary the binary semantic segmentation sample
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

#define IMG_WIDTH  (320)
#define IMG_HEIGHT  (240)
#define THICKNESS (5)

void draw_sideband(uint16_t* buf, int thickness, int color) {
  for (int i = 0; i < IMG_HEIGHT; ++i) {
    for (int j = 0; j < thickness; ++j) {
      buf[i*IMG_WIDTH + j] = color;
      buf[i*IMG_WIDTH + IMG_WIDTH-j-1] = color;
    }
  } 
}

bool draw_box(uint16_t* buf, int sx, int sy, int w, int h) {
  const int thickness = 4; // BOXの線の太さ
  
  if (sx < 0 || sy < 0 || w < 0 || h < 0) { 
    Serial.println("draw_box parameter error");
    return false;
  }
  if (sx+w >= OFFSET_X+CLIP_WIDTH) 
    w = OFFSET_X+CLIP_WIDTH-sx-1;
  if (sy+h >= OFFSET_Y+CLIP_HEIGHT) 
    h = OFFSET_Y+CLIP_HEIGHT-sy;
  
  /* draw the horizontal line of the square */
  for (int j = sx; j < sx+w; ++j) {
    for (int n = 0; n < thickness; ++n) {
      buf[(sy+n)*IMG_WIDTH + j] = ILI9341_RED;
      buf[(sy+h-n)*IMG_WIDTH + j] = ILI9341_RED;
    }
  }
  /* draw the vertical line of the square */
  for (int i = sy; i < sy+h; ++i) {
    for (int n = 0; n < thickness; ++n) { 
      buf[i*IMG_WIDTH+sx+n] = ILI9341_RED;
      buf[i*IMG_WIDTH+sx+w-n] = ILI9341_RED;
    }
  }
  return true;
}
