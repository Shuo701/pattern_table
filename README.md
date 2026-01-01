# LED Pattern Generator Pipeline

## 架構
```
src/                    # 原始碼目錄
├── read_from_control.c    # 讀取 control.json → control.bin
├── read_from_OF.c         # 讀取 of.json + control.bin → OF.txt
├── read_from_LED.c        # 讀取 led.json + control.bin → LED.txt
└── merge_frame.c          # 合併 OF.txt + LED.txt + control.bin → frame.bin

gen_blender/             # 輸入資料目錄（範例）
├── control.json         # 控制參數設定
├── of.json              # OF 動畫資料
└── led.json             # LED 動畫資料

output/                  # 輸出目錄（自動生成）
├── control.bin          # 控制參數二進位檔
├── OF.txt               # OF 動畫文字檔（中間檔案）
├── LED.txt              # LED 動畫文字檔（中間檔案）
└── frame.bin            # 最終動畫二進位檔
```

## 輸入檔案格式

### 1. control.json
```
{
  "fps": 30,                    // 畫面更新率
  "OFPARTS": { ... },           // OF 裝置配置
  "LEDPARTS": { ... }           // LED 裝置配置
}
```
### 2. of.json

包含 OF 動畫的關鍵幀資料，每幀包含：
start: 開始時間
fade: 淡入淡出效果
status: 顏色狀態陣列

### 3. led.json
包含 LED 動畫的關鍵幀資料，每幀包含：
start: 開始時間
fade: 淡入淡出效果
status: 每個 LED 的像素顏色陣列

## 輸出檔案格式

### 1. control.bin 格式

```
偏移量	    大小	                說明
0	        1 byte	        fps (畫面更新率)
1	        1 byte	        OF_num (OF 裝置數量)
2	        1 byte	        LED_num (LED 裝置數量)
3~N	        LED_num bytes	每個 LED 的燈泡數量	[10, 20, 15]
```

### 2. frame.bin 格式

每個動畫幀的結構：

[幀資料] = [幀頭] + [OF資料] + [LED資料]

```
幀頭結構 (4 bytes):
偏移量	    大小	        說明
0-2	    3 bytes	    start_time (24-bit 無號整數)
3	    1 byte	    fade (0=false, 1=true)
```

#### OF資料結構:
```
OF資料 = OF_num × [RGB]
每個 RGB = 3 bytes (R, G, B)
```

#### LED資料結構:
```
LED資料 = ∑(LED_bulbs[i]) × [RGB]
每個 RGB = 3 bytes (R, G, B)
```

#### 完整幀大小計算:
```
幀大小 = 4 + (OF_num × 3) + (總LED燈泡數 × 3)
```

## 使用方法

### 基本使用

```
bash
# 給予執行權限
chmod +x pattern_pipeline.sh

# 執行管線
./run_pipeline.sh <輸入目錄> <輸出目錄>

# 範例
./run_pipeline.sh gen_blender output
```

### 2. 逐步執行

```
bash
# 編譯所有程式
gcc -o src/read_control src/read_from_control.c
gcc -o src/read_OF src/read_from_OF.c
gcc -o src/read_LED src/read_from_LED.c
gcc -o src/merge_frame src/merge_frame.c

# 步驟1: 生成 control.bin
src/read_control gen_blender/control.json output/control.bin

# 步驟2: 生成 OF.txt
src/read_OF gen_blender/of.json output/control.bin output/OF.txt

# 步驟3: 生成 LED.txt
src/read_LED gen_blender/led.json output/control.bin output/LED.txt

# 步驟4: 生成 frame.bin
src/merge_frame output/OF.txt output/LED.txt output/control.bin output/frame.bin
```

### 3. 檢查輸出

```
# 檢查 control.bin
hexdump -C output/control.bin

# 檢查 frame.bin 大小
ls -lh output/frame.bin

# 查看文字檔
head -20 output/OF.txt
```

## 管線流程圖
```
[ control.json ] ───▶ ( read_control ) ───▶ [ control.bin ]
                                                 │
                                                 ▼
[ of.json ]      ───▶ ( read_OF )      ───▶ [ OF.txt ]
                                                 │
                                                 ▼
[ led.json ]     ───▶ ( read_LED )     ───▶ [ LED.txt ]
                                                 │
                                                 ▼
                                          ( merge_frame ) ───▶ [ frame.bin ]
```