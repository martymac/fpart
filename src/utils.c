/*-
 * Copyright (c) 2011-2015 Ganael LAPLANCHE <ganael.laplanche@martymac.org>
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

/* opendir(3) */
#include <dirent.h>

/* fnmatch(3) */
#include <fnmatch.h>

/****************
 Helper functions
 ****************/

/* Return the number of digits necessary to print i */
unsigned int
get_num_digits(double i)
{
    if((int)i == 0)
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
                fprintf(stderr, "%s: filesystem loop detected\n", p->fts_path);
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
abs_path(const char *path)
{
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
        if_not_malloc(cwd, MAXPATHLEN,
            return (NULL);
        )
        if(getcwd(cwd, MAXPATHLEN) == NULL) {
            free(cwd);
            return (NULL);
        }
        malloc_size += strlen(cwd) + 1; /* cwd + '/' */
    }
    malloc_size += strlen(path) + 1; /* path + '\0' */

    if_not_malloc(abs, malloc_size,
        /* just print error message (within macro code) */
    )
    else {
        if(cwd != NULL)
            snprintf(abs, malloc_size, "%s/%s", cwd, path);
        else
            snprintf(abs, malloc_size, "%s", path);
    }

    if(cwd != NULL)
        free(cwd);

    return (abs);
}

/* Push str into array and update num
   - allocate memory for array if NULL
   - return 0 (success) or 1 (failure) */
int
str_push(char ***array, unsigned int *num, const char * const str)
{
    assert(array != NULL);
    assert(num != NULL);
    assert(str != NULL);
    assert(((*array == NULL) && (*num == 0)) ||
        ((*num > 0) && (*array != NULL)));

    /* allocate new string */
    char *tmp_str = NULL;
    size_t malloc_size = strlen(str) + 1;
    if_not_malloc(tmp_str, malloc_size,
        return (1);
    )
    snprintf(tmp_str, malloc_size, "%s", str);

    /* add new char *pointer to array */
    if_not_realloc(*array, sizeof(char *) * ((*num) + 1),
        free(tmp_str);
        return (1);
    )
    (*array)[*num] = tmp_str;
    *num += 1;

    return (0);
}

/* Cleanup str array
   - remove and free() every str from array
   - free() and NULL'ify array
   - update num */
void
str_cleanup(char ***array, unsigned int *num)
{
    assert(num != NULL);
    assert(array != NULL);
    assert(((*array == NULL) && (*num == 0)) ||
        ((*num > 0) && (*array != NULL)));

    while(*num > 0) {
        if((*array)[(*num) - 1] != NULL) {
            free((*array)[(*num) - 1]);
            (*array)[(*num) - 1] = NULL;
            *num -= 1;
        }
    }
    free(*array);
    *array = NULL;

    return;
}

/* Match str against array of str
   - return 0 (no match) or 1 (match) */
int
str_match(const char * const * const array, const unsigned int num,
    const char * const str, const unsigned char ignore_case)
{
    assert(str != NULL);

    if(array == NULL)
        return (0);

    unsigned int i = 0;
    while(i < num) {
        if(fnmatch(array[i], str, ignore_case ? FNM_CASEFOLD : 0) == 0)
            return(1);
        i++;
    }
    return (0);
}

/* Validate a file name regarding program options
   - do not check inclusion lists for directories (we must be able to crawl
     the entire file hierarchy)
   - return 0 if file is not valid, 1 if it is */
int
valid_filename(char *filename, struct program_options *options,
    unsigned char is_leaf)
{
    assert(filename != NULL);
    assert(options != NULL);

    int valid = 1;

#if defined(DEBUG)
    fprintf(stderr, "%s(): checking name validity for %s: %s\n", __func__,
        is_leaf ? "leaf" : "directory", filename);
#endif

    /* check for includes (options -y and -Y), for leaves only */
    if(is_leaf) {
        if((options->include_files != NULL) ||
            (options->include_files_ci != NULL)) {
            /* switch to default exclude, unless file found in lists */
            valid = 0;

            if(str_match((const char * const * const)(options->include_files),
                options->ninclude_files, filename, 0) ||
                str_match((const char * const * const)(options->include_files_ci),
                options->ninclude_files_ci, filename, 1))
                valid = 1;
        }
    }

    /* check for excludes (options -x and -X) */
    if(str_match((const char * const * const)(options->exclude_files),
        options->nexclude_files, filename, 0) ||
        str_match((const char * const * const)(options->exclude_files_ci),
        options->nexclude_files_ci, filename, 1))
        valid = 0;

#if defined(DEBUG)
    fprintf(stderr, "%s(): %s: %s, validity: %s\n", __func__,
        is_leaf ? "leaf" : "directory", filename,
        valid ? "valid" : "invalid");
#endif

    return (valid);
}

/* Create a copy of environ(7) and return its address
   - return a pointer to the copy or NULL if error
   - returned environ must be freed later */
char **
clone_env(void)
{
    unsigned int env_size = 0;
    char **new_env = NULL;

    /* import original environ */
    extern char **environ;

    /* compute environ size */
    while(environ[env_size]) env_size++;
    /* ending NULL */
    env_size++;

    size_t malloc_size = sizeof(char *) * env_size;
    if_not_malloc(new_env, malloc_size,
        /* just print error message (within macro code) */
    )
    else {
        /* copy each pointer, beginning from the ending NULL value */
        while(env_size > 0) {
            new_env[env_size - 1] = environ[env_size - 1];
            env_size--;
        }
    }
    return (new_env);
}

/* Push a str pointer to a cloned environ(7)
   - return enlarged environ through env
   - returned environ must be freed later
   - return 0 (success) or 1 (failure) */
int
push_env(char *str, char ***env)
{
    assert(str != NULL);
    assert(env != NULL);
    assert(*env != NULL);

    unsigned int env_size = 0;
    char **new_env = NULL;

    /* compute environ size */
    while((*env)[env_size]) env_size++;
    /* add our pointer */
    env_size++;
    /* add ending NULL */
    env_size++;

    size_t malloc_size = sizeof(char *) * env_size;
    if_not_malloc(new_env, malloc_size,
        return (1);
    )

    /* copy each pointer, beginning from the ending NULL value */
    new_env[env_size - 1] = NULL;
    new_env[env_size - 2] = str;
    env_size -= 2;
    while(env_size > 0) {
        new_env[env_size - 1] = (*env)[env_size - 1];
        env_size--;
    }

    /* free previous environment and update env */
    free(*env);
    *env = new_env;

    return (0);
}
