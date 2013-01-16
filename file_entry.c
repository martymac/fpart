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
#include "file_entry.h"

/* stat(2) */
#include <sys/types.h>
#include <sys/stat.h>

/* fprintf(3) */
#include <stdio.h>

/* strerror(3), strlen(3) */
#include <string.h>

/* errno */
#include <errno.h>

/* malloc(3) */
#include <stdlib.h>

/* fts(3) */
#include <sys/types.h>
#include <sys/stat.h>
#if defined(EMBED_FTS)
#include "fts.h"
#else
#include <fts.h>
#endif

/* open(2) */
#include <fcntl.h>

/* close(2) */
#include <unistd.h>

/* assert(3) */
#include <assert.h>

/* wait(2) */
#include <sys/wait.h>

/* _PATH_BSHELL */
#if defined(__sun) || defined(__sun__)
#define _PATH_BSHELL    "/bin/sh"
#else
#include <paths.h>
#endif

/* signal(3) */
#include <signal.h>

/****************************
 Live-mode related functions 
 ****************************/

/* Status */
static struct {
    int fd;                      /* current file descriptor
                                    (if option '-o' used) */
    char *filename;              /* current file name */
    pnum_t partition_index;      /* current partition number */
    fsize_t partition_size;      /* current partition size */
    fnum_t partition_num_files;  /* number of files in current partition */
    int exit_summary;            /* 0 if every single hook exit()ed with 0,
                                    else 1 */
    pid_t child_pid;
} live_status = {
    STDOUT_FILENO,
    NULL,
    0,
    0,
    0,
    0,
    -1
};

/* Signal handler, kills child and exit() */
void
kill_child(int sig)
{
#if defined(DEBUG)
    fprintf(stderr, "%s(): killing child process %d\n", __func__,
        live_status.child_pid);
#endif
    if(live_status.child_pid > 1) {
        killpg(live_status.child_pid, sig ? sig : SIGTERM);
        waitpid(live_status.child_pid, NULL, 0);
    }
    exit(1);
}

/* Executes 'cmd' and waits for it to terminate
   - returns 0 if cmd has been executed and its return code was 0,
     else returns 1 */
int
fpart_hook(const char *cmd, const struct program_options *options,
    const char *live_filename, const pnum_t *live_partition_index,
    const fsize_t *live_partition_size, const fnum_t *live_num_files)
{
    assert(cmd != NULL);
    assert(options != NULL);

    int retval = 0;

    /* prepare environment */
    char *env_fpart_hooktype_name = "FPART_HOOKTYPE";
    char *env_fpart_partfilename_name = "FPART_PARTFILENAME";
    char *env_fpart_partnumber_name = "FPART_PARTNUMBER";
    char *env_fpart_partsize_name = "FPART_PARTSIZE";
    char *env_fpart_partnumfiles_name = "FPART_PARTNUMFILES";
    char *env_fpart_pid_name = "FPART_PID";

    char *env_fpart_hooktype_string = NULL;
    char *env_fpart_partfilename_string = NULL;
    char *env_fpart_partnumber_string = NULL;
    char *env_fpart_partsize_string = NULL;
    char *env_fpart_partnumfiles_string = NULL;
    char *env_fpart_pid_string = NULL;

    size_t malloc_size = 1; /* empty string */

    /* FPART_HOOKTYPE */
    malloc_size = strlen(env_fpart_hooktype_name) + 1;

    /* determine the kind of hook we are in */
    if(cmd == options->pre_part_hook) {
        assert(live_partition_index != NULL);
        assert(live_partition_size != NULL);
        assert(live_num_files != NULL);

        if(options->verbose >= OPT_VERBOSE)
            fprintf(stderr, "Executing pre-part #%d hook: '%s'\n",
                *live_partition_index, cmd);

        /* FPART_HOOKTYPE (continued) */
        malloc_size += strlen("pre-part") + 1;
        if((env_fpart_hooktype_string = malloc(malloc_size)) == NULL) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            retval = 1;
            goto cleanup;
        }
        snprintf(env_fpart_hooktype_string, malloc_size, "%s=%s",
            env_fpart_hooktype_name, "pre-part");
    }
    else if(cmd == options->post_part_hook) {
        assert(live_partition_index != NULL);
        assert(live_partition_size != NULL);
        assert(live_num_files != NULL);

        if(options->verbose >= OPT_VERBOSE)
            fprintf(stderr, "Executing post-part #%d hook: '%s'\n",
                *live_partition_index, cmd);

        /* FPART_HOOKTYPE (continued) */
        malloc_size += strlen("post-part") + 1;
        if((env_fpart_hooktype_string = malloc(malloc_size)) == NULL) {
            fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
            retval = 1;
            goto cleanup;
        }
        snprintf(env_fpart_hooktype_string, malloc_size, "%s=%s",
            env_fpart_hooktype_name, "post-part");
    }

    /* FPART_PARTFILENAME */
    if(live_filename != NULL)
        malloc_size = strlen(env_fpart_partfilename_name) + 1 +
            strlen(live_filename) + 1;
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

    /* FPART_PID */
    pid_t fpart_pid = getpid();
    malloc_size = strlen(env_fpart_pid_name) + 1 +
        get_num_digits(fpart_pid) + 1;
    if((env_fpart_pid_string = malloc(malloc_size)) == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        retval = 1;
        goto cleanup;
    }
    snprintf(env_fpart_pid_string, malloc_size, "%s=%d",
        env_fpart_pid_name, fpart_pid);

    char *envp[] = {
        env_fpart_hooktype_string,
        env_fpart_partfilename_string,
        env_fpart_partnumber_string,
        env_fpart_partsize_string,
        env_fpart_partnumfiles_string,
        env_fpart_pid_string,
        NULL };

    /* fork child process */
    int child_status = 0;
    switch(live_status.child_pid = fork()) {
        case -1:            /* error */
            fprintf(stderr, "fork(): %s\n", strerror(errno));
            retval = 1;
            break;
        case 0:             /* child */
        {
            /* become process group leader */
            if(setpgid(live_status.child_pid, 0) != 0) {
                fprintf(stderr, "%s(): setpgid(): %s\n", __func__,
                    strerror(errno));
                exit (1);
            }
            execle(_PATH_BSHELL, "sh", "-c", cmd, (char *)NULL, envp);
            /* if reached, error */
            exit (1);
        }
            break;
        default:            /* parent */
        {
            /* child-killer signal handler */
            signal(SIGTERM, kill_child);
            signal(SIGINT, kill_child);
            signal(SIGHUP, kill_child);

            pid_t wpid;
            do {
                wpid = wait(&child_status);
            } while((wpid != live_status.child_pid) && (wpid != -1));

            /* reset actions for signals */
            signal(SIGTERM, SIG_DFL);
            signal(SIGINT, SIG_DFL);
            signal(SIGHUP, SIG_DFL);
            /* reset child PID */
            live_status.child_pid = -1;

            if(wpid == -1) {
                fprintf(stderr, "%s(): wait(): %s\n", __func__,
                    strerror(errno));
                retval = 1;
            }
            else {
                if(WIFEXITED(child_status)) {
                    /* collect exit code */
                    if(WEXITSTATUS(child_status) != 0) {
                        if(options->verbose >= OPT_VERBOSE)
                            fprintf(stderr, "Hook '%s' exited with error %d\n",
                                cmd, WEXITSTATUS(child_status));
                        retval = 1;
                    }
                }
                else {
                    if(options->verbose >= OPT_VERBOSE)
                        fprintf(stderr, "Hook '%s' terminated prematurely\n",
                            cmd);
                    retval = 1;
                }
            }
        }
            break;
    }

cleanup:
    free(env_fpart_hooktype_string);
    free(env_fpart_partfilename_string);
    free(env_fpart_partnumber_string);
    free(env_fpart_partsize_string);
    free(env_fpart_partnumfiles_string);
    free(env_fpart_pid_string);
    return (retval);
}

/* Print or add a file entry (redirector) */
int
handle_file_entry(struct file_entry **head, char *path, fsize_t size,
    struct program_options *options)
{
    assert(options != NULL);

    if(options->live_mode == OPT_LIVEMODE)
        return (live_print_file_entry(path, size, options->out_filename,
            options));
    else
        return (add_file_entry(head, path, size, options));
}

/* Print a file entry */
int
live_print_file_entry(char *path, fsize_t size, char *out_template,
    struct program_options *options)
{
    assert(path != NULL);
    assert(options != NULL);
    assert(options->live_mode == OPT_LIVEMODE);

    /* beginning of a new partition */
    if(live_status.partition_num_files == 0) {
        /* very first pass of first partition, preload first partition */
        if(live_status.partition_index == 0)
            live_status.partition_size = options->preload_size;

        if(out_template != NULL) {
            /* compute live_status.filename "out_template.i\0" */
            size_t malloc_size = strlen(out_template) + 1 +
                get_num_digits(live_status.partition_index) + 1;
            if((live_status.filename = malloc(malloc_size)) == NULL) {
                fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
                return (1);
            }
            snprintf(live_status.filename, malloc_size, "%s.%d", out_template,
                live_status.partition_index);
        }

        /* execute pre-partition hook */
        if(options->pre_part_hook != NULL) {
            if(fpart_hook(options->pre_part_hook, options, live_status.filename,
                &live_status.partition_index, &live_status.partition_size,
                &live_status.partition_num_files) != 0)
                live_status.exit_summary = 1;
        }

        if(out_template != NULL) {
            /* open file */
            if((live_status.fd =
                open(live_status.filename, O_WRONLY|O_CREAT|O_TRUNC, 0660)) < 0) {
                fprintf(stderr, "%s: %s\n", live_status.filename,
                    strerror(errno));
                free(live_status.filename);
                live_status.filename = NULL;
                return (1);
            }
        }
    }

    /* count file in */
    live_status.partition_size +=
        round_num(size + options->overload_size, options->round_size);
    live_status.partition_num_files++;

    if(out_template == NULL) {
        /* no template provided, just print to stdout */
        fprintf(stdout, "%d (%lld): %s\n", live_status.partition_index, size,
            path);
    }
    else {
        /* print to fd */
        size_t to_write = strlen(path);
        if((write(live_status.fd, path, to_write) != to_write) ||
            (write(live_status.fd, "\n", 1) != 1)) {
            fprintf(stderr, "%s\n", strerror(errno));
            /* do not close(livefd) and free(live_status.filename) here because
               it will be useful and free'd in uninit_file_entries() below */
            return (1);
        }
    }

    /* display added filename */
    if(options->verbose >= OPT_VVERBOSE)
        fprintf(stderr, "%s\n", path);

    /* if end of partition reached */
    if(((options->max_entries > 0) && 
            (live_status.partition_num_files >= options->max_entries)) ||
        ((options->max_size > 0) && 
            (live_status.partition_size >= options->max_size))) {
        /* display added partition */
        if(options->verbose >= OPT_VERBOSE)
            fprintf(stderr, "Filled part #%d: size = %lld, %lld file(s)\n",
                live_status.partition_index, live_status.partition_size,
                live_status.partition_num_files);

        /* close fd or flush buffer */
        if(out_template == NULL)
            fflush(stdout);
        else
            close(live_status.fd);

        /* execute post-partition hook */
        if(options->post_part_hook != NULL) {
            if(fpart_hook(options->post_part_hook, options,
                live_status.filename, &live_status.partition_index,
                &live_status.partition_size,
                &live_status.partition_num_files) != 0)
                live_status.exit_summary = 1;
        }

        if(out_template != NULL) {
            free(live_status.filename);
            live_status.filename = NULL;
        }

        /* reset current partition status */
        live_status.partition_index++;
        live_status.partition_size = options->preload_size;
        live_status.partition_num_files = 0;
    }

    return (0);
}

/*********************************************************
 Double-linked list of file_entries manipulation functions
 *********************************************************/

/* Add a file entry to a double-linked list of file_entries
   - if head is NULL, creates a new file entry ; if not, chains a new file
     entry to it
   - returns with head set to the newly added element */
int
add_file_entry(struct file_entry **head, char *path, fsize_t size,
    struct program_options *options)
{
    assert(head != NULL);
    assert(path != NULL);
    assert(options != NULL);
    assert(options->live_mode == OPT_NOLIVEMODE);

    struct file_entry **current = head; /* current file_entry pointer address */
    struct file_entry *previous = NULL; /* previous file_entry pointer */

    /* backup current structure pointer and initialize a new structure */
    previous = *current;

    *current = malloc(sizeof(struct file_entry));
    if(*current == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        return (1);
    }

    /* set head on first call */
    if(*head == NULL)
        *head = *current;

    /* set current file data */
    size_t malloc_size = strlen(path) + 1;

    (*current)->path = malloc(malloc_size);
    if((*current)->path == NULL) {
        fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
        free(*current);
        *current = previous;
        return (1);
    }
    snprintf((*current)->path, malloc_size, "%s", path);
    (*current)->size = size + options->overload_size;
    (*current)->size = round_num((*current)->size, options->round_size);

    /* set current file entry's index and pointers */
    (*current)->partition_index = 0;    /* set during dispatch */
    (*current)->nextp = NULL;           /* set in next pass (see below) */
    (*current)->prevp = previous;

    /* set previous' nextp pointer */
    if(previous != NULL)
        previous->nextp = *current;

    /* display added filename */
    if(options->verbose >= OPT_VVERBOSE)
        fprintf(stderr, "%s\n", (*current)->path);

    return (0);
}

/* Initialize a double-linked list of file_entries from a path
   - file_path may be a file or directory
   - if head is NULL, creates a new list ; if not, chains a new list to it
   - increments *count with the number of files found
   - returns != 0 if critical error
   - returns with head set to the last element added */
int
init_file_entries(char *file_path, struct file_entry **head, fnum_t *count,
    struct program_options *options)
{
    assert(file_path != NULL);
    assert(head != NULL);
    assert(count != NULL);
    assert(options != NULL);

    /* prepare fts */
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

    /* current dir state */
    unsigned char curdir_empty = 1;
    unsigned char curdir_addme = 0;

    while((p = fts_read(ftsp)) != NULL) {
        switch (p->fts_info) {
            /* misc errors */
            case FTS_ERR:
                fprintf(stderr, "%s: %s\n", p->fts_path,
                    strerror(p->fts_errno));
                continue;

            /* errors for which we know there is a file or directory
               within current directory */
            case FTS_DNR:   /* un-readable directory */
            case FTS_NS:    /* stat() error */
                fprintf(stderr, "%s: %s\n", p->fts_path,
                    strerror(p->fts_errno));
            case FTS_NSOK: /* no stat(2) available (not requested) */
                /* treat dir as not empty if we did not request the -Z option */
                if(options->empty_errs == OPT_NOEMPTYERRS)
                    curdir_empty = 0;
                continue;

            case FTS_DC:
                fprintf(stderr, "%s: file system loop detected\n", p->fts_path);
            case FTS_DOT:  /* ignore "." and ".." */
                continue;

            case FTS_DP:
            {
                /* if empty_dirs display requested and current dir is empty,
                   add directory entry */
                if((options->empty_dirs == OPT_EMPTYDIRS) && curdir_empty)
                    curdir_addme = 1;

                /* add directory if necessary */
                if(curdir_addme) {
                    fsize_t curdir_size = 0;
                    char *curdir_entry_path = NULL;

                    /* count ending '/' and '\0', even if an ending '/' is not
                       added */
                    size_t malloc_size = p->fts_pathlen + 1 + 1;
                    if((curdir_entry_path = malloc(malloc_size)) == NULL) {
                        fprintf(stderr, "%s(): cannot allocate memory\n",
                            __func__);
                        fts_close(ftsp);
                        return (1);
                    }

                    /* add slash if requested and necessary */
                    if((options->add_slash == OPT_ADDSLASH) &&
                        (p->fts_pathlen > 0) &&
                        (p->fts_path[p->fts_pathlen - 1] != '/'))
                        snprintf(curdir_entry_path, malloc_size, "%s/",
                            p->fts_path);
                    else
                        snprintf(curdir_entry_path, malloc_size, "%s",
                            p->fts_path);

                    /* compute current dir size */
                    if((p->fts_level > 0) &&
                        (options->cross_fs_boundaries == OPT_NOCROSSFSBOUNDARIES) &&
                        (p->fts_parent->fts_statp->st_dev != p->fts_statp->st_dev))
                        /* when using option -x, set size to 0 for mountpoint
                           (non-root) directories */
                        curdir_size = 0;
                    else
                        curdir_size =
                            get_size(p->fts_path, p->fts_statp, options);

                    /* add or display it */
                    if(handle_file_entry
                        (head, curdir_entry_path, curdir_size, options) == 0)
                        (*count)++;
                    else {
                        fprintf(stderr, "%s(): cannot add file entry\n",
                            __func__);
                        free(curdir_entry_path);
                        fts_close(ftsp);
                        return (1);
                    }

                    /* cleanup */
                    free(curdir_entry_path);
                }

                /* reset parent (now current) dir state */
                curdir_empty = 0;
                curdir_addme = 0;
                continue;
            }

            case FTS_D:
            {
                curdir_empty = 1; /* enter directory, mark it as empty */

                /* if dir_depth requested and reached,
                   skip descendants but add directory entry (in post order) */
                if((options->dir_depth != OPT_NODIRDEPTH) &&
                    (p->fts_level >= options->dir_depth)) {
                    fts_set(ftsp, p, FTS_SKIP);
                    curdir_addme = 1;
                }
                continue;
            }

            default:
            /* XXX default means remaining file types:
               FTS_F, FTS_SL, FTS_SLNONE, FTS_DEFAULT */
            {
                curdir_empty = 0; /* mark current dir as non empty */

                /* add or display it */
                if(handle_file_entry
                    (head, p->fts_path,
                    get_size(p->fts_path, p->fts_statp, options),
                    options) == 0)
                    (*count)++;
                else {
                    fprintf(stderr, "%s(): cannot add file entry\n", __func__);
                    fts_close(ftsp);
                    return (1);
                }
                continue;
            }
        }
    }

    if(errno != 0) {
        fprintf(stderr, "%s: fts_read()\n", file_path);
        fts_close(ftsp);
        return (1);
    }

    if(fts_close(ftsp) < 0)
        fprintf(stderr, "%s: fts_close()\n", file_path);

    return (0);
}

/* Un-initialize a double-linked list of file_entries */
void
uninit_file_entries(struct file_entry *head, struct program_options *options)
{
    assert(options != NULL);

    /* be sure to start from last file entry */
    fastfw_list(head);

    struct file_entry *current = head;
    struct file_entry *prev = NULL;

    while(current != NULL) {
        if(current->path != NULL) {
            free(current->path);
        }
        prev = current->prevp;
        free(current);
        current = prev;
    }

    /* live mode */
    if(options->live_mode == OPT_LIVEMODE) {
        /* display added partition */
        if((options->verbose >= OPT_VERBOSE) &&
            (live_status.partition_num_files > 0))
            fprintf(stderr, "Filled part #%d: size = %lld, %lld file(s)\n",
                live_status.partition_index, live_status.partition_size,
                live_status.partition_num_files);

        /* flush buffer or close last file if necessary */
        if(options->out_filename == NULL)
            fflush(stdout);
        else if(live_status.filename != NULL)
            close(live_status.fd);

        /* execute last post-partition hook */
        if((options->post_part_hook != NULL) &&
            (live_status.partition_num_files > 0)) {
            if(fpart_hook(options->post_part_hook, options,
                live_status.filename, &live_status.partition_index,
                &live_status.partition_size,
                &live_status.partition_num_files) != 0)
                live_status.exit_summary = 1;
        }

        if(live_status.filename != NULL) {
            free(live_status.filename);
            live_status.filename = NULL;
        }

        /* print hooks' exit codes summary */
        if((options->verbose >= OPT_VERBOSE) && (live_status.exit_summary != 0))
            fprintf(stderr, "Warning: at least one hook exited with error !\n");

    }
    return;
}

/* Print a double-linked list of file_entries from head
   - if no filename template given, print to stdout */
int
print_file_entries(struct file_entry *head, char *out_template,
    pnum_t num_parts)
{
    assert(head != NULL);
    assert(num_parts > 0);

    /* no template provided, just print to stdout and return */
    if(out_template == NULL) {
        while(head != NULL) {
            fprintf(stdout, "%d (%lld): %s\n", head->partition_index,
                head->size, head->path);
            head = head->nextp;
        }
        return (0);
    }

    /* a template has been provided; to avoid opening too many files,
       open chunks of FDs and do as many passes as necessary */
    struct file_entry *start = head;
    pnum_t current_chunk = 0;           /* current chunk */
    pnum_t current_file_entry = 0;      /* current file entry within chunk */

    assert(PRINT_FE_CHUNKS > 0);

    while(((current_chunk * PRINT_FE_CHUNKS) + current_file_entry) <
        num_parts) {
        int fd[PRINT_FE_CHUNKS]; /* our <files per chunk> file descriptors */

        /* open as necessary file descriptors as needed
           to print num_part partitions */
        while((current_file_entry < PRINT_FE_CHUNKS) &&
              (((current_chunk * PRINT_FE_CHUNKS) + current_file_entry) < num_parts)) {
            /* compute out_filename  "out_template.i\0" */
            char *out_filename = NULL;
            size_t malloc_size = strlen(out_template) + 1 +
                get_num_digits
                ((current_chunk * PRINT_FE_CHUNKS) + current_file_entry) + 1;
            if((out_filename = malloc(malloc_size)) == NULL) {
                fprintf(stderr, "%s(): cannot allocate memory\n", __func__);
                /* close all open descriptors and return */
                pnum_t i;
                for(i = 0; i < current_file_entry; i++)
                     close(fd[i]);
                return (1);
            }
            snprintf(out_filename, malloc_size, "%s.%d", out_template,
                (current_chunk * PRINT_FE_CHUNKS) + current_file_entry);

            if((fd[current_file_entry] =
                open(out_filename, O_WRONLY|O_CREAT|O_TRUNC, 0660)) < 0) {
                fprintf(stderr, "%s: %s\n", out_filename, strerror(errno));
                free(out_filename);
                /* close all open descriptors and return */
                pnum_t i;
                for(i = 0; i < current_file_entry; i++)
                     close(fd[i]);
                return (1);
            }
            free(out_filename);
            current_file_entry++;
        }

        while(head != NULL) {
            if((head->partition_index >= (current_chunk * PRINT_FE_CHUNKS)) &&
               (head->partition_index < ((current_chunk + 1) * PRINT_FE_CHUNKS))) {
                size_t to_write = strlen(head->path);
                if((write(fd[head->partition_index % PRINT_FE_CHUNKS], head->path, to_write) != to_write) ||
                    (write(fd[head->partition_index % PRINT_FE_CHUNKS], "\n", 1) != 1)) {
                    fprintf(stderr, "%s\n", strerror(errno));
                    /* close all open descriptors */
                    pnum_t i;
                    for(i = 0; (i < PRINT_FE_CHUNKS) && (((current_chunk * PRINT_FE_CHUNKS) + i) < num_parts); i++)
                        close(fd[i]);
                    return (1);
                }
            }
            head = head->nextp;
        }
        /* come back to first entry */
        head = start;

        /* close file descriptors */
        pnum_t i;
        for(i = 0; (i < PRINT_FE_CHUNKS) && (((current_chunk * PRINT_FE_CHUNKS) + i) < num_parts); i++)
            close(fd[i]);

        current_file_entry = 0;
        current_chunk++;
    }

    return (0);
}

/***************************************************
 Array of file_entry pointers manipulation functions
 ***************************************************/

/* Initialize an array of file_entry pointers from a double-linked
   list of file_entries (head) */
void
init_file_entry_p(struct file_entry **file_entry_p, fnum_t num_entries,
    struct file_entry *head)
{
    assert(file_entry_p != NULL);

    fnum_t i = 0;
    while((head != NULL) && (file_entry_p != NULL) && (i < num_entries)) {
        file_entry_p[i] = head;
        head = head->nextp;
        i++;
    }
    return;
}
