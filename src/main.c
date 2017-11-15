/*
 * Copyright 2017
 * GPL3 Licensed
 * main.c
 * Program entry point
 */

// includes
#include "config.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include "EdsacErrorNotebook.h"
#include <edsac_timer.h>
#include "sql.h"
#include <assert.h>

// functions

static volatile EdsacErrorNotebook *notebook = NULL;

// activate handler for the application
static void activate(GtkApplication *app, __attribute__((unused)) gpointer data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "EDSAC Status Monitor");
    //gtk_window_maximize(GTK_WINDOW(WINDOW));

    // set minimum window size
    GdkGeometry geometry;
    geometry.min_height = 400;
    geometry.min_width = 600;
    gtk_window_set_geometry_hints(GTK_WINDOW(window), NULL, &geometry, GDK_HINT_MIN_SIZE);

    notebook = edsac_error_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(notebook));

    gtk_widget_show_all(window);
}

// handler called just before we terminate
static void shutdown_handler(__attribute__((unused)) GApplication *app, gpointer user_data) {
    //stop_server();
    close_database();

    if (NULL != user_data) {
        timer_t *timer_id = user_data;
        stop_timer(*timer_id);
    }
}

// called periodically in its own thread to 
static void periodic_update(__attribute__((unused)) void *unused) {
    g_idle_add((GSourceFunc) edsac_error_notebook_update, (gpointer) notebook);
}

int main(int argc, char** argv) {
    //const char *default_addr = "127.0.0.1";
    //const uint16_t default_port = 2000;
    const time_t update_time = 2; // seconds
    timer_t timer_id = NULL;

    gtk_init(&argc, &argv);

    GtkApplication *app = gtk_application_new("edsac.motherhip.gui", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(app, "shutdown", G_CALLBACK(shutdown_handler), (gpointer) &timer_id);

    init_database();
    assert(true == create_timer((timer_handler_t) periodic_update, &timer_id, update_time));

    //struct sockaddr *addr = alloc_addr(default_addr, default_port);
    //assert(NULL != addr);
    //assert (true == start_server(addr, sizeof(*addr)));

    return g_application_run(G_APPLICATION(app), argc, argv);
}
