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

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_OF.json> <input_Control.dat> <output_OF.txt>\n", argv[0]);
        return 1;
    }
    
    char *input_file = argv[1];
    char *control_file = argv[2];
    char *output_file = argv[3];
    
    //input OF_num
    int of_count = 0;
    FILE *control_fp = fopen(control_file, "rb");
    if (!control_fp) {
        printf("can't open %s\n", control_file);
        return 1;
    }
    unsigned char fps;
    fread(&fps, 1, 1, control_fp);
    fread(&of_count, 1, 1, control_fp); //read second byte (OF_num)
    fclose(control_fp);
    printf("OF num from %s: %d\n", control_file, of_count);
    
    FILE *fp = fopen(input_file, "r");
    if (!fp){
        printf("can't open %s\n", input_file);
        return 1;
    }
    
    FILE *txt_fp = fopen(output_file, "w");
    if (!txt_fp) {
        printf("can't create %s\n", output_file);
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
    
    ptr++;
    
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
        if (parse_frame(&ptr, of_count, &start_time, &fade, colors)){
            fprintf(txt_fp, "frame %d:\n", frame_num);
            fprintf(txt_fp, "start: %d\n", start_time);
            fprintf(txt_fp, "fade : %s\n", fade ? "true" : "false");
            
            for (int i = 0; i < of_count; i++){
                fprintf(txt_fp, "%d %d %d ", colors[i][0], colors[i][1], colors[i][2]);
            }
            
            if (frame_num < total_frames + 1){
                fprintf(txt_fp, "\n\n");
            }
            
            printf("frame %d:\n", frame_num);
            printf("start: %d\n", start_time);
            printf("fade : %s\n", fade ? "true" : "false");
            
            //print color
            for (int i = 0; i < of_count; i++){
                printf("[%d %d %d]", colors[i][0], colors[i][1], colors[i][2]);
                
                if (i < of_count - 1) {
                    printf(", ");
                }
            }
            
            frame_num++;
            total_frames++;
            printf("\n\n");
            fprintf(txt_fp, "\n\n");
            fflush(stdout);
        }
        else{
            printf("fail detect frame %d\n", frame_num);
            break;
        }
        
        skip_whitespace(&ptr);
        if (*ptr == ',') ptr++;
    }
    
    fclose(txt_fp);
    free(colors);
    free(content);
    
    printf("\nText data written to %s\n", output_file);
    return 0;
}