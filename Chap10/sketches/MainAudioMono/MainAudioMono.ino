/*
 *  MainAudioMono.ino - FFT sample
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

// モノラル、512サンプルでFFTを初期化
FFTClass<AS_CHANNEL_MONO, FFT_LEN> FFT;

AudioClass* theAudio = AudioClass::getInstance();


#include <MP.h>
#include <MPMutex.h>         // サブコア間の同期ライブラリ
MPMutex mutex(MP_MUTEX_ID0);
const int subcore = 1;       // サブコアの番号

void setup() {
  Serial.begin(115200);

  // ハミング窓、モノラル、オーバーラップ50%
  FFT.begin(WindowHamming, AS_CHANNEL_MONO, (FFT_LEN/2));

  Serial.println("Init Audio Recorder");
  theAudio->begin();
  // 入力をマイクに設定
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);
  // 録音設定：フォーマットはPCM (16ビットRAWデータ)、
  // DSPコーデックの場所の指定 (SDカード上のBINディレクトリ)、
  // サンプリグレート 16000Hz、モノラル入力  
  int err = theAudio->initRecorder(AS_CODECTYPE_PCM, 
    "/mnt/sd0/BIN", AS_SAMPLINGRATE_16000 ,AS_CHANNEL_MONO);
  if (err != AUDIOLIB_ECODE_OK) {
    Serial.println("Recorder initialize error");
    while(1);
  }
  
  Serial.println("Start Recorder");                                                     
  theAudio->startRecorder();  // 録音開始 

  MP.begin(subcore);  // サブコア開始
}

void loop() {
  static const uint32_t buffering_time = 
      FFT_LEN*1000/AS_SAMPLINGRATE_16000;
  static const uint32_t buffer_size = FFT_LEN*sizeof(int16_t);
  static char buff[buffer_size]; // 音声データを格納するバッファ
  static float pDst[FFT_LEN]; // FFT演算結果を格納するバッファ  
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
  FFT.get(pDst, 0); // MIC-Aのデータは0番

  if (mutex.Trylock() != 0) return;  // サブコア処理中のためリターン
  // サブコアにFFTデータの96サンプルを渡す
  int8_t sndid = 100;
  static const int disp_samples = 96; // サブコアに渡すサンプル数
  static float data[disp_samples]; // サブコアに渡すデータバッファ  
  memcpy(data, pDst, disp_samples*sizeof(float));
  ret = MP.Send(sndid, &data, subcore);  // サブコアにデータ送信
  if (ret < 0) MPLog("MP.Send Error\n");
  mutex.Unlock();  // MPMutex を解放
}
