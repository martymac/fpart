/*

Inspired from:
https://lists.gnu.org/archive/html/bug-gnulib/2016-06/msg00058.html

$ cc -fPIC -shared -o libfake_readdir.so fake_readdir.c

$ mkdir -p /tmp/test/a/b/c/d/e/f/g/h/i/j
$ LD_PRELOAD=./libfake_readdir.so fpart -L -E -zz -Z -f 2 -vv /tmp/test/a

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include <dlfcn.h>

int verbose = 1;

static size_t counter = 0;
#define FAIL_EVERY 4

static struct dirent *(*real_readdir)(DIR *dirp) = NULL;

struct dirent *readdir(DIR *dirp)
{
    /* Keep original pointer */
    if(real_readdir == NULL)
        real_readdir = dlsym(RTLD_NEXT, "readdir");

    counter++;
    if((counter % FAIL_EVERY) != 0) { 
	    if(verbose) fprintf(stderr, "XXX Real call to readdir(), counter = %zu\n", counter);
        return real_readdir(dirp);
    }
    else {
	    if(verbose) fprintf(stderr, "XXX Failing call to readdir(), counter = %zu\n", counter);
	    errno=EPERM;
	    return NULL;
    }
}
