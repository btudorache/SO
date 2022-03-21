#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hashmap.h"

#define INITIAL_HASHMAP_SIZE 4000
#define FILE_NAME_LENGTH 80
#define INITIAL_HEADER_FILES_LENGTH 100
#define LINE_LENGTH 256
#define NUM_LINES 150
#define SMALL_BUFFER_LEN 20
#define SMALL_ARRAY_SIZE 200

#define SYMBOL_FLAG "-D"
#define HEADERS_PATH_FLAG "-I"
#define OUTPUT_FILE_FLAG "-o"
#define FLAG_START '-'

#define DELIMS "\t []{}<>=+-*/%!&|^.,:;()\\"
#define LINE_PARSING_DELIM " \r\n"
#define EQUAL_SIGN_DELIM "="
#define SLASH_STRING "\\"
#define PATH_CHAR '/'

#define EMPTY_LINE_LINUX "\r\n"
#define EMPTY_LINE_WINDOWS "\n"

#define DEFINE_DIRECTIVE "#define"
#define UNDEF_DIRECTIVE "#undef"
#define INCLUDE_DIRECTIVE "#include"
#define IF_DIRECTIVE "#if"
#define ELIF_DIRECTIVE "#elif"
#define ELSE_DIRECTIVE "#else"
#define ENDIF_DIRECTIVE "#endif"
#define IFDEF_DIRECTIVE "#ifdef"
#define IFNDEF_DIRECTIVE "#ifndef"

char** allocate_lines_array(int num_lines, int line_length) {
    int i = 0;
    char** new_char_matrix = calloc(num_lines, sizeof(char*));
    if (new_char_matrix == NULL) {
        return NULL;
    }
    for (i = 0; i < num_lines; i++) {
        new_char_matrix[i] = calloc(line_length, sizeof(char));
        if (new_char_matrix[i] == NULL) {
            free(new_char_matrix);
            return NULL;
        }
    }
    return new_char_matrix;
}

void free_matrix(char** input_file_lines, int num_lines) {
    int i = 0;
    if (input_file_lines == NULL) {
        return;
    }
    for (i = 0; i < num_lines; i++) {
        free(input_file_lines[i]);
    }
    free(input_file_lines);
}

void free_everything(char** lines_array, Hashmap* hashmap, Hashmap* undefined_hashmap) {
    free_hashmap(hashmap);
    free_hashmap(undefined_hashmap);
    free_matrix(lines_array, NUM_LINES);
}

int line_should_be_skipped(char* line) {
    if (strncmp(line, EMPTY_LINE_LINUX, strlen(line)) == 0 ||
        strncmp(line, EMPTY_LINE_WINDOWS, strlen(line) == 0) ||
        strncmp(line, DEFINE_DIRECTIVE, strlen(DEFINE_DIRECTIVE)) == 0 ||
        strncmp(line, UNDEF_DIRECTIVE, strlen(UNDEF_DIRECTIVE)) == 0 || 
        strncmp(line, IF_DIRECTIVE, strlen(IF_DIRECTIVE)) == 0 ||
        strncmp(line, ENDIF_DIRECTIVE, strlen(ENDIF_DIRECTIVE)) == 0 ||
        strncmp(line, ELSE_DIRECTIVE, strlen(ELSE_DIRECTIVE)) == 0 ||
        strncmp(line, ELIF_DIRECTIVE, strlen(ELIF_DIRECTIVE)) == 0 ||
        strncmp(line, IFDEF_DIRECTIVE, strlen(IFDEF_DIRECTIVE)) == 0 ||
        strncmp(line, IFNDEF_DIRECTIVE, strlen(IFNDEF_DIRECTIVE)) == 0) {
        return 1;
    }

    return 0;
}

void add_input_symbol_mapping(char* symbol, Hashmap* hashmap) {
    char* key = strtok(symbol, EQUAL_SIGN_DELIM);
    char* value = strtok(NULL, EQUAL_SIGN_DELIM);
    if (value == NULL) {
        put_value(hashmap, key, "");
    } else {
        put_value(hashmap, key, value);
    }
}

int intercalate_symbol_if_present(char* key, Hashmap* hashmap, Hashmap* deleted_hashmap, char buffer[LINE_LENGTH], int index) {
    char tmp[LINE_LENGTH] = {0};

    if (key == NULL || has_key(deleted_hashmap, key)) {
        return 0;
    }

    if (has_key(hashmap, key) == 1) {
        strncpy(tmp, buffer, index);
        strcat(tmp, (char*)get_value(hashmap, key));
        strcat(tmp, buffer + index + strlen(key));
        strncpy(buffer, tmp, LINE_LENGTH);
        return 1;
    } else {
        return 0;
    }
}

void toggle_branches(int* include_line, int* chosen_branch, int literal) {
    if (literal != 0 && !(*chosen_branch)) {
        *include_line = 1;
        *chosen_branch = 1;
    } else {
        *include_line = 0;
    }
}

void check_branch(char line[LINE_LENGTH], Hashmap* hashmap, int* include_line, int* chosen_branch) {
    char* literal = NULL;
    int int_literal = 0;
    strtok(line, LINE_PARSING_DELIM);
    literal = strtok(NULL, LINE_PARSING_DELIM);
    if (sscanf(literal, "%d", &int_literal) || 
        (has_key(hashmap, literal) && sscanf((char*)get_value(hashmap, literal), "%d", &int_literal))) {
        toggle_branches(include_line, chosen_branch, int_literal);
    }
}

void check_ifdef_ifndef(char line[LINE_LENGTH], Hashmap* hashmap, int* include_line) {
    char* literal = NULL;
    strtok(line, LINE_PARSING_DELIM);
    literal = strtok(NULL, LINE_PARSING_DELIM);
    
    if (strncmp(line, IFDEF_DIRECTIVE, strlen(IFDEF_DIRECTIVE)) == 0) {
        if (has_key(hashmap, literal)) {
            *include_line = 1;
        } else {
            *include_line = 0;
        }
    } else {
        if (!has_key(hashmap, literal)) {
            *include_line = 1;
        } else {
            *include_line = 0;
        }
    }
}

char** expand_directives(char** input_file_lines, int num_lines, int* new_num_lines, Hashmap* hashmap, Hashmap* deleted_symbols_hashmap) {
    int new_num_lines_var = 0;
    int include_line = 1;
    int chosen_branch = 0;
    int multiline_define_jump = 0;
    int found_symbol = 0;
    char key[SMALL_BUFFER_LEN] = {0};
    char value[SMALL_BUFFER_LEN] = {0};
    char tmp[LINE_LENGTH] = {0};
    char extracted_char[SMALL_BUFFER_LEN] = {0};
    char* token = NULL;
    char* line_token = NULL;
    char* found_slash = NULL;
    char* undef_key = NULL;
    int count = 0;
    int check_next_line = 0;
    int i = 0;
    char** expanded_directives_lines = allocate_lines_array(NUM_LINES, LINE_LENGTH);
    if (expanded_directives_lines == NULL) {
        return NULL;
    }

    for (i = 0; i < num_lines; i++) {
        strcpy(tmp, input_file_lines[i]);

        if (strncmp(input_file_lines[i], DEFINE_DIRECTIVE, strlen(DEFINE_DIRECTIVE)) == 0 && include_line) {
            strcpy(value, "");

            token = strtok(tmp, LINE_PARSING_DELIM);
            count = 0;
            while (token != NULL) {
                if (strncmp(token, SLASH_STRING, strlen(token)) == 0) {
                    check_next_line = 0;
                    do {
                        check_next_line = 0;
                        strcpy(tmp, input_file_lines[multiline_define_jump++ + i + 1]);
                        line_token = strtok(tmp, LINE_PARSING_DELIM);
                        while (line_token != NULL) {
                            found_slash = strrchr(line_token, '\\');
                            if (found_slash != NULL) {
                                strncpy(extracted_char, line_token, found_slash - line_token);
                                sprintf(value + strlen(value), "%s ", extracted_char);
                                check_next_line = 1;
                                break;
                            }

                            if (has_key(hashmap, line_token)) {
                                sprintf(value + strlen(value), "%s ", (char*)get_value(hashmap, line_token));
                            } else {
                                sprintf(value + strlen(value), "%s ", line_token);
                            }
                            line_token = strtok(NULL, LINE_PARSING_DELIM);
                        }
                    } while (check_next_line);
                    i += multiline_define_jump + 1;
                    break;
                }

                if (count == 1) {
                    strcpy(key, token);
                } else if (count >= 2) {
                    if (has_key(hashmap, token)) {
                        sprintf(value + strlen(value), "%s ", (char*)get_value(hashmap, token));
                    } else {
                        sprintf(value + strlen(value), "%s ", token);
                    }
                }
                token = strtok(NULL, LINE_PARSING_DELIM);
                count++;
            }

            value[strlen(value) - 1] = '\0';
            put_value(hashmap, key, value);
        } else if (strncmp(input_file_lines[i], UNDEF_DIRECTIVE, strlen(UNDEF_DIRECTIVE)) == 0) {
            strtok(tmp, LINE_PARSING_DELIM);
            undef_key = strtok(NULL, LINE_PARSING_DELIM);
            put_value(deleted_symbols_hashmap, undef_key, "");
        } else if (strncmp(input_file_lines[i], IFDEF_DIRECTIVE, strlen(IFDEF_DIRECTIVE)) == 0 ||
                   strncmp(input_file_lines[i], IFNDEF_DIRECTIVE, strlen(IFNDEF_DIRECTIVE)) == 0) {
            check_ifdef_ifndef(tmp, hashmap, &include_line);
        } else if (strncmp(input_file_lines[i], IF_DIRECTIVE, strlen(IF_DIRECTIVE)) == 0 ||
                   strncmp(input_file_lines[i], ELIF_DIRECTIVE, strlen(ELIF_DIRECTIVE)) == 0) {
            check_branch(tmp, hashmap, &include_line, &chosen_branch);
        } else if (strncmp(input_file_lines[i], ELSE_DIRECTIVE, strlen(ELSE_DIRECTIVE)) == 0) {
            toggle_branches(&include_line, &chosen_branch, 1);
        } else if (strncmp(input_file_lines[i], ENDIF_DIRECTIVE, strlen(ENDIF_DIRECTIVE)) == 0) {
            include_line = 1;
            chosen_branch = 0;
        }
        else {
            found_symbol = 0;
            do {
                found_symbol = 0;
                strcpy(tmp, input_file_lines[i]);
                
                token = strtok(tmp, DELIMS);
                found_symbol = intercalate_symbol_if_present(token, hashmap, deleted_symbols_hashmap, input_file_lines[i], token - tmp);
                if (found_symbol) {
                    continue;
                }
                
                while (token != NULL) {
                    token = strtok(NULL, DELIMS);
                    found_symbol = intercalate_symbol_if_present(token, hashmap, deleted_symbols_hashmap, input_file_lines[i], token - tmp);
                    if (found_symbol) {
                        break;
                    }
                }
            } while (found_symbol);
        }

        if (include_line) {
            strcpy(expanded_directives_lines[new_num_lines_var++], input_file_lines[i]);
        }
    }

    free_matrix(input_file_lines, NUM_LINES);
    *new_num_lines = new_num_lines_var;
    return expanded_directives_lines;
}

void extract_header_file_name(char line[LINE_LENGTH], char header_file_name[FILE_NAME_LENGTH]) {         
    char tmp[LINE_LENGTH];
    strcpy(tmp, line);
    strtok(tmp, LINE_PARSING_DELIM);
    strcpy(header_file_name, strtok(NULL, LINE_PARSING_DELIM) + 1);
    header_file_name[strlen(header_file_name) - 1] = '\0';
}

int expand_include_file(char*** expanded_include,
                        char header_file_name[FILE_NAME_LENGTH], 
                        char header_file_directory[][FILE_NAME_LENGTH], 
                        int num_header_files, 
                        int* num_include_lines)  {
    char dir_header_file_name[FILE_NAME_LENGTH] = {0};
    char** expanded_includes_file = allocate_lines_array(NUM_LINES, LINE_LENGTH);
    int expanded_includes_file_lines = 0;
    int found_file = 0;
    char** partial_include_file = NULL;
    int partial_include_file_num = 0;
    int partial_status = 0;
    int i = 0;
    FILE* header_file_fp = fopen(header_file_name, "r");
    if (expanded_includes_file == NULL) {
        return 1;
    }

    if (header_file_fp == NULL) {
        for (i = 0; i < num_header_files; i++) {
            sprintf(dir_header_file_name, "%s/%s", header_file_directory[i], header_file_name);
            
            header_file_fp = fopen(dir_header_file_name, "r");
            if (header_file_fp != NULL) {
                found_file = 1;
                break;
            }
        }
    } else {
        found_file = 1;
    }

    if (!found_file) {
        return 1;
    } else {
        while (fgets(expanded_includes_file[expanded_includes_file_lines], LINE_LENGTH, header_file_fp)) {
            if (strncmp(expanded_includes_file[expanded_includes_file_lines], INCLUDE_DIRECTIVE, strlen(INCLUDE_DIRECTIVE)) == 0) {
                char partial_header_file_name[FILE_NAME_LENGTH] = {0};
                extract_header_file_name(expanded_includes_file[expanded_includes_file_lines], partial_header_file_name);

                partial_status = expand_include_file(&partial_include_file,
                                                            partial_header_file_name,
                                                            header_file_directory,
                                                            num_header_files,
                                                            &partial_include_file_num);
                if (partial_status != 0) {
                    free_matrix(partial_include_file, NUM_LINES);
                    return 1;
                }

                for (i = 0; i < partial_include_file_num; i++) {
                    strcpy(expanded_includes_file[expanded_includes_file_lines + i + 1], partial_include_file[i]);
                }

                expanded_includes_file_lines += partial_include_file_num;
                free_matrix(partial_include_file, NUM_LINES);
            }
            expanded_includes_file_lines++;
        }
        fclose(header_file_fp);
    }

    *num_include_lines = expanded_includes_file_lines;
    *expanded_include = expanded_includes_file;
    return 0;
}

int expand_includes(char*** expanded_include_lines, char** input_file_lines, int num_lines, int* new_num_lines, char header_file_directory[][FILE_NAME_LENGTH], int num_header_files) {
    char header_file_name[FILE_NAME_LENGTH] = {0};
    int expanded_includes_file_len = 0;
    int status = 0;
    int i = 0;
    int j = 0;
    int num_include_lines = 0;
    char** expanded_include = NULL;
    char** expanded_includes_file = allocate_lines_array(NUM_LINES, LINE_LENGTH);
    if (expanded_includes_file == NULL) {
        free_matrix(input_file_lines, NUM_LINES);
        return 1;
    }

    for (i = 0; i < num_lines; i++) {
        if (strncmp(input_file_lines[i], INCLUDE_DIRECTIVE, strlen(INCLUDE_DIRECTIVE)) == 0) {         
            extract_header_file_name(input_file_lines[i], header_file_name);

            num_include_lines = 0;
            expanded_include = NULL;
            status = expand_include_file(&expanded_include, header_file_name, header_file_directory, num_header_files, &num_include_lines);
            for(j = 0; j < num_include_lines; j++) {
                strcpy(expanded_includes_file[expanded_includes_file_len++], expanded_include[j]);
            }

            free_matrix(expanded_include, NUM_LINES);
            if (status != 0) {
                break;
            }
        } else {
            strcpy(expanded_includes_file[expanded_includes_file_len++], input_file_lines[i]);
        }
    }

    *new_num_lines = expanded_includes_file_len;
    *expanded_include_lines = expanded_includes_file;
    free_matrix(input_file_lines, NUM_LINES);

    if (status != 0) {
        return 1;
    }
    return 0;
}


int main(int argc, char **argv) {
    int input_file_specified = 0;
    char input_file_name[FILE_NAME_LENGTH] = {0};
    int output_file_specified = 0;
    char output_file_name[FILE_NAME_LENGTH] = {0};
    int num_header_files = 0;
    char header_file_directory[INITIAL_HEADER_FILES_LENGTH][FILE_NAME_LENGTH] = {0};
    char* split_pos = NULL;
    int i = 1;
    int j = 0; 
    char** file_lines = NULL;
    int num_lines = 0;
    FILE* input_fp = stdin;
    FILE* output_fp = stdout;
    int expanded_includes_lines = 0;
    char** expanded_include_lines  = NULL;
    int status = 0;
    int final_num_lines = 0;
    char** expanded_directives_lines = NULL;
    Hashmap* symbol_hashmap = init_hashmap(INITIAL_HASHMAP_SIZE);
    Hashmap* deleted_symbols_hashmap = init_hashmap(INITIAL_HASHMAP_SIZE);

    if (symbol_hashmap == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    if (deleted_symbols_hashmap == NULL) {
        free_hashmap(symbol_hashmap);
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], SYMBOL_FLAG, strlen(argv[i])) == 0) {
            add_input_symbol_mapping(argv[i + 1], symbol_hashmap);
            i++;
        } else if (strncmp(argv[i], SYMBOL_FLAG, strlen(SYMBOL_FLAG)) == 0) {
            add_input_symbol_mapping(argv[i] + 2, symbol_hashmap);
        } else if (strncmp(argv[i], HEADERS_PATH_FLAG, strlen(argv[i])) == 0) {
            strcpy(header_file_directory[num_header_files++], argv[i + 1]);
            i++;
        } else if (strncmp(argv[i], HEADERS_PATH_FLAG, strlen(HEADERS_PATH_FLAG)) == 0) {
            strcpy(header_file_directory[num_header_files++], argv[i] + strlen(HEADERS_PATH_FLAG));
        } else if (strncmp(argv[i], OUTPUT_FILE_FLAG, strlen(OUTPUT_FILE_FLAG)) == 0 && !output_file_specified) {
            output_file_specified = 1;
            strcpy(output_file_name, argv[i + 1]);
            i++;
        } else if (argv[i][0] != FLAG_START && strlen(argv[i]) > 2 && (!input_file_specified || !output_file_specified)) {
            if (!input_file_specified) {
                input_file_specified = 1;
                strcpy(input_file_name, argv[i]);
                split_pos = strrchr(input_file_name, PATH_CHAR);
                if (split_pos != NULL) {
                    for (j = num_header_files - 1; j >= 0; j--) {
                        strcpy(header_file_directory[j + 1], header_file_directory[j]);
                    }
                    memset(header_file_directory[0], 0, FILE_NAME_LENGTH);
                    strncpy(header_file_directory[0], input_file_name, split_pos - input_file_name);
                    num_header_files++;
                }
            } else if (!output_file_specified) {
                output_file_specified = 1;
                strcpy(output_file_name, argv[i]);
            }
        } else {
            perror("Input args");
            exit(EXIT_FAILURE);
        }
    }

    file_lines = allocate_lines_array(NUM_LINES, LINE_LENGTH);
    if (file_lines == NULL) {
        free_hashmap(symbol_hashmap);
        free_hashmap(deleted_symbols_hashmap);
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    num_lines = 0;

    if (input_file_specified) {
        input_fp = fopen(input_file_name, "r");
    }

    if (input_fp == NULL) {
        perror("fopen");
        free_hashmap(symbol_hashmap);
        free_hashmap(deleted_symbols_hashmap);
        exit(EXIT_FAILURE);
    }

    while (fgets(file_lines[num_lines++], LINE_LENGTH, input_fp));
    fclose(input_fp);

    expanded_includes_lines = 0;
    expanded_include_lines  = NULL;
    status = expand_includes(&expanded_include_lines, file_lines, num_lines, &expanded_includes_lines, header_file_directory, num_header_files);
    if (status != 0) {
        free_everything(expanded_include_lines, symbol_hashmap, deleted_symbols_hashmap);
        perror("Couldn't find header file");
        exit(EXIT_FAILURE);
    }

    final_num_lines = 0;
    expanded_directives_lines = expand_directives(expanded_include_lines, expanded_includes_lines, &final_num_lines, symbol_hashmap, deleted_symbols_hashmap);
    if (expanded_directives_lines == NULL) {
        free_everything(expanded_include_lines, symbol_hashmap, deleted_symbols_hashmap);
        perror("Couldn't expand defines");
        exit(EXIT_FAILURE);
    }

    if (output_file_specified) {
        output_fp = fopen(output_file_name, "w");
    }

    if (output_fp == NULL) {
        free_everything(expanded_directives_lines, symbol_hashmap, deleted_symbols_hashmap);
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < final_num_lines; i++) {
        if (!line_should_be_skipped(expanded_directives_lines[i])) {
            fprintf(output_fp, "%s", expanded_directives_lines[i]);
        }
    }
    fclose(output_fp);

    free_everything(expanded_directives_lines, symbol_hashmap, deleted_symbols_hashmap);
    return 0;
}