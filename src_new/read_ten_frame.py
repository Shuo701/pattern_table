#!/usr/bin/env python3
import struct

def read_control(filename):
    with open(filename, "rb") as f:
        v_major, v_minor = struct.unpack('BB', f.read(2))
        of_num = struct.unpack('B', f.read(1))[0]
        strip_num = struct.unpack('B', f.read(1))[0]
        led_nums = [struct.unpack('B', f.read(1))[0] for _ in range(strip_num)]
        frame_num = struct.unpack('<I', f.read(4))[0]
        timestamps = [struct.unpack('<I', f.read(4))[0] for _ in range(frame_num)]
        return (v_major, v_minor), of_num, strip_num, led_nums, frame_num, timestamps

def main():
    info = read_control("control.dat")
    if not info:
        return
    
    version, of_num, strip_num, led_nums, frame_num, timestamps = info
    total_leds = sum(led_nums)
    frame_base = 4 + 1 + (of_num * 3) + (total_leds * 3)
    frame_size = frame_base + 4
    
    with open("frame.dat", "rb") as f:
        f_major, f_minor = struct.unpack('BB', f.read(2))
        
        if (f_major, f_minor) != version:
            print(f"版本不一致: control {version}, frame ({f_major},{f_minor})")
            return
        
        print(f"OF: {of_num}, Strips: {strip_num}, LED per strip: {led_nums}")
        print(f"Frames: {frame_num}, Frame size: {frame_size} bytes\n")
        
        for frame_idx in range(min(10, frame_num)):
            data = f.read(frame_size)
            if len(data) != frame_size:
                break
            
            offset = 0
            start_time = struct.unpack_from('<I', data, offset)[0]
            offset += 4
            fade = data[offset]
            offset += 1
            
            print(f"Frame {frame_idx}: time={start_time}, fade={fade}")
            
            of_colors = []
            for i in range(of_num):
                g, r, b = data[offset], data[offset+1], data[offset+2]
                offset += 3
                of_colors.append((g, r, b))
            
            led_colors = []
            for strip in range(strip_num):
                strip_colors = []
                for _ in range(led_nums[strip]):
                    g, r, b = data[offset], data[offset+1], data[offset+2]
                    offset += 3
                    strip_colors.append((g, r, b))
                led_colors.append(strip_colors)
            
            stored = struct.unpack_from('<I', data, offset)[0]
            calc = sum(data[:frame_base]) & 0xFFFFFFFF
            
            print(f"  OF[0]: G={of_colors[0][0]:3d} R={of_colors[0][1]:3d} B={of_colors[0][2]:3d}")
            print(f"  LED[0][0]: G={led_colors[0][0][0]:3d} R={led_colors[0][0][1]:3d} B={led_colors[0][0][2]:3d}")
            print(f"  Checksum: {stored:08X} ({'OK' if stored == calc else 'ERROR'})\n")

if __name__ == "__main__":
    main()