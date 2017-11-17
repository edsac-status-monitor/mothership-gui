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

#define DEFAULT_DB_PATH "./mothership.db"

int main(int argc, char** argv) {
    const time_t update_time = 2; // seconds
    timer_t timer_id = NULL;

    // option arguments new for this
    char *db_path = NULL;
    bool db_path_set = true;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    GOptionEntry entries[] = {
        {"version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, version_option_callback, NULL, NULL},
        {"db-path", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &db_path, "Path to the database file", "PATH"},
        {NULL}
    };
    #pragma GCC diagnostic pop

    struct sockaddr *addr = get_args(&argc, &argv, gtk_get_option_group(TRUE), entries);
    assert(NULL != addr);

    if (NULL == db_path) {
        init_database(DEFAULT_DB_PATH);
        db_path_set = false;
    } else {
        init_database(db_path);
    }


    if (db_path_set) {
        g_free(db_path);
    }

    assert(true == create_timer((timer_handler_t) periodic_update, &timer_id, update_time));

    assert (true == start_server(addr, sizeof(*addr)));

    return start_ui(&argc, &argv, (gpointer) &timer_id);
}
