/*
 *  fft_datacollection.ino - FFT data collection sample for Autoeconder systems
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

#include <Audio.h>
#include <FFT.h>
#include <SDHCI.h>
SDClass SD;

#define FFT_LEN 1024

// モノラル、1024サンプルでFFTを初期化
FFTClass<AS_CHANNEL_MONO, FFT_LEN> FFT;

AudioClass* theAudio = AudioClass::getInstance();


void avgFilter(float dst[FFT_LEN]) {
  static const int avg_filter_num = 8;
  static float pAvg[avg_filter_num][FFT_LEN/2];
  static int g_counter = 0;
  if (g_counter == avg_filter_num) g_counter = 0;
  for (int i = 0; i < FFT_LEN/2; ++i) {
    pAvg[g_counter][i] = dst[i];
    float sum = 0;
    for (int j = 0; j < avg_filter_num; ++j) {
      sum += pAvg[j][i];
    }
    dst[i] = sum / avg_filter_num;
  }
  ++g_counter;
}

void setup() {
  Serial.begin(115200);
  
  // SDカードの挿入を待つ
  while (!SD.begin()) {Serial.println("Insert SD card");};

  // ハミング窓、モノラル、オーバーラップ50%
  FFT.begin(WindowHamming, AS_CHANNEL_MONO, (FFT_LEN/2));

  Serial.println("Init Audio Recorder");
  theAudio->begin();
  // 入力をマイクに設定
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);
  // 録音設定：フォーマットはPCM (16ビットRAWデータ)、
  // DSPコーデックの場所の指定 (SDカード上のBINディレクトリ)、
  // サンプリグレート 48000Hz、モノラル入力
  int err = theAudio->initRecorder(AS_CODECTYPE_PCM 
    ,"/mnt/sd0/BIN", AS_SAMPLINGRATE_48000, AS_CHANNEL_MONO);                             
  if (err != AUDIOLIB_ECODE_OK) {
    Serial.println("Recorder initialize error");
    while(1);
  }

  Serial.println("Start Recorder");
  theAudio->startRecorder(); // 録音開始
}


void loop() {
  static const uint32_t buffering_time = 
      FFT_LEN*1000/AS_SAMPLINGRATE_48000;
  static const uint32_t buffer_size = FFT_LEN*sizeof(int16_t);
  static const int ch_index = AS_CHANNEL_MONO-1;
  static char buff[buffer_size];
  static float pDst[FFT_LEN];
  uint32_t read_size;

  // buffer_sizeで要求されたデータをbuffに格納する
  // 読み込みできたデータ量は read_size に設定される
  int ret = theAudio->readFrames(buff, buffer_size, &read_size);
  if (ret != AUDIOLIB_ECODE_OK && 
      ret != AUDIOLIB_ECODE_INSUFFICIENT_BUFFER_AREA) {
    Serial.println("Error err = "+String(ret));
    theAudio->stopRecorder();
    while(1);
  }

  if (read_size < buffer_size) {
    delay(buffering_time);
    return;
  }
  
  FFT.put((q15_t*)buff, FFT_LEN);  //FFTを実行
  FFT.get(pDst, 0);  // FFT演算結果を取得
  avgFilter(pDst); // 過去のFFT演算結果で平滑化

  static uint32_t last_capture_time = 0;
  uint32_t capture_interval = millis() - last_capture_time;
  // 1秒経過したら記録する
  if (capture_interval > 1000) {
    theAudio->stopRecorder(); // 録音停止
    // saveData関数：SDカードにデータを記録  
    //   データへのポインタ（pDst）
    //   記録データのサイズ（FFT_LEN/8）
    //   データ保存数（100）
    saveData(pDst, FFT_LEN/8, 100); 
    theAudio->startRecorder(); // 録音再開
    last_capture_time = millis();
  } 
}

// SDカードにデータを記録
void saveData(float* pDst, int dsize, int quantity) {
  static int gCounter = 0;  // ファイル名につける追番
  char filename[16] = {};

  // 指定された保存数以上に達したら何もせずにリターン
  if (gCounter > quantity) {
    Serial.println("Data accumulated");
    return;
  }

  // データ保存用ファイルを開く
  sprintf(filename, "data%03d.csv", gCounter++);
  // すでにファイルがあったら削除する
  if (SD.exists(filename)) SD.remove(filename);
  // ファイルをオープン
  File myFile = SD.open(filename, FILE_WRITE);
  // データの書き込み
  for (int i = 0; i < dsize; ++i) {
    myFile.println(String(pDst[i],6));
  }
  myFile.close();  // ファイルをクローズ
  Serial.println("Data saved as " + String(filename));
}
