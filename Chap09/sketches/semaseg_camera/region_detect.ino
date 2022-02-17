/*
 *  region_detect.ino - Region detect sample for the binary semantic segmentation sample
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


// ピクセルの連続値を記録する閾値
const float threshold = 0.8;

// 横方向の最大領域の開始座標(x)と幅を検出する
//   output: セマンティックセグメンテーションの認識結果
//   x: 認識結果の横幅
//   y: 認識結果の縦幅
//   s_sx: 最大領域の開始座標を得る
//   s_width: 最大領域の幅を得る
bool get_sx_and_width_of_region(DNNVariable &output, 
  int w, int h, int16_t* s_sx, int16_t* s_width) {
  
  // 引数チェック
  if (&output == NULL || w <= 0 || h <= 0) {
    Serial.println(String(__FUNCTION__) + ": param error");
    return false;
  }
  
  // マップ用のメモリを確保
  uint16_t *h_integ = (uint16_t*)malloc(w*h*sizeof(uint16_t));
  memset(h_integ, 0, w*h*sizeof(uint16_t));

  // 横方向の連続値のマップを作成
  float sum = 0.0;
  for (int i = 0; i < h; ++i) {
    int h_val = 0;
    for (int j = 0; j < w; ++j) {
      // ピクセルの出力が閾値より上の場合は加算
      if (output[i*w+j] > threshold) ++h_val;
      else h_val = 0; // 閾値以下は0リセット
      h_integ[i*w+j] = h_val; // マップに追加
    }
  }

  // マップの最大値を探し、最大領域の横幅と終端座標を得る
  int16_t max_h_val = -1;  // 最大幅（水平方向）を格納
  int16_t max_h_point = -1; // 最大幅の終了座標
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; ++j) {
      if (h_integ[i*w+j] > max_h_val) {
        max_h_val = h_integ[i*w+j]; // マップに格納された連続値
        max_h_point = j; // 座標値
      }
    }
  }
  *s_sx = max_h_point - max_h_val; // 開始座標(x)
  *s_width = max_h_val; // 最大領域の幅
  memset(h_integ, NULL, w*h*sizeof(uint16_t));  
  free (h_integ);
  if (*s_sx < 0) return false;
  return true;
}


// 縦方向の最大領域の開始座標(y)と高さを検出する
//   output: セマンティックセグメンテーションの認識結果
//   x: 認識結果の横幅
//   y: 認識結果の縦幅
//   s_sy: 最大領域の開始座標を得る
//   s_height: 最大領域の高さを得る
bool get_sy_and_height_of_region(DNNVariable &output, 
  int w, int h, uint16_t* s_sy, uint16_t* s_height) {
  
  // 引数チェック
  if (&output == NULL || w <= 0 || h <= 0) {
    Serial.println(String(__FUNCTION__) + ": param error");
    return false;
  }
  
  // マップ用のメモリを確保       
  uint16_t *v_integ = (uint16_t*)malloc(w*h*sizeof(uint16_t));
  memset(v_integ, 0, w*h*sizeof(uint16_t));

  // 縦方向の連続値のマップを作成
  for (int j = 0; j < w; ++j) {
    int v_val = 0;
    for (int i = 0; i < h; ++i) {
      // ピクセルの出力が閾値より上の場合は加算     
      if (output[i*w+j] > threshold) ++v_val;
      else v_val = 0;  // 閾値以下は0リセット
      v_integ[i*w+j] = v_val; // マップに追加
    }
  }
  
  // マップの最大値を探し、最大領域の縦幅と終端座標を得る
  int max_v_val = -1;  // 最大幅（高さ方向）を格納
  int max_v_point = -1;  // 最大高さの終了座標
  for (int j = 0; j < w; ++j) {
    for (int i = 0; i < h; ++i) {
      if (v_integ[i*w+j] > max_v_val) {
        max_v_val = v_integ[i*w+j];  // マップに格納された連続値
        max_v_point = i; // 座標値
      }
    }
  }  
  *s_sy = max_v_point - max_v_val; // 開始座標(y)
  *s_height = max_v_val; // 最大領域の高さ
  memset(v_integ, NULL, w*h*sizeof(uint16_t));
  free(v_integ);
  if (*s_sy < 0) return false;
  return true;
}
