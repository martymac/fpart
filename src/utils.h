/*-
 * Copyright (c) 2011-2019 Ganael LAPLANCHE <ganael.laplanche@martymac.org>
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

#ifndef _UTILS_H
#define _UTILS_H

#include "types.h"
#include "options.h"

/* stat(2) */
#include <sys/types.h>
#include <sys/stat.h>

/* fts(3) */
#if defined(EMBED_FTS)
#include "fts.h"
#else
#include <fts.h>
#endif

/* fnmatch(3) and FNM_CASEFOLD
   FNM_CASEFOLD is a GNU extension and may not be available */
#include <fnmatch.h>
#if defined(FNM_CASEFOLD)
#define _HAS_FNM_CASEFOLD
#else
#define FNM_CASEFOLD        0
#if defined(DEBUG)
#warning FNM_CASEFOLD not supported by fnmatch(3), \
    options '-X' and '-Y' disabled
#endif
#endif

#define round_num(x, y) \
    ((((x) % (y)) != 0) ? (((x) / (y)) * (y) + (y)) : (x))

#define rewind_list(head) \
    { while((head) && (head)->prevp) { (head) = (head)->prevp; } }
#define fastfw_list(head) \
    { while((head) && (head)->nextp) { (head) = (head)->nextp; } }

#define min(x, y) (((x) <= (y)) ? (x) : (y))
#define max(x, y) (((x) >= (y)) ? (x) : (y))

#define if_not_malloc(ptr, size, err_action)                            \
    ptr = malloc(size);                                                 \
    if (ptr == NULL) {                                                  \
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);    \
        err_action                                                      \
    }

#define if_not_realloc(ptr, size, err_action)                           \
    ptr = realloc(ptr, size);                                           \
    if (ptr == NULL) {                                                  \
        fprintf(stderr, "%s(): cannot reallocate memory\n", __func__);  \
        err_action                                                      \
    }

unsigned int get_num_digits(double i);
fsize_t get_size(char *file_path, struct stat *file_stat,
    struct program_options *options);
char *abs_path(const char *path);
int str_push(char ***array, unsigned int *num, const char * const str);
void str_cleanup(char ***array, unsigned int *num);
int file_match(const char * const * const array, const unsigned int num,
    const FTSENT * const p, const unsigned char ignore_case);
int valid_file(const FTSENT * const p, struct program_options *options,
    unsigned char is_leaf);
char ** clone_env(void);
int push_env(char *str, char ***env);

#endif /* _UTILS_H */
