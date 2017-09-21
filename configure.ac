AC_PREREQ([2.69])
AC_INIT([fpart], [0.9.4], [ganael.laplanche@martymac.org])
AC_CONFIG_SRCDIR([src/fpart.h])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Checks for programs.
AC_PROG_CC([cc gcc])
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_INSTALL

# Checks for log10() in -lm
AC_CHECK_LIB(m, log10)

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

# OS detection
AC_CANONICAL_HOST
case "${host_os}" in
  solaris*) host_os_solaris=true ;;
  linux*) host_os_linux=true ;;
esac

# Default value for embedded fts support
dflt_embfts=false
# Enabled on Solaris
if test x$host_os_solaris = xtrue
then
  dflt_embfts=true
fi

# Embedded fts option
AC_ARG_ENABLE([embfts],
[  --enable-embfts         enable embedded fts],
[case "${enableval}" in
  yes) embfts=true ;;
  no)  embfts=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-embfts]) ;;
esac],[embfts=${dflt_embfts}])

# Static build option
AC_ARG_ENABLE([static],
[  --enable-static         build static binary],
[case "${enableval}" in
  yes) static=true ;;
  no)  static=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-static]) ;;
esac],[static=false])

# Debug option
AC_ARG_ENABLE([debug],
[  --enable-debug          turn on debugging],
[case "${enableval}" in
  yes) debug=true ;;
  no)  debug=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],[debug=false])

# Disable large file support on Linux when not using embedded fts
# as Linux' fts.h cannot be used with _FILE_OFFSET_BITS==64
if test x$embfts = xfalse
then
  if test x$host_os_linux = xtrue
  then
    enable_largefile=no
  fi
fi

# Large file support
AC_SYS_LARGEFILE

# Automake output
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])
AM_CONDITIONAL([EMBEDDED_FTS], [test x$embfts = xtrue])
AM_CONDITIONAL([SOLARIS], [test x$host_os_solaris = xtrue])
AM_CONDITIONAL([LINUX], [test x$host_os_linux = xtrue])
AM_CONDITIONAL([STATIC], [test x$static = xtrue])

#AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([Makefile src/Makefile tools/Makefile man/Makefile])
AC_OUTPUT
