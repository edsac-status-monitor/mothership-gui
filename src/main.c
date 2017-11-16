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
#include "EdsacErrorNotebook.h"
#include <edsac_timer.h>
#include <edsac_arguments.h>
#include <edsac_server.h>
#include "sql.h"
#include <assert.h>
#include "ui.h"

// ui.c
extern EdsacErrorNotebook *notebook;

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
        g_idle_add((GSourceFunc) edsac_error_notebook_update, (gpointer) notebook);
    }
}

int main(int argc, char** argv) {
    const time_t update_time = 2; // seconds
    timer_t timer_id = NULL;

    init_database();
    assert(true == create_timer((timer_handler_t) periodic_update, &timer_id, update_time));

    struct sockaddr *addr = get_args(&argc, &argv, gtk_get_option_group(TRUE));
    assert(NULL != addr);
    assert (true == start_server(addr, sizeof(*addr)));

    return start_ui(&argc, &argv, (gpointer) &timer_id);
}
