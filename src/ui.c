/*
 * Copyright 2017
 * GPL3 Licensed
 * ui.c
 * UI stuff other than EdsacErrorNotebook
 */

// includes
#include "config.h"
#include "ui.h"
#include <assert.h>
#include <gtk/gtk.h>
#include "EdsacErrorNotebook.h"
#include "sql.h"
#include <edsac_server.h>
#include <edsac_timer.h>

// declarations
static void activate(GtkApplication *app, gpointer data);
static void shutdown_handler(__attribute__((unused)) GApplication *app, gpointer user_data);

static EdsacErrorNotebook *notebook = NULL;
static GtkStatusbar *bar = NULL;

// functions

int start_ui(int *argc, char ***argv, gpointer timer_id) {
    assert(NULL != argc);
    assert(NULL != argv);
    assert(NULL != timer_id);

    gtk_init(argc, argv);

    GtkApplication *app = gtk_application_new("edsac.motherhip.gui", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(app, "shutdown", G_CALLBACK(shutdown_handler), timer_id);
    
    return g_application_run(G_APPLICATION(app), *argc, *argv);
}

static void update_bar(void) {
    const int num_errors = edsac_error_notebook_get_error_count(notebook);

    GString *msg = g_string_new(NULL);
    assert(NULL != msg);

    if (num_errors > 1) {
        g_string_sprintf(msg, "Showing %i errors", num_errors);
    } else if (1 == num_errors) {
        g_string_sprintf(msg, "Showing 1 error");
    } else if (0 == num_errors) {
        g_string_sprintf(msg, "No errors in this filter");
    } else { // negative
        g_string_sprintf(msg, "Failure to count errors (something is probably very wrong)");
    }


    gtk_statusbar_push(bar, 0, msg->str);
    g_string_free(msg, TRUE);
}

// called when gtk gets around to updating the gui
void gui_update(gpointer g_idle_id) {
    assert(TRUE == g_idle_remove_by_data(g_idle_id));
    edsac_error_notebook_update(notebook);
    update_bar();
}

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

    // GTKBox to hold stuff
    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0 /*arbitrary: a guess*/));

    // make notebook
    notebook = edsac_error_notebook_new();
    g_signal_connect_after(G_OBJECT(notebook), "switch-page", G_CALLBACK(update_bar), NULL);
    gtk_box_pack_start(box, GTK_WIDGET(notebook), TRUE, TRUE, 0);

    // make status bar
    bar = GTK_STATUSBAR(gtk_statusbar_new());
    assert(NULL != bar);
    update_bar();
    gtk_box_pack_start(box, GTK_WIDGET(bar), FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(box));
    gtk_widget_show_all(window);
}

// handler called just before we terminate
static void shutdown_handler(__attribute__((unused)) GApplication *app, gpointer user_data) {
    if (NULL != user_data) {
        timer_t *timer_id = user_data;
        stop_timer(*timer_id);
    }

    stop_server();
    close_database();
}