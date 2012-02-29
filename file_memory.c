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

/* This library is a simple replacement for malloc(3) and free(3)
   that operates within files, using mmap(2).
   Limitatons :
     - Memory space is sequentially allocated, not re-used within files.
       Anyway, a useless file is removed to save disk space when a free(3)
       is performed.
     - Code is not (yet ?) MP-safe.
   Usage :
     init_memory(char *base_path, fnum_t max_chunks)
       file_malloc(size_t size) [...] file_free(void *ptr) [...]
     uninit_memory()
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

/* lseek(2), getpagesize(3) */
#include <unistd.h>

/* assert */
#include <assert.h>

/* signal(3) */
#include <signal.h>

/* Describes a file memory chunk (a file) */
struct file_memory;
struct file_memory {
    char *path;                     /* file name */
    int fd;                         /* file descriptor */

    size_t size;                    /* mmap size */
    void *start_addr;               /* mmap addr */
    size_t next_free_offset;        /* next free area offset within map */
    fnum_t ref_count;               /* reference counter */

    struct file_memory* nextp;      /* next file_entry */
    struct file_memory* prevp;      /* previous one */
};

/* Describes a malloc entry within a file_memory */
struct file_malloc {
    struct file_memory* parentp;    /* pointer to parent file_memory */
    unsigned char data[1];          /* data starts here */
/* XXX keep the following definitions up-to-date with structure definition */
#define FILE_MALLOC_HEADER_SIZE \
    (sizeof(struct file_memory*))
#define get_parentp(data_addr) \
    ((size_t)(data_addr) < sizeof(struct file_memory *) ? \
     NULL : (void *)(*((struct file_memory **) \
        ((unsigned char *)(data_addr) - \
        (unsigned char *)(sizeof(struct file_memory *))))))
};

/* Our main memory state descriptor */
static struct mem_manager {
    char *base_path;                /* base path name for each file memory */

    struct file_memory *currentp;   /* pointer to current (last) file_memory
                                       allocated */
    fnum_t next_chunk_index;        /* next file memory entry index */
    fnum_t max_chunks;              /* maximum number of file memory entries
                                       allowed (0 == illimited) */
} mem;

/* Add a file memory entry to a double-linked list of file_memories
   - if head is NULL, creates a new file memory entry ; if not, chains a new
     entry to it
   - returns with head set to the newly added element */
static int
add_file_memory(struct file_memory **head, char *path, size_t size)
{
    assert(head != NULL);
    assert(path != NULL);
    assert(size > 0);

    struct file_memory **current = head; /* current file_memory pointer addr */
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
        /* XXX use delete_file_memory here and for next failures ? */
        free(*current);
        *current = previous;
        return (1);
    }
    snprintf((*current)->path, malloc_size, "%s", path);

    /* open and create file, set fd */
    if (((*current)->fd =
        open((*current)->path, O_RDWR|O_CREAT|O_TRUNC, 0660)) < 0) {
        fprintf(stderr, "%s: %s\n", (*current)->path, strerror(errno));
        free((*current)->path);
        free(*current);
        *current = previous;
        return (1);
    }
  
    /* set file size
       XXX fill file with zeroes to avoid fragmentation, see mmap(2) ? */
    (*current)->size = size;
    if(lseek((*current)->fd, (*current)->size - 1, SEEK_SET) !=
        ((*current)->size - 1)) {
        fprintf(stderr, "%s: %s\n", (*current)->path, strerror(errno));
        close((*current)->fd);
        unlink((*current)->path);
        free((*current)->path);
        free(*current);
        *current = previous;
        return (1);
    }
    char zero = 0;
    if(write((*current)->fd, &zero, 1) != 1) {
        fprintf(stderr, "%s: %s\n", (*current)->path, strerror(errno));
        close((*current)->fd);
        unlink((*current)->path);
        free((*current)->path);
        free(*current);
        *current = previous;
        return (1);
    }

    /* mmap() our file */
    if(((*current)->start_addr =
        mmap(0, (*current)->size, PROT_READ|PROT_WRITE, MAP_SHARED,
        (*current)->fd, 0)) == MAP_FAILED) {
        fprintf(stderr, "%s(): cannot map memory\n", __func__);
        (*current)->start_addr = NULL;
        close((*current)->fd);
        unlink((*current)->path);
        free((*current)->path);
        free(*current);
        *current = previous;
        return (1);
    }

    /* initialize next free offset and counter */
    (*current)->next_free_offset = 0;
    (*current)->ref_count = 0;

    /* set current pointers */
    (*current)->nextp = NULL;   /* set in next pass (see below) */
    (*current)->prevp = previous;

    /* set previous' nextp pointer */
    if(previous != NULL)
        previous->nextp = *current;

    /* update memory manager status */
    size_t needed_chunks = round_num((*current)->size, FILE_MEMORY_CHUNK_SIZE) /
        FILE_MEMORY_CHUNK_SIZE;
    mem.next_chunk_index += needed_chunks;

#if defined(DEBUG)
    fprintf(stderr, "%s(): memory file '%s' created (%zd bytes)\n", __func__,
        (*current)->path, (*current)->size);
    fprintf(stderr, "%s(): struct malloc'ed @%p\n", __func__, *current);
    fprintf(stderr, "%s(): file mmap'ed @%p\n", __func__,
        (*current)->start_addr);
#endif

    return (0);
}

/* Remove a file_memory entry */
static void
delete_file_memory(struct file_memory *fm)
{
    assert(fm != NULL);

    /* handle previous and next entries' pointers */
    if(fm->nextp != NULL) {
        if(fm->prevp != NULL)
            fm->nextp->prevp = fm->prevp;
        else
            fm->nextp->prevp = NULL;
    }
    else {
        /* last element removed, update memory manager status */
        mem.currentp = fm->prevp;
        size_t needed_chunks = round_num(fm->size, FILE_MEMORY_CHUNK_SIZE) /
            FILE_MEMORY_CHUNK_SIZE;
        mem.next_chunk_index -= needed_chunks;
    }
    if(fm->prevp != NULL) {
        if(fm->nextp != NULL)
            fm->prevp->nextp = fm->nextp;
        else
            fm->prevp->nextp = NULL;
    }

    /* munmap, close and unlink file */
    if(fm->start_addr != NULL)
        munmap(fm->start_addr, fm->size);
    if(fm->fd >= 0)
        close(fm->fd);
    if(fm->path != NULL) {
        unlink(fm->path);
#if defined(DEBUG)
        fprintf(stderr, "%s(): memory file '%s' unlinked (%zd bytes)\n",
            __func__, fm->path, fm->size);
#endif
        free(fm->path);
    }

    /* free our structure */
    free(fm);

    return;
}

/* Un-initialize a double-linked list of file_memories */
static void
uninit_file_memories(struct file_memory *head)
{
    /* be sure to start from last entry */
    fastfw_list(head);

    struct file_memory *current = head;
    struct file_memory *prev = NULL;

    while(current != NULL) {
        prev = current->prevp;
        delete_file_memory(current);
        current = prev;
    }
    return;
}

/* Un-initialize memory manager */
void
uninit_memory()
{
    /* un-initialize our file memory entries */
    uninit_file_memories(mem.currentp);

    /* clean our memory manager up */
    mem.max_chunks = 0;
    mem.next_chunk_index = 0;
    mem.currentp = NULL;
    if(mem.base_path != NULL)
        free(mem.base_path);

    return;
}

/* Signal handler */
void
exit_on_signal(int sig)
{
    uninit_memory();
    exit(0);
}

/* Initialize memory manager */
int
init_memory(char *base_path, fnum_t max_chunks)
{
    assert(base_path != NULL);
    assert((FILE_MEMORY_CHUNK_SIZE % getpagesize()) == 0);

    /* record absolute path */
    mem.base_path = abs_path(base_path);
    if(mem.base_path == NULL) {
        fprintf(stderr, "%s(): cannot determine absolute path for file '%s'\n",
            __func__, base_path);
        return (1);
    }

    mem.currentp = NULL;
    mem.next_chunk_index = 0;
    mem.max_chunks = max_chunks;

    /* install signal handler to remove temporary files */
    signal(SIGHUP, exit_on_signal);
    signal(SIGINT, exit_on_signal);
    signal(SIGQUIT, exit_on_signal);
    signal(SIGTERM, exit_on_signal);

    return (0);
}

/* Replacement for malloc(3), allocates memory within a file (file_memory) */
void *
file_malloc(size_t requested_size)
{
    /* invalid requested_size specified, reject call */
    if(requested_size == 0) {
        errno = ENOMEM;
        return (NULL);
    }

    /* size aligned on pointer size */
    size_t aligned_size = round_num(requested_size + FILE_MALLOC_HEADER_SIZE,
        sizeof(void *));
    /* full size rounded on chunks' size */
    size_t chunked_size = round_num(aligned_size, FILE_MEMORY_CHUNK_SIZE);
    /* needed chunks to store data */
    size_t needed_chunks = chunked_size / FILE_MEMORY_CHUNK_SIZE;

#if defined(DEBUG)
    fprintf(stderr, "%s(): FILE_MEMORY_CHUNK_SIZE = %d\n", __func__,
        FILE_MEMORY_CHUNK_SIZE);
    fprintf(stderr, "%s(): FILE_MALLOC_HEADER_SIZE = %zd\n", __func__,
        FILE_MALLOC_HEADER_SIZE);
    fprintf(stderr, "%s(): requested_size = %zd\n", __func__, requested_size);
    fprintf(stderr, "%s(): aligned_size (including header) = %zd\n", __func__,
        aligned_size);
    fprintf(stderr, "%s(): chunked_size = %zd\n", __func__, chunked_size);
    fprintf(stderr, "%s(): needed_chunks = %zd\n", __func__, needed_chunks);
#endif

    /* if first call... */
    if((mem.currentp == NULL) ||
        /* ...or not enough space in current chunk */
        ((mem.currentp->next_free_offset + aligned_size) >
        (mem.currentp->size))) {

        /* allocate a new chunk, can we proceed ? */
        if ((mem.max_chunks > 0) &&
            ((mem.next_chunk_index + needed_chunks) > (mem.max_chunks))) {
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
        snprintf(tmp_path, malloc_size, "%s.%lld", mem.base_path,
            mem.next_chunk_index);

        /* add chunk */
        if(add_file_memory(&mem.currentp, tmp_path, chunked_size) != 0) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            free(tmp_path);
            errno = ENOMEM;
            return (NULL);
        }
        free(tmp_path);
    }

    /* current chunk OK */
    struct file_malloc *fmallocp =
        (struct file_malloc *)((unsigned char *)mem.currentp->start_addr +
        mem.currentp->next_free_offset);
    fmallocp->parentp = mem.currentp;

    /* update current file memory status */
    mem.currentp->next_free_offset += aligned_size;
    mem.currentp->ref_count++;

#if defined(DEBUG)
    fprintf(stderr, "%s(): %zd bytes requested\n", __func__, requested_size);
    fprintf(stderr, "%s(): %zd bytes allocated @%p in %zd chunk(s)\n",
        __func__, aligned_size, &(fmallocp->data[0]), needed_chunks);
#endif

    return (&(fmallocp->data[0]));
}

/* Replacement for free(3) */
void
file_free(void *ptr)
{
    assert(ptr != NULL);

    /* find parent file_memory */
    struct file_memory *parent = get_parentp(ptr);
    assert(parent != NULL);

#if defined(DEBUG)
    fprintf(stderr, "%s(): freeing address %p (parent @%p)\n", __func__,
        ptr, parent);
#endif

    /* update parent's information */
    assert(parent->ref_count >= 1);
    parent->ref_count--;

    /* all file_mallocs have been freed in this file_memory */
    if(parent->ref_count < 1) {
        /* if this file_memory will not be used anymore because :
           - either another one is already chained to it
           - or there is not enough space to handle a 1-byte malloc request,
           remove it */
        if((parent->nextp != NULL) ||
            ((parent->next_free_offset +
            round_num(1 + FILE_MALLOC_HEADER_SIZE, sizeof(void *))) >
            parent->size)) {
#if defined(DEBUG)
            fprintf(stderr, "%s(): memory file @%p useless, deleting\n",
                __func__, parent);
#endif
            delete_file_memory(parent);
        }
    }

#if defined(DEBUG)
    fprintf(stderr, "%s(): memory free'ed @%p\n", __func__, ptr);
#endif

    return;
}
