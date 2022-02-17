/*
 *  Spresense_gesture_collector.ino - Gesture data collection sample
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
#include <float.h>
#include <Flash.h>

// センサー出力更新周期: 25, 50, 100, 200, 400, 800, 1600 (Hz)
const int sense_rate = 200;
// ジャイロセンサ範囲: 125, 250, 500, 1000, 2000 (deg/s)
const int gyro_range = 500;
// 加速度センサ範囲: 2, 4, 8, 16 (G)
const int accl_range = 2;
// Complementary Filter設定値
const float alpha = 0.94;

float rpm_roll[sense_rate];
float rpm_pitch[sense_rate];

// センサー出力値が16bit整数(±32768)のため浮動小数点に変換する
inline float convertRawGyro(int gRaw) {
  return gyro_range * ((float)(gRaw)/32768.0);}
inline float convertRawAccel(int aRaw) {
  return accl_range *((float)(aRaw)/32768.0);}

// ラジアン(rad)から角度(°)に変換 
inline float rad_to_deg(float r) {return r*180./M_PI;}

// 角度速度をRPM(revolutions per minute)に変換
inline float angv_to_rpm(float avel) {
  return avel*60/360.; /* 60s/360deg */
}

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
  BMI160.setAccelerometerRate(sense_rate);  /*200Hz*/
}

void loop() {
  static unsigned long last_msec = 0;
  int rollRaw, pitchRaw, yawRaw;   // ジャイロセンサー出力値
  int accxRaw, accyRaw, acczRaw;   // 加速度センサー出力値 
  static float gyro_roll = 0;  // ジャイロによるロール角度
  static float gyro_pitch = 0; // ジャイロによるピッチ角度
  static float gyro_yaw = 0;   // ジャイロによるヨー角度
  static float cmp_roll = 0;  // Complementary Filter によるロール角度
  static float cmp_pitch = 0; // Complementary Filter によるピッチ角度
  static float last_cmp_roll = 0;
  static float last_cmp_pitch = 0;

  // 経過時間を測定
  unsigned long curr_msec = millis();
  float dt = (float)(curr_msec - last_msec)/1000.0;
  last_msec = curr_msec;
  if (dt > 0.1 || dt == 0.) return;
  
  // ジャイロと加速度の値をセンサーから取得
  BMI160.readGyro(rollRaw, pitchRaw, yawRaw);
  BMI160.readAccelerometer(accxRaw, accyRaw, acczRaw);

  // 取得値を16bit整数から浮動小数点に変換
  float omega_roll  = convertRawGyro(rollRaw);
  float omega_pitch = convertRawGyro(pitchRaw);
  float omega_yaw   = convertRawGyro(yawRaw); 
  float accel_x = convertRawAccel(accxRaw);
  float accel_y = convertRawAccel(accyRaw);
  float accel_z = convertRawAccel(acczRaw);

  float acc_roll  = rad_to_deg(atan2(accel_y, accel_z));
  float acc_pitch = rad_to_deg(atan2(-accel_x, sqrt(accel_y*accel_y+accel_z*accel_z)));

  // Complemetary Filter
  cmp_roll = alpha*(cmp_roll+omega_roll*dt)
          + (1-alpha)*acc_roll;
  cmp_pitch = alpha*(cmp_pitch+omega_pitch*dt)
          + (1-alpha)*acc_pitch;

  float rpm_cmp_roll  = angv_to_rpm(
    (cmp_roll - last_cmp_roll)/dt);
  float rpm_cmp_pitch = angv_to_rpm(
    (cmp_pitch - last_cmp_pitch)/dt);
  last_cmp_roll  = cmp_roll;
  last_cmp_pitch = cmp_pitch;  

  // バッファを確保
  static float rpm_roll[sense_rate];
  static float rpm_pitch[sense_rate];

  // データをシフト
  for (int n = 1; n < sense_rate; ++n) {
    rpm_roll[n-1]  = rpm_roll[n];
    rpm_pitch[n-1] = rpm_pitch[n];    
  }
  // 最新の計測データを最後尾に追加
  rpm_roll[sense_rate-1] = rpm_cmp_roll;
  rpm_pitch[sense_rate-1] = rpm_cmp_pitch;

  // トリガーを設定
  const float threshold = 80.0; /*回転速度(80rpm)*/
  const int point = 20; /*測定ポイント(20)*/
  if ((abs(rpm_roll[point-2]) < threshold 
      && abs(rpm_roll[point]) > threshold)
   || (abs(rpm_pitch[point-2]) < threshold 
      && abs(rpm_pitch[point]) > threshold)) {

    static int g_loop = 0; // ファイルに付与する追番
    // 最初のデータはセンサー起動直後のデータを拾うため捨てる
    if (!g_loop++) return; 

    // 1.0~-1.0 の値になるように値を正規化
    float pmax = FLT_MIN;
    float pmin = FLT_MAX;
    for (int n = 0; n < sense_rate; ++n) {
      if (rpm_pitch[n] > pmax) pmax = rpm_pitch[n];
      if (rpm_pitch[n] < pmin) pmin = rpm_pitch[n];
    }
    // 正負の最大振幅を振幅として正規化
    float range = abs(pmax) > abs(pmin) ? 
                  abs(pmax) : abs(pmin);
    for (int n = 0; n < sense_rate; ++n) {
      rpm_pitch[n] = rpm_pitch[n]/range;
    }

    char fname[16];
    sprintf(fname, "dt%03d.csv", g_loop); // ファイル名生成
    // すでにファイルがあったら削除
    if (Flash.exists(fname)) Flash.remove(fname); 
    File myFile = Flash.open(fname, FILE_WRITE);
    if (!myFile) {
      Serial.println("File Open Error: "+String(fname));
      return; 
    }
    // RPMの履歴をファイルに出力
    for (int n = 0; n < sense_rate; ++n) {
      myFile.println(String(rpm_pitch[n],6));
      Serial.println(String(rpm_pitch[n],6));      
    }
    myFile.close();
    // 多重検出を防ぐためバッファをゼロクリア
    memset(rpm_roll,  0, sense_rate*sizeof(float));
    memset(rpm_pitch, 0, sense_rate*sizeof(float));    
  }
  delay(1000/sense_rate);
}
