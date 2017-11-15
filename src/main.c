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
#include "sql.h"

// functions

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

    EdsacErrorNotebook *notebook = edsac_error_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(notebook));

    gtk_widget_show_all(window);
}

// handler called just before we terminate
static void shutdown_handler(__attribute__((unused)) GApplication *app, __attribute__((unused)) gpointer user_data) {
    close_database();
}

int main(int argc, char** argv) {
    gtk_init(&argc, &argv);

    GtkApplication *app = gtk_application_new("edsac.motherhip.gui", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    init_database();
    g_signal_connect(app, "shutdown", G_CALLBACK(shutdown_handler), NULL);

    return g_application_run(G_APPLICATION(app), argc, argv);
}
