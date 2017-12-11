/*
 * Copyright 2017
 * GPL3 Licensed
 * main.c
 * Program entry point
 */

// includes
#include "config.h"
#include <glib.h>
#include <stdlib.h>
#include <edsac_timer.h>
#include <edsac_arguments.h>
#include <edsac_server.h>
#include "sql.h"
#include <assert.h>
#include "ui.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


#define DEFAULT_PREFIX_PATH "./edsac"
char *g_prefix_path = NULL;

// functions

// called periodically in its own thread to update the database and gui with new messages
static void periodic_update(__attribute__((unused)) void *unused) {
    BufferItem *item = NULL;
    bool need_update = false;

    while (true) {
        // read messages from the server's buffer
        item = read_message();
        if (NULL == item) {
            break;
        }

        // we have an update to do!
        need_update = true;
        // add the error to the database
        assert(true == add_error(item));
        free_bufferitem(item);
        item = NULL;
    }

    if (need_update) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
        g_idle_add((GSourceFunc) gui_update, (gpointer) gui_update); // uses the data parameter to remove itself from g_idle once it has run once
        #pragma GCC diagnostic pop
    }
}

static gboolean version_option_callback(__attribute__((unused)) gchar *option_name, __attribute__((unused)) gchar *value,
                                 __attribute__((unused)) gpointer data, __attribute__((unused)) GError **error) {
    puts(PACKAGE_STRING);
    exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
    const time_t update_time = 2; // seconds
    timer_t timer_id = NULL;

    // option arguments new for this
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    GOptionEntry entries[] = {
        {"version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, version_option_callback, NULL, NULL},
        {"path", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &g_prefix_path, "Path to the prefix directory underwhich the database is stored and other files are expected", "PATH"},
        {NULL}
    };
    #pragma GCC diagnostic pop

    struct sockaddr *addr = get_args(&argc, &argv, gtk_get_option_group(TRUE), entries);
    assert(NULL != addr);

    if (NULL == g_prefix_path) {
        g_prefix_path = (char *) DEFAULT_PREFIX_PATH;
    } 
    
    struct stat statbuf;
    if (0 != stat(g_prefix_path, &statbuf)) {
        // does the prefix directory even exist?
        if (ENOENT == errno) {
            // create missing directories
            printf("Creating prefix directory (%s) and subdirectories (%s/configs)\n", g_prefix_path, g_prefix_path);
            if (0 != mkdir(g_prefix_path, S_IRWXU | S_IRGRP | S_IXGRP /*rwxr-x---*/)) {
                perror("mkdir prefix");
                return EXIT_FAILURE;
            }

            GString *configs = g_string_new(g_prefix_path);
            assert(NULL != configs);
            g_string_append_printf(configs, "/configs");
            if (0 != mkdir(configs->str, S_IRWXU | S_IRGRP | S_IXGRP /*rwxr-x---*/)) {
                perror("mkdir configs");
                return EXIT_FAILURE;
            }
            g_string_free(configs, TRUE);

        } else { // some other error from stat
            perror("stat prefix dir");
            return EXIT_FAILURE;
        }
    } else if (!S_ISDIR(statbuf.st_mode)) { // stat worked but is it a directory?
        fprintf(stderr, "%s is not a directory\n", g_prefix_path);
        return EXIT_FAILURE;
    }

    // initialise database at path/mothership.db
    GString *db_path = g_string_new(g_prefix_path);
    assert(NULL != db_path);
    g_string_append_printf(db_path, "/mothership.db");
    init_database(db_path->str);
    g_string_free(db_path, TRUE);
    db_path = NULL;

    assert(true == create_timer((timer_handler_t) periodic_update, &timer_id, update_time));

   if (false == start_server(addr, sizeof(*addr))) {
       fprintf(stderr, "Unable to bind to address\n");
       exit(EXIT_FAILURE);
   }

    return start_ui(&argc, &argv, (gpointer) &timer_id);
    // g_prefix_path points to a leaked dynamically allocated string if the argument was specified. 
}
