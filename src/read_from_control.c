#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void skip_whitespace(char **ptr){
    while (**ptr && isspace(**ptr)) (*ptr)++;
}

// detect OFPARTS
int count_of_items(char *start){
    char *ptr = start;
    int count = 0;
    
    //skip whitespace to find first {
    skip_whitespace(&ptr);
    if (*ptr != '{') return 0;
    ptr++; //skip {
    
    int depth = 1;
    
    while (*ptr && depth > 0){
        skip_whitespace(&ptr);
        
        if (*ptr == '}'){
            depth--;
            if (depth == 0) break;
            ptr++;
        }
        else if (*ptr == '{'){
            depth++;
            ptr++;
        }
        else if (*ptr == '"'){
            //find a OF object in OFPARTS
            count++;
            
            //skip all obgect
            ptr++;
            while (*ptr && *ptr != '"'){
                if (*ptr == '\\') ptr++;
                ptr++;
            }
            if (*ptr == '"') ptr++;
            
            skip_whitespace(&ptr);
            if (*ptr == ':') ptr++;
            skip_whitespace(&ptr);
            
            while (*ptr && *ptr != ',' && *ptr != '}'){
                ptr++;
            }
            
            skip_whitespace(&ptr);
            if (*ptr == ',') ptr++;
        }
        else ptr++;
    }
    return count;
}

// detect OFPARTS & record len of each LED
int count_led_items_and_get_lens(char *start, int **len_array){
    char *ptr = start;
    int count = 0;
    
    //skip whitespace to find first {
    skip_whitespace(&ptr);
    if (*ptr != '{') return 0;
    ptr++;
    
    int depth = 1;
    char *scan_ptr = ptr;
    int temp_count = 0;
    while (*scan_ptr && depth > 0){
        skip_whitespace(&scan_ptr);
        
        if (*scan_ptr == '}'){
            depth--;
            if (depth == 0) break;
            scan_ptr++;
        }
        else if (*scan_ptr == '{'){
            depth++;
            scan_ptr++;
        }
        else if (*scan_ptr == '"'){
            //find a LED object
            temp_count++;
            
            //skip a object
            scan_ptr++;
            while (*scan_ptr && *scan_ptr != '"'){
                if (*scan_ptr == '\\') scan_ptr++;
                scan_ptr++;
            }
            if (*scan_ptr == '"') scan_ptr++;
            
            skip_whitespace(&scan_ptr);
            if (*scan_ptr == ':') scan_ptr++;
            skip_whitespace(&scan_ptr);
            
            if (*scan_ptr == '{'){
                int obj_depth = 1;
                scan_ptr++;
                while (*scan_ptr && obj_depth > 0){
                    if (*scan_ptr == '{') obj_depth++;
                    else if (*scan_ptr == '}') obj_depth--;
                    scan_ptr++;
                }
            }
            
            skip_whitespace(&scan_ptr);
            if (*scan_ptr == ',') scan_ptr++;
        }
        else scan_ptr++;
    }
    
    count = temp_count;
    
    //allocate buffer to save len
    if (count > 0){
        *len_array = (int *)malloc(count * sizeof(int));
        if (*len_array == NULL) return 0;
    }
    else{
        *len_array = NULL;
        return 0;
    }
    
    //rescan to record len
    ptr = start;
    skip_whitespace(&ptr);
    if (*ptr != '{') return 0;
    ptr++;
    
    depth = 1;
    int index = 0;
    
    while (*ptr && depth > 0 && index < count){
        skip_whitespace(&ptr);
        
        if (*ptr == '}'){
            depth--;
            if (depth == 0) break;
            ptr++;
        }
        else if (*ptr == '{'){
            depth++;
            ptr++;
        }
        else if (*ptr == '"'){
            ptr++;
            while (*ptr && *ptr != '"'){
                if (*ptr == '\\') ptr++;
                ptr++;
            }
            if (*ptr == '"') ptr++;
            
            skip_whitespace(&ptr);
            if (*ptr == ':') ptr++;
            skip_whitespace(&ptr);
            
            if (*ptr == '{'){
                ptr++;
                int obj_depth = 1;
                
                while (*ptr && obj_depth > 0){
                    skip_whitespace(&ptr);
                    
                    if (*ptr == '}'){
                        obj_depth--;
                        if (obj_depth == 0){
                            ptr++;
                            break;
                        }
                    }
                    else if (*ptr == '{'){
                        obj_depth++;
                        ptr++;
                    }
                    else if (*ptr == '"'){
                        if (strncmp(ptr, "\"len\"", 5) == 0){
                            ptr += 5;
                            
                            skip_whitespace(&ptr);
                            if (*ptr == ':') ptr++;
                            skip_whitespace(&ptr);
                            
                            if (*ptr >= '0' && *ptr <= '9'){
                                (*len_array)[index] = atoi(ptr);
                                index++;
                            }
                            
                            while (*ptr && *ptr >= '0' && *ptr <= '9') ptr++; 
                        }
                        else{
                            ptr++;
                            while (*ptr && *ptr != '"'){
                                if (*ptr == '\\') ptr++;
                                ptr++;
                            }
                            if (*ptr == '"') ptr++;
                            
                            skip_whitespace(&ptr);
                            if (*ptr == ':') ptr++;
                            skip_whitespace(&ptr);
                            
                            if (*ptr == '"'){
                                ptr++;
                                while (*ptr && *ptr != '"'){
                                    if (*ptr == '\\') ptr++;
                                    ptr++;
                                }
                                if (*ptr == '"') ptr++;
                            }
                            else{
                                while (*ptr && *ptr != ',' && *ptr != '}') ptr++;
                            }
                        }
                        
                        skip_whitespace(&ptr);
                        if (*ptr == ',') ptr++;
                    }
                    else ptr++;
                }
            }
            
            skip_whitespace(&ptr);
            if (*ptr == ',') ptr++;
        }
        else ptr++;
    }
    return count;
}

int main(int argc, char *argv[]){
    if (argc < 3) {
        printf("Usage: %s <input_Control.json> <output_Control.dat>\n", argv[0]);
        return 1;
    }
    FILE *fp = fopen(argv[1], "r");
    if (!fp){
        printf("can't open input_Control.json\n");
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *content = (char *)malloc(file_size + 1);
    fread(content, 1, file_size, fp);
    content[file_size] = '\0';
    fclose(fp);

    int fps = 0;
    int of_count = 0;
    int led_count = 0;
    int *led_lens = NULL;
    
    char *ptr = content;

    // 1. find fps
    char *fps_key = strstr(ptr, "\"fps\"");
    if (fps_key){
        fps_key = strchr(fps_key, ':');
        if (fps_key) fps = atoi(fps_key + 1);
    }

    // 2. find OFPARTS
    char *of_start = strstr(ptr, "\"OFPARTS\"");
    if (of_start){
        of_start = strchr(of_start, ':');
        if (of_start){
            of_start++;
            of_count = count_of_items(of_start);
        }
    }

    // 3. find LEDPARTS
    char *led_start = strstr(ptr, "\"LEDPARTS\"");
    if (led_start){
        led_start = strchr(led_start, ':');
        if (led_start){
            led_start++;
            led_count = count_led_items_and_get_lens(led_start, &led_lens);
        }
    }

    free(content);
    FILE *out = fopen(argv[2], "wb");
    if (!out){
        printf("can't construct output_Control.dat\n");
        if (led_lens) free(led_lens);
        return 1;
    }

    unsigned char fps_byte = (unsigned char)fps;
    fwrite(&fps_byte, 1, 1, out);

    unsigned char of_count_byte = (unsigned char)of_count;
    fwrite(&of_count_byte, 1, 1, out);

    unsigned char led_count_byte = (unsigned char)led_count;
    fwrite(&led_count_byte, 1, 1, out);

    if (led_count > 0 && led_lens != NULL){
        for (int i = 0; i < led_count; i++){
            unsigned char len_byte = (unsigned char)led_lens[i];
            fwrite(&len_byte, 1, 1, out);
        }
    }
    
    fclose(out);

    printf("\nresult in %s\n", argv[2]);
    printf("fps = %d\n", fps);
    printf("OFPARTS num = %d\n", of_count);
    printf("LEDPARTS num = %d\n", led_count);
    
    if (led_count > 0 && led_lens != NULL){
        printf("LED len : ");
        for (int i = 0; i < led_count; i++){
            printf("%d", led_lens[i]);
            if (i < led_count - 1) printf(" ");
        }
        printf("\n\n");
    }
    if (led_lens) free(led_lens);
    return 0;
}