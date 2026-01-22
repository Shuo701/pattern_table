import struct

OF_NUM = 40
STRIP_NUM = 8
LED_num_array = [100, 100, 100, 100, 100, 100, 100, 100]
VERSION_MAJOR = 1
VERSION_MINOR = 0
FRAME_NUM = 360

def main():
    TOTAL_LEDS = sum(LED_num_array)
    
    with open("control.dat", "wb") as f:
        f.write(struct.pack('<BB', VERSION_MAJOR, VERSION_MINOR))
        f.write(struct.pack('<B', OF_NUM))
        f.write(struct.pack('<B', STRIP_NUM))
        for n in LED_num_array:
            f.write(struct.pack('<B', n))
        f.write(struct.pack('<I', FRAME_NUM))
        for k in range(FRAME_NUM):
            f.write(struct.pack('<I', k * 100))

    with open("frame.dat", "wb") as f:
        f.write(struct.pack('<BB', VERSION_MAJOR, VERSION_MINOR))
        
        of_buf = [[0, 0, 0] for _ in range(OF_NUM)]
        led_buf = [[[0, 0, 0] for _ in range(n)] for n in LED_num_array]
        
        for k in range(FRAME_NUM):
            data = bytearray()
            data.extend(struct.pack('<I', k * 500))
            data.append(0)
            
            for i in range(OF_NUM):
                if k == 0:
                    color = i % 3
                    if color == 0:
                        g, r, b = 0, 255, 0
                    elif color == 1:
                        g, r, b = 255, 0, 0
                    else:
                        g, r, b = 0, 0, 255
                else:
                    nxt = (i + 1) % OF_NUM
                    g, r, b = of_buf[nxt]
                data.extend([g, r, b])
                of_buf[i] = [g, r, b]
            
            for s in range(STRIP_NUM):
                for l in range(LED_num_array[s]):
                    if k == 0:
                        phase = (s * 45 + l * 3.6) % 360
                        if phase < 120:
                            g, r, b = int((120-phase)/120*255), int(phase/120*255), 0
                        elif phase < 240:
                            g, r, b = 0, int((240-phase)/120*255), int((phase-120)/120*255)
                        else:
                            g, r, b = int((phase-240)/120*255), 0, int((360-phase)/120*255)
                    else:
                        nxt = (l + 1) % LED_num_array[s]
                        g, r, b = led_buf[s][nxt]
                    data.extend([g, r, b])
                    led_buf[s][l] = [g, r, b]
            
            checksum = sum(data) & 0xFFFFFFFF
            f.write(data)
            f.write(struct.pack('<I', checksum))

if __name__ == "__main__":
    main()