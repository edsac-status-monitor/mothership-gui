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
static GtkWindow *main_window = NULL;

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


    gtk_statusbar_pop(bar, 0);
    gtk_statusbar_push(bar, 0, msg->str);
    g_string_free(msg, TRUE);
}

// called when gtk gets around to updating the gui
void gui_update(gpointer g_idle_id) {
    assert(TRUE == g_idle_remove_by_data(g_idle_id));
    edsac_error_notebook_update(notebook);
    update_bar();
}

// handles the quit action
static void quit_activate(void) {
    if (NULL != main_window) {
        gtk_window_close(main_window);
    }
}

// handles the add_node action
static void add_node_activate(void) {
    puts("Add node");
}

static void show_disabled_change_state(GSimpleAction *simple) {
    assert(NULL != simple);
    gboolean show_disabled = g_variant_get_boolean(g_action_get_state(G_ACTION(simple)));

    puts("In handler");
    // toggle
    if (show_disabled) {
        g_simple_action_set_state(simple, g_variant_new_boolean(FALSE));
        puts("Now Not showing disabled items");
    } else {
        g_simple_action_set_state(simple, g_variant_new_boolean(TRUE));
        puts("Now Showing disabled items");
    }
}

static void node_prototype_activate(__attribute__((unused)) GSimpleAction *simple, GVariant *parameter) {
    printf("Node %i clicked\n", g_variant_get_int32(parameter));
}

static GMenu *update_nodes(void) {
    static int32_t call_count = 10;
    call_count += 1;

    GMenu *nodes = g_menu_new();
    assert(NULL != nodes);


    GMenuItem *node_1 = g_menu_item_new("Node 1", "app.node_prototype");
    assert(NULL != node_1);
    g_menu_item_set_action_and_target_value(node_1, "app.node_prototype", g_variant_new_int32(1));
    g_menu_append_item(nodes, node_1);

    GMenuItem *node_2 = g_menu_item_new("Node 2", "app.node_prototype");
    assert(NULL != node_2);
    g_menu_item_set_action_and_target_value(node_2, "app.node_prototype", g_variant_new_int32(2));
    g_menu_append_item(nodes, node_2);

    GMenuItem *node_n = g_menu_item_new("Node n", "app.node_prototype");
    assert(NULL != node_n);
    g_menu_item_set_action_and_target_value(node_n, "app.node_prototype", g_variant_new_int32(call_count));
    g_menu_append_item(nodes, node_n);

    g_menu_freeze(nodes);

    return nodes;
}

typedef void (*action_handler_t)(GSimpleAction *simple, GVariant *parameter, gpointer user_data);

// activate handler for the application
static void activate(GtkApplication *app, __attribute__((unused)) gpointer data) {
    main_window = GTK_WINDOW(gtk_application_window_new(app));
    gtk_window_set_title(main_window, "EDSAC Status Monitor");
    //gtk_window_maximize(GTK_WINDOW(WINDOW));

    // set minimum window size
    GdkGeometry geometry;
    geometry.min_height = 400;
    geometry.min_width = 600;
    gtk_window_set_geometry_hints(main_window, NULL, &geometry, GDK_HINT_MIN_SIZE);

    // GTKBox to hold stuff
    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0 /*arbitrary: a guess*/));

    // Menu bar actions
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    static const GActionEntry actions[] = {
        {"add_node", (action_handler_t) add_node_activate},
        {"quit", (action_handler_t) quit_activate},
        {"show_disabled", NULL, "b", "true", (action_handler_t) show_disabled_change_state},
        {"node_prototype", (action_handler_t) node_prototype_activate, "i"}
    };
    #pragma GCC diagnostic pop
    g_action_map_add_action_entries(G_ACTION_MAP(app), actions, G_N_ELEMENTS(actions), NULL);

    // File menu model 
    GMenu *file = g_menu_new();
    assert(NULL != file);
    g_menu_append(file, "Add Node", "app.add_node");
    g_menu_append(file, "Quit", "app.quit");
    const char *quit_accels[] = {"<Control>Q", NULL};
    gtk_application_set_accels_for_action(app, "app.quit", quit_accels);
    g_menu_freeze(file);

    // View menu model
    GMenu *view = g_menu_new();
    assert(NULL != view);
    GMenuItem *show_disabled = g_menu_item_new("Show Disabled", "app.show_disabled");
    assert(NULL != show_disabled);
    g_menu_item_set_action_and_target_value(show_disabled, "app.show_disabled", g_variant_new_boolean(TRUE));
    g_menu_append_item(view, show_disabled);
    g_menu_freeze(view);

    // Nodes menu model
    GMenu *nodes = update_nodes();
    
    // Menu bar model
    GMenu *model = g_menu_new();
    assert(NULL != model);
    g_menu_append_submenu(model, "File", G_MENU_MODEL(file));
    g_menu_append_submenu(model, "View", G_MENU_MODEL(view));
    g_menu_append_submenu(model, "Nodes", G_MENU_MODEL(nodes));
    g_menu_freeze(model);

    // Menu bar widget
    GtkWidget *menu = gtk_menu_bar_new_from_model(G_MENU_MODEL(model));
    assert(NULL != menu);
    gtk_box_pack_start(box, menu, FALSE, FALSE, 0);

    // test updating nodes
    g_menu_remove(model, 2);
    g_menu_append_submenu(model, "Nodes", G_MENU_MODEL(update_nodes()));
    g_menu_append_submenu(model, "Nodes", G_MENU_MODEL(update_nodes()));

    // make notebook
    notebook = edsac_error_notebook_new();
    g_signal_connect_after(G_OBJECT(notebook), "switch-page", G_CALLBACK(update_bar), NULL);
    gtk_box_pack_start(box, GTK_WIDGET(notebook), TRUE, TRUE, 0);

    // make status bar
    bar = GTK_STATUSBAR(gtk_statusbar_new());
    assert(NULL != bar);
    update_bar();
    gtk_box_pack_start(box, GTK_WIDGET(bar), FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(main_window), GTK_WIDGET(box));
    gtk_widget_show_all(GTK_WIDGET(main_window));
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