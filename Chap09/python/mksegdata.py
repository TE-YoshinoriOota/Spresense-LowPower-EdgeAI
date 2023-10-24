import os
import argparse
import random
from PIL import Image
from PIL import ImageOps

BACKGROUND_X = 1920
BACKGROUND_Y = 1080

g_imgSizeW = 0
g_imgSizeH = 0
g_bgPath = ''
g_imagePath = ''
g_maskImgPath = ''
g_blackImgPath = ''
g_nTrain = 0
g_nValid = 0


def check_input_output_dir(path):
  input_dir_path = path + "/input"
  if not os.path.isdir(input_dir_path):
    print("  make an input directory")
    os.mkdir(input_dir_path)
  else: 
    print("  /.../input dir -- OK")

  output_dir_path = path + "/output"
  if not os.path.isdir(output_dir_path):
    print("  make an outpu directory")
    os.mkdir(output_dir_path)
  else: 
    print("  /.../output dir -- OK")


def environment_check(bg):
  global g_bgPath, g_imagePath, g_maskPath, g_blackPath

  # 現在のパスを取得
  print("========== DIRECTORY CHECK =============")
  path = os.getcwd()

  # 背景画像の有無を確認
  g_bgPath = path + "/" + bg  # 背景画像へのパス
  if not os.path.isfile(g_bgPath):
    print("ERROR: background image does not exist")
    exit()
  else:
    print('background image: ' + g_bgPath + ' -- OK')


  # 認識対象画像(image.png)を確認
  g_imagePath = path + "/image" # フォルダーへのパス
  if not os.path.isdir(g_imagePath): # フォルダーがない場合
    print("image dir does not exisit")
    exit() # プログラム終了
  else: # フォルダーがあった場合
    print('/image dir -- OK')
    g_imagePath += "/image.png" # 認識対象画像へのパス 
    if not os.path.isfile(g_imagePath): # 画像がなかった場合
      print("ERROR: image.png does not exisit")
      exit() # プログラム終了
    else:
      print('  image.png -- OK')


  # マスク画像(mask.png)の有無を確認
  mask_dir_path = path + "/mask" # フォルダーへのパス
  if not os.path.isdir(mask_dir_path): # フォルダーがない場合
    print("mask dir does not exist")
    exit() # プログラム終了
  else: # フォルダーがあった場合
    print('/mask dir -- OK')
    g_maskPath = mask_dir_path + "/mask.png" # マスク画像のパス
    if not os.path.isfile(g_maskPath): # マスク画像がない場合
      print("ERROR: mask.png does not exisit")
      exit() # プログラム終了
    else:
      print('  mask.png -- OK')

    # マスク背景画像(black.png)の有無を確認
    g_blackPath = mask_dir_path + "/black.png" # マスク背景画像のパス
    if not os.path.isfile(g_blackPath): # マスク背景画像がない場合
      print("ERROR: black.png does not exisit")
      exit() # プログラム終了
    else:
      print('  black.png -- OK')

  # 学習データ格納フォルダ(train)の準備
  train_dir_path = path + "/train" # フォルダーへのパス
  if not os.path.isdir(train_dir_path): # フォルダーがない場合
    print("make a train directory")
    os.mkdir(train_dir_path) # trainフォルダーを生成
  else:
    print("/train dir -- OK")
  # trainフォルダー内のinput/outputフォルダーの準備
  check_input_output_dir(train_dir_path)


  # 評価データ格納フォルダ(valid)の準備
  valid_dir_path = path + "/valid" # フォルダーへのパス
  if not os.path.isdir(valid_dir_path): # フォルダーがない場合
    print("make a valid directory")
    os.mkdir(valid_dir_path) # validフォルダーを生成
  else:
    print("/valid dir -- OK")
  # valid フォルダー内のinput/outputフォルダーの準備
  check_input_output_dir(valid_dir_path)

  print('\n')


def check_arguments():
  print("========== ARGUMENT  CHECK =============")
  parse = argparse.ArgumentParser()
  parse.add_argument('-x', action='store', dest='iwidth', \
    default='28', help='-x: output image width')
  parse.add_argument('-y', action='store', dest='iheight',\
    default='28', help='-y: output image height')
  parse.add_argument('-b', action='store', dest='bgimage',\
    default='background.jpg', help='-b: background image (larger than 1920x1080)')
  parse.add_argument('-t', action='store', dest='train', \
    default='600', help='-t: quantity of generated training data')
  parse.add_argument('-v', action='store', dest='valid', \
    default='200', help='-v: quantity of generated validation data')
  results = parse.parse_args()
  return int(results.iwidth), int(results.iheight), \
    results.bgimage, int(results.train), int(results.valid)


def dataset_make_loop(quantity, arg_path):
  path = os.getcwd() # 現在のフォルダーのパスを取得

  print("open background image: " + g_bgPath)
  background = Image.open(g_bgPath) # 背景画像をオープン
  print("open and resize image: " + g_imagePath)
  image = Image.open(g_imagePath).resize(\
           (g_imgSizeW, g_imgSizeH)) # 認識対象画像をオープン
  print("open and resize mask: " + g_maskPath)
  mask = Image.open(g_maskPath).resize(\
           (g_imgSizeW, g_imgSizeH)) # マスク画像をオープン
  print("open and resize black: " + g_blackPath)
  black = Image.open(g_blackPath).resize(\
           (g_imgSizeW, g_imgSizeH)) # マスク背景画像をオープン
    
  # 引数で指定された名前の管理ファイルを生成、
  csv_file_path = path + "/" + arg_path + "/" + arg_path + ".csv"
  print("create the csv list file: " + csv_file_path)
  if os.path.isfile(csv_file_path):
    os.remove(csv_file_path)
  csvfile = open(csv_file_path, 'w')  #  
  csvfile.write("x:in,y:out\n")   # １一行目のヘッダーを書き込み

  for i in range(quantity):
    print("----" + str(i) + "----")

    # 背景画像を切り抜く座標をランダムに生成
    rand_x = BACKGROUND_X - g_imgSizeW
    rand_y = BACKGROUND_Y - g_imgSizeH
    start_x = random.randint(0, rand_x)
    start_y = random.randint(0, rand_y)
    end_x = start_x + g_imgSizeW
    end_y = start_y + g_imgSizeH
    print(" crop the background image => " + \
      "[" + str(start_x) + ":" + str(end_x) + "," + \
      str(start_y) + ":" + str(end_y) + "]")
    # 背景画像から画像を切り抜き
    cropped_image = background.crop((start_x, start_y, end_x, end_y))

    # 認識対象画像とマスク画像の縮小
    rand_s = random.uniform(1.0, 2.0) # 縮小率をランダムに生成
    shrink_w = round(g_imgSizeW/rand_s) # 横幅の縮小率
    shrink_h = round(g_imgSizeH/rand_s) # 縦幅の縮小率
    print(" resize the target image => [" + \
      str(shrink_w) + ":" + str(shrink_h) + "]")
    resize_image = image.resize((shrink_w, shrink_h)) # 認識対象画像の縮小
    print(" resize the mask image => [" + \
      str(shrink_w) + ":" + str(shrink_h) + "]")
    resize_mask = mask.resize((shrink_w, shrink_h)) # マスク画像の縮小

    # 認識対象画像と背景、マスクとマスク背景画像の合成
    range_x = g_imgSizeW - shrink_w 
    range_y = g_imgSizeH - shrink_h
    paste_x = random.randint(0, range_x) # 合成座標(x)をランダムに生成
    paste_y = random.randint(0, range_y) # 合成座標(y)をランダムに生成
    ## x:in image
    print(" put the image on the background => [" + \
      str(paste_x) + ":" + str(paste_y)  + "]")
    cropped_image.paste(resize_image, 
      (paste_x, paste_y), resize_image.split()[3]) # 入力画像を合成
    cropped_image.convert('RGB')
    ## y:out image
    print(" put the mask on the black => [" + \
      str(paste_x) + ":" + str(paste_y) + "]")
    tmp_black = black.copy()
    tmp_black.paste(resize_mask, (paste_x, paste_y)) # マスク画像を合成
    # memo: convert('L') not working. 4 channels are still remained
    gray_out = ImageOps.grayscale(tmp_black) # マスク画像を8ビット化
    # 画像の保存
    ## 入力画像(input)の保存
    inp_path = path + "/" + arg_path + "/input/" + str(i) + ".png"
    print(" save the input data as " + inp_path)
    cropped_image.save(inp_path)
    out_path = path + "/" + arg_path + "/output/" + str(i) + ".png"
    ## マスク画像(output)の保存
    print(" save the output data as " + out_path)
    gray_out.save(out_path)

    # 管理ファイルに入力画像と出力画像のパスを追加
    csvfile.write(inp_path + "," + out_path + "\n")

  csvfile.close() # 管理ファイルをクローズ
        

def main():
  global g_imgSizeW, g_imgSizeH, g_nTrain, g_nValid
  # 1.プログラム引数の解析　
  g_imgSizeW, g_imgSizeH, bg_filename, g_nTrain, g_nValid \
  = check_arguments()
  print('Image size width: ' + str(g_imgSizeW))
  print('Image size height: ' + str(g_imgSizeH))
  print('Background image: ' + bg_filename)
  print('Quantity of training data: ' + str(g_nTrain))
  print('Quantity of validation data: ' + str(g_nValid))
  print('\n')
  # 2.環境のチェック
  environment_check(bg_filename)
  # 3.学習データの生成
  dataset_make_loop(g_nTrain, "train")
  # 4.評価データの生成
  dataset_make_loop(g_nValid, "valid")


if __name__ == "__main__":
    main()
