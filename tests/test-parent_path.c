/* cc -I../src -lm -o test-parent_path test-parent_path.c ../src/utils.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utils.h"

#define NB_STRINGS 38
static char strings[NB_STRINGS][255] = {
    "/foo/bar///baz///",
    "/foo///bar///baz///",
    "foo/bar///baz///",
    "foo///",
    "///foo",
    "foo",
    "",
    "/a/a///",
    "/a/a/",
    "/a/a",
    "/a///",
    "/a/",
    "/a",
    "/abcd/abcd///",
    "/abcd/abcd/",
    "/abcd/abcd",
    "/abcd///",
    "/abcd/",
    "/abcd",
    "///",
    "/",
    "",
    "a/a///",
    "a/a/",
    "a/a",
    "a///",
    "a/",
    "a",
    "abcd/abcd///",
    "abcd/abcd/",
    "abcd/abcd",
    "abcd///",
    "abcd/",
    "abcd",
    "/abcd///abcd///",
    "abcd///abcd///",
    "/abcd//abcd///",
    "abcd//abcd///"
} ;

int main(void)
{
  int i = 0;
  char *parent = NULL;

  while (i < NB_STRINGS) {
      parent = parent_path(strings[i], 0);

      printf("%d: %s\t\t => %s\n", i, strings[i], parent);
      free(parent);
      i++;
  }

  return(0);
}

