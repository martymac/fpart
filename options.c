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

/* assert(3) */
#include <assert.h>

/*************************
 Program options functions
 *************************/

/* Initialize global options structure */
void
init_options(struct program_options *options)
{
    /* check our default values */
    assert(DFLT_OPT_NUM_PARTS >= 0);
    assert(DFLT_OPT_MAX_ENTRIES >= 0);
    assert(DFLT_OPT_MAX_SIZE >= 0);
    assert((DFLT_OPT_ARBITRARYVALUES == OPT_NOARBITRARYVALUES) ||
           (DFLT_OPT_ARBITRARYVALUES == OPT_ARBITRARYVALUES));
    assert(DFLT_OPT_DIR_DEPTH >= OPT_NODIRDEPTH);
    assert((DFLT_OPT_ADDSLASH == OPT_NOADDSLASH) ||
           (DFLT_OPT_ADDSLASH == OPT_ADDSLASH));
    assert((DFLT_OPT_VERBOSE == OPT_NOVERBOSE) ||
           (DFLT_OPT_VERBOSE == OPT_VERBOSE) ||
           (DFLT_OPT_VERBOSE == OPT_VVERBOSE));
    assert((DFLT_OPT_LIVEMODE == OPT_NOLIVEMODE) ||
           (DFLT_OPT_LIVEMODE == OPT_LIVEMODE));
    assert((DFLT_OPT_FOLLOWSYMLINKS == OPT_FOLLOWSYMLINKS) ||
           (DFLT_OPT_FOLLOWSYMLINKS == OPT_NOFOLLOWSYMLINKS));
    assert((DFLT_OPT_CROSSFSBOUNDARIES == OPT_NOCROSSFSBOUNDARIES) ||
           (DFLT_OPT_CROSSFSBOUNDARIES == OPT_CROSSFSBOUNDARIES));
    assert(DFLT_OPT_PRELOAD_SIZE >= 0);
    assert(DFLT_OPT_OVERLOAD_SIZE >= 0);
    assert(DFLT_OPT_ROUND_SIZE >= 1);

    /* set default options */
    options->num_parts = DFLT_OPT_NUM_PARTS;
    options->max_entries = DFLT_OPT_MAX_ENTRIES;
    options->max_size = DFLT_OPT_MAX_SIZE;
    options->in_filename = NULL;
    options->arbitrary_values = DFLT_OPT_ARBITRARYVALUES;
    options->out_filename = NULL;
    options->dir_depth = DFLT_OPT_DIR_DEPTH;
    options->add_slash = DFLT_OPT_ADDSLASH;
    options->verbose = DFLT_OPT_VERBOSE;
    options->live_mode = DFLT_OPT_LIVEMODE;
    options->pre_part_hook = NULL;
    options->post_part_hook = NULL;
    options->follow_symbolic_links = DFLT_OPT_FOLLOWSYMLINKS;
    options->cross_fs_boundaries = DFLT_OPT_CROSSFSBOUNDARIES;
    options->preload_size = DFLT_OPT_PRELOAD_SIZE;
    options->overload_size = DFLT_OPT_OVERLOAD_SIZE;
    options->round_size = DFLT_OPT_ROUND_SIZE;
#if defined(WITH_FILE_MEMORY)
    options->mem_filename = NULL;
#endif
}

/* Un-initialize global options structure */
void
uninit_options(struct program_options *options)
{
#if defined(WITH_FILE_MEMORY)
    if(options->mem_filename != NULL)
        free(options->mem_filename);
#endif
    options->round_size = DFLT_OPT_ROUND_SIZE;
    options->overload_size = DFLT_OPT_OVERLOAD_SIZE;
    options->preload_size = DFLT_OPT_PRELOAD_SIZE;
    options->cross_fs_boundaries = DFLT_OPT_CROSSFSBOUNDARIES;
    options->follow_symbolic_links = DFLT_OPT_FOLLOWSYMLINKS;
    if(options->post_part_hook != NULL)
        free(options->post_part_hook);
    if(options->pre_part_hook != NULL)
        free(options->pre_part_hook);
    options->live_mode = DFLT_OPT_LIVEMODE;
    options->verbose = DFLT_OPT_VERBOSE;
    options->add_slash = DFLT_OPT_ADDSLASH;
    options->dir_depth = DFLT_OPT_DIR_DEPTH;
    if(options->out_filename != NULL)
        free(options->out_filename);
    options->arbitrary_values = DFLT_OPT_ARBITRARYVALUES;
    if(options->in_filename != NULL)
        free(options->in_filename);
    options->max_size = DFLT_OPT_MAX_SIZE;
    options->max_entries = DFLT_OPT_MAX_ENTRIES;
    options->num_parts = DFLT_OPT_NUM_PARTS;
}
