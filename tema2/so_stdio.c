#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "so_stdio.h"

#define READ_MODE "r"
#define READ_MODE_PLUS "r+"
#define WRITE_MODE "w"
#define WRITE_MODE_PLUS "w+"
#define APPEND_MODE "a"
#define APPEND_MODE_PLUS "a+"

#define BUFFER_SIZE 4096
#define COMMAND_SIZE 80

typedef struct _so_file {
    unsigned char file_buffer[BUFFER_SIZE];
    int fd;
    pid_t pid;
    int has_error;
    int reached_feof;

    int last_op_is_write;
    int file_pointer;
    int buffer_pointer;
    int read_buffer_size;
} SO_FILE;

static int write_file(SO_FILE *stream) {
    int res = 0;
    int total = stream->buffer_pointer;
    do {
        res = write(stream->fd, stream->file_buffer + stream->buffer_pointer - total, total);
        total -= res; 
    } while (total != 0 && res != SO_EOF);

    return res;
}

SO_FILE *so_fopen(const char *pathname, const char *mode) {
    SO_FILE* file_struct = NULL;
    int fp_flags = 0;
    int fd = 0;

    if (strncmp(mode, READ_MODE_PLUS, strlen(READ_MODE_PLUS)) == 0) {
        fp_flags = O_RDWR;
    } else if (strncmp(mode, READ_MODE, strlen(READ_MODE)) == 0) {
        fp_flags = O_RDONLY;
    } else if (strncmp(mode, WRITE_MODE_PLUS, strlen(WRITE_MODE_PLUS)) == 0) {
        fp_flags = O_RDWR | O_CREAT | O_TRUNC;
    } else if (strncmp(mode, WRITE_MODE, strlen(WRITE_MODE)) == 0) {
        fp_flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strncmp(mode, APPEND_MODE_PLUS, strlen(APPEND_MODE_PLUS)) == 0) {
        fp_flags = O_RDWR | O_CREAT | O_APPEND;
    } else if (strncmp(mode, APPEND_MODE, strlen(APPEND_MODE)) == 0) {
        fp_flags = O_WRONLY | O_CREAT | O_APPEND;
    } else {
        return NULL;
    }

    fd = open(pathname, fp_flags, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0) {
        return NULL;
    }

    file_struct = calloc(1, sizeof(SO_FILE));
    file_struct->fd = fd;
    file_struct->last_op_is_write = -1;

    return file_struct;
}

int so_fclose(SO_FILE *stream) {
    int close_res, flush_res;
    if (stream == NULL) {
        return SO_EOF;
    }

    flush_res = so_fflush(stream);
    close_res = close(stream->fd);
    free(stream);
    if (close_res == SO_EOF || flush_res == SO_EOF) {
        return SO_EOF;
    }
    return 0;
}

int so_fileno(SO_FILE *stream) {
    return stream->fd;
}

int so_fflush(SO_FILE *stream) {
    if (stream->buffer_pointer != 0 && stream->last_op_is_write) {
        int count = write_file(stream);

        if (count == SO_EOF) {
            stream->has_error = 1;
            return SO_EOF;
        }
        memset(stream->file_buffer, 0, BUFFER_SIZE);
        stream->buffer_pointer = 0;
    }
    return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence) {
    if (stream->last_op_is_write) {
        if (so_fflush(stream) == SO_EOF) {
            return SO_EOF;
        }
    } else {
        memset(stream->file_buffer, 0, BUFFER_SIZE);
        stream->buffer_pointer = 0;
        stream->read_buffer_size = 0;
    }

    stream->file_pointer = lseek(stream->fd, offset, whence);
    stream->reached_feof = 0;
    if (stream->file_pointer == SO_EOF) {
        return SO_EOF;
    }
    return 0;
}

long so_ftell(SO_FILE *stream) {
    return stream->file_pointer;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    long count = 0;
    for (long i = 0; i < nmemb * size; i++) {
        int read_chr = so_fgetc(stream);
        if (read_chr != SO_EOF) {
            *((unsigned char*)(ptr + i)) = (unsigned char)read_chr;
            count += 1;
        } else {
            stream->has_error = 1;
            break;
        }
    }
    return count / size;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    long count = 0;
    for (long i = 0; i < nmemb * size; i++) {
        unsigned char wrote_chr = so_fputc(*((int*)(ptr + i)), stream);
        if (wrote_chr != SO_EOF) {
            count += 1;
        } else {
            stream->has_error = 1;
            break;
        }
    }
    return count / size;
}

int so_fgetc(SO_FILE *stream) {
    unsigned char read_chr;
    if (stream->last_op_is_write == 1) {
        so_fseek(stream, 0, SEEK_CUR);
    }
    stream->file_pointer++;
    stream->last_op_is_write = 0;
    if (stream->buffer_pointer == 0) {
        int count = read(stream->fd, stream->file_buffer, BUFFER_SIZE);
        if (count == SO_EOF || count == 0) {
            stream->reached_feof = 1;
            stream->has_error = 1;
            return SO_EOF;
        }
        stream->read_buffer_size = count;
    }

    read_chr = stream->file_buffer[stream->buffer_pointer++];
    if (stream->buffer_pointer == stream->read_buffer_size) {
        memset(stream->file_buffer, 0, BUFFER_SIZE);
        stream->buffer_pointer = 0;
        stream->read_buffer_size = 0;
    }

    return (int)read_chr;
}

int so_fputc(int c, SO_FILE *stream) {
    if (stream->last_op_is_write == 0) {
        so_fseek(stream, 0, SEEK_CUR);
    }
    stream->file_pointer++;
    stream->last_op_is_write = 1;

    if (stream->buffer_pointer == BUFFER_SIZE) {
        int count = write_file(stream);
        if (count == SO_EOF) {
            stream->has_error = 1;
            return SO_EOF;
        }
    
        memset(stream->file_buffer, 0, BUFFER_SIZE);
        stream->buffer_pointer = 0;
    }
    stream->file_buffer[stream->buffer_pointer++] = (unsigned char)c; 
    return c;
}

int so_feof(SO_FILE *stream) {
    return stream->reached_feof;
}

int so_ferror(SO_FILE *stream) {
    return stream->has_error;
}

SO_FILE *so_popen(const char *command, const char *type) {
    int pipefd[2];
    pipe(pipefd);

    SO_FILE *process_so_file = calloc(1, sizeof(SO_FILE));
    process_so_file->pid = fork();

    switch(process_so_file->pid) {
    case -1:
        free(process_so_file);
        return NULL;
    case 0:
        if (strncmp(type, READ_MODE, strlen(READ_MODE)) == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
        } else if (strncmp(type, WRITE_MODE, strlen(WRITE_MODE)) == 0) {
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[1]);
        }
        char non_const_command[COMMAND_SIZE];
        strcpy(non_const_command, command);
        char *const argvec[] = {"sh", "-c", non_const_command, NULL};

        execvp("sh", argvec);
        exit(EXIT_FAILURE);
    default:
        break;
    }
    
    if (strncmp(type, READ_MODE, strlen(READ_MODE)) == 0) {
        process_so_file->fd = pipefd[0];
        close(pipefd[1]);
    } else if (strncmp(type, WRITE_MODE, strlen(WRITE_MODE)) == 0) {
        process_so_file->fd = pipefd[1];
        close(pipefd[0]);
    }

    return process_so_file;
}

int so_pclose(SO_FILE *stream) {
    int status = 0, flush_res, close_res, waitpid_res;

    flush_res = so_fflush(stream);
    close_res = close(stream->fd);
    waitpid_res = waitpid(stream->pid, &status, 0);
    free(stream);
    if (close_res == SO_EOF || flush_res == SO_EOF || !WIFEXITED(status) || waitpid_res == SO_EOF) {
        return SO_EOF;
    }

    return WEXITSTATUS(status); 
}