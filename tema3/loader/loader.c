/*
 * Loader Implementation
 *
 * 2018, Operating Systems
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "exec_parser.h"

#define ERROR_RES -1

static int page_size;
static so_exec_t *exec;
static struct sigaction default_action;
char* exec_p;

typedef struct page_table {
    int num_vmem_pages;
    int num_file_pages;
    unsigned char* marked_pages;
} page_table_t;

void unmapped_access_handler(int signum, siginfo_t *info, void *context) {
    if (signum != SIGSEGV) {
        default_action.sa_sigaction(signum, info, context);
		return;
    }

    uintptr_t addr_value = (uintptr_t)(void*)info->si_addr;

    for (int i = 0; i < exec->segments_no; i++) {
        so_seg_t* segment = &exec->segments[i];
        if (addr_value >= segment->vaddr && addr_value <= segment->vaddr + segment->mem_size) {
            page_table_t* page_table = (page_table_t*)segment->data;
            int page_num = (addr_value - segment->vaddr) / page_size;
            if (page_table->marked_pages[page_num] == 0) {
                int vmem_to_allocate = page_size;
                int mem_to_copy = page_size;
                int rc;
                char* mapped_area;

                if (page_num == page_table->num_vmem_pages - 1 && segment->mem_size % page_size != 0) {
                    vmem_to_allocate = segment->mem_size % page_size;
                }

                if (page_num >= page_table->num_file_pages) {
                    mem_to_copy = 0;
                } else if (page_num == page_table->num_file_pages - 1 && segment->file_size % page_size != 0) {
                    mem_to_copy = segment->file_size % page_size;
                }

                page_table->marked_pages[page_num] = 1;

                mapped_area = mmap((void*)(segment->vaddr + page_num * page_size), vmem_to_allocate, PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, 0, 0);
                if (mapped_area == MAP_FAILED) {
                    perror("mmap");
                    exit(EXIT_FAILURE);
                }

                memcpy(mapped_area, exec_p + segment->offset + page_num * page_size, mem_to_copy);
                rc = mprotect(mapped_area, vmem_to_allocate, segment->perm);
                if (rc == ERROR_RES) {
                    perror("mprotect");
			        exit(EXIT_FAILURE);
                }

                return;
            } else {
                default_action.sa_sigaction(signum, info, context);
		        return;
            }
        }
    }

    default_action.sa_sigaction(signum, info, context);
    return;
}

int so_init_loader(void) {
	struct sigaction action = { 0 };

	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = &unmapped_access_handler;
    sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGSEGV);
	if (sigaction(SIGSEGV, &action, &default_action) == ERROR_RES) {
        perror("sigaction");
        exit(EXIT_FAILURE);
	}

    page_size = getpagesize();

	return 0;
}

int so_execute(char *path, char *argv[]) {
    int fexec;
    long file_size;

	exec = so_parse_exec(path);
	if (!exec) {
		return -1;
    }

    for (int i = 0; i < exec->segments_no; i++) {
        so_seg_t* segment = &exec->segments[i];
        page_table_t* page_table = calloc(1, sizeof(page_table_t));

        page_table->num_vmem_pages = segment->mem_size / page_size;
        if (segment->mem_size % page_size != 0) {
            page_table->num_vmem_pages += 1;
        }

        page_table->num_file_pages = segment->file_size / page_size;
        if(segment->file_size % page_size != 0) {
            page_table->num_file_pages += 1;
        }

        page_table->marked_pages = calloc(page_table->num_vmem_pages, sizeof(unsigned char));
        segment->data = page_table;
    }

    fexec = open(path, O_RDONLY, 0644);
    if (fexec == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
	
    file_size = lseek(fexec, 0L, SEEK_END);
    lseek(fexec, 0L, SEEK_SET);

    exec_p = mmap(0, file_size, PROT_READ, MAP_PRIVATE, fexec, 0);
    if (exec_p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    so_start_exec(exec, argv);

	return 0;
}
