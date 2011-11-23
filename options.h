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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "types.h"

#include <sys/types.h>

/* Program options */
struct program_options {
#define DFLT_OPT_NUM_PART           0
    pnum_t num_parts;
#define DFLT_OPT_MAX_ENTRIES        0
    fnum_t max_entries;
#define DFLT_OPT_MAX_SIZE           0
    fsize_t max_size;
    char *in_filename;
    char *out_filename;
#define DFLT_OPT_DIR_DEPTH          -1
    int dir_depth;
    unsigned char add_slash;
    unsigned char verbose;
};

void init_options(struct program_options *options);
void uninit_options(struct program_options *options);

#endif /* _OPTIONS_H */
