#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define OF_NUM 10
#define STRIP_NUM 5
#define VERSION_MAJOR 1
#define VERSION_MINOR 3
#define FRAME_NUM 30
const uint8_t LED_num_array[STRIP_NUM] = {5, 10, 15, 20, 25};
//範例版本1.3，有10條光纖，5條燈條上面有{5, 10, 15, 20, 25}顆LED，總共30個frame

uint32_t calculate_checksum(uint8_t *frame_data, uint32_t frame_size_without_checksum){
    uint32_t sum = 0;
    for (uint32_t i = 0; i < frame_size_without_checksum; i++) {
        sum += frame_data[i];
    }
    return sum;
}

int main(){
    //算total led
    uint32_t total_leds = 0;
    for (int i = 0; i < STRIP_NUM; i++){
        total_leds += LED_num_array[i];
    }
    //算frame_size
    uint32_t frame_size_without_checksum = 4 + 1 + (OF_NUM * 3) + (total_leds * 3);
    uint32_t frame_size_with_checksum = frame_size_without_checksum + 4;
    
    //1.生control.dat
    FILE *control_file = fopen("control.dat", "wb");
    uint8_t version[2] = {VERSION_MAJOR, VERSION_MINOR};
    fwrite(version, 1, 2, control_file);
    
    uint8_t of_num = OF_NUM;
    uint8_t strip_num = STRIP_NUM;
    fwrite(&of_num, 1, 1, control_file);
    fwrite(&strip_num, 1, 1, control_file);
    
    for (int i = 0; i < STRIP_NUM; i++){
        fwrite(&LED_num_array[i], 1, 1, control_file);
    }
    
    uint32_t frame_num_le = FRAME_NUM;
    fwrite(&frame_num_le, 4, 1, control_file);
    
    for (int k = 0; k < FRAME_NUM; k++){
        uint32_t timestamp = k * 100;
        fwrite(&timestamp, 4, 1, control_file);
    }
    //範例，第k frame的timestamp是100*k
    fclose(control_file);
    printf("control.dat finish\n");
    
    //2.生frame.dat
    FILE *frame_file = fopen("frame.dat", "wb");
    
    fwrite(version, 1, 2, frame_file);
    
    //生成每個frame
    for (int k = 0; k < FRAME_NUM; k++){
        uint8_t *frame_data = malloc(frame_size_without_checksum);
        uint32_t offset = 0;
        uint32_t start_time = k * 100;
        memcpy(frame_data + offset, &start_time, 4);
        offset += 4;
        
        uint8_t fade = 1;
        frame_data[offset] = fade;
        offset += 1;
        
        //範例第k frame的OF GRB生成邏輯是: OF[i].G/R/B = (i+1/2/3 + k) mod 255
        for (int i = 0; i < OF_NUM; i++){
            frame_data[offset] = (i + 1 + k) % 255;
            frame_data[offset + 1] = (i + 2 + k) % 255;
            frame_data[offset + 2] = (i + 3 + k) % 255;
            offset += 3;
        }
        
        //範例第k frame的LED GRB生成邏輯是: LED[i][j].G/R/B = ((i*10+j) + 1/2/3 + k) mod 255
        for (int i = 0; i < STRIP_NUM; i++){
            for (int j = 0; j < LED_num_array[i]; j++){
                int base = i * 10 + j;
                frame_data[offset] = (base + 1 + k) % 255; // G
                frame_data[offset + 1] = (base + 2 + k) % 255; // R
                frame_data[offset + 2] = (base + 3 + k) % 255; // B
                offset += 3;
            }
        }
        
        uint32_t checksum = calculate_checksum(frame_data, frame_size_without_checksum);
        fwrite(frame_data, 1, frame_size_without_checksum, frame_file);
        fwrite(&checksum, 4, 1, frame_file);
        free(frame_data);
    }
    
    fclose(frame_file);

    printf("\n");
    printf("frame.dat finish\n");
    printf("OF_num: %d\n", OF_NUM);
    printf("Strip_num: %d\n", STRIP_NUM);
    printf("LED_num: ");
    for (int i = 0; i < STRIP_NUM; i++){
        printf("%d ", LED_num_array[i]);
    }
    printf("\n");
    printf("total LED: %u\n", total_leds);
    printf("Version: %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
    printf("Frame_num: %d\n", FRAME_NUM);
    printf("frame_size with checksum: %u byte\n", frame_size_with_checksum);
    printf("\n");
    return 0;
}