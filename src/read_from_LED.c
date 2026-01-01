#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void skip_whitespace(char **ptr){
    while (**ptr && isspace(**ptr)){
        (*ptr)++;
    }
}

int read_number(char **ptr){
    skip_whitespace(ptr);
    int num = 0;
    while (**ptr && **ptr >= '0' && **ptr <= '9'){
        num = num * 10 + (**ptr - '0');
        (*ptr)++;
    }
    return num;
}

int main(int argc, char *argv[]){
    if (argc != 4) {
        printf("Usage: %s <input_LED.json> <input_Control.bin> <output_LED.txt>\n", argv[0]);
        return 1;
    }
    
    char *input_file = argv[1];
    char *control_file = argv[2];
    char *output_file = argv[3];
    
    int led_count = 0;
    int *led_lens = NULL;

    FILE *control_fp = fopen(control_file, "rb");
    if (!control_fp) {
        printf("Cannot open %s\n", control_file);
        return 1;
    }

    fseek(control_fp, 2, SEEK_SET);
    unsigned char led_count_byte;
    fread(&led_count_byte, 1, 1, control_fp); //read third byte (LED_num)
    led_count = (int)led_count_byte;
    
    printf("LED count from %s: %d\n", control_file, led_count);

    if (led_count > 0){
        led_lens = malloc(led_count * sizeof(int));
        
        for (int i = 0; i < led_count; i++){
            unsigned char len_byte;
            fread(&len_byte, 1, 1, control_fp);
            led_lens[i] = (int)len_byte;
            printf("LED%d bulbs: %d\n", i, led_lens[i]);
        }
    }
    fclose(control_fp);

    if (led_count == 0 || led_lens == NULL){
        printf("Error: No LED data found in %s\n", control_file);
        if (led_lens) free(led_lens);
        return 1;
    }


    FILE *fp = fopen(input_file, "r");
    if (!fp){
        printf("Cannot open %s\n", input_file);
        free(led_lens);
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *content = malloc(file_size + 1);
    fread(content, 1, file_size, fp);
    content[file_size] = '\0';
    fclose(fp);
    
    char *ptr = content;
    int frame_count = 0;
    
    char *first_led = strstr(ptr, "\"LED0\"");
    if (!first_led){
        printf("LED0 not found\n");
        free(led_lens);
        free(content);
        return 1;
    }
    
    first_led = strchr(first_led, '[');
    if (!first_led){
        printf("Invalid LED0 format\n");
        free(led_lens);
        free(content);
        return 1;
    }
    
    first_led++; 
    
    char *temp = first_led;
    skip_whitespace(&temp);
    while (*temp && *temp != ']'){
        if (*temp == '{'){
            frame_count++;
            int depth = 1;
            temp++;
            while (*temp && depth > 0){
                if (*temp == '{') depth++;
                else if (*temp == '}') depth--;
                temp++;
            }
        }
        skip_whitespace(&temp);
        if (*temp == ',') temp++;
        skip_whitespace(&temp);
    }
    printf("Found %d frames\n", frame_count);
    
    FILE *txt_fp = fopen(output_file, "w");
    if (!txt_fp) {
        printf("Cannot create %s\n", output_file);
        free(led_lens);
        free(content);
        return 1;
    }
    
    int *frame_starts = malloc(frame_count * sizeof(int));
    int *frame_fades = malloc(frame_count * sizeof(int));
    
    int ****frame_data = malloc(frame_count * sizeof(int ***));
    for (int f = 0; f < frame_count; f++){
        frame_data[f] = malloc(led_count * sizeof(int **));
        for (int l = 0; l < led_count; l++){
            frame_data[f][l] = malloc(led_lens[l] * sizeof(int *));
            for (int p = 0; p < led_lens[l]; p++){
                frame_data[f][l][p] = malloc(3 * sizeof(int));
            }
        }
    }
    
    for (int led_idx = 0; led_idx < led_count; led_idx++){
        char led_key[50];
        sprintf(led_key, "\"LED%d\"", led_idx);
        char *led_ptr = strstr(ptr, led_key);
        
        if (!led_ptr){
            printf("LED%d not found in JSON\n", led_idx);
            continue;
        }
        
        led_ptr = strchr(led_ptr, '[');
        if (!led_ptr){
            printf("Invalid LED%d format\n", led_idx);
            continue;
        }
        led_ptr++;
        
        for (int frame_idx = 0; frame_idx < frame_count; frame_idx++){
            skip_whitespace(&led_ptr);
            
            if (*led_ptr != '{'){
                printf("Error: Expected '{' at frame %d, LED%d\n", frame_idx, led_idx);
                break;
            }
            led_ptr++;
            
            char *start_key = strstr(led_ptr, "\"start\"");
            if (start_key){
                start_key = strchr(start_key, ':');
                if (start_key){
                    start_key++;
                    frame_starts[frame_idx] = read_number(&start_key);
                    led_ptr = start_key;
                }
            }
            
            char *fade_key = strstr(led_ptr, "\"fade\"");
            if (fade_key){
                fade_key = strchr(fade_key, ':');
                if (fade_key){
                    fade_key++;
                    skip_whitespace(&fade_key);
                    if (strncmp(fade_key, "true", 4) == 0){
                        frame_fades[frame_idx] = 1;
                        fade_key += 4;
                    } else if (strncmp(fade_key, "false", 5) == 0){
                        frame_fades[frame_idx] = 0;
                        fade_key += 5;
                    }
                    led_ptr = fade_key;
                }
            }
            
            char *status_key = strstr(led_ptr, "\"status\"");
            if (!status_key){
                printf("Error: status not found at frame %d, LED%d\n", frame_idx, led_idx);
                break;
            }
            
            status_key = strchr(status_key, '[');
            if (!status_key){
                printf("Error: Invalid status array at frame %d, LED%d\n", frame_idx, led_idx);
                break;
            }
            
            status_key++;
            led_ptr = status_key;
            
            for (int pixel_idx = 0; pixel_idx < led_lens[led_idx]; pixel_idx++){
                skip_whitespace(&led_ptr);
                
                if (*led_ptr != '['){
                    printf("Error: Expected '[' at pixel %d, frame %d, LED%d\n", 
                           pixel_idx, frame_idx, led_idx);
                    break;
                }
                led_ptr++;
                skip_whitespace(&led_ptr);
                
                for (int rgb_idx = 0; rgb_idx < 3; rgb_idx++){
                    int color_value = read_number(&led_ptr);
                    frame_data[frame_idx][led_idx][pixel_idx][rgb_idx] = color_value;
                    
                    skip_whitespace(&led_ptr);
                    if (rgb_idx < 2 && *led_ptr == ','){
                        led_ptr++;
                    }
                    skip_whitespace(&led_ptr);
                }
                
                skip_whitespace(&led_ptr);
                if (*led_ptr == ','){
                    led_ptr++;
                    read_number(&led_ptr);
                }
                
                skip_whitespace(&led_ptr);
                if (*led_ptr == ']'){
                    led_ptr++;
                }
                
                skip_whitespace(&led_ptr);
                if (*led_ptr == ','){
                    led_ptr++;
                }
            }
            while (*led_ptr && *led_ptr != ']'){
                led_ptr++;
            }
            if (*led_ptr == ']'){
                led_ptr++;
            }
            skip_whitespace(&led_ptr);
            while (*led_ptr && *led_ptr != '}'){
                led_ptr++;
            }
            if (*led_ptr == '}'){
                led_ptr++;
            }
            skip_whitespace(&led_ptr);
            if (*led_ptr == ','){
                led_ptr++;
            }
        }
        ptr = led_ptr;
    }
    
    //write txt
    for (int frame_idx = 0; frame_idx < frame_count; frame_idx++){
        fprintf(txt_fp, "frame %d\n", frame_idx + 1);
        fprintf(txt_fp, "start: %d\n", frame_starts[frame_idx]);
        fprintf(txt_fp, "fade : %s\n", frame_fades[frame_idx] ? "true" : "false");
        
        for (int led_idx = 0; led_idx < led_count; led_idx++){
            for (int pixel_idx = 0; pixel_idx < led_lens[led_idx]; pixel_idx++){
                int r = frame_data[frame_idx][led_idx][pixel_idx][0];
                int g = frame_data[frame_idx][led_idx][pixel_idx][1];
                int b = frame_data[frame_idx][led_idx][pixel_idx][2];
                
                fprintf(txt_fp, "%d %d %d ", r, g, b);
            }
            
            if (led_idx < led_count - 1) {
                fprintf(txt_fp, "\n");
            }
        }
        
        if (frame_idx < frame_count - 1) {
            fprintf(txt_fp, "\n\n");
        }
    }
    
    fclose(txt_fp);
    
    for (int frame_idx = 0; frame_idx < frame_count; frame_idx++){
        printf("\nframe %d:\n", frame_idx + 1);
        printf("start: %d\n", frame_starts[frame_idx]);
        printf("fade : %s\n", frame_fades[frame_idx] ? "true" : "false");
        
        for (int led_idx = 0; led_idx < led_count; led_idx++){
            for (int pixel_idx = 0; pixel_idx < led_lens[led_idx]; pixel_idx++){
                int r = frame_data[frame_idx][led_idx][pixel_idx][0];
                int g = frame_data[frame_idx][led_idx][pixel_idx][1];
                int b = frame_data[frame_idx][led_idx][pixel_idx][2];
                
                printf("[%d %d %d]", r, g, b);
                
                if (pixel_idx < led_lens[led_idx] - 1) {
                    printf(", ");
                }
            }
            
            if (led_idx < led_count - 1) {
                printf("\n\n");
            }
        }
        printf("\n");
    }
    
    for (int f = 0; f < frame_count; f++){
        for (int l = 0; l < led_count; l++){
            for (int p = 0; p < led_lens[l]; p++){
                free(frame_data[f][l][p]);
            }
            free(frame_data[f][l]);
        }
        free(frame_data[f]);
    }
    free(frame_data);
    free(frame_starts);
    free(frame_fades);
    free(led_lens);
    free(content);
    
    printf("\nText data written to %s\n", output_file);
    return 0;
}