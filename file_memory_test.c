#define FILE_MEMORY_CHUNK_SIZE  (32 * 1024 * 1024) /* 32 MB */
#define MAX_FILE_MEMORY_CHUNKS  0

#include "file_memory.h"

/* XXX defined in file_memory.c */
#define FILE_MALLOC_HEADER_SIZE \
    (sizeof(struct file_memory*))

#include <stdio.h>

int main(void)
{
    void *ptr = NULL;

    /* init memory manager */
    if(init_memory("/tmp/.fpart_mem", MAX_FILE_MEMORY_CHUNKS) != 0) {
        fprintf(stderr, "%s(): could not initialize memory manager\n",
            __func__);
        return (1);
    }

    fprintf(stderr, "allocating (FILE_MEMORY_CHUNK_SIZE * 3 - FILE_MALLOC_HEADER_SIZE) bytes (should create a new file_memory of FILE_MEMORY_CHUNK_SIZE * 3 bytes)\n");
    if((ptr = fmalloc(FILE_MEMORY_CHUNK_SIZE * 3 - FILE_MALLOC_HEADER_SIZE)) == NULL) {
        fprintf(stderr, "could not allocate (FILE_MEMORY_CHUNK_SIZE * 3) bytes\n");
    }
    else {
        fprintf(stderr, "==\n");
        ffree(ptr);
    }
    fprintf(stderr, "==\n");

    fprintf(stderr, "allocating (FILE_MEMORY_CHUNK_SIZE * 3 - FILE_MALLOC_HEADER_SIZE + 1) bytes (should create a new file_memory of FILE_MEMORY_CHUNK_SIZE * 4 bytes)\n");
    if((ptr = fmalloc(FILE_MEMORY_CHUNK_SIZE * 3 - FILE_MALLOC_HEADER_SIZE + 1)) == NULL) {
        fprintf(stderr, "could not allocate (FILE_MEMORY_CHUNK_SIZE * 3 + 1) bytes\n");
    }
    else {
        fprintf(stderr, "==\n");
        ffree(ptr);
    }
    fprintf(stderr, "==\n");

    fprintf(stderr, "allocating (FILE_MEMORY_CHUNK_SIZE * 3 - FILE_MALLOC_HEADER_SIZE + 1) bytes (should create a new file_memory of FILE_MEMORY_CHUNK_SIZE * 4 bytes)\n");
    if((ptr = fmalloc(FILE_MEMORY_CHUNK_SIZE * 3 - FILE_MALLOC_HEADER_SIZE + 1)) == NULL) {
        fprintf(stderr, "could not allocate (FILE_MEMORY_CHUNK_SIZE * 3 + 1) bytes\n");
    }
    else {
        fprintf(stderr, "==\n");
        ffree(ptr);
    }
    fprintf(stderr, "==\n");

    /* uninit memory manager */
    uninit_memory();
    return (0);
}
