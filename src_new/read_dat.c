#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint32_t calculate_checksum(uint8_t *data, uint32_t size);
void read_frame(uint8_t *frame_data, int frame_index, int of_num, int strip_num, uint8_t *led_num_array, uint32_t frame_size_without_checksum);
int read_control_file(const char *filename, uint8_t *version, uint8_t *of_num, uint8_t *strip_num, uint8_t **led_num_array, uint32_t *frame_num);
void read_frame_file(const char *filename, int of_num, int strip_num, uint8_t *led_num_array);

int main(){
    printf("Start reading Pattern Table file\n\n");
    uint8_t version[2];
    uint8_t of_num, strip_num;
    uint8_t *led_num_array = NULL;
    uint32_t frame_num;

    if (!read_control_file("control.dat", version, &of_num, &strip_num, &led_num_array, &frame_num)){
        return 1;
    }
    read_frame_file("frame.dat", of_num, strip_num, led_num_array);
    free(led_num_array);
    return 0;
}

int read_control_file(const char *filename, uint8_t *version, uint8_t *of_num, uint8_t *strip_num, uint8_t **led_num_array, uint32_t *frame_num){
    FILE *file = fopen(filename, "rb");
    if (!file){
        perror("can't open control.dat");
        return 0;
    }
    printf("=== control.dat ===\n");
    
    //讀版本跟of_num, strip_num, led_num[]
    fread(version, 1, 2, file);
    printf("version: %d.%d\n", version[0], version[1]);
    
    fread(of_num, 1, 1, file);
    fread(strip_num, 1, 1, file);
    printf("OF_num: %d\n", *of_num);
    printf("LStrip_num: %d\n", *strip_num);
    
    *led_num_array = malloc(*strip_num);
    printf("LED_num[]: ");
    for (int i = 0; i < *strip_num; i++){
        fread(&(*led_num_array)[i], 1, 1, file);
        printf("%d ", (*led_num_array)[i]);
    }
    printf("\n");
    
    //讀frame_num, time_stamp[]
    fread(frame_num, 4, 1, file);
    printf("Frame_num: %u\n", *frame_num);
    
    printf("\ntime_stamp[]:\n");
    for (int i = 0; i < *frame_num; i++){
        uint32_t timestamp;
        fread(&timestamp, 4, 1, file);
        printf(" %u", timestamp);
    }
    fclose(file);
    return 1;
}
void read_frame_file(const char *filename, int of_num, int strip_num, uint8_t *led_num_array){
    FILE *file = fopen(filename, "rb");
    if (!file){
        perror("can't open frame.dat");
        return;
    }
    
    //讀版本
    uint8_t version[2];
    fread(version, 1, 2, file);
    printf("\n=== frame.dat ===\n");
    printf("version: %d.%d\n", version[0], version[1]);
    
    //算frame_size
    uint32_t total_leds = 0;
    for (int i = 0; i < strip_num; i++){
        total_leds += led_num_array[i];
    }
    uint32_t frame_size_without_checksum = 4 + 1 + (of_num * 3) + (total_leds * 3);
    uint32_t frame_size_with_checksum = frame_size_without_checksum + 4;
    printf("frame size with checksum: %u bytes\n\n", frame_size_with_checksum);

    int frame_count = 0;
    while (!feof(file)){
        //依序讀每一個frame
        long current_pos = ftell(file);
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, current_pos, SEEK_SET);
        
        if (current_pos >= file_size) break; //大小有錯
        
        uint8_t *frame_data = malloc(frame_size_with_checksum);
        size_t bytes_read = fread(frame_data, 1, frame_size_with_checksum, file);
        if (bytes_read != frame_size_with_checksum){
            free(frame_data);
            break;
        }
        //讀frame
        read_frame(frame_data, frame_count, of_num, strip_num, led_num_array, frame_size_without_checksum);
        free(frame_data);
        frame_count++;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    long header_size = 2;
    long total_frames = (file_size - header_size) / frame_size_with_checksum;

    printf("file size: %ld bytes\n", file_size);
    printf("total frames: %ld\n", total_frames);
    fclose(file);
}


void read_frame(uint8_t *frame_data, int frame_index, int of_num, int strip_num, uint8_t *led_num_array, uint32_t frame_size_without_checksum){
    printf("Frame %d:\n", frame_index);
    
    uint32_t offset = 0;
    uint32_t start_time;
    memcpy(&start_time, frame_data + offset, 4);
    offset += 4;
    printf("  start_time: %u\n", start_time);
    
    uint8_t fade = frame_data[offset];
    offset += 1;
    printf("  fade: %s\n", fade ? "True" : "False");
    
    printf("  OF colors:\n");
    for (int i = 0; i < of_num; i++){
        uint8_t g = frame_data[offset];
        uint8_t r = frame_data[offset + 1];
        uint8_t b = frame_data[offset + 2];
        offset += 3;
        printf("      OF[%d]: G=%03d, R=%03d, B=%03d\n", i, g, r, b);
    }
    
    printf("  LED colors:\n");
    for (int i = 0; i < strip_num; i++){
        printf("    strip %d:\n", i);
        for (int j = 0; j < led_num_array[i]; j++){
            uint8_t g = frame_data[offset];
            uint8_t r = frame_data[offset + 1];
            uint8_t b = frame_data[offset + 2];
            offset += 3;
            printf("      LED[%d][%d]: G=%03d, R=%03d, B=%03d\n", i, j, g, r, b);
        }
    }
    
    //計算checksum有沒有錯
    uint32_t stored_checksum;
    memcpy(&stored_checksum, frame_data + offset, 4);
    uint32_t calculated_checksum = calculate_checksum(frame_data, frame_size_without_checksum);
    
    printf("  Checksum: stored=%08X, calculated=%08X ", stored_checksum, calculated_checksum);
    if (stored_checksum == calculated_checksum) printf("OK\n\n");
    else printf("ERROR\n\n");
}


uint32_t calculate_checksum(uint8_t *data, uint32_t size){
    uint32_t sum = 0;
    for (uint32_t i = 0; i < size; i++){
        sum += data[i];
    }
    return sum;
}