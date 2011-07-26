#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([robotxq], [0.1.0], [gucong43216@gmail.com])
AC_CONFIG_SRCDIR([src/eleeye_xb/mips.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(robotxq ,0.1.0)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/timeb.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_CONFIG_FILES([Makefile src/Makefile src/eleeye_xb/Makefile src/io/Makefile src/learn_brd/Makefile data/Makefile doc/Makefile conf/Makefile])

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([dup2 ftime memchr memset select strchr strncasecmp strrchr])

AC_OUTPUT
