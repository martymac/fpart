/*-
 * Copyright (c) 2011 Ganael LAPLANCHE <ganael.laplanche@martymac.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _FILE_MEMORY_H
#define _FILE_MEMORY_H

#include "types.h"

/* size_t */
#include <stdlib.h>

#if !defined(FILE_MEMORY_CHUNK_SIZE)
#define FILE_MEMORY_CHUNK_SIZE (32 * 1024 * 1024) /* 32 MB */
#endif

/* Our main memory state descriptor */
struct mem_manager {
    char *base_path;                /* base path name for each file memory */

    struct file_memory *currentp;   /* pointer to current (last) file_memory allocated */
    fnum_t next_chunk_index;        /* next file memory entry index */
    fnum_t max_chunks;              /* maximum number of file memory entries allowed (0 == illimited) */
};

/* Describes a file memory chunk */
struct file_memory;
struct file_memory {
    char *path;                     /* file name */
    int fd;                         /* file descriptor */

    size_t size;                    /* mmap size */
    void *start_addr;               /* mmap addr */
    size_t next_free_offset;        /* next free area offset within map */

    struct file_memory* nextp;      /* next file_entry */
    struct file_memory* prevp;      /* previous one */
};

int init_memory(char *base_path, fnum_t max_chunks);   /* entry point */
void uninit_memory();

int add_file_memory(struct file_memory **head, char *filename, size_t len);
void uninit_file_memories(struct file_memory *head);

void *file_malloc(size_t size);     /* simple malloc() */
void file_free(void *ptr);          /* simple free() */

#endif /* _FILE_MEMORY_H */
