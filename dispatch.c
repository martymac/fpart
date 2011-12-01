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
#include "dispatch.h"

/* NULL */
#include <stdlib.h>

/* fprintf(3) */
#include <stdio.h>

/* assert(3) */
#include <assert.h>

/*****************************
 File entry dispatch functions
 *****************************/

/* Sort an array of file_entry pointers given file size, biggest to smallest
   This function is used by qsort(3) */
int
sort_file_entry_p(const void *a, const void *b)
{
    assert((a != NULL) && (*(struct file_entry **)a != NULL));
    assert((b != NULL) && (*(struct file_entry **)b != NULL));

    if((*(struct file_entry **)a)->size < (*(struct file_entry **)b)->size)
        return (1);
    else if((*(struct file_entry **)a)->size > (*(struct file_entry **)b)->size)
        return (-1);
    else
        return (0);
}

/* Dispatch file_entries by assigning them a partition number
   - a sorted array of file entry pointers must be provided as an argument
   - as well as a pointer to a double linked-list of partitions' head
     that will contain the total amount of data of each assigned file */
int
dispatch_file_entry_p_by_size(struct file_entry **file_entry_p,
    fnum_t num_entries, struct partition *head, pnum_t num_parts)
{
    assert(head != NULL);
    assert(num_parts > 0);

    fnum_t i = 0;
    while((file_entry_p != NULL) && (file_entry_p[i] != NULL) &&
        (i < num_entries)) {
        /* find most approriate partition */
        pnum_t smallest_partition_index = find_smallest_partition_index(head);
        struct partition *smallest_partition =
            get_partition_at(head, smallest_partition_index);
        if(smallest_partition == NULL) {
            fprintf(stderr, "%s(): get_partition_at() returned NULL\n",
                __func__);
            return (1);
        }
        /* assign it */
        file_entry_p[i]->partition_index = smallest_partition_index;
#if defined(DEBUG)
        fprintf(stderr, "%s(): %s added to partition %d (%p)\n", __func__,
            file_entry_p[i]->path, file_entry_p[i]->partition_index,
            smallest_partition);
#endif
        /* and load the partition with file size */
        smallest_partition->size += file_entry_p[i]->size;
        smallest_partition->num_files++;
        i++;
    }
    return (0);
}

/* Dispatch empty file_entries (files with zero-byte size) from head by
   assigning them a more appropriate partition number.
   The idea is to get empty files spread accross partitions and not get them
   all in the last one.
   - a double-linked list of partitions is provided as an argument */
int
dispatch_empty_file_entries(struct file_entry *head, fnum_t num_entries,
    struct partition *part_head, pnum_t num_parts)
{
    assert(head != NULL);
    assert(part_head != NULL);
    assert(num_parts > 0);

    /* backup head */
    struct file_entry *start = head;

    /* first pass: count empty files */
    fnum_t num_empty_entries = 0;
    while(head != NULL) {
        if(head->size == 0)
            num_empty_entries++;
        head = head->nextp;
    }
    /* go back to original head */
    head = start;

    /* compute mean file entry number per partition */
    fnum_t mean_files = (num_entries / num_parts);

    /* be sure to start at first partition as we are handling indexes here.
       Starting at first file_entry is not necessary as we would not corrupt
       any information, but just skip a few file entries */
    rewind_list(part_head);

    /* for each empty file, associate it with the first partition
       having less files than mean_files */
    while(head != NULL) {
        if(head->size == 0) {
            /* empty file found */
            pnum_t j = 0;
            /* backup partition head */
            struct partition *part_start = part_head;

            while(part_head != NULL) {
                if((head->partition_index != j) &&
                   (part_head->num_files < mean_files)) {
                    struct partition *previous_partition =
                        get_partition_at(part_start, head->partition_index);
                    if(previous_partition == NULL) {
                        fprintf(stderr, "%s(): "
                            "get_partition_at() returned NULL\n", __func__);
                        return (1);
                    }
                    /* unload the previous part (only affects the number
                       of files, size does not change) */
                    previous_partition->num_files--;
                    /* load the new part */
                    part_head->num_files++;
                    /* assign new index to file entry */
                    head->partition_index = j;
#if defined(DEBUG)
                    fprintf(stderr, "%s(): %s (empty) re-assigned to partition "
                        "%d (%p)\n", __func__, head->path,
                        head->partition_index, part_head);
#endif
                    break;
                }
                part_head = part_head->nextp;
                j++;
            }
            /* go back to original head */
            part_head = part_start;
        }
        head = head->nextp;
    }
    return (0);
}

/* Dispatch file_entries from head into partitions that will be created
   on-the-fly, with respect to max_entries (maximum files per partitions)
   and max_size (max partition size)
   - must be called with *part_head == NULL (will create partitions)
   - if max_size > 0, partition 0 will hold files that cannot be held by other
     partitions
   - returns the number of parts created with part_head set to the last
     element */
pnum_t
dispatch_file_entries_by_limits(struct file_entry *head,
    struct partition **part_head, fnum_t max_entries, fsize_t max_size)
{
    assert(head != NULL);
    assert((part_head != NULL) && (*part_head == NULL));
    assert(max_size >= 0);

    /* number of partitions created, our return value */
    pnum_t num_parts_created = 0;

    /* when max_size is used, create a default partition (partition 0) 
       that will hold files that does not match criteria */
    if(max_size > 0) {
        if(add_partitions(part_head, 1) != 0) {
            fprintf(stderr, "%s(): cannot init default partition\n", __func__);
            return (num_parts_created);
        }
        num_parts_created++;
    }
    struct partition *default_partition = *part_head;
    pnum_t default_partition_index = 0;

    /* create a first data partition and keep a pointer to it */
    if(add_partitions(part_head, 1) != 0) {
        fprintf(stderr, "%s(): cannot create partition\n", __func__);
        return (num_parts_created);
    }
    num_parts_created++;
    struct partition *start_partition = *part_head;
    pnum_t start_partition_index = num_parts_created - 1;

    /* for each file, associate it with current partition
       (or default_partition) */
    pnum_t current_partition_index = start_partition_index;
    while(head != NULL) {
        /* max_size provided and file size > max_size,
           associate file to default partition */
        if((max_size > 0) && (head->size > max_size)) {
            head->partition_index = default_partition_index;
            default_partition->size += head->size;
            default_partition->num_files++;
#if defined(DEBUG)
            fprintf(stderr, "%s(): %s added to partition %d (%p)\n",
                __func__, head->path, head->partition_index, default_partition);
#endif
        }
        else {
            /* examine each partition */
            while((*part_head) != NULL) {
                /* if file does not fit in partition */
                if(((max_entries > 0) && (((*part_head)->num_files + 1) > max_entries)) ||
                    ((max_size > 0) && (((*part_head)->size + head->size) > max_size))) {
                    /* and we reached last partition, chain a new one */
                    if((*part_head)->nextp == NULL) {
                        if(add_partitions(part_head, 1) != 0) {
                            fprintf(stderr, "%s(): cannot create partition\n",
                                __func__);
                            return (num_parts_created);
                        }
                        num_parts_created++;
#if defined(DEBUG)
                        fprintf(stderr, "%s(): chained one partition (%p)\n",
                            __func__, *part_head);
#endif
                    }
                    else {
                        /* examine next partition */
                        (*part_head) = (*part_head)->nextp;
                    }
                    current_partition_index++;
                }
                else {
                    /* file fits in current partition, add it */
                    head->partition_index = current_partition_index;
                    (*part_head)->size += head->size;
                    (*part_head)->num_files++;
#if defined(DEBUG)
                    fprintf(stderr, "%s(): %s added to partition %d (%p)\n",
                        __func__, head->path, head->partition_index,
                        *part_head);
#endif

                    /* examine next file */
                    break;
                }
            }

            assert(*part_head != NULL);
        }

        /* examine next file */
        head = head->nextp;

        /* come back to the first partition */
        current_partition_index = start_partition_index;
        (*part_head) = start_partition;
    }
    return (num_parts_created);
}
