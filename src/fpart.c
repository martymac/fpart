/*-
 * Copyright (c) 2011-2025 Ganael LAPLANCHE <ganael.laplanche@martymac.org>
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

#include "fpart.h"
#include "types.h"
#include "utils.h"
#include "options.h"
#include "partition.h"
#include "file_entry.h"
#include "dispatch.h"

/* NULL, exit(3) */
#include <stdlib.h>

/* strtoumax(3) */
#include <limits.h>
#include <inttypes.h>

/* fprintf(3), fopen(3), fclose(3), fgets(3), foef(3) */
#include <stdio.h>

/* getopt(3) / getopt_long(3) */
#include <unistd.h>
#if defined(HAVE_GETOPT_H)
#include <getopt.h>
#endif

/* Short options */
#if defined(_HAS_FNM_CASEFOLD)
#define OPTIONS "+hVn:f:s:i:ao:0ePvlby:Y:x:X:zZd:DELSw:W:R:p:q:r:"
#else
#define OPTIONS "+hVn:f:s:i:ao:0ePvlby:x:zZd:DELSw:W:R:p:q:r:"
#endif

/* Long options */
#if defined(HAVE_GETOPT_LONG)
#define GETOPT getopt_long
static struct option long_options[] =
{
    { "help",           no_argument,        NULL, 'h' },
    { "version",        no_argument,        NULL, 'V' },
    { "parts",          required_argument,  NULL, 'n' },
    { "files",          required_argument,  NULL, 'f' },
    { "size",           required_argument,  NULL, 's' },
    { "arbitrary",      no_argument,        NULL, 'a' },
    { "verbose",        no_argument,        NULL, 'v' },
    { "include",        required_argument,  NULL, 'y' },
    { "exclude",        required_argument,  NULL, 'x' },
    { "leaf-dirs",      no_argument,        NULL, 'D' },
    { "dirs-only",      no_argument,        NULL, 'E' },
    { "live",           no_argument,        NULL, 'L' },
    { "pre-part-cmd",   required_argument,  NULL, 'w' },
    { "post-part-cmd",  required_argument,  NULL, 'W' },
    { "post-run-cmd",  required_argument,  NULL, 'R' },
    { NULL, 0, NULL, 0 }
};
#else
#define GETOPT getopt
#endif

/* strlen(3) */
#include <string.h>

/* bzero(3) */
#include <strings.h>

/* errno */
#include <errno.h>

/* assert(3) */
#include <assert.h>

/* Print version */
static void
version(void)
{
    fprintf(stderr, "fpart v" FPART_VERSION "\n"
        "Copyright (c) 2011-2025 "
        "Ganael LAPLANCHE <ganael.laplanche@martymac.org>\n"
        "WWW: https://www.fpart.org, https://contribs.martymac.org\n");
    fprintf(stderr, "Build options: debug=");
#if defined(DEBUG)
    fprintf(stderr, "yes, fts=");
#else
    fprintf(stderr, "no, fts=");
#endif
#if defined(EMBED_FTS)
    fprintf(stderr, "embedded\n");
#else
    fprintf(stderr, "system\n");
#endif
    fprintf(stderr, "Long options are ");
#if defined(HAVE_GETOPT_LONG)
    fprintf(stderr, "supported\n");
#else
    fprintf(stderr, "not supported\n");
#endif
}

/* Print usage */
static void
usage(void)
{
    fprintf(stderr, "fpart v" FPART_VERSION " - Sort and pack files into "
        "partitions\n");
    fprintf(stderr, "Usage: fpart [OPTIONS] -n num | -f files | -s size "
        "[FILE or DIR...]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "General options:\n");
    fprintf(stderr, "  -h, --help           print this help\n");
    fprintf(stderr, "  -V, --version        print version\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Partition control:\n");
    fprintf(stderr, "  -n, --parts          pack files into <num> "
        "partitions\n");
    fprintf(stderr, "  -f, --files          limit partitions to <files> files "
        "or directories\n");
    fprintf(stderr, "  -s, --size           limit partitions to <size> "
        "bytes\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Input control:\n");
    fprintf(stderr, "  -i                   read file list from <infile> "
        "(stdin if '-' is specified)\n");
    fprintf(stderr, "  -a, --arbitrary      input contains arbitrary values "
        "(do not crawl filesystem)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Output control:\n");
    fprintf(stderr, "  -o                   output partitions to <outfile> "
        "template (stdout if '-' is\n");
    fprintf(stderr, "                       specified)\n");
    fprintf(stderr, "  -0                   end filenames with a null (\\0) "
        "character when using\n");
    fprintf(stderr, "                       option -o\n");
    fprintf(stderr, "  -e                   add ending slash to directories\n");
    fprintf(stderr, "  -P                   add parent directories when "
        "closing intermediate\n");
    fprintf(stderr, "                       partitions (needs -L)\n");
    fprintf(stderr, "  -v, --verbose        verbose mode (may be specified "
        "more than once to increase\n");
    fprintf(stderr, "                       verbosity)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Filesystem crawling control:\n");
    fprintf(stderr, "  -l                   follow symbolic links\n");
    fprintf(stderr, "  -b                   do not cross filesystem "
        "boundaries\n");
    fprintf(stderr, "  -y, --include        include files matching <pattern> "
        "only (may be specified\n");
    fprintf(stderr, "                       more than once)\n");
#if defined(_HAS_FNM_CASEFOLD)
    fprintf(stderr, "  -Y                   same as -y, but ignore case\n");
#endif
    fprintf(stderr, "  -x, --exclude        exclude files matching <pattern> "
        "(may be specified more\n");
    fprintf(stderr, "                       than once)\n");
#if defined(_HAS_FNM_CASEFOLD)
    fprintf(stderr, "  -X                   same as -x, but ignore case\n");
#endif
    fprintf(stderr, "\n");
    fprintf(stderr, "Directory handling:\n");
    fprintf(stderr, "  -z                   pack empty directories too "
        "(default: pack files only)\n");
    fprintf(stderr, "  -zz                  treat un-readable directories as "
        "empty\n");
    fprintf(stderr, "  -zzz                 pack all directories (as empty)\n");
    fprintf(stderr, "  -Z                   pack un-readable directories in "
        "separate partitions\n");
    fprintf(stderr, "  -d                   pack directories instead of files "
        "after a certain <depth>\n");
    fprintf(stderr, "  -D, --leaf-dirs      pack leaf directories (i.e. "
        "containing files only,\n");
    fprintf(stderr, "                       implies -z)\n");
    fprintf(stderr, "  -E, --dirs-only      pack directories instead of files "
        "(implies -D)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Live mode:\n");
    fprintf(stderr, "  -L, --live           live mode: generate partitions "
        "during filesystem crawling\n");
    fprintf(stderr, "  -S                   do not pack files bigger than "
        "specified maximum partition\n");
    fprintf(stderr, "                       size but print them to stdout "
        "instead (needs -L and -s)\n");
    fprintf(stderr, "  -w, --pre-part-cmd   pre-partition hook: execute <cmd> "
        "at partition start\n");
    fprintf(stderr, "  -W, --post-part-cmd  post-partition hook: execute <cmd> "
        "at partition end\n");
    fprintf(stderr, "  -R, --post-run-cmd   post-run hook: execute <cmd> "
        "before exiting\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Size handling:\n");
    fprintf(stderr, "  -p                   preload each partition with <num> "
        "bytes\n");
    fprintf(stderr, "  -q                   overload each file with <num> "
        "bytes\n");
    fprintf(stderr, "  -r                   round each file size up to next "
        "<num> bytes multiple\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example: fpart -n 3 -o var-parts /var\n");
    fprintf(stderr, "\n");
#if !defined(HAVE_GETOPT_LONG)
    fprintf(stderr, "Note: Long options are not supported on this platform.\n");
#endif
    fprintf(stderr, "Please report bugs to Ganael LAPLANCHE "
        "<ganael.laplanche@martymac.org>\n");
    fprintf(stderr, "WWW: https://www.fpart.org, "
        "https://contribs.martymac.org\n");
    return;
}

/* Handle one argument (either a path to crawl or an arbitrary
   value) and update file entries (head)
   - returns != 0 if a critical error occurred
   - returns with head set to the last element added
   - updates main_status.total_size and main_status.total_num_files with
     the number of elements added */
static int
handle_argument(char *argument, struct file_entry **head,
    struct program_options *options, struct program_status *status)
{
    assert(argument != NULL);
    assert(head != NULL);
    assert(options != NULL);
    assert(status != NULL);

    if(options->arbitrary_values == OPT_ARBITRARYVALUES) {
    /* handle arbitrary values */
        fsize_t input_size = 0;
        char *input_path = NULL;
        if_not_malloc(input_path, strlen(argument) + 1,
            return (1);
        )

        if(sscanf(argument, "%ju %[^\n]", &input_size, input_path) == 2) {
            if(handle_file_entry(head, input_path, input_size,
                   0, options, status) /* entry_errno irrelevant here */ < 0) {
                fprintf(stderr, "%s(): cannot add file entry\n", __func__);
                free(input_path);
                return (1);
            }
        }
        else
            fprintf(stderr, "error parsing input values: %s\n", argument);

        /* cleanup */
        free(input_path);
    }
    else {
    /* handle paths, must examine filesystem */
        char *input_path = NULL;
        size_t input_path_len = strlen(argument);
        size_t malloc_size = input_path_len + 1;
        if_not_malloc(input_path, malloc_size,
            return (1);
        )
        snprintf(input_path, malloc_size, "%s", argument);

        /* remove multiple ending slashes */
        cleanslash_path(input_path);

        /* crawl path */
        if(input_path[0] != '\0') {
#if defined(DEBUG)
            fprintf(stderr, "init_file_entries(): examining %s\n",
                input_path);
#endif
            if(init_file_entries(input_path, head, options, status) != 0) {
                fprintf(stderr, "%s(): cannot initialize file entries\n",
                    __func__);
                free(input_path);
                return (1);
            }
        }

        /* cleanup */
        free(input_path);
    }

    return (0);
}

/* Handle options parsing
   - initializes options structure using argc and argv (through pointers)
   - returns a value defined by the mask below */
static int
handle_options(struct program_options *options, int *argcp, char ***argvp)
{
    /* Return code mask */
#define FPART_OPTS_OK       0 /* OK */
#define FPART_OPTS_NOK      1 /* Error */
#define FPART_OPTS_EXIT     2 /* exit(3) required */
#define FPART_OPTS_USAGE    4 /* usage() call required */
#define FPART_OPTS_VERSION  8 /* version() call required */

    assert(options != NULL);
    assert(argcp != NULL);
    assert(*argcp > 0);
    assert(argvp != NULL);
    assert(*argvp != NULL);

    /* Options handling */
    extern char *optarg;
    extern int optind;
    int ch;
    while((ch = GETOPT(*argcp, *argvp, OPTIONS
#if defined(HAVE_GETOPT_LONG)
        , long_options, NULL
#endif
        )) != -1) {
        switch(ch) {
            case 'h':
                return (FPART_OPTS_USAGE | FPART_OPTS_OK | FPART_OPTS_EXIT);
            case 'V':
                return (FPART_OPTS_VERSION | FPART_OPTS_OK | FPART_OPTS_EXIT);
            case 'n':
            {
                uintmax_t num_parts = str_to_uintmax(optarg, 0);
                if(num_parts == 0) {
                    fprintf(stderr,
                        "Option -n requires a value greater than 0.\n");
                    return (FPART_OPTS_USAGE |
                        FPART_OPTS_NOK | FPART_OPTS_EXIT);
                }
                options->num_parts = (pnum_t)num_parts;
                break;
            }
            case 'f':
            {
                uintmax_t max_entries = str_to_uintmax(optarg, 0);
                if(max_entries == 0) {
                    fprintf(stderr,
                        "Option -f requires a value greater than 0.\n");
                    return (FPART_OPTS_USAGE |
                        FPART_OPTS_NOK | FPART_OPTS_EXIT);
                }
                options->max_entries = (fnum_t)max_entries;
                break;
            }
            case 's':
            {
                uintmax_t max_size = str_to_uintmax(optarg, 1);
                if(max_size == 0) {
                    fprintf(stderr,
                        "Option -s requires a value greater than 0.\n");
                    return (FPART_OPTS_USAGE |
                        FPART_OPTS_NOK | FPART_OPTS_EXIT);
                }
                options->max_size = (fsize_t)max_size;
                break;
            }
            case 'i':
            {
                /* check for empty argument */
                if(strlen(optarg) == 0)
                    break;
                /* replace previous filename if '-i' specified multiple times */
                if(options->in_filename != NULL)
                    free(options->in_filename);
                options->in_filename = abs_path(optarg);
                if(options->in_filename == NULL) {
                    fprintf(stderr, "%s(): cannot determine absolute path for "
                        "file '%s'\n", __func__, optarg);
                    return (FPART_OPTS_NOK | FPART_OPTS_EXIT);
                }
                break;
            }
            case 'a':
                options->arbitrary_values = OPT_ARBITRARYVALUES;
                break;
            case 'o':
            {
                /* check for empty argument */
                if(strlen(optarg) == 0)
                    break;
                /* replace previous filename if '-o' specified multiple times */
                if(options->out_filename != NULL)
                    free(options->out_filename);
                /* '-' goes to stdout */
                if((optarg[0] == '-') && (optarg[1] == '\0')) {
                    options->out_filename = NULL;
                } else {
                    options->out_filename = abs_path(optarg);
                    if(options->out_filename == NULL) {
                        fprintf(stderr, "%s(): cannot determine absolute path "
                            "for file '%s'\n", __func__, optarg);
                        return (FPART_OPTS_NOK | FPART_OPTS_EXIT);
                    }
                }
                break;
            }
            case '0':
                options->out_zero = OPT_OUT0;
                break;
            case 'e':
                options->add_slash = OPT_ADDSLASH;
                break;
            case 'P':
                options->add_parents = OPT_ADDPARENTS;
                break;
            case 'v':
                options->verbose++;
                break;
            case 'l':
                options->follow_symbolic_links = OPT_FOLLOWSYMLINKS;
                break;
            case 'b':
                options->cross_fs_boundaries = OPT_NOCROSSFSBOUNDARIES;
                break;
            case 'y':
            case 'Y':   /* needs _HAS_FNM_CASEFOLD */
            case 'x':
            case 'X':   /* needs _HAS_FNM_CASEFOLD */
            {
                char ***dst_list = NULL;
                unsigned int *dst_num = NULL;
                switch(ch) {
                    case 'y':
                        dst_list = &(options->include_files);
                        dst_num = &(options->ninclude_files);
                    break;
                    case 'Y':
                        dst_list = &(options->include_files_ci);
                        dst_num = &(options->ninclude_files_ci);
                    break;
                    case 'x':
                        dst_list = &(options->exclude_files);
                        dst_num = &(options->nexclude_files);
                    break;
                    case 'X':
                        dst_list = &(options->exclude_files_ci);
                        dst_num = &(options->nexclude_files_ci);
                    break;
                }
                /* check for empty argument */
                if(strlen(optarg) == 0)
                    break;
                /* push string */
                if(str_push(dst_list, dst_num, optarg) != 0)
                    return (FPART_OPTS_NOK | FPART_OPTS_EXIT);
                break;
            }
            case 'z':
                options->dirs_include++;
                break;
            case 'Z':
                options->dnr_split = OPT_DNRSPLIT;
                break;
            case 'd':
            {
                char *endptr = NULL;
                long dir_depth = strtol(optarg, &endptr, 10);
                /* refuse values < 0 (-1 being used to disable this option) */
                if((endptr == optarg) || (*endptr != '\0') || (dir_depth < 0))
                    return (FPART_OPTS_USAGE |
                        FPART_OPTS_NOK | FPART_OPTS_EXIT);
                options->dir_depth = (int)dir_depth;
                break;
            }
            case 'D':
                options->leaf_dirs = OPT_LEAFDIRS;
                break;
            case 'E':
                options->dirs_only = OPT_DIRSONLY;
                options->leaf_dirs = OPT_LEAFDIRS;
                break;
            case 'L':
                options->live_mode = OPT_LIVEMODE;
                break;
            case 'S':
                options->skip_big = OPT_SKIPBIG;
                break;
            case 'w':
            {
                /* check for empty argument */
                size_t malloc_size = strlen(optarg) + 1;
                if(malloc_size <= 1)
                    break;
                /* replace previous hook if '-w' specified multiple times */
                if(options->pre_part_hook != NULL)
                    free(options->pre_part_hook);
                if_not_malloc(options->pre_part_hook, malloc_size,
                    return (FPART_OPTS_NOK | FPART_OPTS_EXIT);
                )
                snprintf(options->pre_part_hook, malloc_size, "%s", optarg);
                break;
            }
            case 'W':
            {
                /* check for empty argument */
                size_t malloc_size = strlen(optarg) + 1;
                if(malloc_size <= 1)
                    break;
                /* replace previous hook if '-W' specified multiple times */
                if(options->post_part_hook != NULL)
                    free(options->post_part_hook);
                if_not_malloc(options->post_part_hook, malloc_size,
                    return (FPART_OPTS_NOK | FPART_OPTS_EXIT);
                )
                snprintf(options->post_part_hook, malloc_size, "%s", optarg);
                break;
            }
            case 'R':
            {
                /* check for empty argument */
                size_t malloc_size = strlen(optarg) + 1;
                if(malloc_size <= 1)
                    break;
                /* replace previous hook if '-R' specified multiple times */
                if(options->post_run_hook != NULL)
                    free(options->post_run_hook);
                if_not_malloc(options->post_run_hook, malloc_size,
                    return (FPART_OPTS_NOK | FPART_OPTS_EXIT);
                )
                snprintf(options->post_run_hook, malloc_size, "%s", optarg);
                break;
            }
            case 'p':
            {
                uintmax_t preload_size = str_to_uintmax(optarg, 1);
                if(preload_size == 0) {
                    fprintf(stderr,
                        "Option -p requires a value greater than 0.\n");
                    return (FPART_OPTS_USAGE |
                        FPART_OPTS_NOK | FPART_OPTS_EXIT);
                }
                options->preload_size = (fsize_t)preload_size;
                break;
            }
            case 'q':
            {
                uintmax_t overload_size = str_to_uintmax(optarg, 1);
                if(overload_size == 0) {
                    fprintf(stderr,
                        "Option -q requires a value greater than 0.\n");
                    return (FPART_OPTS_USAGE |
                        FPART_OPTS_NOK | FPART_OPTS_EXIT);
                }
                options->overload_size = (fsize_t)overload_size;
                break;
            }
            case 'r':
            {
                uintmax_t round_size = str_to_uintmax(optarg, 1);
                if(round_size <= 1) {
                    fprintf(stderr,
                        "Option -r requires a value greater than 1.\n");
                    return (FPART_OPTS_USAGE |
                        FPART_OPTS_NOK | FPART_OPTS_EXIT);
                }
                options->round_size = (fsize_t)round_size;
                break;
            }
            case '?':
            default:
                return (FPART_OPTS_USAGE | FPART_OPTS_NOK | FPART_OPTS_EXIT);
        }
    }
    *argcp -= optind;
    *argvp += optind;

    /* check for options consistency */
    if((options->num_parts == DFLT_OPT_NUM_PARTS) &&
        (options->max_entries == DFLT_OPT_MAX_ENTRIES) &&
        (options->max_size == DFLT_OPT_MAX_SIZE)) {
        fprintf(stderr, "Please specify either -n, -f or -s.\n");
        return (FPART_OPTS_USAGE | FPART_OPTS_NOK | FPART_OPTS_EXIT);
    }

    if((options->num_parts != DFLT_OPT_NUM_PARTS) &&
        ((options->max_entries != DFLT_OPT_MAX_ENTRIES) ||
                    (options->max_size != DFLT_OPT_MAX_SIZE) ||
                    (options->live_mode != DFLT_OPT_LIVEMODE))) {
        fprintf(stderr,
            "Option -n is incompatible with options -f, -s and -L.\n");
        return (FPART_OPTS_USAGE | FPART_OPTS_NOK | FPART_OPTS_EXIT);
    }

    if(options->arbitrary_values == OPT_ARBITRARYVALUES) {
        if((options->add_slash != DFLT_OPT_ADDSLASH) ||
            (options->follow_symbolic_links != DFLT_OPT_FOLLOWSYMLINKS) ||
            (options->cross_fs_boundaries != DFLT_OPT_CROSSFSBOUNDARIES) ||
            (options->include_files != NULL) ||
            (options->include_files_ci != NULL) ||
            (options->exclude_files != NULL) ||
            (options->exclude_files_ci != NULL) ||
            (options->dirs_include != DFLT_OPT_DIRSINCLUDE) ||
            (options->dir_depth != DFLT_OPT_DIR_DEPTH) ||
            (options->leaf_dirs != DFLT_OPT_LEAFDIRS) ||
            (options->dirs_only != DFLT_OPT_DIRSONLY)) {
            fprintf(stderr,
                "Option -a is incompatible with crawling-related options.\n");
            return (FPART_OPTS_USAGE | FPART_OPTS_NOK | FPART_OPTS_EXIT);
        }
    }

    if((options->out_zero == OPT_OUT0) &&
        options->out_filename == NULL) {
        fprintf(stderr,
            "Option -0 is valid only when used with option -o.\n");
        return (FPART_OPTS_USAGE | FPART_OPTS_NOK | FPART_OPTS_EXIT);
    }

    /* option -P (needs '-L') */
    if((options->add_parents == OPT_ADDPARENTS) &&
        (options->live_mode == OPT_NOLIVEMODE)) {
        fprintf(stderr,
            "Option -P can only be used with options -L.\n");
        return (FPART_OPTS_USAGE | FPART_OPTS_NOK | FPART_OPTS_EXIT);
    }

    /* We do not want to mix -E and -d as directory sizes are computed
       differently for those options: -E produces a single-depth total while -d
       computes a recursive total */
    if((options->dirs_only == OPT_DIRSONLY) &&
        (options->dir_depth != DFLT_OPT_DIR_DEPTH)) {
        fprintf(stderr,
            "Option -E is incompatible with option -d.\n");
        return (FPART_OPTS_USAGE | FPART_OPTS_NOK | FPART_OPTS_EXIT);
    }

    /* Options -D and -E imply empty dirs request (option -z) */
    if(options->leaf_dirs == OPT_LEAFDIRS)
        options->dirs_include = max(options->dirs_include, OPT_EMPTYDIRS);

    if((options->live_mode == OPT_NOLIVEMODE) &&
        ((options->pre_part_hook != NULL) ||
        (options->post_part_hook != NULL) ||
        (options->post_run_hook != NULL))) {
        fprintf(stderr,
            "Hooks can only be used with option -L.\n");
        return (FPART_OPTS_USAGE | FPART_OPTS_NOK | FPART_OPTS_EXIT);
    }

    /* Option -Z requires -L and -zz (or -zzz) */
    if((options->dnr_split == OPT_DNRSPLIT) &&
        ((options->live_mode == OPT_NOLIVEMODE) ||
         (options->dirs_include < OPT_DNREMPTY))) {
        fprintf(stderr,
            "Option -Z requires options -L and -zz (or -zzz).\n");
        return (FPART_OPTS_USAGE | FPART_OPTS_NOK | FPART_OPTS_EXIT);
    }

    /* option -S (needs '-L' and '-s') */
    if((options->skip_big == OPT_SKIPBIG) &&
        ((options->live_mode == OPT_NOLIVEMODE) ||
        (options->max_size == DFLT_OPT_MAX_SIZE))) {
        fprintf(stderr,
            "Option -S can only be used with options -L and -s.\n");
        return (FPART_OPTS_USAGE | FPART_OPTS_NOK | FPART_OPTS_EXIT);
    }

    if((options->in_filename == NULL) && (*argcp <= 0)) {
        /* no file specified, force stdin */
        char *opt_input = "-";
        size_t malloc_size = strlen(opt_input) + 1;
        if_not_malloc(options->in_filename, malloc_size,
            return (FPART_OPTS_NOK | FPART_OPTS_EXIT);
        )
        snprintf(options->in_filename, malloc_size, "%s", opt_input);
    }

    return (FPART_OPTS_OK);
}

int main(int argc, char **argv)
{
    /* Status */
    struct program_status main_status = {
        0,    /* total partitions size created so far */
        0,    /* total number of files added so far */
        0     /* total number of partitions created so far */
    };

/******************
  Handle options
 ******************/

    /* Program options */
    struct program_options options;

    /* Set default options */
    init_options(&options);

    /* Parse and initialize options */
    int options_init_res = handle_options(&options, &argc, &argv);

    if(options_init_res & FPART_OPTS_USAGE)
        usage();
    if(options_init_res & FPART_OPTS_VERSION)
        version();
    if(options_init_res & FPART_OPTS_EXIT) {
        uninit_options(&options);
        exit(options_init_res & FPART_OPTS_NOK ?
            EXIT_FAILURE : EXIT_SUCCESS);
    }

/**************
  Handle stdin
***************/

    /* our main double-linked file list */
    struct file_entry *head = NULL;

    if(options.verbose >= OPT_VERBOSE)
        fprintf(stderr, "Examining filesystem...\n");

    /* work on each file provided through input file (or stdin) */
    if(options.in_filename != NULL) {
        /* handle fd opening */
        FILE *in_fp = NULL;
        if((options.in_filename[0] == '-') &&
            (options.in_filename[1] == '\0')) {
            /* working from stdin */
            in_fp = stdin;
        }
        else {
            /* working from a filename */
            if((in_fp = fopen(options.in_filename, "r")) == NULL) {
                fprintf(stderr, "%s: %s\n", options.in_filename,
                    strerror(errno));
                uninit_options(&options);
                exit(EXIT_FAILURE);
            }
        }

        /* read fd and do the work */
        char line[MAX_LINE_LENGTH];
        char *line_end_p = NULL;
        bzero(line, MAX_LINE_LENGTH);
        while(fgets(line, MAX_LINE_LENGTH, in_fp) != NULL) {
            /* replace '\n' with '\0' */
            if((line_end_p = strchr(line, '\n')) != NULL)
                *line_end_p = '\0';

            if(handle_argument(line, &head, &options, &main_status) != 0) {
                uninit_file_entries(head, &options, &main_status);
                uninit_options(&options);
                exit(EXIT_FAILURE);
            }

            /* cleanup */
            bzero(line, MAX_LINE_LENGTH);
        }

        /* check for error reading input */
        if(ferror(in_fp) != 0) {
            fprintf(stderr, "error reading from input stream\n"); 
        }

        /* cleanup */
        fclose(in_fp);
    }

/******************
  Handle arguments
*******************/

    /* now, work on each path provided as arguments */
    int i;
    for(i = 0 ; i < argc ; i++) {
        if(handle_argument(argv[i], &head, &options, &main_status) != 0) {
            uninit_file_entries(head, &options, &main_status);
            uninit_options(&options);
            exit(EXIT_FAILURE);
        }
    }

    /* come back to the first element */
    rewind_list(head);

/****************
  Display status
*****************/

    /* no file found or live mode */
    if((main_status.total_num_files == 0) || (options.live_mode == OPT_LIVEMODE)) {
        uninit_file_entries(head, &options, &main_status);
        /* display final summary */
        if(options.verbose >= OPT_VERBOSE)
            display_final_summary(main_status.total_num_parts,
            main_status.total_size, main_status.total_num_files);

        uninit_options(&options);
        exit(EXIT_SUCCESS);
    }

/************************************************
  Sort entries with a fixed number of partitions
*************************************************/

    if(options.verbose >= OPT_VERBOSE) {
        fprintf(stderr, "Sorting entries...\n");
    }

    /* our list of partitions */
    struct partition *part_head = NULL;

    /* sort files with a fixed size of partitions */
    if(options.num_parts != DFLT_OPT_NUM_PARTS) {
        /* create a fixed-size array of pointers to sort */
        struct file_entry **file_entry_p = NULL;

        if_not_malloc(file_entry_p, sizeof(struct file_entry *) * main_status.total_num_files,
            uninit_file_entries(head, &options, &main_status);
            uninit_options(&options);
            exit(EXIT_FAILURE);
        )

        /* initialize array */
        init_file_entry_p(file_entry_p, main_status.total_num_files, head);
    
        /* sort array */
        qsort(&file_entry_p[0], main_status.total_num_files, sizeof(struct file_entry *),
            &sort_file_entry_p);
    
        /* create a double_linked list of partitions
           which will hold dispatched files */
        if(add_partitions(&part_head, options.num_parts, &options, &main_status) != 0) {
            fprintf(stderr, "%s(): cannot init list of partitions\n",
                __func__);
            uninit_partitions(part_head);
            free(file_entry_p);
            uninit_file_entries(head, &options, &main_status);
            uninit_options(&options);
            exit(EXIT_FAILURE);
        }
        /* come back to the first element */
        rewind_list(part_head);
    
        /* dispatch files */
        if(dispatch_file_entry_p_by_size
            (file_entry_p, main_status.total_num_files, part_head, options.num_parts) != 0) {
            fprintf(stderr, "%s(): unable to dispatch file entries\n",
                __func__);
            uninit_partitions(part_head);
            free(file_entry_p);
            uninit_file_entries(head, &options, &main_status);
            uninit_options(&options);
            exit(EXIT_FAILURE);
        }
    
        /* re-dispatch empty files */
        if(dispatch_empty_file_entries
            (head, main_status.total_num_files, part_head, options.num_parts) != 0) {
            fprintf(stderr, "%s(): unable to dispatch empty file entries\n",
                __func__);
            uninit_partitions(part_head);
            free(file_entry_p);
            uninit_file_entries(head, &options, &main_status);
            uninit_options(&options);
            exit(EXIT_FAILURE);
        }

        /* cleanup */
        free(file_entry_p);
    }

/***************************************************
  Sort entries with a variable number of partitions
****************************************************/

    /* sort files with a file number or size limit per-partitions.
       In this case, partitions are dynamically-created */
    else {
        if(dispatch_file_entries_by_limits
            (head, &part_head, options.max_entries, options.max_size,
            &options, &main_status) == 0) {
            fprintf(stderr, "%s(): unable to dispatch file entries\n",
                __func__);
            uninit_partitions(part_head);
            uninit_file_entries(head, &options, &main_status);
            uninit_options(&options);
            exit(EXIT_FAILURE);
        }
        /* come back to the first element
           (we may have exited with part_head set to partition 1, 
           after default partition) */
        rewind_list(part_head);
    }

/***********************
  Print result and exit
************************/

    /* print partitions summary */
    print_partitions(part_head, &options);

    /* display final summary */
    if(options.verbose >= OPT_VERBOSE) {
        display_final_summary(main_status.total_num_parts,
            main_status.total_size, main_status.total_num_files);
    }

    if(options.verbose >= OPT_VERBOSE)
        fprintf(stderr, "Writing output lists...\n");

    /* print file entries */
    print_file_entries(head, part_head, main_status.total_num_parts, &options);

    if(options.verbose >= OPT_VERBOSE)
        fprintf(stderr, "Cleaning up...\n");

    /* free stuff */
    uninit_partitions(part_head);
    uninit_file_entries(head, &options, &main_status);
    uninit_options(&options);
    exit(EXIT_SUCCESS);
}
