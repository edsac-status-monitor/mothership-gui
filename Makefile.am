# make static library target
bin_PROGRAMS = mothership_gui
mothership_gui_SOURCES = src/main.c src/EdsacErrorNotebook.c include/EdsacErrorNotebook.h src/sql.c include/sql.h src/ui.c include/ui.h src/node_setup.c include/node_setup.h
mothership_gui_LDADD = $(GLIB_LIBS) $(GTK_LIBS) $(LIBEDSACNETWORKING_LIBS) $(PTHREAD_LIBS) $(SQLITE_LIBS)

# make subdirectories work
ACLOCAL_AMFLAGS = -I m4 --install
AC_LOCAL_AMFLAGS = -I m4 --install

# CFLAGS
AM_CFLAGS = -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wno-long-long -Wuninitialized -Wconversion -Wstrict-prototypes -Werror -O -g -std=c11 -fstack-protector-strong -I include -I$(top_srcdir)/include $(GLIB_CFLAGS) $(GTK_CFLAGS) $(LIBEDSACNETWORKING_CFLAGS) $(PTHREAD_CFLAGS) $(SQLITE_CFLAGS)

# Unit tests
check_PROGRAMS = sql.test add_errors.test
sql_test_SOURCES = src/test/sql-test.c src/sql.c include/sql.h
sql_test_LDADD = $(PTHREAD_LIBS) $(SQLITE_LIBS) $(GLIB_LIBS) $(LIBEDSACNETWORKING_LIBS)
add_errors_test_SOURCES = src/sql.c include/sql.h src/test/add_errors.c
add_errors_test_LDADD = $(PTHREAD_LIBS) $(SQLITE_LIBS) $(GLIB_LIBS) $(LIBEDSACNETWORKING_LIBS)
TESTS = sql.test

