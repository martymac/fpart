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

/* wait(2) */
#include <sys/wait.h>

/* _PATH_BSHELL */
#include <paths.h>

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

    char * fts_argv[] = { file_path, NULL };
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

#if 0
/* Replace all occurences of pattern by subst in str
   - returns str if no change to str was performed
   - returns NULL if error
   - returned pointer must be manually freed later if != str */
char *
strsubst(const char *str, const char *pattern, const char *subst)
{
    assert(str != NULL);
    assert(pattern != NULL);
    assert(subst != NULL);

    if((strlen(str) == 0) || (strlen(pattern) == 0)) {
        return ((char *)str);
    }

    char *strp = (char *)str;       /* current lookup start pointer */
    char *cursp = (char *)str;      /* current lookup result pointer */

    char *ret_str = (char *)str;    /* returned str */
    char *tmp_str = NULL;           /* temporary str */
    while(((cursp = strstr(strp, pattern)) != NULL) ||
        ((strp != str) && (strp < (str + strlen(str))))) {
        /* very last iteration, nothing matched, but we still have to copy the
           remaining of the string. Simulate a last substitution */
        if(cursp == NULL) {
            cursp = strp;
            subst = strp;
            pattern = strp;
        }

        /* compute size of previous temporary string */
        size_t tmp_strlen = 0;
        if(tmp_str != NULL)
            tmp_strlen = strlen(tmp_str);

        /* compute size of new return string including substitution */
        size_t malloc_size = tmp_strlen + (cursp - strp) + strlen(subst) + 1;

        /* allocate new return string */
        ret_str = malloc(malloc_size);
        if(ret_str == NULL) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            if(tmp_str != NULL)
                free(tmp_str);
            return (NULL);
        }

        /* copy previous tmp_str if necessary */
        if(tmp_str != NULL)
            bcopy(tmp_str, ret_str, tmp_strlen);
        /* add string before pattern */
        bcopy(strp, ret_str + tmp_strlen, (cursp - strp));
        /* add substitution string */
        bcopy(subst, ret_str + tmp_strlen + (cursp - strp), strlen(subst));
        /* force end of string */
        *(ret_str + tmp_strlen + (cursp - strp) + strlen(subst)) = '\0';

        /* skip pattern for next lookup */
        strp = cursp + strlen(pattern);

        /* temporary string becomes return string */
        if(tmp_str != NULL)
            free(tmp_str);
        tmp_str = ret_str;
    }

    return (ret_str);
}
#endif

int
fpart_hook(const char *cmd, const struct program_options *options,
    const char *live_filename, const pnum_t *live_partition_index,
    const fsize_t *live_partition_size, const fnum_t *live_num_files)
{
    assert(cmd != NULL);
    assert(options != NULL);

    int retval = 0;

    /* determine the kind of hook we are in */
    if(cmd == options->pre_part_hook) {
        assert(live_partition_index != NULL);
        assert(live_partition_size != NULL);
        assert(live_num_files != NULL);

        if(options->verbose >= OPT_VERBOSE)
            fprintf(stderr, "Executing pre-part #%d hook: '%s'\n",
                *live_partition_index, options->pre_part_hook);
    }
    else if(cmd == options->post_part_hook) {
        assert(live_partition_index != NULL);
        assert(live_partition_size != NULL);
        assert(live_num_files != NULL);

        if(options->verbose >= OPT_VERBOSE)
            fprintf(stderr, "Executing post-part #%d hook: '%s'\n",
                *live_partition_index, options->post_part_hook);
    }

    /* prepare environment */
    char *env_fpart_partfilename_name = "FPART_PARTFILENAME";
    char *env_fpart_partnumber_name = "FPART_PARTNUMBER";
    char *env_fpart_partsize_name = "FPART_PARTSIZE";
    char *env_fpart_partnumfiles_name = "FPART_PARTNUMFILES";

    char *env_fpart_partfilename_string = NULL;
    char *env_fpart_partnumber_string = NULL;
    char *env_fpart_partsize_string = NULL;
    char *env_fpart_partnumfiles_string = NULL;

    size_t malloc_size = 0;

    /* FPART_PARTFILENAME */
    if(live_filename != NULL)
        malloc_size = strlen(env_fpart_partfilename_name) + 1 +
            strlen(live_filename) + 1;
    else
        malloc_size = 1; /* empty string */
    if((env_fpart_partfilename_string = malloc(malloc_size)) == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        retval = 1;
        goto cleanup;
    }
    if(live_filename != NULL)
        snprintf(env_fpart_partfilename_string, malloc_size, "%s=%s",
            env_fpart_partfilename_name, live_filename);
    else
        snprintf(env_fpart_partfilename_string, malloc_size, "%s", "");

    /* FPART_PARTNUMBER */
    if(live_partition_index != NULL)
        malloc_size = strlen(env_fpart_partnumber_name) + 1 +
            get_num_digits(*live_partition_index) + 1;
    else
        malloc_size = 1; /* empty string */
    if((env_fpart_partnumber_string = malloc(malloc_size)) == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        retval = 1;
        goto cleanup;
    }
    if(live_partition_index != NULL)
        snprintf(env_fpart_partnumber_string, malloc_size, "%s=%d",
            env_fpart_partnumber_name, *live_partition_index);
    else
        snprintf(env_fpart_partnumber_string, malloc_size, "%s", "");

    /* FPART_PARTSIZE */
    if(live_partition_size != NULL)
        malloc_size = strlen(env_fpart_partsize_name) + 1 +
            get_num_digits(*live_partition_size) + 1;
    else
        malloc_size = 1; /* empty string */
    if((env_fpart_partsize_string = malloc(malloc_size)) == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        retval = 1;
        goto cleanup;
    }
    if(live_partition_size != NULL)
        snprintf(env_fpart_partsize_string, malloc_size, "%s=%lld",
            env_fpart_partsize_name, *live_partition_size);
    else
        snprintf(env_fpart_partsize_string, malloc_size, "%s", "");

    /* FPART_PARTNUMFILES */
    if(live_num_files != NULL)
        malloc_size = strlen(env_fpart_partnumfiles_name) + 1 +
            get_num_digits(*live_num_files) + 1;
    else
        malloc_size = 1; /* empty string */
    if((env_fpart_partnumfiles_string = malloc(malloc_size)) == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        retval = 1;
        goto cleanup;
    }
    if(live_num_files != NULL)
        snprintf(env_fpart_partnumfiles_string, malloc_size, "%s=%llu",
            env_fpart_partnumfiles_name, *live_num_files);
    else
        snprintf(env_fpart_partnumfiles_string, malloc_size, "%s", "");

    char *envp[] = {
        env_fpart_partfilename_string,
        env_fpart_partnumber_string,
        env_fpart_partsize_string,
        env_fpart_partnumfiles_string,
        NULL };

    /* fork child process */
    pid_t pid;
    int child_status;
    switch(pid = fork()) {
        case -1:            /* error */
            fprintf(stderr, "fork(): %s\n", strerror(errno));
            retval = 1;
            goto cleanup;
            break;
        case 0:             /* child */
        {
            execle(_PATH_BSHELL, "sh", "-c", cmd, (char *)NULL, envp);
            /* if reached, error */
            exit (1);
        }
            break;
        default:            /* parent */
        {
            pid_t wpid;
            do {
                wpid = wait(&child_status);
            } while(wpid != pid);
            retval = 0;
        }
            break;
    }

cleanup:
    free(env_fpart_partfilename_string);
    free(env_fpart_partnumber_string);
    free(env_fpart_partsize_string);
    free(env_fpart_partnumfiles_string);
    return (retval);
}
