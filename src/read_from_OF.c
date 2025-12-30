#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void skip_whitespace(char **ptr){
    while (**ptr && isspace(**ptr)){
        (*ptr)++;
    }
}

//detect a frame
int parse_frame(char **ptr, int of_count, int *start_time, int *fade, int colors[][3]) {
    skip_whitespace(ptr);
    if (**ptr != '{') return 0;
    (*ptr)++; //skip '{'

    int colors_found = 0;
    
    while (**ptr) {
        skip_whitespace(ptr);
        
        if (**ptr == '}') {
            (*ptr)++; //skip '}'
            return 1; //sucess detect a frame
        }
        
        if (**ptr == '"') {
            (*ptr)++;
            
            char key[50] = {0};
            int key_idx = 0;
            while (**ptr && **ptr != '"'){
                key[key_idx++] = **ptr;
                (*ptr)++;
            }
            key[key_idx] = '\0';
            
            if (**ptr == '"') (*ptr)++;
            
            skip_whitespace(ptr);
            
            if (**ptr == ':'){
                (*ptr)++;
                skip_whitespace(ptr);
                
                if (strcmp(key, "start") == 0) {
                    *start_time = atoi(*ptr);
                    while (**ptr && (**ptr >= '0' && **ptr <= '9' || **ptr == '-')) (*ptr)++;
                }
                else if (strcmp(key, "fade") == 0){
                    if (strncmp(*ptr, "true", 4) == 0){
                        *fade = 1;
                        *ptr += 4;
                    }
                    else if (strncmp(*ptr, "false", 5) == 0){
                        *fade = 0;
                        *ptr += 5;
                    }
                }
                else if (strcmp(key, "status") == 0){
                    skip_whitespace(ptr);
                    if (**ptr == ':'){
                        (*ptr)++;
                        skip_whitespace(ptr);
                    }
                    
                    if (**ptr == '{'){
                        (*ptr)++;
                        
                        //read all color
                        while (**ptr && **ptr != '}') {
                            skip_whitespace(ptr);
                            if (**ptr == '"') {
                                (*ptr)++;
                                while (**ptr && **ptr != '"') (*ptr)++;
                                if (**ptr == '"') (*ptr)++;
                                
                                skip_whitespace(ptr);
                                
                                if (**ptr == ':') {
                                    (*ptr)++;
                                    skip_whitespace(ptr);
                                    
                                    //read color array
                                    if (**ptr == '[') {
                                        (*ptr)++;
                                        skip_whitespace(ptr);
                                        
                                        for (int i = 0; i < 3 && colors_found < of_count; i++){
                                            if (**ptr >= '0' && **ptr <= '9') {
                                                colors[colors_found][i] = atoi(*ptr);
                                                while (**ptr && (**ptr >= '0' && **ptr <= '9')) {
                                                    (*ptr)++;
                                                }
                                            }
                                            skip_whitespace(ptr);
                                            
                                            if (i < 2 && **ptr == ','){
                                                (*ptr)++;
                                                skip_whitespace(ptr);
                                            }
                                        }
                                        
                                        while (**ptr && **ptr != ']'){
                                            if (**ptr == ',') {
                                                (*ptr)++;
                                                while (**ptr && (**ptr >= '0' && **ptr <= '9')) (*ptr)++;
                                            }
                                            else (*ptr)++;

                                            skip_whitespace(ptr);
                                        }
                                        
                                        if (**ptr == ']') (*ptr)++;
                                        colors_found++;
                                    }
                                }
                            }
                    
                            skip_whitespace(ptr);
                            if (**ptr == ',') (*ptr)++;
                        }
                        
                        if (**ptr == '}') (*ptr)++;
                    }
                }
                else{
                    if (**ptr == '"'){
                        (*ptr)++;
                        while (**ptr && **ptr != '"'){
                            if (**ptr == '\\') (*ptr)++;
                            (*ptr)++;
                        }
                        if (**ptr == '"') (*ptr)++;
                    }
                    else if (**ptr == '['){
                        int depth = 1;
                        (*ptr)++;
                        while (**ptr && depth > 0){
                            if (**ptr == '[') depth++;
                            else if (**ptr == ']') depth--;
                            (*ptr)++;
                        }
                    }
                    else if (**ptr == '{'){
                        int depth = 1;
                        (*ptr)++;
                        while (**ptr && depth > 0){
                            if (**ptr == '{') depth++;
                            else if (**ptr == '}') depth--;
                            (*ptr)++;
                        }
                    }
                    else{
                        while (**ptr && **ptr != ',' && **ptr != '}') (*ptr)++;
                    }
                }
            }
        }
        skip_whitespace(ptr);
        if (**ptr == ',') (*ptr)++;
    }
    
    return 0;
}

int main() {
    //input OF_num
    int of_count;
    printf("OF num: ");
    scanf("%d", &of_count);
    
    FILE *fp = fopen("gen_blender/OF.json", "r");
    if (!fp){
        printf("can't open OF.json\n");
        return 1;
    }
    
    FILE *bin_fp = fopen("OF.bin", "wb");
    if (!bin_fp) {
        printf("can't create OF.bin\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *content = (char *)malloc(file_size + 1);
    fread(content, 1, file_size, fp);
    content[file_size] = '\0';
    fclose(fp);
    
    char *ptr = content;
    skip_whitespace(&ptr);
    
    ptr++; //skip '['
    
    int frame_num = 1;
    
    //color buffer
    int (*colors)[3] = malloc(of_count * sizeof(int[3]));
    if (!colors){
        printf("fail buffer for color\n");
        free(content);
        return 1;
    }
    
    int total_frames = 0;
    //detect all frame
    while (*ptr) {
        skip_whitespace(&ptr);
        if (*ptr == ']') break;
        
        int start_time = 0;
        int fade = 0;
        
        //initial color array
        for (int i = 0; i < of_count; i++){
            colors[i][0] = colors[i][1] = colors[i][2] = 0;
        }
        
        //detect a frame
        if (parse_frame(&ptr, of_count, &start_time, &fade, colors)) {
            printf("frame %d:\n", frame_num);
            printf("start: %d\n", start_time);
            printf("fade: %s\n", fade ? "true" : "false");
            
            // 1. Write start time (4 bytes)
            fwrite(&start_time, sizeof(int), 1, bin_fp);
            
            // 2. Write fade (1 byte)
            unsigned char fade_byte = (unsigned char)fade;
            fwrite(&fade_byte, 1, 1, bin_fp);
            
            // 3. Write all colors (3 bytes per OF)
            for (int i = 0; i < of_count; i++){
                unsigned char r = (unsigned char)colors[i][0];
                unsigned char g = (unsigned char)colors[i][1];
                unsigned char b = (unsigned char)colors[i][2];
                fwrite(&r, 1, 1, bin_fp);
                fwrite(&g, 1, 1, bin_fp);
                fwrite(&b, 1, 1, bin_fp);
            }

            //print color
            for (int i = 0; i < of_count; i++){
                printf("%d %d %d\n", colors[i][0], colors[i][1], colors[i][2]);
            }
            
            frame_num++;
            total_frames++;
            printf("\n");
            fflush(stdout);
        }
        else{
            printf("fail detect frame %d\n", frame_num);
            break;
        }
        
        skip_whitespace(&ptr);
        if (*ptr == ',') ptr++;
    }
    
    fclose(bin_fp);
    free(colors);
    free(content);
    return 0;
}