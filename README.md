# LED Pattern Generator Pipeline

## 快速開始
### 1. 環境準備
```
gcc -o src/read_control src/read_from_control.c
gcc -o src/read_OF src/read_from_OF.c
gcc -o src/read_LED src/read_from_LED.c
gcc -o src/merge_frame src/merge_frame.c
chmod +x pattern_pipeline.sh
```
### 2. 使用
```
./pattern_pipeline.sh <輸入目錄> <輸出目錄>

# 範例
./pattern_pipeline.sh gen_blender/ output/
```

## 架構
```
src/                       # 原始碼目錄
├── read_from_control.c    # 讀取 control.json → control.dat
├── read_from_OF.c         # 讀取 of.json + control.dat → OF.txt
├── read_from_LED.c        # 讀取 led.json + control.dat → LED.txt
└── merge_frame.c          # 合併 OF.txt + LED.txt + control.dat → frame.dat
```

## 詳細使用方法 (optional)
### 1. 直接使用

```
bash
# 給予執行權限
chmod +x pattern_pipeline.sh

# 執行管線
./pattern_pipeline.sh <輸入目錄> <輸出目錄>

# 範例
./pattern_pipeline.sh gen_blender output
```

### 2. 逐步執行

```
bash
# 編譯所有程式
gcc -o src/read_control src/read_from_control.c
gcc -o src/read_OF src/read_from_OF.c
gcc -o src/read_LED src/read_from_LED.c
gcc -o src/merge_frame src/merge_frame.c

# 步驟1: 生成 control.dat
src/read_control gen_blender/control.json output/control.dat

# 步驟2: 生成 OF.txt
src/read_OF gen_blender/of.json output/control.dat output/OF.txt

# 步驟3: 生成 LED.txt
src/read_LED gen_blender/led.json output/control.dat output/LED.txt

# 步驟4: 生成 frame.dat
src/merge_frame output/OF.txt output/LED.txt output/control.dat output/frame.dat
```


## 管線流程圖
```
[ control.json ] ───▶ ( read_control ) ──────────────────────▶ [ control.dat ]
                                                 │
                                                 ▼
[ of.json ]      ───▶ ( read_OF )      ───▶ [ OF.txt ]
                                                 │
                                                 ▼
[ led.json ]     ───▶ ( read_LED )     ───▶ [ LED.txt ]
                                                 │
                                                 ▼
                                          ( merge_frame ) ───▶ [ frame.dat ]
```