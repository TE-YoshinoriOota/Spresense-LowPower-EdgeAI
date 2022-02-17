/*
 *  stresense_4led.ino - Simple 4 led example application
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

void setup() {
  // LED0～3の端子を出力に設定する
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
}

void loop() {
  digitalWrite(LED0, HIGH); // LED0をON(HIGH)
  delay(100); // 100ミリ秒待つ
  digitalWrite(LED1, HIGH); // LED1をON(HIGH) 
  delay(100); // 100ミリ秒待つ
  digitalWrite(LED2, HIGH); // LED2をON(HIGH) 
  delay(100); // 100ミリ秒待つ
  digitalWrite(LED3, HIGH); // LED3をON(HIGH) 
  delay(1000); // 1000ミリ秒（1秒）待つ
  digitalWrite(LED0, LOW); // LED0をOFF（LOW）
  delay(100); // 100ミリ秒待つ
  digitalWrite(LED1, LOW); // LED1をOFF（LOW）
  delay(100); // 100ミリ秒待つ
  digitalWrite(LED2, LOW); // LED2をOFF（LOW） 
  delay(100); // 100ミリ秒待つ
  digitalWrite(LED3, LOW); // LED3をOFF（LOW） 
  delay(1000); // 1000ミリ秒（1秒）待つ
}
