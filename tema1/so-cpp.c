#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hashmap.h"

#define INITIAL_HASHMAP_SIZE 10000

#define FILE_NAME_LENGTH 80
#define INITIAL_HEADER_FILES_LENGTH 100
#define LINE_LENGTH 256
#define NUM_LINES 500
#define SMALL_BUFFER_LEN 20

#define SYMBOL_FLAG "-D"
#define HEADERS_PATH_FLAG "-I"
#define OUTPUT_FILE_FLAG "-o"
#define FLAG_START '-'

#define DELIMS "\t []{}<>=+-*/%!&|^.,:;()\\"
#define DEFINE_PARSING_DELIMS " \r\n"

#define EMPTY_LINE_LINUX "\r\n"
#define EMPTY_LINE_WINDOWS "\n"
#define DEFINE_DIRECTIVE "#define"
#define UNDEF_DIRECTIVE "#undef"
#define INCLUDE_DIRECTIVE "#include"

int line_should_be_skipped(char* line) {
    if (strncmp(line, EMPTY_LINE_LINUX, strlen(line)) == 0 ||
        strncmp(line, EMPTY_LINE_WINDOWS, strlen(line) == 0) ||
        strncmp(line, DEFINE_DIRECTIVE, strlen(DEFINE_DIRECTIVE)) == 0 ||
        strncmp(line, UNDEF_DIRECTIVE, strlen(UNDEF_DIRECTIVE)) == 0) {
        return 1;
    }

    return 0;
}

void add_input_symbol_mapping(char* symbol, Hashmap* hashmap) {
    char key[SMALL_BUFFER_LEN];
    char value[SMALL_BUFFER_LEN];

    strcpy(key, strtok(symbol, "="));
    strcpy(value, strtok(NULL, symbol));
    put_value(hashmap, key, value);
}

void add_symbol_mapping(char* key, char* value, Hashmap* hashmap) {
    // TODO: more covering impl
    value[strlen(value) - 1] = '\0';
    put_value(hashmap, key, value);
}

int intercalate_symbol_if_present(char* key, Hashmap* hashmap, Hashmap* deleted_hashmap, char buffer[LINE_LENGTH]) {
    if (key == NULL || has_key(deleted_hashmap, key)) {
        return 0;
    }

    if (has_key(hashmap, key) == 1) {
        char* occurence = strstr(buffer, key);
        int position = occurence - buffer;
        char tmp[LINE_LENGTH] = {0};

        strncpy(tmp, buffer, position);
        strcat(tmp, (char*)get_value(hashmap, key));
        strcat(tmp, buffer + position + strlen(key));
        strncpy(buffer, tmp, LINE_LENGTH);

        return 1;
    } else {
        return 0;
    }
}

void expand_defines(char** input_file_lines, int num_lines, int* new_num_lines, Hashmap* hashmap, Hashmap* deleted_symbols_hashmap) {
    for (int i = 0; i < num_lines; i++) {
        if (strncmp(input_file_lines[i], DEFINE_DIRECTIVE, strlen(DEFINE_DIRECTIVE)) == 0) {
            char tmp[LINE_LENGTH];
            strcpy(tmp, input_file_lines[i]);
            char key[SMALL_BUFFER_LEN];
            char value[SMALL_BUFFER_LEN];
            strcpy(value, "");

            char* token = strtok(tmp, DEFINE_PARSING_DELIMS);
            int count = 0;
            while (token != NULL) {
                if (count == 1) {
                    strcpy(key, token);
                } else if (count >= 2) {
                    if (has_key(hashmap, token)) {
                        sprintf(value + strlen(value), "%s ", (char*)get_value(hashmap, token));
                    } else {
                        sprintf(value + strlen(value), "%s ", token);
                    }
                }
                token = strtok(NULL, DEFINE_PARSING_DELIMS);
                count++;
            }

            add_symbol_mapping(key, value, hashmap);
        } else if (strncmp(input_file_lines[i], UNDEF_DIRECTIVE, strlen(UNDEF_DIRECTIVE)) == 0) {
            char tmp[LINE_LENGTH];
            strcpy(tmp, input_file_lines[i]);

            strtok(tmp, DEFINE_PARSING_DELIMS);
            char* key = strtok(NULL, DEFINE_PARSING_DELIMS);
            put_value(deleted_symbols_hashmap, key, "");
        } else {
            int found_symbol = 0;
            do {
                found_symbol = 0;
                char tmp[LINE_LENGTH];
                strcpy(tmp, input_file_lines[i]);
                
                char* token;
                token = strtok(tmp, DELIMS);
                found_symbol = intercalate_symbol_if_present(token, hashmap, deleted_symbols_hashmap, input_file_lines[i]);
                
                while (token != NULL) {
                    token = strtok(NULL, DELIMS);
                    found_symbol = intercalate_symbol_if_present(token, hashmap, deleted_symbols_hashmap, input_file_lines[i]);
                }
            } while (found_symbol);
        }
    }
}

void expand_includes() {

}

void expand_conditionals() {

}

int main(int argc, char **argv) {
    int input_file_specified = 0;
    char input_file_name[FILE_NAME_LENGTH] = {0};

    int output_file_specified = 0;
    char output_file_name[FILE_NAME_LENGTH] = {0};

    int num_header_files = 0;
    char header_file_names[INITIAL_HEADER_FILES_LENGTH][FILE_NAME_LENGTH];

    Hashmap* symbol_hashmap = init_hashmap(INITIAL_HASHMAP_SIZE);
    Hashmap* deleted_symbols_hashmap = init_hashmap(INITIAL_HASHMAP_SIZE);


    // TODO: extract input flags as function
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], SYMBOL_FLAG, strlen(argv[i])) == 0) {
            add_input_symbol_mapping(argv[i + 1], symbol_hashmap);
            i++;
        } else if (strncmp(argv[i], SYMBOL_FLAG, strlen(SYMBOL_FLAG)) == 0) {
            add_input_symbol_mapping(argv[i] + 2, symbol_hashmap);
        } else if (strncmp(argv[i], HEADERS_PATH_FLAG, strlen(argv[i])) == 0) {
            strcpy(header_file_names[num_header_files++], argv[i + 1]);
            i++;
        } else if (strncmp(argv[i], HEADERS_PATH_FLAG, strlen(HEADERS_PATH_FLAG)) == 0) {
            strcpy(header_file_names[num_header_files++], argv[i] + strlen(HEADERS_PATH_FLAG));
        } else if (strncmp(argv[i], OUTPUT_FILE_FLAG, strlen(OUTPUT_FILE_FLAG)) == 0 && !output_file_specified) {
            output_file_specified = 1;
            strcpy(output_file_name, argv[i + 1]);
            i++;
        } else if (argv[i][0] != FLAG_START && strlen(argv[i]) > 2 && (!input_file_specified || !output_file_specified)) {
            if (!input_file_specified) {
                input_file_specified = 1;
                strcpy(input_file_name, argv[i]);
            } else if (!output_file_specified) {
                output_file_specified = 1;
                strcpy(output_file_name, argv[i]);
            }
        } else {
            perror("Input args");
            exit(EXIT_FAILURE);
        }
    }

    if (!input_file_specified) {
        scanf("%s", input_file_name);
    }

    char** file_lines = calloc(NUM_LINES, sizeof(char*));
    for (int i = 0; i < NUM_LINES; i++) {
        file_lines[i] = calloc(LINE_LENGTH, sizeof(char));
    }

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

    int new_num_lines = 0;
    expand_defines(file_lines, num_lines, &new_num_lines, symbol_hashmap, deleted_symbols_hashmap);

    if (output_file_specified) {
        FILE* output_fp = fopen(output_file_name, "w");
        if (output_fp == NULL) {
            perror("fopen");
            free_hashmap(symbol_hashmap);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_lines; i++) {
            if (!line_should_be_skipped(file_lines[i])) {
                fprintf(output_fp, "%s", file_lines[i]);
            }
        }
        fclose(output_fp);
    } else {
        for (int i = 0; i < num_lines; i++) {
            if (!line_should_be_skipped(file_lines[i])) {
                printf("%s", file_lines[i]);
            }
        }
    }

    fclose(input_fp);
    free_hashmap(symbol_hashmap);
    free_hashmap(deleted_symbols_hashmap);
    for (int i = 0; i < NUM_LINES; i++) {
        free(file_lines[i]);
    }
    free(file_lines);
    return 0;
}