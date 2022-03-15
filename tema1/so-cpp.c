#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hashmap.h"

#define INITIAL_HASHMAP_SIZE 10

#define FILE_NAME_LENGTH 80
#define INITIAL_HEADER_FILES_LENGTH 100
#define LINE_LENGTH 256
#define NUM_LINES 500

#define SYMBOL_FLAG "-D"
#define HEADERS_PATH_FLAG "-I"
#define OUTPUT_FILE_FLAG "-o"
#define FLAG_START '-'

void parse_input_symbol(char* symbol, Hashmap* hashmap) {

}

int main(int argc, char **argv) {
    int input_file_specified = 0;
    char input_file_name[FILE_NAME_LENGTH] = {0};

    int output_file_specified = 0;
    char output_file_name[FILE_NAME_LENGTH] = {0};

    int num_header_files = 0;
    char header_file_names[INITIAL_HEADER_FILES_LENGTH][FILE_NAME_LENGTH];

    Hashmap* symbol_hashmap = init_hashmap(INITIAL_HASHMAP_SIZE);
    // put_value(symbol_hashmap, "ALA", "BALA");
    // printf("%s\n", (char*)get_value(symbol_hashmap, "ALA"));
    // put_value(symbol_hashmap, "PORTO", "CALA");
    // printf("%s\n", (char*)get_value(symbol_hashmap, "PORTO"));
    // put_value(symbol_hashmap, "ALA", "BALA2");
    // printf("%s\n", (char*)get_value(symbol_hashmap, "ALA"));
    // put_value(symbol_hashmap, "LAST", "BALA3");
    // printf("%s\n", (char*)get_value(symbol_hashmap, "LAST"));

    // TODO: extract input flags as function
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], SYMBOL_FLAG, strlen(argv[i])) == 0) {
            // TODO: process token func
            i++;
        } else if (strncmp(argv[i], SYMBOL_FLAG, strlen(SYMBOL_FLAG)) == 0) {
            // TODO: process token func
        } else if (strncmp(argv[i], HEADERS_PATH_FLAG, strlen(argv[i])) == 0) {
            strcpy(header_file_names[num_header_files++], argv[i + 1]);
            i++;
        } else if (strncmp(argv[i], HEADERS_PATH_FLAG, strlen(HEADERS_PATH_FLAG)) == 0) {
            strcpy(header_file_names[num_header_files++], argv[i] + strlen(HEADERS_PATH_FLAG));
        } else if (strncmp(argv[i], OUTPUT_FILE_FLAG, strlen(OUTPUT_FILE_FLAG)) == 0 && !output_file_specified) {
            output_file_specified = 1;
            strcpy(output_file_name, argv[i + 1]);
            i++;
        } else if (argv[i][0] != FLAG_START && strlen(argv[i]) > 2 && !input_file_specified) {
            input_file_specified = 1;
            strcpy(input_file_name, argv[i]);
        } else {
            //printf("Bad input\n");
            perror("Input args");
            exit(EXIT_FAILURE);
        }
    }

    if (!input_file_specified) {
        scanf("%s", input_file_name);
    }

    char file_lines[NUM_LINES][LINE_LENGTH] = {0};
    char line[LINE_LENGTH] = {0};
    int num_lines = 0;

    FILE* input_fp = fopen(input_file_name, "r");
    if (input_fp == NULL) {
        perror("fopen");
        free_hashmap(symbol_hashmap);
        exit(EXIT_FAILURE);
    }

    while (fgets(line, LINE_LENGTH, input_fp)) {
        num_lines++;
        strcpy(file_lines[num_lines++], line);
    }

    if (output_file_specified) {
        FILE* output_fp = fopen(output_file_name, "w");
        if (output_fp == NULL) {
            perror("fopen");
            free_hashmap(symbol_hashmap);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_lines; i++) {
            fprintf(output_fp, "%s", file_lines[i]);
        }
        fclose(output_fp);
    } else {
        for (int i = 0; i < num_lines; i++) {
            printf("%s", file_lines[i]);
        }
    }

    fclose(input_fp);
    free_hashmap(symbol_hashmap);
    return 0;
}