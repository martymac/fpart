/*-
 * Copyright (c) 2011-2022 Ganael LAPLANCHE <ganael.laplanche@martymac.org>
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

/* stat(2) */
#include <sys/types.h>
#include <sys/stat.h>

/* Program options */
struct program_options {
/* number of partitions (option -n) */
#define DFLT_OPT_NUM_PARTS          0
    pnum_t num_parts;
/* maximum files per partition (option -f) */
#define DFLT_OPT_MAX_ENTRIES        0
    fnum_t max_entries;
/* maximum partition size (option -s) */
#define DFLT_OPT_MAX_SIZE           0
    fsize_t max_size;
/* input file (option -i); NULL = undefined, "-" = stdin, "filename" */
    char *in_filename;
/* arbitrary values (option -a) */
#define OPT_NOARBITRARYVALUES       0
#define OPT_ARBITRARYVALUES         1
#define DFLT_OPT_ARBITRARYVALUES    OPT_NOARBITRARYVALUES
    unsigned char arbitrary_values;
/* output file (option -o); NULL = stdout, "filename" */
    char *out_filename;
/* add a null character after filename in file lists */
#define OPT_NOOUT0                  0
#define OPT_OUT0                    1
#define DFLT_OPT_OUT0               OPT_NOOUT0
    unsigned char out_zero;
/* add slash to directories (option -e) */
#define OPT_NOADDSLASH              0
#define OPT_ADDSLASH                1
#define DFLT_OPT_ADDSLASH           OPT_NOADDSLASH
    unsigned char add_slash;
/* verbose output (option -v) */
#define OPT_NOVERBOSE               0
#define OPT_VERBOSE                 1
#define OPT_VVERBOSE                2
#define OPT_VVVERBOSE               3
#define DFLT_OPT_VERBOSE            OPT_NOVERBOSE
    unsigned char verbose;
/* follow symbolic links (option -l) */
#define OPT_FOLLOWSYMLINKS          0
#define OPT_NOFOLLOWSYMLINKS        1
#define DFLT_OPT_FOLLOWSYMLINKS     OPT_NOFOLLOWSYMLINKS
    unsigned char follow_symbolic_links;
/* cross fs boundaries (option -b) */
#define OPT_NOCROSSFSBOUNDARIES     0
#define OPT_CROSSFSBOUNDARIES       1
#define DFLT_OPT_CROSSFSBOUNDARIES  OPT_CROSSFSBOUNDARIES
    unsigned char cross_fs_boundaries;
/* include files, case sensitive (option -y) */
    char **include_files;
    unsigned int ninclude_files;
/* include files, case insensitive (option -Y) */
    char **include_files_ci;
    unsigned int ninclude_files_ci;
/* exclude files, case sensitive (option -x) */
    char **exclude_files;
    unsigned int nexclude_files;
/* exclude files, case insensitive (option -X) */
    char **exclude_files_ci;
    unsigned int nexclude_files_ci;
/* include certain directories (option -z) */
#define OPT_NOEMPTYDIRS             0
#define OPT_EMPTYDIRS               1   /* include empty directories */
#define OPT_DNREMPTY                2   /* also add un-readable directories */
#define OPT_ALLDIRS                 3   /* include all directories */
#define DFLT_OPT_DIRSINCLUDE        OPT_NOEMPTYDIRS
    unsigned char dirs_include;
/* reported negative part size for un-readable directories (option -Z) */
    unsigned char dnr_negative_size;
/* display directories after certain depth (option -d) */
#define OPT_NODIRDEPTH              -1
#define DFLT_OPT_DIR_DEPTH          OPT_NODIRDEPTH
    int dir_depth;
/* pack leaf directories (option -D) */
#define OPT_NOLEAFDIRS              0
#define OPT_LEAFDIRS                1
#define DFLT_OPT_LEAFDIRS           OPT_NOLEAFDIRS
    unsigned char leaf_dirs;
/* pack directories only (option -E) */
#define OPT_NODIRSONLY              0
#define OPT_DIRSONLY                1
#define DFLT_OPT_DIRSONLY           OPT_NODIRSONLY
    unsigned char dirs_only;
/* live mode (option -L) */
#define OPT_NOLIVEMODE              0
#define OPT_LIVEMODE                1
#define DFLT_OPT_LIVEMODE           OPT_NOLIVEMODE
    unsigned char live_mode;
/* skip big files (option -S) */
#define OPT_NOSKIPBIG            0
#define OPT_SKIPBIG              1
#define DFLT_OPT_SKIPBIG         OPT_NOSKIPBIG
    unsigned char skip_big;
/* pre-partition hook (option -w) */
    char *pre_part_hook;
/* post-partition hook (option -W) */
    char *post_part_hook;
/* preload partitions (option -p) */
#define DFLT_OPT_PRELOAD_SIZE       0
    fsize_t preload_size;
/* overload file entries (option -q) */
#define DFLT_OPT_OVERLOAD_SIZE      0
    fsize_t overload_size;
/* round file size up (option -r) */
#define DFLT_OPT_ROUND_SIZE         1
    fsize_t round_size;
};

void init_options(struct program_options *options);
void uninit_options(struct program_options *options);

#endif /* _OPTIONS_H */
