AC_PREREQ([2.69])
AC_INIT([fpart], [0.8], [ganael.laplanche@martymac.org])
AC_CONFIG_SRCDIR([src/fpart.h])
AM_INIT_AUTOMAKE(foreign -Wall -Werror)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h paths.h stdlib.h string.h strings.h sys/mount.h sys/param.h sys/statfs.h sys/statvfs.h sys/vfs.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero fchdir getcwd memmove memset strchr strerror strrchr strtol])

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
