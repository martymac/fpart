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

#ifndef _FILE_ENTRY_H
#define _FILE_ENTRY_H

#include "types.h"
#include "options.h"

#include <sys/types.h>

#if !defined(PRINT_FE_CHUNKS)
#define PRINT_FE_CHUNKS 32          /* files per chunk when flushing
                                       partitions to disk */
#endif

/* A file entry */
struct file_entry;
struct file_entry {
    char *path;                     /* file name */
    fsize_t size;                   /* size in bytes */
    pnum_t partition_index;         /* assigned partition index */

    struct file_entry* nextp;       /* next file_entry */
    struct file_entry* prevp;       /* previous one */
};

int add_file_entry(struct file_entry **head, char *path, fsize_t size,
    struct program_options *options);
fnum_t init_file_entries(char *file_path, struct file_entry **head,
    struct program_options *options);
void uninit_file_entries(struct file_entry *head,
    struct program_options *options);
int print_file_entries(struct file_entry *head, char *out_template,
    pnum_t num_parts);
void init_file_entry_p(struct file_entry **file_entry_p, fnum_t num_entries,
    struct file_entry *head);

#endif /* _FILE_ENTRY_H */
