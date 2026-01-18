import struct
import sys

OF_NUM = 5
STRIP_NUM = 2
VERSION_MAJOR = 1
VERSION_MINOR = 0
FRAME_NUM = 5
LED_num_array = [10, 20]
# 範例，這是版本1.0，有五條光纖，兩條燈條上面有{10, 20}顆LED，總共5個frame

def calculate_checksum(frame_data):
    #計算checksum=所有byte的和 mod 2^32
    return sum(frame_data) & 0xFFFFFFFF

def main():
    total_leds = sum(LED_num_array)
    
    frame_size_without_checksum = 4+1+(OF_NUM *3)+(total_leds *3)
    frame_size_with_checksum = frame_size_without_checksum+4
    
    #1.生control.dat
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
            # 範例，第k frame的timestamp是100*k
    
    print("control.dat finish")
    
    #2.生frame.dat
    with open("frame.dat", "wb") as frame_file:
        frame_file.write(struct.pack('<BB', VERSION_MAJOR, VERSION_MINOR))
    
        for k in range(FRAME_NUM):
            frame_data = bytearray()
            start_time = k * 100
            frame_data.extend(struct.pack('<I', start_time))
            fade = 1
            frame_data.append(fade)
            
            # 範例，第k frame的OF GRB生成邏輯是: OF[i].G/R/B = (i+1/2/3 + k) mod 255
            for i in range(OF_NUM):
                # G, R, B 各 1 byte
                frame_data.append((i + 1 + k) % 255)  # G
                frame_data.append((i + 2 + k) % 255)  # R
                frame_data.append((i + 3 + k) % 255)  # B
            
            # 範例，第k frame的LED GRB生成邏輯是: LED[i][j].G/R/B = ((i*10+j) + 1/2/3 + k) mod 255
            for i in range(STRIP_NUM):
                for j in range(LED_num_array[i]):
                    base = i * 10 + j
                    frame_data.append((base + 1 + k) % 255)  # G
                    frame_data.append((base + 2 + k) % 255)  # R
                    frame_data.append((base + 3 + k) % 255)  # B
            
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
    print()

if __name__ == "__main__":
    main()