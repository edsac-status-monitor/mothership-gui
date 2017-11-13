# make static library target
bin_PROGRAMS = mothership_gui
mothership_gui_SOURCES = src/main.c src/EdsacErrorNotebook.c include/EdsacErrorNotebook.h
mothership_gui_LDADD = $(GLIB_LIBS) $(GTK_LIBS) $(LIBEDSACNETWORKING_LIBS) $(PTHREAD_LIBS)

# make subdirectories work
ACLOCAL_AMFLAGS = -I m4 --install
AC_LOCAL_AMFLAGS = -I m4 --install

# CFLAGS
AM_CFLAGS = -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wno-long-long -Wuninitialized -Wconversion -Wstrict-prototypes -Werror -O -g -std=c11 -fstack-protector-strong -I include -I$(top_srcdir)/include $(GLIB_CFLAGS) $(GTK_CFLAGS) $(LIBEDSACNETWORKING_CFLAGS) $(PTHREAD_CFLAGS)

# Unit tests
check_PROGRAMS = 
#representation_test_SOURCES = src/test/representation.c
#representation_test_LDADD = libnetworking.la $(GLIB_LIBS) $(PTHREAD_LIBS) $(RT_LIBS)
#TESTS = representation.test system.test 

