/*
 *  flash_sample.ino - Test sketch for Spresense Flash library
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
#include <Flash.h>
#include <File.h>

File myFile;

void setup() {
  Serial.begin(115200);
  Flash.mkdir("dir/"); // ディレクトリを生成
  
  // 書き込みモードでオープン
  myFile = Flash.open("dir/test.txt", FILE_WRITE); 
  if (myFile) {
    myFile.println("testing 1,2,3…"); // 文字列を書き込む
    myFile.close();  // ファイルをクローズ
  }

  // 読み込みモードでオープン
  myFile = Flash.open("dir/test.txt"); 
  if (myFile) {
    // 読み込み可能なデータがある間は true を返す
    while (myFile.available()) { 
      Serial.write(myFile.read());  // 文字列の読み込む
    }
  }
  
  myFile.close();  // ファイルのクローズ

  // ファイルが存在していると true を返す
  if (Flash.exists("dir/test.txt")) { 
    Flash.remove("dir/test.txt");
  }
  Flash.rmdir("dir/");
}

void loop() {}
