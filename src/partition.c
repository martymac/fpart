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

#include "types.h"
#include "utils.h"
#include "options.h"
#include "partition.h"

/* fprintf(3) */
#include <stdio.h>

/* malloc(3) */
#include <stdlib.h>

/* assert(3) */
#include <assert.h>

/*******************************************************
 Double-linked list of partitions manipulation functions
 *******************************************************/

/* Add num_parts empty partitions to a double-linked list of partitions
   from head
   - if head is NULL, creates a new list ; if not, chains a new list to it
   - returns with head set to the last element */
int
add_partitions(struct partition **head, pnum_t num_parts,
    struct program_options *options)
{
    assert(head != NULL);
    assert(num_parts > 0);
    assert(options != NULL);

    struct partition **current = head; /* current partition pointer address */
    struct partition *previous = NULL; /* previous partition pointer */

    pnum_t i = 0;
    while(i < num_parts) {
        /* backup current structure pointer and initialize a new structure */
        previous = *current;
        if_not_malloc(*current, sizeof(struct partition),
            return (1);
        )

        /* set head on first pass */
        if(*head == NULL)
            *head = *current;

        /* initialize partition data */
        (*current)->size = options->preload_size;
        (*current)->num_files = 0;
        (*current)->nextp = NULL;   /* will be set in next pass (see below) */
        (*current)->prevp = previous;

        /* set previous' nextp pointer */
        if(previous != NULL)
            previous->nextp = *current;

        i++;
    }
    return (0);
}

/* Remove a specific partition from the chain */
int
remove_partition(struct partition *part)
{
    assert(part != NULL);

    /* unlink partition */
    if(part->prevp != NULL)
            part->prevp->nextp = part->nextp;
    if(part->nextp != NULL)
            part->nextp->prevp = part->prevp;

    /* free memory */
    free(part);

    return (0);
}

/* Un-initialize a double-linked list of partitions */
void
uninit_partitions(struct partition *head)
{
    /* be sure to start from last partition */
    fastfw_list(head);

    struct partition *current = head;
    struct partition *prev = NULL;

    while(current != NULL) {
        prev = current->prevp;
        free(current);
        current = prev;
    }
    return;
}

/* Crawl partitions and return the least-loaded partition index */
pnum_t
find_smallest_partition_index(struct partition *head)
{
    assert(head != NULL);

    /* be sure to start at first partition */
    rewind_list(head);

    /* start values */
    fsize_t smallest_partition_value = head->size;
    pnum_t smallest_partition_index = 0;

    pnum_t i = 0;
    while(head != NULL) {
        if(head->size < smallest_partition_value) {
            smallest_partition_value = head->size;
            smallest_partition_index = i;
        }
        head = head->nextp;
        i++;
    }
    return (smallest_partition_index);
}

/* Return a pointer to a given partition */
struct partition *
get_partition_at(struct partition *head, pnum_t index)
{
    assert(head != NULL);

    /* be sure to start at first partition */
    rewind_list(head);

    pnum_t i = 0;
    while((head != NULL) && (i < index)) {
        head = head->nextp;
        i++;
    }
    return (head);
}

/* Adapt partition index for output, regarding program options
   - returns an index suitable for user output (display or filename) */
pnum_t
adapt_partition_index(pnum_t index, const struct program_options *options)
{
    assert(options != NULL);

    pnum_t offset = 1;

    /* compute output index offset: fpart always produces partitions starting
       from '1' (but they internally start from '0'). Partition '0' -used for
       large files that do not fit in regular partitions- only appears when
       option '-s' is passed *and* non-live mode is used */
    if((options->max_size != DFLT_OPT_MAX_SIZE) &&
       (options->live_mode == OPT_NOLIVEMODE))
        offset = 0;

    return (index + offset);
}

/* Print partitions from head */
void
print_partitions(struct partition *head, struct program_options *options)
{
    assert(options != NULL);

    pnum_t partition_index = 0;
    while(head != NULL) {
        fprintf(stderr, "Part #%ju: size = %ju, %ju file(s)\n",
            adapt_partition_index(partition_index, options),
            head->size, head->num_files);
        head = head->nextp;
        partition_index++;
    }
    return;
}
