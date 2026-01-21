import struct
import colorsys
import math

# 配置參數
OF_NUM = 2                    # 沒有光纖
STRIP_NUM = 8                 # 1條燈條
VERSION_MAJOR = 2
VERSION_MINOR = 0
FRAME_NUM = 360               # 360幀，每度一幀
LED_num_array = [100,100,100,100,100,100,100,100]         # 100顆LED
TOTAL_LEDS = sum(LED_num_array)

def hsv_to_grb(h, s=1.0, v=1.0):
    """
    將HSV轉換為GRB格式 (0-255)
    HSV範圍: h[0-360], s[0-1], v[0-1]
    """
    # 將HSV轉換為RGB (h範圍0-1)
    r, g, b = colorsys.hsv_to_rgb(h/360.0, s, v)
    
    # 轉換為0-255整數並調整為GRB順序
    r_int = int(r * 255)
    g_int = int(g * 255)
    b_int = int(b * 255)
    
    # 返回GRB順序
    return g_int, r_int, b_int

def calculate_checksum(frame_data):
    """計算checksum=所有byte的和 mod 2^32"""
    return sum(frame_data) & 0xFFFFFFFF

def create_wave_effect(frame_index, total_frames=360, num_leds=100):
    """
    創建波形光效：
    - frame_index: 當前幀索引
    - 整個色環360度
    - LED形成波形漸變
    """
    led_colors = []
    
    # 基礎色相偏移（整個色環）
    base_hue = (frame_index / total_frames) * 360.0
    
    for led_index in range(num_leds):
        # 每顆LED有額外的相位偏移，形成波形
        # 相位偏移範圍：0-180度
        phase_offset = (led_index / num_leds) * 180.0
        
        # 計算這顆LED的色相
        led_hue = (base_hue + phase_offset) % 360.0
        
        # 飽和度和亮度保持固定
        saturation = 1.0  # 全飽和
        value = 1.0       # 最大亮度
        
        # 轉換為GRB
        g, r, b = hsv_to_grb(led_hue, saturation, value)
        led_colors.append((g, r, b))
    
    return led_colors

def create_spiral_effect(frame_index, total_frames=360, num_leds=100):
    """
    創建螺旋光效：
    - 螺旋狀顏色變化
    - 每顆LED有稍微不同的起始相位
    """
    led_colors = []
    
    # 基礎色相
    base_hue = (frame_index / total_frames) * 360.0
    
    for led_index in range(num_leds):
        # 螺旋參數：每顆LED有2圈螺旋
        spiral_cycles = 2.0
        spiral_offset = (led_index / num_leds) * spiral_cycles * 360.0
        
        # 計算色相
        led_hue = (base_hue + spiral_offset) % 360.0
        
        # 飽和度和亮度
        saturation = 1.0
        value = 1.0
        
        g, r, b = hsv_to_grb(led_hue, saturation, value)
        led_colors.append((g, r, b))
    
    return led_colors

def create_rainbow_chase(frame_index, total_frames=360, num_leds=100, wave_length=20):
    """
    創建彩虹追逐效果：
    - wave_length: 波形長度（LED數量）
    """
    led_colors = []
    
    for led_index in range(num_leds):
        # 計算相位：LED位置 + 時間偏移
        phase = (led_index / wave_length) * 360.0 + (frame_index / total_frames) * 360.0
        
        hue = phase % 360.0
        
        g, r, b = hsv_to_grb(hue, 1.0, 1.0)
        led_colors.append((g, r, b))
    
    return led_colors

def main():
    print("=== 生成彩虹漸變LED光效 ===")
    print(f"LED數量: {TOTAL_LEDS}")
    print(f"總幀數: {FRAME_NUM}")
    print(f"版本: {VERSION_MAJOR}.{VERSION_MINOR}")
    
    # 計算幀大小
    frame_size_without_checksum = 4 + 1 + (OF_NUM * 3) + (TOTAL_LEDS * 3)
    frame_size_with_checksum = frame_size_without_checksum + 4
    
    print(f"\n每幀大小: {frame_size_with_checksum} 字節")
    print(f"預估檔案大小: {2 + FRAME_NUM * frame_size_with_checksum} 字節")
    
    # 1. 生成 control.dat
    print("\n生成 control.dat...")
    with open("control.dat", "wb") as control_file:
        # 版本號
        control_file.write(struct.pack('<BB', VERSION_MAJOR, VERSION_MINOR))
        
        # OF數量和燈條數量
        control_file.write(struct.pack('<B', OF_NUM))
        control_file.write(struct.pack('<B', STRIP_NUM))
        
        # LED數量數組
        for led_num in LED_num_array:
            control_file.write(struct.pack('<B', led_num))
        
        # 總幀數
        control_file.write(struct.pack('<I', FRAME_NUM))
        
        # 時間戳（每幀100ms間隔）
        for k in range(FRAME_NUM):
            timestamp = k * 100  # 100ms間隔
            control_file.write(struct.pack('<I', timestamp))
    
    print("✓ control.dat 生成完成")
    
    # 2. 生成 frame.dat
    print("\n生成 frame.dat...")
    
    # 選擇效果類型
    EFFECT_TYPE = "rainbow_chase"  # 可選: "wave", "spiral", "rainbow_chase"
    
    with open("frame.dat", "wb") as frame_file:
        # 版本號
        frame_file.write(struct.pack('<BB', VERSION_MAJOR, VERSION_MINOR))
        
        # 生成每一幀
        for frame_index in range(FRAME_NUM):
            frame_data = bytearray()
            
            # 時間戳（毫秒）
            start_time = frame_index * 100  # 每幀100ms
            frame_data.extend(struct.pack('<I', start_time))
            
            # fade標誌（總是啟用淡入淡出）
            fade = 1
            frame_data.append(fade)
            
            # 選擇不同的效果
            if EFFECT_TYPE == "wave":
                led_colors = create_wave_effect(frame_index, FRAME_NUM, TOTAL_LEDS)
            elif EFFECT_TYPE == "spiral":
                led_colors = create_spiral_effect(frame_index, FRAME_NUM, TOTAL_LEDS)
            elif EFFECT_TYPE == "rainbow_chase":
                led_colors = create_rainbow_chase(frame_index, FRAME_NUM, TOTAL_LEDS, wave_length=25)
            else:
                # 默認：簡單的彩虹效果
                led_colors = []
                for led_index in range(TOTAL_LEDS):
                    hue = (frame_index * 360 / FRAME_NUM + led_index * 360 / TOTAL_LEDS) % 360
                    g, r, b = hsv_to_grb(hue, 1.0, 1.0)
                    led_colors.append((g, r, b))
            
            # 光纖部分（沒有光纖）
            for i in range(OF_NUM):
                frame_data.append(0)  # G
                frame_data.append(0)  # R
                frame_data.append(0)  # B
            
            # LED燈條部分
            for g, r, b in led_colors:
                frame_data.append(g)  # G
                frame_data.append(r)  # R
                frame_data.append(b)  # B
            
            # 計算checksum
            checksum = calculate_checksum(frame_data)
            frame_file.write(frame_data)
            frame_file.write(struct.pack('<I', checksum))
            
            # 顯示進度
            if (frame_index + 1) % 36 == 0:  # 每10%顯示一次
                progress = (frame_index + 1) * 100 // FRAME_NUM
                print(f"  進度: {progress}% - 幀 {frame_index+1}/{FRAME_NUM}")
    
    print("✓ frame.dat 生成完成")

if __name__ == "__main__":
    main()