#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

int read_int_from_file(FILE *fp){
    int value;
    while (fscanf(fp, "%d", &value) != 1){
        if (feof(fp)) return -1;
        fgetc(fp);
    }
    return value;
}

bool read_bool_from_file(FILE *fp){
    char word[10];
    while (fscanf(fp, "%9s", word) == 1){
        if (strcasecmp(word, "true") == 0) return true;
        if (strcasecmp(word, "false") == 0) return false;
    }
    return false;
}
void skip_line(FILE *fp){
    int c;
    while ((c = fgetc(fp)) != EOF && c != '\n');
}

int find_keyword_in_file(FILE *fp, const char *keyword){
    char line[256];
    long current_pos = ftell(fp);
    
    while (fgets(line, sizeof(line), fp) != NULL){
        if (strstr(line, keyword) != NULL){
            return 1;
        }
    }
    
    fseek(fp, current_pos, SEEK_SET);
    return 0;
}

int main(){
    FILE *of_file = fopen("OF.txt", "r");
    FILE *led_file = fopen("LED.txt", "r");
    FILE *out_file = fopen("frame.bin", "wb");
    
    if (!of_file){
        printf("can't open OF.txt\n");
        return 1;
    }
    if (!led_file){
        printf("can't open LED.txt\n");
        fclose(of_file);
        return 1;
    }
    if (!out_file){
        printf("can't open frame.bin\n");
        fclose(of_file);
        fclose(led_file);
        return 1;
    }
    
    int OF_num, LED_num;
    printf("input OF_num: ");
    scanf("%d", &OF_num);
    printf("input LED_num: ");
    scanf("%d", &LED_num);
    
    int *LED_bulb = malloc(LED_num * sizeof(int));
    if (!LED_bulb){
        printf("fail to buffer\n");
        fclose(of_file);
        fclose(led_file);
        fclose(out_file);
        return 1;
    }
    
    printf("num of LED_bulb[%d]: ", LED_num);
    for (int i = 0; i < LED_num; i++){
        if (scanf("%d", &LED_bulb[i]) != 1){
            printf("wrong input\n");
            free(LED_bulb);
            fclose(of_file);
            fclose(led_file);
            fclose(out_file);
            return 1;
        }
    }
    
    int frame_count = 0;
    rewind(of_file);
    rewind(led_file);
    
    while (1){
        int c = fgetc(of_file);
        if (c == EOF) break;
        ungetc(c, of_file);
        
        if (!find_keyword_in_file(of_file, "frame")){
            break;
        }
        
        c = fgetc(led_file);
        if (c == EOF) break;
        ungetc(c, led_file);
        
        if (!find_keyword_in_file(led_file, "frame")){
            printf("Warnimg: OF.txt has frame but LED.txt has no same frame\n");
            break;
        }
        
        printf("frame %d\n", frame_count + 1);
        
        int of_start = read_int_from_file(of_file);
        bool of_fade = read_bool_from_file(of_file);
        int led_start = read_int_from_file(led_file);
        bool led_fade = read_bool_from_file(led_file);
        
        //detect start & fade same
        if (of_start != led_start){
            printf("Warning: different start at frame %d (OF:%d, LED:%d)\n", 
                   frame_count + 1, of_start, led_start);
        }
        if (of_fade != led_fade){
            printf("Warning: different fade at frame %d (OF:%s, LED:%s)\n", 
                   frame_count + 1, of_fade ? "true" : "false", led_fade ? "true" : "false");
        }
        
        int start = of_start;
        bool fade = of_fade;
        printf("  start: %d, fade: %s\n", start, fade ? "true" : "false");
        
        unsigned char start_bytes[3];
        start_bytes[0] = start & 0xFF;
        start_bytes[1] = (start >> 8) & 0xFF;
        start_bytes[2] = (start >> 16) & 0xFF;
        if (fwrite(start_bytes, 1, 3, out_file) != 3){
            printf("fail to write start\n");
            break;
        }
        
        unsigned char fade_byte = fade ? 1 : 0;
        if (fwrite(&fade_byte, 1, 1, out_file) != 1){
            printf("fail to write fade\n");
            break;
        }
        
        printf("    read OF data (%d OF)\n", OF_num);
        int of_error = 0;
        for (int i = 0; i < OF_num; i++){
            int r = read_int_from_file(of_file);
            int g = read_int_from_file(of_file);
            int b = read_int_from_file(of_file);
            
            if (r == -1 || g == -1 || b == -1){
                printf("wrong data of OF (group %d)\n", i);
                of_error = 1;
                break;
            }
            
            unsigned char rgb[3] ={(unsigned char)r, (unsigned char)g, (unsigned char)b};
            if (fwrite(rgb, 1, 3, out_file) != 3){
                printf("fail to write OF RGB\n");
                of_error = 1;
                break;
            }
        }
        
        if (of_error){
            printf("fail to read OF, skip frame\n");
            frame_count++;
            continue;
        }
        
        printf("    read LED data\n");
        int led_error = 0;
        for (int i = 0; i < LED_num; i++){
            for (int j = 0; j < LED_bulb[i]; j++){
                int r = read_int_from_file(led_file);
                int g = read_int_from_file(led_file);
                int b = read_int_from_file(led_file);
                
                if (r == -1 || g == -1 || b == -1){
                    printf("fail to read LED (LED%d, bulb %d)\n", i, j);
                    led_error = 1;
                    break;
                }
                
                unsigned char rgb[3] ={(unsigned char)r, (unsigned char)g, (unsigned char)b};
                if (fwrite(rgb, 1, 3, out_file) != 3){
                    printf("fail to write LED RGB\n");
                    led_error = 1;
                    break;
                }
            }
            if (led_error) break;
        }
        
        if (led_error){
            printf("fail to read data of LED, skip frame\n");
        }
        
        frame_count++;
        printf("    Done frame %d\n\n", frame_count);
    }
    
    fclose(of_file);
    fclose(led_file);
    fclose(out_file);
    
    printf("Done, total %d frame\n", frame_count);
    
    FILE *check = fopen("frame.bin", "rb");
    if (check){
        fseek(check, 0, SEEK_END);
        long size = ftell(check);
        fclose(check);
        
        int frame_size = 3 + 1 + (OF_num * 3);
        int total_led_bulbs = 0;
        for (int i = 0; i < LED_num; i++){
            total_led_bulbs += LED_bulb[i];
        }
        frame_size += total_led_bulbs * 3;
    }
    
    free(LED_bulb);
    return 0;
}