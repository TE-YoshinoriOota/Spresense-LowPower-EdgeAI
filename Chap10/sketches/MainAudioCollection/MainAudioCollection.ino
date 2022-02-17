/*
 *  MainAudioCollection.ino - Spectrogram image collector for voice command systems
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

#ifdef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <Audio.h>
#include <FFT.h>

#define FFT_LEN 512

// ステレオ、512サンプルでFFTを初期化
FFTClass<AS_CHANNEL_STEREO, FFT_LEN> FFT;

AudioClass* theAudio = AudioClass::getInstance();

#include <MP.h>
#include <MPMutex.h>          // サブコア間の同期ライブラリ
MPMutex mutex(MP_MUTEX_ID0);
const int subcore = 1;        // サブコアの番号

#include <SDHCI.h>
SDClass SD;
File myFile;

#include <float.h> // FLT_MAX, FLT_MINの定義ヘッダ
#include <BmpImage.h>
BmpImage bmp;


void setup() {
  Serial.begin(115200);
  // SDカードの挿入を待つ
  while (!SD.begin()) {Serial.println("insert SD card");} 

  // ハミング窓、ステレオ、オーバーラップ50%  
  FFT.begin(WindowHamming, AS_CHANNEL_STEREO, (FFT_LEN/2));
  
  Serial.println("Init Audio Recorder");
  theAudio->begin();
  // 入力をマイクに設定
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);
  // 録音設定：フォーマットはPCM (16ビットRAWデータ)、
  // DSPコーデックの場所の指定 (SDカード上のBINディレクトリ)、
  // サンプリグレート 16000Hz、ステレオ入力   
  int err = theAudio->initRecorder(AS_CODECTYPE_PCM, 
    "/mnt/sd0/BIN", AS_SAMPLINGRATE_16000, AS_CHANNEL_STEREO);
    if (err != AUDIOLIB_ECODE_OK) {
    Serial.println("Recorder initialize error");
    while(1);
  }

  Serial.println("Start Recorder");                                 
  theAudio->startRecorder(); // 録音開始

  MP.begin(subcore); // サブコア開始
}

void loop() {
  static const uint32_t buffering_time = 
      FFT_LEN*1000/AS_SAMPLINGRATE_16000;
  static const uint32_t buffer_size = 
      FFT_LEN*sizeof(int16_t)*AS_CHANNEL_STEREO;
  static char buff[buffer_size]; // 音声データを格納するバッファ
  static float pDstFG[FFT_LEN];  // MIC-A FFT演算結果を格納するバッファ
  static float pDstBG[FFT_LEN];  // MIC-B FFT演算結果を格納するバッファ
  static float pDst[FFT_LEN/2];  // MIC-A/B の差分を格納するバッファ   
  uint32_t read_size; 
  int ret;

  // buffer_sizeで要求されたデータをbuffに格納する
  // 読み込みできたデータ量は read_size に設定される   
  ret = theAudio->readFrames(buff, buffer_size, &read_size);
  if (ret != AUDIOLIB_ECODE_OK && 
      ret != AUDIOLIB_ECODE_INSUFFICIENT_BUFFER_AREA) {
    Serial.println("Error err = " + String(ret));
    theAudio->stopRecorder();
    while(1);
  }
  
  // 読み込みサイズがbuffer_sizeに満たない場合    
  if (read_size < buffer_size) {
    delay(buffering_time); // データが蓄積されるまで待つ
    return;
  } 
  
  FFT.put((q15_t*)buff, FFT_LEN); // FFTを実行
  FFT.get(pDstFG, 0);  //MIC-A(音声用)のデータは0番
  FFT.get(pDstBG, 1);  //MIC-B(外部用)のデータは1番
  
  // MIC-A/BのFFT演算結果の差分を計算
  const float alpha = 0.8;
  for (int f = 0;  f < FFT_LEN/2; ++f)  {
    float fval = pDstFG[f] - alpha*pDstBG[f];
    pDst[f] = fval < 0. ? 0. : fval;
  }
  averageSmooth(pDst); // データをスムース化

  // 音圧データのヒストグラム用バッファ
  static const int frames =  32;
  static float hist[frames];
  // スペクトログラム用バッファ
  static const int fft_samples = 96; // 3000Hz
  static float spc_data[frames*fft_samples];
   
  // ヒストグラムとスペクトログラムのデータをシフト
  for (int t = 1; t < frames; ++t) {
    float* sp0 = spc_data+(t-1)*fft_samples;
    float* sp1 = spc_data+(t  )*fft_samples;
    memcpy(sp0, sp1, fft_samples*sizeof(float));
    hist[t-1] = hist[t];
  } 

  // 背景ノイズ低減後の音圧レベルの合計
  float sound_power_nc =  0; 
  for (int f = 0; f < FFT_LEN/2; ++f) {
    sound_power_nc += pDst[f];
  }
  
  // 最新の音圧レベルデータをヒストグラムに追加
  hist[frames-1] = sound_power_nc;  
  // 最新のFFT演算結果をスペクトログラムに追加
  float* sp_last = spc_data + (frames-1)*fft_samples;
  memcpy(sp_last, pDst, fft_samples*sizeof(float));

  // 音声閾値、静寂閾値を設定
  const float sound_th = 70;
  const float silent_th = 10;
  float pre_area = 0;
  float post_area = 0;
  float target_area = 0;
  // 前半250msec、中央500msec、後半250msecの音圧レベルを合計
  for (int t = 0; t < frames; ++t) {
    if (t < frames/4) pre_area += hist[t];
    else if (t >= frames/4 && t < frames*3/4) target_area += hist[t];
    else if (t >= frames*3/4) post_area += hist[t];
  }

  // 前後半250ミリ秒が静寂閾値未満で
  // 中央500ミリ秒が音声閾値以上か判定
  if (pre_area < silent_th && target_area >= sound_th && post_area < silent_th) {
    // 多重判定にならないようヒストグラムをリセット
    memset(hist, 0, frames*sizeof(float)); 
    
    uint8_t bmp_data[frames/2*fft_samples/2]; // BMP用バッファ

    // 正規化のために最大値、最小値を算出
    float spmax = FLT_MIN;
    float spmin = FLT_MAX;
    for (int n = 0; n < frames*fft_samples; ++n) {
      if (spc_data[n] > spmax) spmax = spc_data[n];
      if (spc_data[n] < spmin) spmin = spc_data[n];
    }
    
    // 周波数：横軸 x 時間：縦軸 を 時間:横軸 x 周波数：縦軸に変換
    int bf = fft_samples/2-1;
    for (int f = 0; f < fft_samples; f += 2) {
      int bt = 0;
      for (int t = frames/4; t < frames*3/4; ++t) {
        // 正規化
        float val0 = (spc_data[fft_samples*t+f]-spmin)
                    /(spmax-spmin);
        float val1 = (spc_data[fft_samples*t+f+1]-spmin)
                    /(spmax-spmin);
        float val = (val0 + val1)/2*255.;
        val = val <  0  ?  0  : val; 
        val = val > 255 ? 255 : val;
        bmp_data[frames/2*bf+bt] = (uint8_t)val;
        ++bt;
      }
      --bf;
    }
 
    // SDカード書き込みのためにレコーダーを一時停止
    theAudio->stopRecorder();

    static int n = 0; // ファイル名につける追番
    char fname[16]; memset(fname, 0, 16);
    sprintf(fname, "%03d.bmp", n++); // ファイル名生成
    if (SD.exists(fname)) SD.remove(fname); // ファイルがあったら削除
    File myFile = SD.open(fname, FILE_WRITE); // ファイルオープン
    // BMPデータを作成
    bmp.begin(BmpImage::BMP_IMAGE_GRAY8, 
      frames/2, fft_samples/2, bmp_data);
    myFile.write(bmp.getBmpBuff(), bmp.getBmpSize());
    myFile.close();
    bmp.end();
    Serial.println("save image as " + String(fname));

    // レコーダーを再開
    theAudio->startRecorder();
  }

  if (mutex.Trylock() != 0) return;  // subcore 処理中のためリターン
  int8_t sndid = 100;
  static const int disp_samples = 96; // サブコアに渡すサンプル数
  static float data[disp_samples]; // サブコアに渡すデータバッファ   
  memcpy(data, pDst, disp_samples*sizeof(float));
  ret = MP.Send(sndid, &data, subcore); // サブコアにデータ送信
  if (ret < 0) MPLog("FFT data Send Error\n");
  mutex.Unlock(); // MPMutexを解放
}

void averageSmooth(float* dst) {
  const int array_size = 4;
  static float pArray[array_size][FFT_LEN/2];
  static int g_counter = 0;
  if (g_counter == array_size) g_counter = 0;
  for (int i = 0; i < FFT_LEN/2; ++i) {
    pArray[g_counter][i] = dst[i];
    float sum = 0;
    for (int j = 0; j < array_size; ++j) {
      sum += pArray[j][i];
    }
    dst[i] = sum / array_size;
  }
  ++g_counter;
}
