import struct
import sys

OF_NUM = 2
STRIP_NUM = 8
VERSION_MAJOR = 2
VERSION_MINOR = 0
FRAME_NUM = 360
LED_num_array = [100,100,100,100,100,100,100,100]
# 範例版本1.3，有10條光纖，5條燈條上面有{5, 10, 15, 20, 25}顆LED，總共30個frame

def calculate_checksum(frame_data):
    # 計算checksum=所有byte的和 mod 2^32
    return sum(frame_data) & 0xFFFFFFFF

def main():
    total_leds = sum(LED_num_array)
    
    frame_size_without_checksum = 4 + 1 + (OF_NUM * 3) + (total_leds * 3)
    frame_size_with_checksum = frame_size_without_checksum + 4
    
    # 1. 生成 control.dat
    with open("control.dat", "wb") as control_file:
        control_file.write(struct.pack('<BB', VERSION_MAJOR, VERSION_MINOR))
        control_file.write(struct.pack('<B', OF_NUM))
        control_file.write(struct.pack('<B', STRIP_NUM))
        
        for led_num in LED_num_array:
            control_file.write(struct.pack('<B', led_num))

        control_file.write(struct.pack('<I', FRAME_NUM))
        
        for k in range(FRAME_NUM):
            timestamp = k * 100
            control_file.write(struct.pack('<I', timestamp))
            # 範例第k frame的timestamp是 100*k
    
    print("control.dat finish")
    
    # 2. 生成 frame.dat
    with open("frame.dat", "wb") as frame_file:
        frame_file.write(struct.pack('<BB', VERSION_MAJOR, VERSION_MINOR))
    
        for k in range(FRAME_NUM):
            frame_data = bytearray()
            start_time = k * 1000
            frame_data.extend(struct.pack('<I', start_time))
            fade = 1
            frame_data.append(fade)
            
            # 根據 frame/3 的餘數決定顏色
            color_mod = k % 3
            if color_mod == 0:
                # 紅色: GRB = [0, 255, 0] (注意是GRB格式)
                g, r, b = 0, 255, 0
            elif color_mod == 1:
                # 綠色: GRB = [255, 0, 0]
                g, r, b = 255, 0, 0
            else:  # color_mod == 2
                # 藍色: GRB = [0, 0, 255]
                g, r, b = 0, 0, 255
            
            # 光纖部分 (如果有)
            for i in range(OF_NUM):
                frame_data.append(g)  # G
                frame_data.append(r)  # R
                frame_data.append(b)  # B
            
            # LED燈條部分
            for i in range(STRIP_NUM):
                for j in range(LED_num_array[i]):
                    frame_data.append(g)  # G
                    frame_data.append(r)  # R
                    frame_data.append(b)  # B
            
            checksum = calculate_checksum(frame_data)
            frame_file.write(frame_data)
            frame_file.write(struct.pack('<I', checksum))
    
    print("\nframe.dat finish")
    print(f"OF_num: {OF_NUM}")
    print(f"Strip_num: {STRIP_NUM}")
    print(f"LED_num: {' '.join(map(str, LED_num_array))}")
    print(f"total LED: {total_leds}")
    print(f"Version: {VERSION_MAJOR}.{VERSION_MINOR}")
    print(f"Frame_num: {FRAME_NUM}")
    print(f"frame_size with checksum: {frame_size_with_checksum} byte")

if __name__ == "__main__":
    main()