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

/* strerror(3), strlen(3), strchr(3) */
#include <string.h>

/* errno */
#include <errno.h>

/* getcwd(3) */
#include <unistd.h>

/* MAXPATHLEN */
#include <sys/param.h>

/* assert(3) */
#include <assert.h>

/* opendir(3) */
#include <dirent.h>

/* fnmatch(3) */
#include <fnmatch.h>

/* isblank(3) */
#include <ctype.h>

/* strtoumax(3) */
#include <limits.h>
#include <inttypes.h>

/****************
 Helper functions
 ****************/

/* Convert a char (K, M, G, ...) to a size multiplier */
uintmax_t
char_to_multiplier(const char c)
{
    uintmax_t ret = 0;

    switch(c) {
        case 'k':
        case 'K':
            ret = 1 << 10;
            break;
        case 'm':
        case 'M':
            ret = 1 << 20;
            break;
        case 'g':
        case 'G':
            ret = 1 << 30;
            break;
        case 't':
        case 'T':
            ret = (uintmax_t)1 << 40;
            break;
        case 'p':
        case 'P':
            ret = (uintmax_t)1 << 50;
            break;
    }

    return (ret);
}

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
   - a pointer to an existing stat must be provided

   We assume that when that function is called, then the choice of including or
   excluding the related file or directory has already been made. Thus,
   exclusion list is only honored when computing size of a directory and when
   depth is > 0 (i.e. we always accept the root dir but may skip subdirs).
*/
fsize_t
get_size(char *file_path, struct stat *file_stat,
    struct program_options *options)
{
    assert(file_path != NULL);
    assert(file_stat != NULL);
    assert(options != NULL);

    fsize_t file_size = 0;  /* current return value */

    /* if file_path is not a directory, return st_size for regular files (only).
       We do *not* check for valid_file() here because if the function has been
       called, then the choice of including the file has already been made
       before */
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
        if(options->verbose >= OPT_VVVERBOSE) {
            fprintf(stderr, "%s(%s): fts_info=%d, ftp_errno=%d\n", __func__,
                p->fts_path, p->fts_info, p->fts_errno);
        }
        switch (p->fts_info) {
            case FTS_ERR:   /* misc error */
            case FTS_DNR:   /* un-readable directory */
            case FTS_NS:    /* stat() error */
                fprintf(stderr, "%s: %s\n", p->fts_path,
                    strerror(p->fts_errno));
            case FTS_NSOK: /* no stat(2) available (not requested) */
                continue;

            case FTS_DC:
                fprintf(stderr, "%s: filesystem loop detected\n", p->fts_path);
            case FTS_DOT:  /* ignore "." and ".." */
            case FTS_DP:
                continue;

            case FTS_D:
                /* Excluded directories do not account for returned size.
                   Always accept root dir here because, if the function has been
                   called, then the choice of including the directory has
                   already been made before */
                if((!valid_file(p, options, VF_EXCLUDEONLY)) &&
                    (p->fts_level > 0)) {
                    if(options->verbose >= OPT_VVVERBOSE) {
                        fprintf(stderr, "%s(): skipping directory: %s\n", __func__,
                            p->fts_path);
                    }
                    fts_set(ftsp, p, FTS_SKIP);
                }
                continue;

            default:
                /* XXX default means remaining file types:
                   FTS_F, FTS_SL, FTS_SLNONE, FTS_DEFAULT */

                /* Excluded files do not account for returned size */
                if(!valid_file(p, options, VF_EXCLUDEONLY)) {
                    if(options->verbose >= OPT_VVVERBOSE) {
                        fprintf(stderr, "%s(): skipping file: %s\n", __func__,
                            p->fts_path);
                    }
                }
                else
                    file_size += p->fts_statp->st_size;
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

/* In-place remove multiple ending slashes from path
   leaving a single ending slash, if any
   - path must point to a writeable address */
void
cleanslash_path(char * const path)
{
    assert(path != NULL);

    size_t path_len = strlen(path);

    while((path_len > 1) &&
        (path[path_len - 1] == '/')  &&
        (path[path_len - 2] == '/')) {
        path[path_len - 1] = '\0';
        path_len--;
    }

    return;
}

#if 0
/* Add an ending slash to path
   - returns a new slashed path address
   - returned memory must be freed afterwards
   - NULL is returned when an error occurs */
char *
slash_path(const char * const path)
{
    assert(path != NULL);

    size_t path_len = strlen(path);
    char *slashed_path = NULL;    /* will be returned */

    if_not_malloc(slashed_path, path_len + 1 + 1,
        return (NULL);
    )
    snprintf(slashed_path, path_len + 1 + 1, "%s/", path);

    return (slashed_path);
}
#endif

/* Allocate and return path's parent
   - returned memory must be freed afterwards
   - returned parent ends with zero or a single ending '/'
   - multiple intermediate slashes are *not* removed
   - an empty string is returned when attempting to retrieve a single-level
     relative path's parent
   - NULL is returned when an error occurs

   Examples :
     "/foo/bar///baz///" => "/foo/bar/"
     "/foo///bar///baz///" => "/foo///bar/"
     "foo/bar///baz///" => "foo/bar/"
     "foo///" => ""
     "///foo" => "/"
     "foo" => ""
     "" => ""
*/
char *
parent_path(const char * const path, const unsigned char keep_ending_slash)
{
    assert(path != NULL);

    char *parent = NULL;    /* will be returned */
    char pe_seen = 0;       /* has a path element (any char != '/')
                               been seen ? */

    size_t path_len = strlen(path);

    if_not_malloc(parent, path_len + 1,
        return (NULL);
    )
    snprintf(parent, path_len + 1, "%s", path);

    /* crawl the string from the end and erase last path element */
    while(
        /* absolute paths: allow erasing up to the initial '/' */
        (((parent[0] == '/') && (path_len > 1)) ||
        /* relative paths: allow erasing up to the empty string */
            ((parent[0] != '/') && (path_len >= 1))) &&
        /* only slashes seen yet */
        ((pe_seen == 0) ||
            /* current char belongs to last path element */
            ((pe_seen == 1) &&
                (parent[path_len - 1] != '/')) ||
            /* we erased last path element but next char is also a slash */
            ((pe_seen == 1) &&
                (parent[path_len - 1] == '/') && (parent[path_len - 2] == '/')))
    ) {
        if(parent[path_len - 1] != '/')
            pe_seen = 1;
        parent[path_len - 1] = '\0';
        path_len--;
    }

    /* remove ending slash if requested, always leaving initial '/' intact */
    if((!keep_ending_slash) && (path_len > 1) && (parent[path_len - 1] == '/')) {
        parent[path_len - 1] = '\0';
        path_len--;
    }

    return (parent);
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
    {
        if_not_realloc(*array, sizeof(char *) * ((*num) + 1),
            free(tmp_str);
            return (1);
        )
    }
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

/* Check if a string begins with a '-' sign
   - return 1 if it is the case, else 0 */
int
str_is_negative(const char *str)
{
    assert(str != NULL);

    /* skip blanks to test first character */
    while(isblank(*str))
        str++;

    if(*str == '-')
        return (1);
    else
        return (0);
}

/* Convert a str to a uintmax > 0
   - support human-friendly multipliers
   - only accept values > 0 as input
   - return 0 if an error occurs */
uintmax_t
str_to_uintmax(const char *str, const unsigned char handle_multiplier)
{
    assert(str != NULL);

    char *endptr = NULL;
    uintmax_t val = 0;
    uintmax_t multiplier = 0;

    /* check if a negative value has been provided */
    if(str_is_negative(str))
        return (0);

    errno = 0;
    val = strtoumax(str, &endptr, 10);
    /* check that something was converted and refuse invalid values */
    if((endptr == optarg) || (val == 0))
        return (0);
    /* check for other errors */
    if(errno != 0) {
        fprintf(stderr, "%s(): %s\n", __func__, strerror(errno));
        return (0);
    }
    /* if characters remain, handle multiplier */
    if(*endptr != '\0') {
        /* return an error if we do not want to handle multiplier */
        if(!handle_multiplier) {
            fprintf(stderr, "%s(): %s\n", __func__, "unexpected unit provided");
            return (0);
        }

        uintmax_t orig_val = val;
        /* more than one character remain or invalid multiplier specified */
        if((*(endptr + 1) != '\0') ||
            (multiplier = char_to_multiplier(*endptr)) == 0) {
            fprintf(stderr, "%s(): %s\n", __func__, "unknown unit provided");
            return (0);
        }
        /* check for overflow */
        val *= multiplier;
        if((val / multiplier) != orig_val) {
            fprintf(stderr, "%s(): %s\n", __func__, strerror(ERANGE));
            return (0);
        }
    }
#if defined(DEBUG)
    fprintf(stderr, "%s(): converted string %s to value %ju\n", __func__,
        optarg, val);
#endif
    return (val);
}

/* Match an fts entry against an array of strings
   - return 0 (no match) or 1 (match) */
int
file_match(const char * const * const array, const unsigned int num,
    const FTSENT * const p, const unsigned char ignore_case)
{
    assert(p != NULL);
    assert(p->fts_name != NULL);
    assert(p->fts_path != NULL);

    if(array == NULL)
        return (0);

    unsigned int i = 0;
    while(i < num) {
        if(strchr(array[i], '/') == NULL) {
            /* Current string contains a file name */
            if(fnmatch(array[i], p->fts_name, FNM_PERIOD |
                (ignore_case ? FNM_CASEFOLD : 0)) == 0)
                return (1);
        }
        else {
            /* Current string contains a path */
            if(fnmatch(array[i], p->fts_path, FNM_PATHNAME | FNM_PERIOD |
                (ignore_case ? FNM_CASEFOLD : 0)) == 0)
                return (1);
        }
        i++;
    }
    return (0);
}

/* Validate a file regarding program options
   - exclude_only (ignore include lists) is useful to:
     - be able to crawl the entire file hierarchy (honoring include lists would
       prevent the caller from entering a non-included directory and break
       crawling)
     - compute leaf directory size, when only exclude lists are needed
   - return 0 if file is not valid, 1 if it is */
int
valid_file(const FTSENT * const p, struct program_options *options,
    unsigned char exclude_only)
{
    assert(p != NULL);
    assert(p->fts_name != NULL);
    assert(p->fts_path != NULL);
    assert(options != NULL);

    int valid = 1;

#if defined(DEBUG)
    fprintf(stderr, "%s(): checking name validity (%s includes): %s (path: %s)\n",
        __func__, exclude_only ? "without" : "with",
        (p->fts_namelen > 0) ? p->fts_name : "<empty>", p->fts_path);
#endif

    /* check for includes (options -y and -Y), if requested */
    if(!exclude_only) {
        if((options->include_files != NULL) ||
            (options->include_files_ci != NULL)) {
            /* switch to default exclude, unless file found in lists */
            valid = 0;

            if(file_match((const char * const * const)(options->include_files),
                options->ninclude_files, p, 0) ||
                file_match((const char * const * const)(options->include_files_ci),
                options->ninclude_files_ci, p, 1))
                valid = 1;
        }
    }

    /* check for excludes (options -x and -X) */
    if(file_match((const char * const * const)(options->exclude_files),
        options->nexclude_files, p, 0) ||
        file_match((const char * const * const)(options->exclude_files_ci),
        options->nexclude_files_ci, p, 1))
        valid = 0;

#if defined(DEBUG)
    fprintf(stderr, "%s(): %s, validity: %s\n", __func__,
        (p->fts_namelen > 0) ? p->fts_name : "<empty>",
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

/* Display final summary */
void
display_final_summary(const pnum_t total_num_parts, const fsize_t total_size,
    const fnum_t total_num_files)
{
    fprintf(stderr, "Summary: size = %ju, files = %ju, parts = %ju\n",
        total_size, total_num_files, total_num_parts);

    return;
}
