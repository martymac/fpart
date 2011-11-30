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

#include "fpart.h"
#include "types.h"
#include "utils.h"
#include "options.h"
#include "partition.h"
#include "file_entry.h"
#include "dispatch.h"

/* NULL */
#include <stdlib.h>

/* fprintf(3), fopen(3), fclose(3), fgets(3), foef(3) */
#include <stdio.h>

/* getopt(3) */
#include <unistd.h>

/* getopt(3) */
#include <getopt.h>

/* strnlen(3) */
#include <string.h>

/* bzero(3) */
#include <strings.h>

/* errno */
#include <errno.h>

/* Print version */
void
version(void)
{
    fprintf(stderr, "fpart v." FPART_VERSION "\n"
        "Copyright (c) 2011 Ganael LAPLANCHE <ganael.laplanche@martymac.org>\n"
        "WWW: http://contribs.martymac.org\n");
}

/* Print usage */
void
usage(void)
{
    fprintf(stderr, "Usage: fpart [-h] [-V] -n num | -f files | -s size "
        "[-i infile] [-o outfile]\n"
        "             [-d depth] [-e] [-v] [-l] [-x] [file or dir ...]\n");
    fprintf(stderr, "Sort and divide files into partitions.\n");
    fprintf(stderr, "Example: fpart -n 3 -o var-parts /var\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "General:\n");
    fprintf(stderr, "  -h\tthis help\n");
    fprintf(stderr, "  -V\tprint version\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Partition control:\n");
    fprintf(stderr, "  -n\tset number of desired partitions\n");
    fprintf(stderr, "  -f\tlimit files per partition\n");
    fprintf(stderr, "  -s\tlimit partition size\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Input/output control:\n");
    fprintf(stderr, "  -i\tinput file (stdin if '-' is specified)\n");
    fprintf(stderr, "  -o\toutput file template "
        "(stdout if '-' is specified)\n");
    fprintf(stderr, "  -d\tswitch to directory names display "
        "after certain <depth>\n");
    fprintf(stderr, "  -e\tadd ending slash to directory names\n");
    fprintf(stderr, "  -v\tverbose mode\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Behaviour:\n");
    fprintf(stderr, "  -l\tfollow symbolic links (default: do not follow)\n");
    fprintf(stderr, "  -x\tdo not cross file system boundaries "
        "(default: cross)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Please report bugs to Ganael LAPLANCHE "
        "<ganael.laplanche@martymac.org>\n");
    return;
}

/****
 Main
 ****/

int main(int argc, char** argv)
{
    fnum_t totalfiles = 0;

    /* Program options */
    struct program_options options;

    /* Set default options */
    init_options(&options);

    /* Options handling */
    extern char *optarg;
    extern int optind;
    int ch;
    while((ch = getopt(argc, argv, "?hVn:f:s:i:o:d:evlx")) != -1) {
        switch(ch) {
            case '?':
            case 'h':
                usage();
                uninit_options(&options);
                return (0);
                break;
            case 'V':
                version();
                uninit_options(&options);
                return (0);
                break;
            case 'n':
            {
                if((options.max_entries != DFLT_OPT_MAX_ENTRIES) ||
                    (options.max_size != DFLT_OPT_MAX_SIZE)) {
                    fprintf(stderr,
                        "Option -n is incompatible with options -f and -s.\n");
                    usage();
                    uninit_options(&options);
                    return (1);
                }

                char *endptr = NULL;
                long num_parts = strtol(optarg, &endptr, 10);
                /* refuse values <= 0 and partially converted arguments */
                if((endptr == optarg) || (*endptr != '\0') ||
                    (num_parts <= 0)) {
                    usage();
                    uninit_options(&options);
                    return (1);
                }
                options.num_parts = (pnum_t)num_parts;
            }
                break;
            case 'f':
            {
                if(options.num_parts != DFLT_OPT_NUM_PART) {
                    fprintf(stderr,
                        "Option -f is incompatible with option -n.\n");
                    usage();
                    uninit_options(&options);
                    return (1);
                }

                char *endptr = NULL;
                long long max_entries = strtoll(optarg, &endptr, 10);
                /* refuse values <= 0 and partially converted arguments */
                if((endptr == optarg) || (*endptr != '\0') ||
                    (max_entries <= 0)) {
                    usage();
                    uninit_options(&options);
                    return (1);
                }
                options.max_entries = (fnum_t)max_entries;
            }
                break;
            case 's':
            {
                if(options.num_parts != DFLT_OPT_NUM_PART) {
                    fprintf(stderr,
                        "Option -s is incompatible with option -n.\n");
                    usage();
                    uninit_options(&options);
                    return (1);
                }

                char *endptr = NULL;
                long long max_size = strtoll(optarg, &endptr, 10);
                /* refuse values <= 0 and partially converted arguments */
                if((endptr == optarg) || (*endptr != '\0') || (max_size <= 0)) {
                    usage();
                    uninit_options(&options);
                    return (1);
                }
                options.max_size = (fsize_t)max_size;
            }
                break;
            case 'i':
                options.in_filename = malloc(strnlen(optarg, FILENAME_MAX) + 1);
                if(options.in_filename == NULL) {
                    fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
                    uninit_options(&options);
                    return (1);
                }
                snprintf(options.in_filename, FILENAME_MAX + 1, "%s", optarg);
                break;
            case 'o':
                if((optarg[0] == '-') && (optarg[1] == '\0'))
                    /* output goes to stdout */
                    break;

                options.out_filename =
                    malloc(strnlen(optarg, FILENAME_MAX) + 1);
                if(options.out_filename == NULL) {
                    fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
                    uninit_options(&options);
                    return (1);
                }
                snprintf(options.out_filename, FILENAME_MAX + 1, "%s", optarg);
                break;
            case 'd':
            {
                char *endptr = NULL;
                long dir_depth = strtol(optarg, &endptr, 10);
                /* refuse values < 0 (-1 being used to disable this option) */
                if((endptr == optarg) || (*endptr != '\0') || (dir_depth < 0)) {
                    usage();
                    uninit_options(&options);
                    return (1);
                }
                options.dir_depth = (int)dir_depth;
            }
                break;
            case 'e':
                options.add_slash = OPT_ADDSLASH;
                break;
            case 'v':
                options.verbose = OPT_VERBOSE;
                break;
            case 'l':
                options.follow_symbolic_links = OPT_FOLLOWSYMLINKS;
                break;
            case 'x':
                options.cross_fs_boundaries = OPT_NOCROSSFSBOUNDARIES;
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if((options.num_parts == DFLT_OPT_NUM_PART) &&
        (options.max_entries == DFLT_OPT_MAX_ENTRIES) &&
        (options.max_size == DFLT_OPT_MAX_SIZE)) {
        fprintf(stderr, "Please specify either -n, -f or -s.\n");
        usage();
        uninit_options(&options);
        return (1);
    }

    if((options.in_filename == NULL) && (argc <= 0)) {
        /* no file specified, force stdin */
        char *opt_input = "-";
        options.in_filename = malloc(strnlen(opt_input, FILENAME_MAX) + 1);
        if(options.in_filename == NULL) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            uninit_options(&options);
            return (1);
        }
        snprintf(options.in_filename, FILENAME_MAX + 1, "%s", opt_input);
    }

    /* our main double-linked file list */
    struct file_entry *head = NULL;

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
                return (1);
            }
        }

        /* read fd and do the work */
        char line[MAX_LINE_LENGTH];
        bzero(line, MAX_LINE_LENGTH);
        while(fgets(line, MAX_LINE_LENGTH, in_fp) != NULL) {
            char *input_path = NULL;
            size_t input_path_len = strnlen(line, FILENAME_MAX);
            input_path  = malloc(input_path_len + 1);
            if(input_path == NULL) {
                fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
                uninit_file_entries(head);
                uninit_options(&options);
                return (1);
            }
            snprintf(input_path, FILENAME_MAX + 1, "%s", line);
    
            /* remove ending junk */
            while((input_path_len > 0) &&
                ((input_path[input_path_len - 1] == '\n') ||
                ((input_path_len > 1) && (input_path[input_path_len - 1] == '/')  && (input_path[input_path_len - 2] == '/')))) {
                input_path[input_path_len - 1] = '\0';
                input_path_len--;
            }

            /* crawl path */
#if defined(DEBUG) 
            fprintf(stderr, "init_file_entries(): examining %s\n", input_path); 
#endif
            totalfiles += init_file_entries(input_path, &head, &options);

            /* cleanup */
            free(input_path);
            bzero(line, MAX_LINE_LENGTH);
        }

        /* check for error reading input */
        if(ferror(in_fp) != 0) {
            fprintf(stderr, "error reading from input stream\n"); 
        }

        /* cleanup */
        if(in_fp != NULL)
            fclose(in_fp);
    }

    /* now, work on each path provided through CLI */
    unsigned int i;
    for(i = 0 ; i < argc ; i++) {
        /* remove ending slashes, if any */
        size_t argv_len = strnlen(argv[i], FILENAME_MAX);
        while((argv_len > 1) && (argv[i][argv_len - 1] == '/') &&
            (argv[i][argv_len - 2] == '/')) {
            argv[i][argv_len - 1] = '\0';
            argv_len--;
        }
#if defined(DEBUG) 
        fprintf(stderr, "init_file_entries(): examining %s\n", argv[i]); 
#endif
        totalfiles += init_file_entries(argv[i], &head, &options);
    }
    /* come back to the first element and display status */
    rewind_list(head);
    fprintf(stderr, "%lld file(s) found.\n", totalfiles);

    /* no file found */
    if(totalfiles <= 0) {
        uninit_file_entries(head);
        uninit_options(&options);
        return (0);
    }

    /* our list of partitions */
    struct partition *part_head = NULL;
    pnum_t num_parts = options.num_parts;

    /* sort files with a fixed size of partitions */
    if(options.num_parts != DFLT_OPT_NUM_PART) {
        /* create a fixed-size array of pointers to sort */
        struct file_entry *file_entry_p[totalfiles];
        bzero(file_entry_p, sizeof(struct file_entry *) * totalfiles);
    
        /* initialize array */
        init_file_entry_p(file_entry_p, totalfiles, head);
    
        /* sort array */
        qsort(&file_entry_p[0], totalfiles, sizeof(struct file_entry *),
            &sort_file_entry_p);
    
        /* create a double_linked list of partitions
           which will hold dispatched files */
        if(init_partitions(&part_head, options.num_parts) != 0) {
                fprintf(stderr, "%s(): cannot init list of partitions\n",
                    __func__);
                uninit_partitions(part_head);
                uninit_file_entries(head);
                uninit_options(&options);
                return (1);
        }
        /* come back to the first element */
        rewind_list(part_head);
    
        /* dispatch files */
        if(dispatch_file_entry_p_by_size(file_entry_p, totalfiles, part_head, options.num_parts) != 0) {
                fprintf(stderr, "%s(): unable to dispatch file entries\n",
                    __func__);
                uninit_partitions(part_head);
                uninit_file_entries(head);
                uninit_options(&options);
                return (1);
        }
    
        /* re-dispatch empty files */
        if(dispatch_empty_file_entries(head, totalfiles, part_head, options.num_parts) != 0) {
                fprintf(stderr, "%s(): unable to dispatch empty file entries\n",
                    __func__);
                uninit_partitions(part_head);
                uninit_file_entries(head);
                uninit_options(&options);
                return (1);
        }
    }
    /* sort files with a file number or size limit per-partitions.
       In this case, partitions are dynamically-created */
    else {
        if((num_parts = dispatch_file_entries_by_limits(head, &part_head, options.max_entries, options.max_size)) == 0) {
                fprintf(stderr, "%s(): unable to dispatch file entries\n",
                    __func__);
                uninit_partitions(part_head);
                uninit_file_entries(head);
                uninit_options(&options);
                return (1);
        }
        /* come back to the first element
           (we may have exited with part_head set to partition 1, 
           after default partition) */
        rewind_list(part_head);
    }

    /* print result summary */
    print_partitions(part_head);

    /* print file entries */
    print_file_entries(head, options.out_filename, num_parts);

    /* free stuff */
    uninit_partitions(part_head);
    uninit_file_entries(head);
    uninit_options(&options);
    return (0);
}
