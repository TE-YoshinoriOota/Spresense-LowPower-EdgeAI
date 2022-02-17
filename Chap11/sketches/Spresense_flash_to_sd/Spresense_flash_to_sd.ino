/*
 *  Spresense_flash_to_sd.ino - Copy data from flash to SD card sample
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

#include <SDHCI.h>
#include <Flash.h>
SDClass SD;

void setup() {
  Serial.begin(115200);
  while (!SD.begin()) { Serial.println("Insert SD Card"); }
  
  File flash_root = Flash.open("/");
  while (true) {
    // ルートディレクトリにある次のファイルをオープン
    File from = flash_root.openNextFile();
    if (!from) break; // ファイルがない場合処理終了
    //ディレクトリはスキップ
    if (from.isDirectory()) continue; 
    String str(from.name());
    // ファイル名からパス(/mnt/spif/)を削除
    str = str.substring(11); 
    // CSVファイルでない場合はスキップ
    if (!str.endsWith(".csv")) continue;
    Serial.println(str);
    // SDカード上の同一名のファイルを削除
    if (SD.exists(str)) SD.remove(str);
    // SDカード保存用のファイルをオープン
    File to = SD.open(str, FILE_WRITE);
    while (from.available()) {
      to.write(from.read()); // データコピー
    }
    Serial.println("Copied " + str);
    from.close(); // フラッシュROM上のファイルをクローズ
    Flash.remove(str); // フラッシュROM上のファイルを削除
    to.close(); // SDカードのファイルをクローズ
  }
  Serial.println("All files copied");
}

void loop() {}
