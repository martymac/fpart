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

#include "types.h"
#include "utils.h"
#include "options.h"
#include "file_entry.h"

/* stat(2) */
#include <sys/types.h>
#include <sys/stat.h>

/* fprintf(3) */
#include <stdio.h>

/* strerror(3) */
#include <string.h>

/* errno */
#include <errno.h>

/* malloc(3) */
#include <stdlib.h>

/* opendir(3) */
#include <dirent.h>

/* open(2) */
#include <fcntl.h>

/* close(2) */
#include <unistd.h>

/* assert(3) */
#include <assert.h>

/*********************************************************
 Double-linked list of file_entries manipulation functions
 *********************************************************/

#define print_file_entry(file_entry) \
    { if(file_entry) printf("@%p : path = %s, size = %lld, \
        partindex = %d\n", file_entry, (file_entry)->path, \
        (file_entry)->size, (file_entry)->partition_index); }

/* Initialize a double-linked list of file_entries from a path
   - file_path may be a file or directory
   - if head is NULL, creates a new list ; if not, chains a new list to it
   - returns the number of files found with head set to the last element */
fnum_t
init_file_entries(const char *file_path, struct file_entry **head,
    struct program_options *options, int cur_depth)
{
    fnum_t num_files = 0;  /* number of files added to the list */
    struct stat file_stat;   /* file stat */

    struct file_entry **current = head; /* current file_entry pointer address */
    struct file_entry *previous = NULL; /* previous file_entry pointer */

    assert(file_path != NULL);
    assert(head != NULL);
    assert(options != NULL);

    /* perform stat (and chech path validity and accessibility) */
    if(lstat(file_path, &file_stat)<0) {
        fprintf(stderr, "%s: %s\n", file_path, strerror(errno));
        return (0);
    }

    if((!S_ISDIR(file_stat.st_mode)) ||
       ((options->dir_depth >= 0) && (cur_depth >= options->dir_depth))) {
        /* file found, or dir_depth reached : add entry */

        /* backup current structure pointer and initialize a new structure */
        previous = *current;
        if((*current = malloc(sizeof(struct file_entry))) == NULL) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            return (0);
        }

        /* set head on first pass */
        if(*head == NULL)
            *head = *current;

        /* update current structure data */
        if(options->verbose)
            fprintf(stderr, "%s\n", file_path);
        size_t malloc_size = strnlen(file_path, FILENAME_MAX) + 1;
        if(S_ISDIR(file_stat.st_mode) && options->add_slash)
            malloc_size += 1; /* ending slash */
        if(((*current)->path = (char *)malloc(malloc_size)) == NULL) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            return (0);
        }
        if(S_ISDIR(file_stat.st_mode) && options->add_slash)
            snprintf((*current)->path, FILENAME_MAX + 1, "%s/", file_path);
        else
            snprintf((*current)->path, FILENAME_MAX + 1, "%s", file_path);
        (*current)->size = get_size(file_path, &file_stat);
        (*current)->partition_index = 0; /* will be set during dispatch */
        (*current)->nextp = NULL;   /* will be set in next pass (see below) */
        (*current)->prevp = previous;

        /* set previous' nextp pointer */
        if(previous != NULL)
            previous->nextp = *current;

        /* count file */ 
        num_files++;
    }
    else {
        /* directory found and dir_depth not reached : crawl it */
        struct dirent * dir_entry = NULL;
        DIR * dir_handle = opendir(file_path);
        if(dir_handle == NULL) {
            fprintf(stderr, "%s: %s\n", file_path, strerror(errno));
            return (0);
        }
        while((dir_entry = readdir(dir_handle)) != NULL) {
            /* ignore "." and ".." */
            if(dir_entry->d_name[0] == '.' &&
              (dir_entry->d_name[1] == 0 || (dir_entry->d_name[1] == '.' && dir_entry->d_name[2] == 0)))
              continue;

            /* compute entry's full path */
            char *dir_entry_path;
            if((dir_entry_path = (char *)malloc(strnlen(file_path, FILENAME_MAX) + 1 + strnlen(dir_entry->d_name, FILENAME_MAX) + 1)) == NULL) {
                fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
                closedir(dir_handle);
                return (0);
            }
            snprintf(dir_entry_path, FILENAME_MAX + 1, "%s/%s", file_path,
                dir_entry->d_name);

            /* recurse for each file */
            num_files += init_file_entries(dir_entry_path, current,
                options, cur_depth + 1);
            free(dir_entry_path);
        }
        closedir(dir_handle);
    }

    return (num_files);
}

/* Un-initialize a double-linked list of file_entries */
void
uninit_file_entries(struct file_entry *head)
{
    /* be sure to start at first file entry */
    rewind_list(head);

    struct file_entry *current = head;
    struct file_entry *next;

    while(current != NULL) {
        if(current->path != NULL)
            free(current->path);
        next = current->nextp;
        free(current);
        current = next;
    }
    return;
}

/* Print a double-linked list of file_entries from head
   - if no filename template given, print to stdout */
int
print_file_entries(struct file_entry *head, char *out_template,
    pnum_t num_parts)
{
    assert(head != NULL);
    assert(num_parts > 0);

    /* no template provided, just print to stdout and return */
    if(out_template == NULL) {
        while(head != NULL) {
            fprintf(stdout, "%d: %s\n", head->partition_index, head->path);
            head = head->nextp;
        }
        return (0);
    }

    /* a template has been provided; to avoid opening too many files,
       open chunks of FDs and do as many passes as necessary */
#define PRINT_FE_CHUNKS     32              /* files per chunk */
    struct file_entry *start = head;
    pnum_t current_chunk = 0;         /* current chunk */
    pnum_t current_file_entry = 0;    /* current file entry within chunk */

    assert(PRINT_FE_CHUNKS > 0);

    while(((current_chunk * PRINT_FE_CHUNKS) + current_file_entry) <
        num_parts) {
        int fd[PRINT_FE_CHUNKS]; /* our <files per chunk> file descriptors */

        /* open as necessary file descriptors as needed
           to print num_part partitions */
        while((current_file_entry < PRINT_FE_CHUNKS) &&
              (((current_chunk * PRINT_FE_CHUNKS) + current_file_entry) < num_parts)) {
            /* compute out_filename  "out_template.i\0" */
            char *out_filename;
            if((out_filename = 
                (char *)malloc(strnlen(out_template, FILENAME_MAX) + 1 +
                get_num_digits((current_chunk * PRINT_FE_CHUNKS) +
                current_file_entry) + 1)) == NULL) {
                fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
                /* close all open descriptors and return */
                pnum_t i;
                for(i = 0; i < current_file_entry; i++)
                     close(fd[i]);
                return (1);
            }
            snprintf(out_filename, FILENAME_MAX + 1, "%s.%d", out_template,
                (current_chunk * PRINT_FE_CHUNKS) + current_file_entry);

            if ((fd[current_file_entry] =
                open(out_filename, O_WRONLY|O_CREAT|O_TRUNC, 0660)) < 0) {
                fprintf(stderr, "%s: %s\n", out_filename, strerror(errno));
                free(out_filename);
                /* close all open descriptors and return */
                pnum_t i;
                for(i = 0; i < current_file_entry; i++)
                     close(fd[i]);
                return (1);
            }
            free(out_filename);
            current_file_entry++;
        }

        while(head != NULL) {
            if((head->partition_index >= (current_chunk * PRINT_FE_CHUNKS)) &&
               (head->partition_index < ((current_chunk + 1) * PRINT_FE_CHUNKS))) {
                size_t to_write = strnlen(head->path, FILENAME_MAX);
                if((write(fd[head->partition_index % PRINT_FE_CHUNKS], head->path, to_write) != to_write) ||
                    (write(fd[head->partition_index % PRINT_FE_CHUNKS], "\n", 1) != 1)) {
                    fprintf(stderr, "%s\n", strerror(errno));
                    /* close all open descriptors */
                    pnum_t i;
                    for(i = 0; (i < PRINT_FE_CHUNKS) && (((current_chunk * PRINT_FE_CHUNKS) + i) < num_parts); i++)
                        close(fd[i]);
                    return (1);
                }
            }
            head = head->nextp;
        }
        /* come back to first entry */
        head = start;

        /* close file descriptors */
        pnum_t i;
        for(i = 0; (i < PRINT_FE_CHUNKS) && (((current_chunk * PRINT_FE_CHUNKS) + i) < num_parts); i++)
            close(fd[i]);

        current_file_entry = 0;
        current_chunk++;
    }

    return (0);
}

/***************************************************
 Array of file_entry pointers manipulation functions
 ***************************************************/

/* Initialize an array of file_entry pointers from a double-linked
   list of file_entries (head) */
void
init_file_entry_p(struct file_entry **file_entry_p, fnum_t num_entries,
    struct file_entry *head)
{
    fnum_t i = 0;
    while((head != NULL) && (file_entry_p != NULL) && (i < num_entries)) {
        file_entry_p[i] = head;
        head = head->nextp;
        i++;
    }
    return;
}
