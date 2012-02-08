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

#include "utils.h"
#include "file_memory.h"

/* mmap(2) */
#include <sys/mman.h>

/* open(2) */
#include <fcntl.h>

/* strerror(3) */
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* lseek(2) */
#include <unistd.h>

/* assert */
#include <assert.h>

/* Our main memory state descriptor instance */
static struct mem_manager mem;

/* Add a file memory entry to a double-linked list of file_memories
   - if head is NULL, creates a new file memory entry ; if not, chains a new
     entry to it
   - returns with head set to the newly added element */
int
add_file_memory(struct file_memory **head, char *path, size_t size)
{
    assert(head != NULL);
    assert(path != NULL);
    assert(size > 0);

    struct file_memory **current = head; /* current file_memory pointer address */
    struct file_memory *previous = NULL; /* previous file_memory pointer */

    /* backup current structure pointer and initialize a new structure */
    previous = *current;
    if((*current = malloc(sizeof(struct file_memory))) == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        return (1);
    }

    /* set head on first call */
    if(*head == NULL)
        *head = *current;

    /* set path */
    size_t malloc_size = strlen(path) + 1;
    if(((*current)->path = malloc(malloc_size)) == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        return (1);
    }
    snprintf((*current)->path, malloc_size, "%s", path);

    /* open and create file, set fd */
    if (((*current)->fd = open((*current)->path, O_RDWR|O_CREAT|O_EXCL, 0660)) < 0) {
        fprintf(stderr, "%s: %s\n", (*current)->path, strerror(errno));
        return (1);
    }
  
    /* set file size
       XXX fill file with zeroes to avoid fragmentation, see mmap(2) ? */
    (*current)->size = size;
    if(lseek((*current)->fd, (*current)->size - 1, SEEK_SET) != ((*current)->size - 1)) {
        fprintf(stderr, "%s: %s\n", (*current)->path, strerror(errno));
        return (1);
    }
    char zero = 0;
    if(write((*current)->fd, &zero, 1) != 1) {
        fprintf(stderr, "%s: %s\n", (*current)->path, strerror(errno));
        return (1);
    }

    /* mmap() our file */
    if(((*current)->start_addr =
        mmap(0, (*current)->size, PROT_READ|PROT_WRITE, MAP_SHARED, (*current)->fd, 0)) == MAP_FAILED) {
        fprintf(stderr, "%s(): cannot map memory\n", __func__);
        (*current)->start_addr = NULL;
        return (1);
    }

    /* initialize next free offset */
    (*current)->next_free_offset = 0;

    /* set pointers */
    (*current)->nextp = NULL;           /* set in next pass (see below) */
    (*current)->prevp = previous;
    if(previous != NULL)
        previous->nextp = *current;

    return (0);
}

/* Un-initialize a double-linked list of file_memories */
void
uninit_file_memories(struct file_memory *head)
{
    /* be sure to start at first entry */
    rewind_list(head);

    struct file_memory *current = head;
    struct file_memory *next = NULL;

    while(current != NULL) {
        if(current->start_addr != NULL)
            munmap(current->start_addr, current->size);
        if(current->fd >= 0)
            close(current->fd);
        if(current->path != NULL) {
            unlink(current->path);
            free(current->path);
        }

        next = current->nextp;
        free(current);
        current = next;
    }
    return;
}

int
init_memory(char *base_path, fnum_t max_chunks)
{
    assert(base_path != NULL);

    /* set path */
    size_t malloc_size = strlen(base_path) + 1;
    if((mem.base_path = malloc(malloc_size)) == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        return (1);
    }
    snprintf(mem.base_path, malloc_size, "%s", base_path);

    mem.currentp = NULL;
    mem.next_chunk_index = 0;
    mem.max_chunks = max_chunks;

    return (0);
}

void
uninit_memory()
{
    /* un-initialize our file memory entries */
    uninit_file_memories(mem.currentp);

    /* clean our memory manager up */
    if(mem.base_path != NULL)
        free(mem.base_path);
    mem.currentp = NULL;
    mem.next_chunk_index = 0;
    mem.max_chunks = 0;

    return;
}

void *
file_malloc(size_t size)
{
    /* no size specified, reject call */
    if(size == 0) {
        errno = EINVAL;
        return (NULL);
    }

    /* requested size too big, our malloc() cannot span accross multiple chunks, so reject call */
    if(size > FILE_MEMORY_CHUNK_SIZE) {
        errno = ENOMEM;
        return (NULL);
    }

    /* if first call... */
    if((mem.currentp == NULL) ||
        /* ...or not enough space in current chunk */
        ((mem.currentp->next_free_offset + round_page(size)) >
        (mem.currentp->size))) {
        /* allocate a new chunk, can we proceed ? */
        if ((mem.max_chunks > 0) && (mem.next_chunk_index > (mem.max_chunks - 1))) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            errno = ENOMEM;
            return (NULL);
        }

        /* compute tmp_path "base_path.i\0" */
        char *tmp_path = NULL;
        size_t malloc_size = strlen(mem.base_path) + 1 +
            get_num_digits(mem.next_chunk_index) + 1;
        if((tmp_path = malloc(malloc_size)) == NULL) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            errno = ENOMEM;
            return (NULL);
        }
        snprintf(tmp_path, malloc_size, "%s.%lld", mem.base_path, mem.next_chunk_index);

        /* add chunk */
        if(add_file_memory(&mem.currentp, tmp_path, FILE_MEMORY_CHUNK_SIZE) != 0) {
            free(tmp_path);
            uninit_file_memories(mem.currentp);
            errno = ENOMEM;
            return (NULL);
        }
        free(tmp_path);
        mem.next_chunk_index++;
    }

    /* current chunk OK, return value and increment next offset */
    void *malloc_value = (void *)((char *)mem.currentp->start_addr + mem.currentp->next_free_offset);
    mem.currentp->next_free_offset += round_page(size);

    return (malloc_value);
}

void
file_free(void *ptr)
{
    /* does nothing */
    return;
}
