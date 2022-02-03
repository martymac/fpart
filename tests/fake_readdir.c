/*

Small library to test implementation of option -Z and FPART_PARTERRNO

Inspired from:
https://lists.gnu.org/archive/html/bug-gnulib/2016-06/msg00058.html

$ cc -fPIC -shared -o libfake_readdir.so fake_readdir.c

$ mkdir -p /tmp/test/a/b/c/d/e/f/g/h/i/j
$ LD_PRELOAD=./libfake_readdir.so fpart -L -E -zz -Z -f 2 -vvv /tmp/test

$ mkdir -p /tmp/test/b/{c,d}
$ touch /tmp/test/b/{c,d}/{a..z}
$ LD_PRELOAD=./libfake_readdir.so fpart -L -E -zz -Z -f 2 -vvv /tmp/test/b

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include <dlfcn.h>

static size_t counter = 0;
#define FAIL_EVERY 7

static struct dirent *(*real_readdir)(DIR *dirp) = NULL;

struct dirent *readdir(DIR *dirp)
{
    /* Keep original pointer */
    if(real_readdir == NULL)
        real_readdir = dlsym(RTLD_NEXT, "readdir");

    counter++;

    if((counter % FAIL_EVERY) == 0) {
        fprintf(stderr, "XXX Failing call to readdir(), counter = %zu\n", counter);
        fflush(stderr);
        errno=EIO;
        return NULL;
    }

    return real_readdir(dirp);
}
