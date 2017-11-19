#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# require autoconf 2.69 or newer
AC_PREREQ([2.69])

# project metadata
AC_INIT([mothership-gui], [0.0.1], [tde1g14 soton ac uk])

# put auxiliary files in subdirectories to reduce clutter
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AC_USE_SYSTEM_EXTENSIONS

# sanity check. At least this should exist
AC_CONFIG_SRCDIR([./src/main.c])

# use automake
# std-options: executables should have --version and --help
# subdir-objects: put objects in the same subdir as the source file
# silent-rules: be quiet
AM_INIT_AUTOMAKE([std-options subdir-objects -Wall -Werror silent-rules])
AC_CONFIG_FILES([Makefile])

# generate include/config.h
AC_CONFIG_HEADERS([include/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_AWK
# for making static libraries:
AM_PROG_AR

# check GLIB, libnetworking and GTK are installed
PKG_PREREQ([0.29])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.32])
PKG_CHECK_MODULES([LIBEDSACNETWORKING], [libedsacnetworking >= 1.3.0])
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.22.11])
PKG_CHECK_MODULES([SQLITE], [sqlite3 >= 3.8.7])

# use libtool
LT_PREREQ([2.4.6])
LT_INIT([])
AC_SUBST([LIBTOOL_DEPS])

# pthreads
AX_PTHREAD()

# Checks for header files.
AC_CHECK_HEADERS([float.h limits.h locale.h stddef.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([localeconv memset])

AC_OUTPUT
