# make static library target
bin_PROGRAMS = mothership_gui
mothership_gui_SOURCES = src/main.c src/EdsacErrorNotebook.c include/EdsacErrorNotebook.h src/sql.c src/sql.h
mothership_gui_LDADD = $(GLIB_LIBS) $(GTK_LIBS) $(LIBEDSACNETWORKING_LIBS) $(PTHREAD_LIBS) $(SQLITE_LIBS)

# make subdirectories work
ACLOCAL_AMFLAGS = -I m4 --install
AC_LOCAL_AMFLAGS = -I m4 --install

# CFLAGS
AM_CFLAGS = -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wno-long-long -Wuninitialized -Wconversion -Wstrict-prototypes -Werror -O -g -std=c11 -fstack-protector-strong -I include -I$(top_srcdir)/include $(GLIB_CFLAGS) $(GTK_CFLAGS) $(LIBEDSACNETWORKING_CFLAGS) $(PTHREAD_CFLAGS) $(SQLITE_CFLAGS)

# Unit tests
check_PROGRAMS = sql.test
sql_test_SOURCES = src/test/sql-test.c src/sql.c src/sql.h
sql_test_LDADD = $(PTHREAD_LIBS) $(SQLITE_LIBS) $(GLIB_LIBS) $(LIBEDSACNETWORKING_LIBS)
TESTS = sql.test

