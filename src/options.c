/*-
 * Copyright (c) 2011-2023 Ganael LAPLANCHE <ganael.laplanche@martymac.org>
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
    assert((DFLT_OPT_OUT0 == OPT_NOOUT0) ||
           (DFLT_OPT_OUT0 == OPT_OUT0));
    assert((DFLT_OPT_ADDSLASH == OPT_NOADDSLASH) ||
           (DFLT_OPT_ADDSLASH == OPT_ADDSLASH));
    assert((DFLT_OPT_VERBOSE == OPT_NOVERBOSE) ||
           (DFLT_OPT_VERBOSE == OPT_VERBOSE) ||
           (DFLT_OPT_VERBOSE == OPT_VVERBOSE));
    assert((DFLT_OPT_FOLLOWSYMLINKS == OPT_FOLLOWSYMLINKS) ||
           (DFLT_OPT_FOLLOWSYMLINKS == OPT_NOFOLLOWSYMLINKS));
    assert((DFLT_OPT_CROSSFSBOUNDARIES == OPT_NOCROSSFSBOUNDARIES) ||
           (DFLT_OPT_CROSSFSBOUNDARIES == OPT_CROSSFSBOUNDARIES));
    assert((DFLT_OPT_DIRSINCLUDE == OPT_NOEMPTYDIRS) ||
           (DFLT_OPT_DIRSINCLUDE == OPT_EMPTYDIRS) ||
           (DFLT_OPT_DIRSINCLUDE == OPT_DNREMPTY) ||
           (DFLT_OPT_DIRSINCLUDE == OPT_ALLDIRS));
    assert((DFLT_OPT_DNRSPLIT == OPT_NODNRSPLIT) ||
           (DFLT_OPT_DNRSPLIT == OPT_DNRSPLIT));
    assert(DFLT_OPT_DIR_DEPTH >= OPT_NODIRDEPTH);
    assert((DFLT_OPT_LEAFDIRS == OPT_NOLEAFDIRS) ||
           (DFLT_OPT_LEAFDIRS == OPT_LEAFDIRS));
    assert((DFLT_OPT_DIRSONLY == OPT_NODIRSONLY) ||
           (DFLT_OPT_DIRSONLY == OPT_DIRSONLY));
    assert((DFLT_OPT_LIVEMODE == OPT_NOLIVEMODE) ||
           (DFLT_OPT_LIVEMODE == OPT_LIVEMODE));
    assert((DFLT_OPT_SKIPBIG == OPT_NOSKIPBIG) ||
           (DFLT_OPT_SKIPBIG == OPT_SKIPBIG));
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
    options->out_zero = DFLT_OPT_OUT0;
    options->add_slash = DFLT_OPT_ADDSLASH;
    options->verbose = DFLT_OPT_VERBOSE;
    options->follow_symbolic_links = DFLT_OPT_FOLLOWSYMLINKS;
    options->cross_fs_boundaries = DFLT_OPT_CROSSFSBOUNDARIES;
    options->include_files = NULL;
    options->ninclude_files = 0;
    options->include_files_ci = NULL;
    options->ninclude_files_ci = 0;
    options->exclude_files = NULL;
    options->nexclude_files = 0;
    options->exclude_files_ci = NULL;
    options->nexclude_files_ci = 0;
    options->dirs_include = DFLT_OPT_DIRSINCLUDE;
    options->dir_depth = DFLT_OPT_DIR_DEPTH;
    options->dnr_split = OPT_NODNRSPLIT;
    options->leaf_dirs = DFLT_OPT_LEAFDIRS;
    options->dirs_only = DFLT_OPT_DIRSONLY;
    options->live_mode = DFLT_OPT_LIVEMODE;
    options->skip_big = DFLT_OPT_SKIPBIG;
    options->pre_part_hook = NULL;
    options->post_part_hook = NULL;
    options->preload_size = DFLT_OPT_PRELOAD_SIZE;
    options->overload_size = DFLT_OPT_OVERLOAD_SIZE;
    options->round_size = DFLT_OPT_ROUND_SIZE;
}

/* Un-initialize global options structure */
void
uninit_options(struct program_options *options)
{
    options->round_size = DFLT_OPT_ROUND_SIZE;
    options->overload_size = DFLT_OPT_OVERLOAD_SIZE;
    options->preload_size = DFLT_OPT_PRELOAD_SIZE;
    if(options->post_part_hook != NULL)
        free(options->post_part_hook);
    if(options->pre_part_hook != NULL)
        free(options->pre_part_hook);
    options->skip_big = DFLT_OPT_SKIPBIG;
    options->live_mode = DFLT_OPT_LIVEMODE;
    options->dirs_only = DFLT_OPT_DIRSONLY;
    options->leaf_dirs = DFLT_OPT_LEAFDIRS;
    options->dnr_split = OPT_NODNRSPLIT;
    options->dir_depth = DFLT_OPT_DIR_DEPTH;
    options->dirs_include = DFLT_OPT_DIRSINCLUDE;
    if(options->exclude_files_ci != NULL)
        str_cleanup(&(options->exclude_files_ci),
            &(options->nexclude_files_ci));
    if(options->exclude_files != NULL)
        str_cleanup(&(options->exclude_files),
            &(options->nexclude_files));
    if(options->include_files_ci != NULL)
        str_cleanup(&(options->include_files_ci),
            &(options->ninclude_files_ci));
    if(options->include_files != NULL)
        str_cleanup(&(options->include_files),
            &(options->ninclude_files));
    options->cross_fs_boundaries = DFLT_OPT_CROSSFSBOUNDARIES;
    options->follow_symbolic_links = DFLT_OPT_FOLLOWSYMLINKS;
    options->verbose = DFLT_OPT_VERBOSE;
    options->add_slash = DFLT_OPT_ADDSLASH;
    options->out_zero = DFLT_OPT_OUT0;
    if(options->out_filename != NULL)
        free(options->out_filename);
    options->arbitrary_values = DFLT_OPT_ARBITRARYVALUES;
    if(options->in_filename != NULL)
        free(options->in_filename);
    options->max_size = DFLT_OPT_MAX_SIZE;
    options->max_entries = DFLT_OPT_MAX_ENTRIES;
    options->num_parts = DFLT_OPT_NUM_PARTS;
}
