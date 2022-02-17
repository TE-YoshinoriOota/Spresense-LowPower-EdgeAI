/*
 *  mic_mp3_test.ino - Simple recording example application
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
#include <SDHCI.h>

SDClass SD;
File myFile;
AudioClass *theAudio = AudioClass::getInstance();

static const int32_t recording_time_ms = 10000; /* 10秒 */
static int32_t start_time_ms;

void setup() {
  Serial.begin(115200);
  while (!SD.begin()){ Serial.println("Insert SD card."); }
  
  theAudio->begin();
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);
  theAudio->initRecorder(AS_CODECTYPE_MP3, "/mnt/sd0/BIN"
                   , AS_SAMPLINGRATE_48000, AS_CHANNEL_MONO);
                   
  if (SD.exists("Sound.mp3")) {
    SD.remove("Sound.mp3");
  }
  
  myFile = SD.open("Sound.mp3", FILE_WRITE);
  if (!myFile) {
    Serial.println("File open error\n");
    while(1);
  }
  
  theAudio->startRecorder();
  start_time_ms = millis();
  Serial.println("Start Recording");
}

void loop() {
  uint32_t duration_ms = millis() - start_time_ms;
  err_t err = theAudio->readFrames(myFile);
  
  if (duration_ms > recording_time_ms 
    || err != AUDIOLIB_ECODE_OK) {
    theAudio->stopRecorder();
    theAudio->closeOutputFile(myFile);
    theAudio->setReadyMode();
    theAudio->end();
    Serial.println("End Recording");
    while(1);
  }
}
