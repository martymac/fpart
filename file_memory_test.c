#define DEBUG                   1
#define WITH_FILE_MEMORY        1
#define FILE_MEMORY_CHUNK_SIZE  (32 * 1024 * 1024) /* 32 MB */
#define MAX_FILE_MEMORY_CHUNKS  0

#include "file_memory.h"

#include <stdio.h>

int main(void)
{
    void *ptr = NULL;

    if(init_memory("/tmp/.fpart_mem", MAX_FILE_MEMORY_CHUNKS) != 0) {
        fprintf(stderr, "%s(): could not initialize memory manager\n",
            __func__);
        return (1);
    }

    fprintf(stderr, "allocating FILE_MEMORY_CHUNK_SIZE bytes (should fill a full file_memory)\n");
    if((ptr = fmalloc(FILE_MEMORY_CHUNK_SIZE)) == NULL) {
        fprintf(stderr, "could not allocate FILE_MEMORY_CHUNK_SIZE bytes\n");
    }
    else
        ffree(ptr);
    fprintf(stderr, "==\n");

    fprintf(stderr, "allocating FILE_MEMORY_CHUNK_SIZE bytes (should fill a full file_memory)\n");
    if(fmalloc(FILE_MEMORY_CHUNK_SIZE) == NULL) {
        fprintf(stderr, "could not allocate FILE_MEMORY_CHUNK_SIZE bytes\n");
    }
    fprintf(stderr, "==\n");

    fprintf(stderr, "allocating 1 byte (should create a new file_memory)\n");
    if(fmalloc(1) == NULL) {
        fprintf(stderr, "could not allocate 1 byte\n");
    }
    fprintf(stderr, "==\n");

    fprintf(stderr, "allocating 1 byte (should continue within the same file_memory)\n");
    if(fmalloc(1) == NULL) {
        fprintf(stderr, "could not allocate 1 byte\n");
    }
    fprintf(stderr, "==\n");

    fprintf(stderr, "allocating 8 bytes (should continue within the same file_memory)\n");
    if(fmalloc(8) == NULL) {
        fprintf(stderr, "could not allocate 8 bytes\n");
    }
    fprintf(stderr, "==\n");

    fprintf(stderr, "allocating FILE_MEMORY_CHUNK_SIZE bytes (should create a new file_memory)\n");
    if(fmalloc(FILE_MEMORY_CHUNK_SIZE) == NULL) {
        fprintf(stderr, "could not allocate FILE_MEMORY_CHUNK_SIZE bytes\n");
    }
    fprintf(stderr, "==\n");

    fprintf(stderr, "allocating (FILE_MEMORY_CHUNK_SIZE * 3 + 1) bytes (should create a new file_memory of FILE_MEMORY_CHUNK_SIZE * 4 bytes)\n");
    if(fmalloc(FILE_MEMORY_CHUNK_SIZE * 3 + 1) == NULL) {
        fprintf(stderr, "could not allocate (FILE_MEMORY_CHUNK_SIZE * 3 + 1) bytes\n");
    }
    fprintf(stderr, "==\n");

    uninit_memory();
    return (0);
}
