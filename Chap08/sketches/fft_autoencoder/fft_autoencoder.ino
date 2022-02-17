/*
 *  fft_autoencoder.ino - Autoencoder sample for a pipe anomaly detection
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

#include <DNNRT.h>
DNNRT dnnrt;

#define FFT_LEN 1024

// モノラル、1024サンプルでFFTを初期化
FFTClass<AS_CHANNEL_MONO, FFT_LEN> FFT;

AudioClass *theAudio = AudioClass::getInstance();

void avgFilter(float dst[FFT_LEN]) {
  static const int avg_filter_num = 8;
  static float pAvg[avg_filter_num][FFT_LEN];
  static int g_counter = 0;
  if (g_counter == avg_filter_num) g_counter = 0;
  for (int i = 0; i < FFT_LEN; ++i) {
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
  // SDカードの入力を待つ
  while (!SD.begin()) {Serial.println("Insert SD card");};

  Serial.println("Initialize DNNRT");
  // SDカード上にある学習済モデルを読み込む
  File nnbfile = SD.open("model.nnb");;
  if (!nnbfile) {
    Serial.print("nnb not found");
    while(1);
  }
  // 学習済モデルでDNNRTを開始する
  int ret = dnnrt.begin(nnbfile);
  if (ret < 0) {
    Serial.print("DNN Runtime begin fail: " + String(ret));
    while(1);
  }
  // ハミング窓、モノラル、オーバーラップ50%
  FFT.begin(WindowHamming, AS_CHANNEL_MONO, (FFT_LEN/2));

  Serial.println("Init Audio Recorder");
  // 入力をマイクに設定  
  theAudio->begin();
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);
  // 録音設定：フォーマットはPCM (16ビットRAWデータ)、
  // DSPコーデックの場所の指定 (SDカード上のBINディレクトリ)、
  // サンプリグレート 48000Hz、モノラル入力  
  int err = theAudio->initRecorder(AS_CODECTYPE_PCM,
      "/mnt/sd0/BIN", AS_SAMPLINGRATE_48000 ,AS_CHANNEL_MONO);                             
  if (err != AUDIOLIB_ECODE_OK) {
    Serial.println("Recorder initialize error");
    while(1);
  }

  Serial.println("Start Recorder");
  theAudio->startRecorder(); // 録音開始
}


void loop(){
  static const uint32_t buffering_time = 
    FFT_LEN*1000/AS_SAMPLINGRATE_48000;
  static const uint32_t buffer_size = FFT_LEN*sizeof(int16_t);
  static const int ch_index = AS_CHANNEL_MONO-1;
  static char buff[buffer_size]; // 録音データを格納するバッファ
  static float pDst[FFT_LEN];  // FFT演算結果を格納するバッファ
  uint32_t read_size; 
  
  // マイクやファン、パイプ、マイクの状態で数値は大きく変動します
  // 実測をしてみて適切と思われる数値に設定してください
  static const float maxSpectrum = 1.5;  // FFT演算結果を見て調整
  static const float threshold = 1.0; // RSMEのばらつきを見て調整

  // buffer_sizeで要求されたデータをbuffに格納する
  // 読み込みできたデータ量は read_size に設定される
  int ret = theAudio->readFrames(buff, buffer_size, &read_size);
  if (ret != AUDIOLIB_ECODE_OK && 
      ret != AUDIOLIB_ECODE_INSUFFICIENT_BUFFER_AREA) {
    Serial.println("Error err = "+String(ret));
    theAudio->stopRecorder();
    exit(1);
  }
  
  if (read_size < buffer_size) {
    delay(buffering_time);
    return;
  }

  FFT.put((q15_t*)buff, FFT_LEN);  //FFTを実行
  FFT.get(pDst, 0);  // FFT演算結果を取得
  avgFilter(pDst); // 過去のFFT演算結果で平滑化
  
  // DNNRTの入力データにFFT演算結果を設定
  DNNVariable input(FFT_LEN/8); 
  float *dnnbuf = input.data();
  for (int i = 0; i < FFT_LEN/8; ++i) {
    pDst[i] /= maxSpectrum;  // 0.0~1.0に正規化
    dnnbuf[i] = pDst[i];
  }
  // 推論を実行
  dnnrt.inputVariable(input, 0);
  dnnrt.forward(); 
  DNNVariable output = dnnrt.outputVariable(0);
  // 二乗平均平方根誤差(RSME)を計算
  float sqr_err = 0.0;
  for (int i = 0; i < FFT_LEN/8; ++i) {
    float err = pDst[i] - output[i];
    sqr_err += sqrt(err*err/(FFT_LEN/8));
  } 
 
  // RSMEの結果を平均化
  static const int delta_average = 16; // 平均回数
  static float average[delta_average];
  static uint8_t gCounter = 0;
  
  average[gCounter++] = sqr_err;
  if (gCounter == delta_average) gCounter = 0;
  float avg_err = 0.0;
  for (int i = 0; i < delta_average; ++i) {
    avg_err += average[i];
  }
  avg_err /= delta_average;
  Serial.println("Result: " + String(avg_err, 7));

  // 閾値でOK/NGを判定
  bool bNG = false;
  avg_err > threshold ? bNG = true : bNG = false;
  if (bNG) Serial.println("Fault on the pipe");

}
