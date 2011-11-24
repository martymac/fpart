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

/* fprintf(3) */
#include <stdio.h>

/* opendir(3) */
#include <dirent.h>

/* strerror(3) */
#include <string.h>

/* errno */
#include <errno.h>

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
   - a pointer to an existing stat may be provided
   - parent dir's dev_t may be provided ; it will be used to detect file
     system boundaries */
fsize_t
get_size(const char *file_path, struct stat *file_stat,
    dev_t parent_dir_dev, struct program_options *options)
{
    unsigned char stat_provided = 1;    /* file_stat provided */
    fsize_t file_size = 0;              /* current return value */

    assert(file_path != NULL);

    /* handle stat if none provided */
    if(file_stat == NULL) {
        stat_provided = 0;
        if((file_stat = malloc(sizeof(struct stat))) == NULL) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            return (0);
        }

        /* perform stat */
        if(options->stat_function(file_path, file_stat) < 0) {
            fprintf(stderr, "%s: %s\n", file_path, strerror(errno));
            if(!stat_provided)
                free(file_stat);
            return (0);
        }
    }

    /* check for file system boundary */
    if((parent_dir_dev != NODEV) &&
        (options->cross_fs_boundaries == OPT_NOCROSSFSBOUNDARIES) &&
        (parent_dir_dev != file_stat->st_dev)) {
        /* file system boundary detected, skip */
        if(!stat_provided)
            free(file_stat);
        return (0);
    }

    /* file */
    if(!S_ISDIR(file_stat->st_mode)) {
        if(!stat_provided)
            free(file_stat);
        /* return size of regular files only */
        return (S_ISREG(file_stat->st_mode) ? file_stat->st_size : 0);
    }

    /* update parent_dir_dev for next recusrive call and free() stat */
    parent_dir_dev = file_stat->st_dev;
    if(!stat_provided)
        free(file_stat);

    /* directory */
    struct dirent * dir_entry = NULL;
    DIR * dir_handle = opendir(file_path);
    if(dir_handle == NULL) {
        fprintf(stderr, "%s: %s\n", file_path, strerror(errno));
        return (0);
    }
    while((dir_entry = readdir(dir_handle)) != NULL) {
        /* ignore "." and ".." */
        if(dir_entry->d_name[0] == '.' &&
            (dir_entry->d_name[1] == 0 || (dir_entry->d_name[1] == '.' && dir_entry->d_name[2] == 0)))
            continue;

        /* compute entry's full path */
        char *dir_entry_path;
        size_t file_path_len = strnlen(file_path, FILENAME_MAX);
        if((dir_entry_path = (char *)malloc(file_path_len + 1 + strnlen(dir_entry->d_name, FILENAME_MAX) + 1)) == NULL) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            closedir(dir_handle);
            return (0);
        }
        if((file_path_len > 0) && (file_path[file_path_len - 1] == '/'))
            /* ending slash already present */
            snprintf(dir_entry_path, FILENAME_MAX + 1, "%s%s",
                file_path, dir_entry->d_name);
        else
            snprintf(dir_entry_path, FILENAME_MAX + 1, "%s/%s",
                file_path, dir_entry->d_name);

        /* recurse for each file */
        file_size += get_size(dir_entry_path, NULL, parent_dir_dev, options);
        free(dir_entry_path);
    }
    closedir(dir_handle);

    return (file_size);
}
