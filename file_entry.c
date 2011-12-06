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

/* strerror(3), strlen(3) */
#include <string.h>

/* errno */
#include <errno.h>

/* malloc(3) */
#include <stdlib.h>

/* fts(3) */
#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

/* open(2) */
#include <fcntl.h>

/* close(2) */
#include <unistd.h>

/* assert(3) */
#include <assert.h>

/*********************************************************
 Double-linked list of file_entries manipulation functions
 *********************************************************/

/* Add a file entry to a double-linked list of file_entries
   - if head is NULL, creates a new file entry ; if not, chains a new file
     entry to it
   - returns with head set to the newly added element */
int
add_file_entry(struct file_entry **head, char *path, fsize_t size,
    struct program_options *options)
{
    assert(head != NULL);
    assert(path != NULL);
    assert(options != NULL);

    struct file_entry **current = head; /* current file_entry pointer address */
    struct file_entry *previous = NULL; /* previous file_entry pointer */

    /* backup current structure pointer and initialize a new structure */
    previous = *current;
    if((*current = malloc(sizeof(struct file_entry))) == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        return (1);
    }

    /* set head on first call */
    if(*head == NULL)
        *head = *current;

    /* set current file data */
    size_t malloc_size = strlen(path) + 1;
    if(((*current)->path = malloc(malloc_size)) == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        return (1);
    }
    snprintf((*current)->path, malloc_size, "%s", path);
    (*current)->size = size;

    /* set current file entry's index and pointers */
    (*current)->partition_index = 0;    /* set during dispatch */
    (*current)->nextp = NULL;           /* set in next pass (see below) */
    (*current)->prevp = previous;

    /* display added filename */
    if(options->verbose == OPT_VERBOSE)
        fprintf(stderr, "%s\n", (*current)->path);

    /* set previous' nextp pointer */
    if(previous != NULL)
        previous->nextp = *current;

    return (0);
}

/* Initialize a double-linked list of file_entries from a path
   - file_path may be a file or directory
   - if head is NULL, creates a new list ; if not, chains a new list to it
   - returns the number of files found with head set to the last element */
fnum_t
init_file_entries(char *file_path, struct file_entry **head,
    struct program_options *options)
{
    assert(file_path != NULL);
    assert(head != NULL);
    assert(options != NULL);

    fnum_t num_files = 0;   /* number of files added to the list */

    /* prepare fts */
    FTS *ftsp = NULL;
    FTSENT *p = NULL;
    int fts_options = (options->follow_symbolic_links == OPT_FOLLOWSYMLINKS) ?
        FTS_LOGICAL : FTS_PHYSICAL;
    fts_options |= (options->cross_fs_boundaries == OPT_NOCROSSFSBOUNDARIES) ?
        FTS_XDEV : 0;

    char *fts_argv[] = { file_path, NULL };
    if((ftsp = fts_open(fts_argv, fts_options, NULL)) == NULL) {
        fprintf(stderr, "%s: fts_open()\n", file_path);
        return (0);
    }

    while((p = fts_read(ftsp)) != NULL) {
        switch (p->fts_info) {
            case FTS_DNR:   /* un-readable directory */
            case FTS_ERR:   /* misc error */
            case FTS_NS:    /* stat() error */
                fprintf(stderr, "%s: %s\n", p->fts_path,
                    strerror(p->fts_errno));
                continue;

            case FTS_DC:
                fprintf(stderr, "%s: file system loop detected\n", p->fts_path);
            case FTS_DOT:  /* ignore "." and ".." */
            case FTS_DP:   /* ignore post-order visits */
            case FTS_NSOK: /* no stat(2) available (not requested) */
                continue;

            case FTS_D:
                /* if dir_depth is reached, skip descendants
                   but add directory entry */
                if((options->dir_depth != OPT_NODIRDEPTH) &&
                    (p->fts_level >= options->dir_depth))
                    fts_set(ftsp, p, FTS_SKIP);
                /* else, if dir_depth no requested or not reached,
                   skip directory entry but continue examining files within */
                else
                    continue;
                /* FALLTHROUGH and add directory to entries */

            default:
            /* XXX default means remaining file types:
               FTS_D (dir_depth reached), FTS_F, FTS_SL, FTS_SLNONE,
               FTS_DEFAULT */
            {
                char *file_entry_path = NULL;
                fsize_t file_entry_size = 0;

                /* count ending '/' and '\0', even if an ending '/' is not
                   added */
                size_t malloc_size = p->fts_pathlen + 1 + 1;
                if((file_entry_path = malloc(malloc_size)) == NULL) {
                    fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
                    return (num_files);
                }

                if(S_ISDIR(p->fts_statp->st_mode) &&
                    (options->add_slash == OPT_ADDSLASH) &&
                    (p->fts_pathlen > 0) &&
                    (p->fts_path[p->fts_pathlen - 1] != '/'))
                    /* file is a directory, user requested to add a slash and
                       file_path does not already end with a '/' so we can
                       add one */
                    snprintf(file_entry_path, malloc_size, "%s/",
                        p->fts_path);
                else
                    snprintf(file_entry_path, malloc_size, "%s",
                        p->fts_path);

                /* compute current file entry's size */
                if(S_ISDIR(p->fts_statp->st_mode) &&
                    (p->fts_level > 0) &&
                    (options->cross_fs_boundaries == OPT_NOCROSSFSBOUNDARIES) &&
                    (p->fts_parent->fts_statp->st_dev != p->fts_statp->st_dev))
                    /* keep mountpoint for non-root directories
                       and set size to 0 */
                    file_entry_size = 0;
                else
                    file_entry_size =
                        get_size(p->fts_path, p->fts_statp, options);

                /* add it */
                if(add_file_entry
                    (head, file_entry_path, file_entry_size, options) == 0)
                    num_files++;

                /* cleanup */
                free(file_entry_path);

                continue;
            }
        }
    }

    if(errno != 0)
        fprintf(stderr, "%s: fts_read()\n", file_path);

    if(fts_close(ftsp) < 0)
        fprintf(stderr, "%s: fts_close()\n", file_path);

    return (num_files);
}

/* Un-initialize a double-linked list of file_entries */
void
uninit_file_entries(struct file_entry *head)
{
    /* be sure to start at first file entry */
    rewind_list(head);

    struct file_entry *current = head;
    struct file_entry *next = NULL;

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
            fprintf(stdout, "%d (%lld): %s\n", head->partition_index,
                head->size, head->path);
            head = head->nextp;
        }
        return (0);
    }

    /* a template has been provided; to avoid opening too many files,
       open chunks of FDs and do as many passes as necessary */
#define PRINT_FE_CHUNKS     32          /* files per chunk */
    struct file_entry *start = head;
    pnum_t current_chunk = 0;           /* current chunk */
    pnum_t current_file_entry = 0;      /* current file entry within chunk */

    assert(PRINT_FE_CHUNKS > 0);

    while(((current_chunk * PRINT_FE_CHUNKS) + current_file_entry) <
        num_parts) {
        int fd[PRINT_FE_CHUNKS]; /* our <files per chunk> file descriptors */

        /* open as necessary file descriptors as needed
           to print num_part partitions */
        while((current_file_entry < PRINT_FE_CHUNKS) &&
              (((current_chunk * PRINT_FE_CHUNKS) + current_file_entry) < num_parts)) {
            /* compute out_filename  "out_template.i\0" */
            char *out_filename = NULL;
            size_t malloc_size = strlen(out_template) + 1 +
                get_num_digits
                ((current_chunk * PRINT_FE_CHUNKS) + current_file_entry) + 1;
            if((out_filename = malloc(malloc_size)) == NULL) {
                fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
                /* close all open descriptors and return */
                pnum_t i;
                for(i = 0; i < current_file_entry; i++)
                     close(fd[i]);
                return (1);
            }
            snprintf(out_filename, malloc_size, "%s.%d", out_template,
                (current_chunk * PRINT_FE_CHUNKS) + current_file_entry);

            if((fd[current_file_entry] =
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
                size_t to_write = strlen(head->path);
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
