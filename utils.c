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

/* log10(3) */
#include <math.h>

/* malloc(3) */
#include <stdlib.h>

/* fprintf(3), snprintf(3) */
#include <stdio.h>

/* fts(3) */
#include <sys/types.h>
#include <sys/stat.h>
#if defined(EMBED_FTS)
#include "fts.h"
#else
#include <fts.h>
#endif

/* strerror(3) */
#include <string.h>

/* errno */
#include <errno.h>

/* getcwd(3) */
#include <unistd.h>

/* MAXPATHLEN */
#include <sys/param.h>

/* strlen(3) */
#include <string.h>

/* assert(3) */
#include <assert.h>

/****************
 Helper functions
 ****************/

/* Return the number of digits necessary to print i */
unsigned int
get_num_digits(double i)
{
    if(i < 0)
        return (0);
    else if(i == 0)
        return (1);

    double logvalue = log10(i);
    return (logvalue >= 0 ? (unsigned int)logvalue + 1 : 0);
}

/* Return the size of a file or directory
   - a pointer to an existing stat must be provided */
fsize_t
get_size(char *file_path, struct stat *file_stat,
    struct program_options *options)
{
    assert(file_path != NULL);
    assert(file_stat != NULL);
    assert(options != NULL);

    fsize_t file_size = 0;  /* current return value */

    /* if file_path is not a directory,
       return st_size for regular files (only) */
    if(!S_ISDIR(file_stat->st_mode)) {
        return (S_ISREG(file_stat->st_mode) ? file_stat->st_size : 0);
    }

    /* directory, use fts */
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
                continue;

            case FTS_F:
                file_size += p->fts_statp->st_size;
                continue;

            /* skip everything else (only count regular files' size) */
            default:
                continue;
        }
    }

    if(errno != 0)
        fprintf(stderr, "%s: fts_read()\n", file_path);

    if(fts_close(ftsp) < 0)
        fprintf(stderr, "%s: fts_close()\n", file_path);

    return (file_size);
}

/* Return absolute path for given path
   - '/xxx' and '-' are considered absolute, e.g.
     will not be prefixed by cwd. Everything else will.
   - returned pointer must be manually freed later */
char *
abs_path(const char *path) {
    assert(path != NULL);

    char *cwd = NULL;       /* current working directory */
    char *abs = NULL;       /* will be returned */
    size_t malloc_size = 0;

    if(path[0] == '\0') {
        errno = ENOENT;
        return (NULL);
    }

    if((path[0] != '/') &&
        ((path[0] != '-') || (path[1] != '\0'))) {
        /* relative path given */
        cwd = malloc(MAXPATHLEN);
        if(cwd == NULL) 
            return (NULL);
        if(getcwd(cwd, MAXPATHLEN) == NULL) {
            free(cwd);
            return (NULL);
        }
        malloc_size += strlen(cwd) + 1; /* cwd + '/' */
    }   
    malloc_size += strlen(path) + 1; /* path + '\0' */

    abs = malloc(malloc_size);
    if(abs != NULL) {
        if(cwd != NULL)
            snprintf(abs, malloc_size, "%s/%s", cwd, path);
        else
            snprintf(abs, malloc_size, "%s", path);
    }

    if(cwd != NULL)
        free(cwd);

    return (abs);
}
