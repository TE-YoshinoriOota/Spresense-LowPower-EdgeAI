/*
 *  Spresense_BMI160.ino - BMI160 sample
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

#include <BMI160Gen.h>

// センサー出力更新周期: 25,50,100,200,400,800,1600 (Hz)
const int sense_rate = 200;
// ジャイロセンサ範囲: 125, 250,500,1000,2000 (deg/s)
const int gyro_range = 500;
// 加速度センサ範囲: 2,4,8,16 (G)
const int accl_range = 2;
// Complementary Filter設定値
const float alpha = 0.94;

// センサー出力値の16bit整数(±32768)を浮動小数点に変換
inline float convertRawGyro(int gRaw) {
  return gyro_range * ((float)(gRaw)/32768.0);}
inline float convertRawAccel(int aRaw) {
  return accl_range *((float)(aRaw)/32768.0);}

// ラジアン(rad)から角度(°)に変換
inline float rad_to_deg(float r) { return r*180./M_PI; }

void setup() {
  Serial.begin(115200);

  // I2CモードでBMI160を開始
  BMI160.begin(BMI160GenClass::I2C_MODE);
  // ジャイロセンサーの範囲を設定
  BMI160.setGyroRange(gyro_range);
  // 加速度センサーの範囲を設定
  BMI160.setAccelerometerRange(accl_range);
  // ジャイロセンサーの更新周期を設定
  BMI160.setGyroRate(sense_rate);
  // 加速度センサーの更新周期を設定
  BMI160.setAccelerometerRate(sense_rate);  // 200Hz
  
  Serial.println("gyro,accel,comp");
}

void loop() {
  static unsigned long last_msec = 0;
  int rollRaw, pitchRaw, yawRaw; // ジャイロセンサー出力値
  int accxRaw, accyRaw, acczRaw; // 加速度センサー出力値
  static float gyro_roll = 0; // ジャイロによるロール角度
  static float gyro_pitch = 0; // ジャイロによるピッチ角度
  static float gyro_yaw = 0; // ジャイロによるヨー角度
  static float cmp_roll = 0; // フィルターによるロール角度
  static float cmp_pitch = 0; // フィルターによるピッチ角度

  // 経過時間を測定
  unsigned long curr_msec = millis();
  float dt = (float)(curr_msec - last_msec)/1000.0;
  last_msec = curr_msec;
  if (dt > 0.1) return;
  
  // ジャイロと加速度の値をセンサーから取得
  BMI160.readGyro(rollRaw, pitchRaw, yawRaw);
  BMI160.readAccelerometer(accxRaw, accyRaw, acczRaw);

  // 取得値を16bit整数から浮動小数点に変換
  float omega_roll  = convertRawGyro(rollRaw);
  float omega_pitch = convertRawGyro(pitchRaw);
  float accel_x = convertRawAccel(accxRaw);
  float accel_y = convertRawAccel(accyRaw);
  float accel_z = convertRawAccel(acczRaw);

  // ジャイロによる角度を算出
  gyro_roll  += omega_roll*dt;
  gyro_pitch += omega_pitch*dt;

  float acc_roll  = rad_to_deg(atan2(accel_y, accel_z));
  float acc_pitch = rad_to_deg(
    atan2(-accel_x, sqrt(accel_y*accel_y+accel_z*accel_z)));

  // Complementary Filterによるロール・ピッチ算出
  cmp_roll = alpha*(cmp_roll+omega_roll*dt)
             + (1-alpha)*acc_roll;
  cmp_pitch = alpha*(cmp_pitch+omega_pitch*dt)
             + (1-alpha)*acc_pitch;

  Serial.println(String(gyro_roll,6)+","
               + String(acc_roll,6)+","
               + String(comp_roll,6));
  delay(1000/sense_rate); // データ出力まで待つ
}
