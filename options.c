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

#include "options.h"

/* NULL */
#include <stdlib.h>

/*************************
 Program options functions
 *************************/

/* Initialize global options structure */
void
init_options(struct program_options *options)
{
    /* set default options */
    options->num_parts = DFLT_OPT_NUM_PART;
    options->max_entries = DFLT_OPT_MAX_ENTRIES;
    options->max_size = DFLT_OPT_MAX_SIZE;
    options->in_filename = NULL;
    options->out_filename = NULL;
    options->dir_depth = DFLT_OPT_DIR_DEPTH;
    options->add_slash = DFLT_OPT_ADDSLASH;
    options->verbose = DFLT_OPT_VERBOSE;
    options->stat_function = DFLT_OPT_FOLLOWSYMLINKS;
    options->cross_fs_boundaries = DFLT_OPT_CROSSFSBOUNDARIES;
}

/* Un-initialize global options structure */
void
uninit_options(struct program_options *options)
{
    options->cross_fs_boundaries = DFLT_OPT_CROSSFSBOUNDARIES;
    options->stat_function = DFLT_OPT_FOLLOWSYMLINKS;
    options->verbose = DFLT_OPT_VERBOSE;
    options->add_slash = DFLT_OPT_ADDSLASH;
    options->dir_depth = DFLT_OPT_DIR_DEPTH;
    if(options->out_filename != NULL)
        free(options->out_filename);
    if(options->in_filename != NULL)
        free(options->in_filename);
    options->max_size = DFLT_OPT_MAX_SIZE;
    options->max_entries = DFLT_OPT_MAX_ENTRIES;
    options->num_parts = DFLT_OPT_NUM_PART;
}
