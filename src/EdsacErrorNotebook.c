/*
 * Copyright 2017
 * GPL3 Licensed
 * EdsacErrorNotebook.c
 * GObject Class inheriting from GtkNotebook implementing the tabbed list of errors
 */

// includes
#include "config.h"
#include "EdsacErrorNotebook.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "sql.h"

// declarations

// context for an open tab
typedef struct _LinkyTextBuffer {
    pthread_mutex_t mutex;  // controlls access to the structure
    Clickable *description; // information about what this is a list of
    GtkTextBuffer *buffer;  // the text buffer
    GSList *g_string_list;  // stuff to call g_string_free(., TRUE) on when we are deleted
    GSList *clickables;     // Clickables *within the text buffer* we need to free
    gint page_id;           // the gtknotebook page id
} LinkyBuffer;

// private object data
typedef struct _EdsacErrorNotebookPrivate {
    pthread_mutex_t mutex;  // controlls access to this structure
    GSList *open_tabs_list; // list of open tabs (LinkyBuffers)
} EdsacErrorNotebookPrivate;

static gpointer edsac_error_notebook_parent_class = NULL;
#define EDSAC_ERROR_NOTEBOOK_GET_PRIVATE(_o) (G_TYPE_INSTANCE_GET_PRIVATE((_o), EDSAC_TYPE_ERROR_NOTEBOOK, EdsacErrorNotebookPrivate))

/**** local function declarations ****/
// My Structures
static bool clickable_compare(const Clickable *a, const Clickable *b);
static gint open_tabs_list_compare_by_id(gconstpointer a, gconstpointer b);
static gint open_tabs_list_compare_by_desc(gconstpointer a, gconstpointer b);
static void open_tabs_list_dec_id(gpointer data, gpointer unused);
static LinkyBuffer *new_linky_buffer(const char* text, Clickable *description);
static void append_linky_text_buffer(LinkyBuffer *linky_buffer, const unsigned int rack_no, const unsigned int chassis_no, const int valve_no, const char* msg);
static void free_g_string(gpointer g_string);
static void free_linky_buffer(LinkyBuffer *linky_buffer);
static void add_link(size_t start_pos, size_t end_pos, GtkTextBuffer *buffer, Clickable* data);
static void update_tab(gpointer data, gpointer unused);

// GTK
static GtkWidget *new_text_view(void);
static GtkWidget *put_in_scroll(GtkWidget *thing);
static GtkWidget *tab_label(const char *msg, GtkWidget *contents);
static GtkWidget *get_parent(const GtkWidget *child);

// Signal Handlers
static void close_button_handler(GtkWidget *button, GdkEvent *event, GtkWidget *contents);
static void clicked(const GtkTextTag *tag, const GtkTextView *parent, const GdkEvent *event, const GtkTextIter *iter, Clickable *data);

/**** Public Methods ****/
// update data to be in line with the database
void update(EdsacErrorNotebook *self) {
    assert(0 == pthread_mutex_lock(&self->priv->mutex));

    g_slist_foreach(self->priv->open_tabs_list, update_tab, NULL);

    assert(0 == pthread_mutex_unlock(&self->priv->mutex));
}

// add a new page to the notebook
notebook_page_id_t add_new_page_to_notebook(EdsacErrorNotebook *self, Clickable *data) {
    if ((NULL == self) || (NULL == data)) {
        return NULL;
    }

    GtkNotebook *notebook = &self->parent_instance;

    // string to go into the new tab
    GString *message = g_string_new(NULL);
    assert(NULL != message);

    switch(data->type) {
        case ALL:
            break;
        case RACK:
            g_string_printf(message, "Rack %i", data->rack_num);
            break;
        case CHASSIS:
            g_string_printf(message, "Rack %i, Chassis: %i", data->rack_num, data->chassis_num);
            break;
        case VALVE:
            g_string_printf(message, "Rack %i, Chassis %i, Valve: %i", data->rack_num, data->chassis_num, data->valve_num);
            break;
        default:
            g_string_printf(message, "(Unknown)");
    }

    GtkWidget *msg = new_text_view(); 
    assert(NULL != msg);

    // LinkyBuffer to describe the notebook
    LinkyBuffer *linky_buffer = new_linky_buffer(message->str, data);
    assert(NULL != linky_buffer);

    // no need to lock the mutex because nobody else has a reference to linky_buffer yet

    linky_buffer->g_string_list = g_slist_prepend(linky_buffer->g_string_list, message);

    gtk_text_view_set_buffer(GTK_TEXT_VIEW(msg), linky_buffer->buffer);

    GtkWidget *scroll = put_in_scroll(msg);
    assert(NULL != scroll);

    GString *tab_str;
    if (ALL == data->type) {
        tab_str = g_string_new("All");
        linky_buffer->g_string_list = g_slist_prepend(linky_buffer->g_string_list, tab_str);
    } else {
        tab_str = message;
    }

    gint index = gtk_notebook_append_page(notebook, scroll, tab_label(tab_str->str, scroll));
    assert(-1 != index);
    linky_buffer->page_id = index;

    // add the new tab to our open tabs list
    assert(0 == pthread_mutex_lock(&self->priv->mutex));
    self->priv->open_tabs_list = g_slist_insert_sorted(self->priv->open_tabs_list, linky_buffer, open_tabs_list_compare_by_id);
    assert(0 == pthread_mutex_unlock(&self->priv->mutex));

    // update the new page
    update_tab((gpointer) linky_buffer, NULL);

    // show the new page
    GtkWidget *page = gtk_notebook_get_nth_page(notebook, index);
    gtk_widget_show_all(page);

    // change to the new page
    gtk_notebook_set_current_page(notebook, index);

    return linky_buffer;
}




/*** stuff to do with internal structures ***/

// set default argument so that we can iterate through teh list more easily
static void free_g_string(gpointer g_string) {
    g_string_free((GString *) g_string, TRUE);
}

// free a LinkyBuffer
static void free_linky_buffer(LinkyBuffer *linky_buffer) {
    assert(NULL != linky_buffer);

    assert(0 == pthread_mutex_destroy(&linky_buffer->mutex));

    g_slist_free_full(linky_buffer->g_string_list, free_g_string);
    g_slist_free_full(linky_buffer->clickables, g_free); // invalid free
    // don't free the description because it is in someone else's clickables list

    g_free(linky_buffer);
}

// are two Clickables equal? Ignores undefined fields.
static bool clickable_compare(const Clickable *a, const Clickable *b) {
    assert(NULL != a);
    assert(NULL != b);

    if (a->type != b->type) {
        return false;
    }

    if (ALL == a->type) {
        return true;
    }

    const bool rack_num = (a->rack_num == b->rack_num);
    if ((RACK == a->type) && rack_num) {
        return true;
    }

    const bool chassis_num = (a->chassis_num == b->chassis_num);
    if ((CHASSIS == a->type) && rack_num && chassis_num) {
        return true;
    }

    const bool valve_num = (a->chassis_num == b->chassis_num);
    if ((VALVE == a->type) && rack_num && chassis_num && valve_num) {
        return true;
    }

    return false;
}

// open tabs list compare func for searching by id
// implements GCompareFunc: 0 is equal
static gint open_tabs_list_compare_by_id(gconstpointer a, gconstpointer b) {
    assert(NULL != a);
    assert(NULL != b);

    LinkyBuffer *A = (LinkyBuffer *) a;
    LinkyBuffer *B = (LinkyBuffer *) b;

    assert(0 == pthread_mutex_lock(&A->mutex));
    assert(0 == pthread_mutex_lock(&B->mutex));

    const gint ret = A->page_id - B->page_id;

    assert(0 == pthread_mutex_unlock(&A->mutex));
    assert(0 == pthread_mutex_unlock(&B->mutex));

    return ret;
}

// decrements the page id of something on the open tabs list. Prototype to match glib foreach
static void open_tabs_list_dec_id(gpointer data, __attribute__((unused)) gpointer unused) {
    assert(NULL != data);

    LinkyBuffer *tab_page_desc = (LinkyBuffer *) data;
    assert(0 == pthread_mutex_lock(&tab_page_desc->mutex));
 
    tab_page_desc->page_id -= 1;

    assert(0 == pthread_mutex_unlock(&tab_page_desc->mutex));
}

// creates a new LinkyBuffer optionally with the given text
static LinkyBuffer *new_linky_buffer(const char* text, Clickable *desc) {
    LinkyBuffer *linky_buffer = malloc(sizeof(LinkyBuffer));
    assert(NULL != linky_buffer);

    // default values
    linky_buffer->g_string_list = NULL;
    linky_buffer->clickables = NULL;
    linky_buffer->page_id = -1;
    linky_buffer->buffer = gtk_text_buffer_new(NULL);
    assert(NULL != linky_buffer->buffer);

    // set up mutex
    assert(0 == pthread_mutex_init(&linky_buffer->mutex, NULL));

    // set stuff given to us
    linky_buffer->description = desc;
    gtk_text_buffer_set_text(linky_buffer->buffer, text, -1);
    
    // make the text buffer un-editable
    GtkTextTag *un_editable_tag = gtk_text_buffer_create_tag(linky_buffer->buffer, "un_editable", "editable", FALSE, "editable-set", TRUE, NULL);
    assert(NULL != un_editable_tag);
    GtkTextIter start; 
    GtkTextIter end;
    gtk_text_buffer_get_start_iter(linky_buffer->buffer, &start);
    gtk_text_buffer_get_end_iter(linky_buffer->buffer, &end);
    gtk_text_buffer_apply_tag(linky_buffer->buffer, un_editable_tag, &start, &end);

    return linky_buffer;
}

// appends the specified error message to the linky buffer
// negative valve_no to not specify it
// assumes that linky_buffer is already locked
static void append_linky_text_buffer(LinkyBuffer *linky_buffer, const unsigned int rack_no, const unsigned int chassis_no, const int valve_no, const char* msg) {
    // generate message string
    GString *message = g_string_new(NULL);
    assert(NULL != message);
    linky_buffer->g_string_list = g_slist_prepend(linky_buffer->g_string_list, message); // so we know to free message

    GtkTextIter buffer_end;
    gtk_text_buffer_get_end_iter(linky_buffer->buffer, &buffer_end);
    const gsize offset = (gsize) gtk_text_iter_get_offset(&buffer_end);

    g_string_printf(message, "Rack: %i, ", rack_no);
    const gsize rack_start = offset;
    const gsize rack_end = offset + message->len - 2; // comma, space 
    const gsize chassis_start = offset + message->len; 

    g_string_append_printf(message, "Chassis: %i, ", chassis_no);
    const gsize chassis_end = offset + message->len - 2; // comma, space 
    const gsize valve_start = offset + message->len; 

    if (valve_no >= 0) {
        g_string_append_printf(message, "Valve: %i: ", valve_no);
    }
    const gsize valve_end = offset + message->len - 2; // colon, space. (this is unused when valve_no < 0 so it remains valid)
    
    g_string_append_printf(message, "%s\n", msg);
    
    gtk_text_buffer_insert(linky_buffer->buffer, &buffer_end, message->str, (gint) message->len);

    // clickable objects to describe the links in this row
    Clickable *rack_data = malloc(sizeof(Clickable));
    assert(NULL != rack_data);
    rack_data->type = RACK;
    rack_data->rack_num = rack_no;

    Clickable *chassis_data = malloc(sizeof(Clickable));
    assert(NULL != chassis_data);
    chassis_data->type = CHASSIS;
    chassis_data->rack_num = rack_no;
    chassis_data->chassis_num = chassis_no;

    Clickable *valve_data = NULL;
    if (valve_no >= 0) {
        valve_data = malloc(sizeof(Clickable));
        assert(NULL != valve_data);
        valve_data->type = VALVE;
        valve_data->rack_num = rack_no;
        valve_data->chassis_num = chassis_no;
        valve_data->valve_num = valve_no; 
    }

    // so we know to free them
    linky_buffer->clickables = g_slist_prepend(linky_buffer->clickables, rack_data);
    linky_buffer->clickables = g_slist_prepend(linky_buffer->clickables, chassis_data);

    if (valve_no >= 0) {
        linky_buffer->clickables = g_slist_prepend(linky_buffer->clickables, valve_data);
    }

    add_link(rack_start, rack_end, linky_buffer->buffer, rack_data);
    add_link(chassis_start, chassis_end, linky_buffer->buffer, chassis_data);
    if (valve_no >= 0) {
        add_link(valve_start, valve_end, linky_buffer->buffer, valve_data);
    }
}

// adds a hyperlink to a text buffer
static void add_link(size_t start_pos, size_t end_pos, GtkTextBuffer *buffer, Clickable *data) {
    GtkTextTag *url = gtk_text_buffer_create_tag(buffer, NULL,
        "underline", PANGO_UNDERLINE_SINGLE, "underline-set", TRUE,
        "foreground", "blue", NULL);

    // make it clickable
    // event is the only signal allowed on a tag
    g_signal_connect(G_OBJECT(url), "event", G_CALLBACK(clicked), (gpointer) data);
    
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_iter_at_offset(buffer, &start, (gint) start_pos);
    gtk_text_buffer_get_iter_at_offset(buffer, &end, (gint) end_pos);
    gtk_text_buffer_apply_tag(buffer, url, &start, &end);
}

// open tabs list compare func for searching by description
// implements GCompareFunc: 0 is equal
static gint open_tabs_list_compare_by_desc(gconstpointer a, gconstpointer b) {
    assert(NULL != a);
    assert(NULL != b);

    LinkyBuffer *A = (LinkyBuffer *) a;
    LinkyBuffer *B = (LinkyBuffer *) b;

    gint ret = 1;

    assert(0 == pthread_mutex_lock(&A->mutex));
    assert(0 == pthread_mutex_lock(&B->mutex));

    if (clickable_compare(A->description, B->description)) {
        ret = 0;
    } 

    assert(0 == pthread_mutex_unlock(&A->mutex));
    assert(0 == pthread_mutex_unlock(&B->mutex));

    return ret;
}

static void insert_search_result(gpointer data, gpointer user_data) {
    if ((NULL == data) || (NULL == user_data)) {
        return;
    }

    puts("insert search result");

    SearchResult *res = (SearchResult *) data;
    LinkyBuffer *linky_buffer = (LinkyBuffer *) user_data;

    append_linky_text_buffer(linky_buffer, res->rack_no, res->chassis_no, res->valve_no, res->message);
}

static void update_tab(gpointer data, __attribute__((unused)) gpointer unused) {
    assert(NULL != data);
    LinkyBuffer *linky_buffer = (LinkyBuffer *) data;
    assert(0 == pthread_mutex_lock(&linky_buffer->mutex));

    puts("update_tab");

    // query the database
    GList *results = search_clickable(linky_buffer->description);

    // clear stuff already in the buffer
    GtkTextIter start;
    gtk_text_buffer_get_start_iter(linky_buffer->buffer, &start);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(linky_buffer->buffer, &end);
    
    gtk_text_buffer_delete(linky_buffer->buffer, &start, &end);

    // TODO
    //g_slist_free_full(linky_buffer->g_string_list, free_g_string);
    // TODO! I don't think we can free the clickables without upsetting other linky_buffers!

    g_list_foreach(results, insert_search_result, (gpointer) linky_buffer);

    assert(0 == pthread_mutex_unlock(&linky_buffer->mutex));
}



/**** GTK Signal Handlers ****/
// handler for when a text tag is clicked
static void clicked(__attribute__((unused)) const GtkTextTag *tag, const GtkTextView *parent, const GdkEvent *event,
                    __attribute__((unused)) const GtkTextIter *iter, Clickable *data) {
    assert(NULL != event);
    // work out if the event was a clicked
    GdkEventButton *event_btn = (GdkEventButton *) event;
    if (event->type == GDK_BUTTON_RELEASE && event_btn->button == 1) {
        assert(NULL != data);
        assert(NULL != parent);
        // work up the tree to the notebook
        GtkWidget *text_view = GTK_WIDGET(parent);
        GtkWidget *scrolled_window = get_parent(text_view);
        EdsacErrorNotebook *notebook = EDSAC_ERROR_NOTEBOOK(get_parent(scrolled_window));

        // check if the tab we want is already open
        LinkyBuffer data_desc;
        data_desc.page_id = -1;
        data_desc.description = data;
        assert(0 == pthread_mutex_init(&data_desc.mutex, NULL));
        assert(0 == pthread_mutex_lock(&notebook->priv->mutex));
        // LinkyBuffer mutex locking done in open_tabs_list_compare_by_desc
        GSList *found = g_slist_find_custom(notebook->priv->open_tabs_list, (gconstpointer) &data_desc, open_tabs_list_compare_by_desc);
        assert(0 == pthread_mutex_unlock(&notebook->priv->mutex));
        if (NULL != found) {
            LinkyBuffer *tab = (LinkyBuffer *) found->data;
            assert(NULL != tab);

            gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), tab->page_id); // I don't think there is any reason to re-lock for this?
            // assumes that a and b are locked by the caller
            return;
        }

        // our tab is not open so we need to make a new one...
        add_new_page_to_notebook(notebook, data);
    }
}

// handler for the close button on tab labels
static void close_button_handler(GtkWidget *button, __attribute__((unused)) GdkEvent *event, GtkWidget *contents) {
    assert(NULL != button);
    assert(NULL != contents);

    // get the notebook
    GtkWidget *grid = GTK_WIDGET(get_parent(button));
    GtkWidget *frame = GTK_WIDGET(get_parent(grid));
    EdsacErrorNotebook *notebook = EDSAC_ERROR_NOTEBOOK(get_parent(frame));
    assert(NULL != notebook);

    gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), contents);
    if (-1 == page_num)
        return;

    // try to find this tab in the open tabs list
    LinkyBuffer example;
    example.page_id = page_num;
    assert(0 == pthread_mutex_init(&example.mutex, NULL));
    assert(0 == pthread_mutex_lock(&notebook->priv->mutex));
    // open_tabs_list_compate_by_id does the locking on the LinkyBuffers
    GSList *result = g_slist_find_custom(notebook->priv->open_tabs_list, (gconstpointer) &example, open_tabs_list_compare_by_id);
    if (NULL == result) {
        assert(0 == pthread_mutex_unlock(&notebook->priv->mutex));
        g_print("Closing a tab which was not open! id=%i\n", page_num);
        return;
    } else {
        LinkyBuffer *tab_page_desc = (LinkyBuffer *) result->data;

        // any tabs after this one need to have their id adjusted down by 1 (assuming list sorted by id)
        // open_tabs_list_dec_id does locking
        g_slist_foreach(result, open_tabs_list_dec_id, NULL);

        // remove tab from the list
        // TODO are you very sure this shouldn't be g_slist_delete_link?
        notebook->priv->open_tabs_list = g_slist_remove_link(notebook->priv->open_tabs_list, result);

        // free the linky buffer
        free_linky_buffer(tab_page_desc);

        // remove page from notebook
        gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_num);

        assert(0 == pthread_mutex_unlock(&notebook->priv->mutex));

        // close the window if the last page is closed
        if (0 == gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook))) {
            GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(notebook));
            if (gtk_widget_is_toplevel(toplevel)) { // docs recommend this extra check https://developer.gnome.org/gtk3/stable/GtkWidget.html#gtk-widget-get-toplevel
                GtkWindow *window = GTK_WINDOW(toplevel);
                assert(NULL != window);

                gtk_window_close(window);
            } else {
                perror("Could not find the top level widget");
                exit(EXIT_FAILURE);
            }
        }

    } 
}



/**** GTK stuff ****/
// new gtk text view without a visale cursor
static GtkWidget *new_text_view(void) {
    GtkWidget *text_view = gtk_text_view_new_with_buffer(NULL);
    
    // get rid of the cursor
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);

    return text_view;
}

// puts thing into a scrolled window
static GtkWidget *put_in_scroll(GtkWidget *thing) {
    assert(NULL != thing);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    assert(NULL != scroll);

    gtk_container_add(GTK_CONTAINER(scroll), thing);

    return scroll;
}

// creates the widget used to label a tab
static GtkWidget *tab_label(const char *msg, GtkWidget *contents) {
    assert(NULL != msg);
    assert(NULL != contents);

    // text
    GtkWidget *text = gtk_label_new(msg);

    // close button
    GtkWidget *close = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(G_OBJECT(close), "button-press-event", (GCallback) close_button_handler, contents);

    // container
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
    gtk_grid_attach(GTK_GRID(grid), text, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), close, 2, 1, 1, 1);

    // put it in a frame so we get boarders
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), grid);

    gtk_widget_show_all(frame);
    return GTK_WIDGET(frame);
} 

// get the Gtk Parent from the parent property
static GtkWidget *get_parent(const GtkWidget *child) {
    assert(NULL != child);

    GValue parent_g_value;
    memset(&parent_g_value, '\0', sizeof(parent_g_value));
    g_value_init(&parent_g_value, GTK_TYPE_CONTAINER);

    g_object_get_property(G_OBJECT(child), "parent", &parent_g_value);

    return GTK_WIDGET(g_value_get_object(&parent_g_value));
}



/**** internal GObject stuff ****/
EdsacErrorNotebook *edsac_error_notebook_construct(GType object_type) {
    EdsacErrorNotebook *self = (EdsacErrorNotebook *) g_object_new(object_type, NULL);
    return self;
}

// DESTROY PRIVATE MEMBER DATA HERE
static void edsac_error_notebook_finalize(GObject *obj) {
    EdsacErrorNotebook *self = EDSAC_ERROR_NOTEBOOK(obj);

    assert(0 == pthread_mutex_destroy(&self->priv->mutex));
    g_slist_free_full(self->priv->open_tabs_list, (GDestroyNotify) free_linky_buffer);

    G_OBJECT_CLASS(edsac_error_notebook_parent_class)->finalize(obj);
}

static void edsac_error_notebook_class_init(EdsacErrorNotebookClass *class) {
    edsac_error_notebook_parent_class = g_type_class_peek_parent(class);
    g_type_class_add_private(class, sizeof(EdsacErrorNotebookPrivate));
    G_OBJECT_CLASS(class)->finalize = edsac_error_notebook_finalize;
}

// CONSTRUCT PRIVATE MEMBER DATA HERE
static void edsac_error_notebook_instance_init(EdsacErrorNotebook *self) {
    self->priv = EDSAC_ERROR_NOTEBOOK_GET_PRIVATE(self);

    assert(0 == pthread_mutex_init(&self->priv->mutex, NULL));
    self->priv->open_tabs_list = NULL; // empty slist

    Clickable *all_desc = malloc(sizeof(Clickable));
    assert(NULL != all_desc);
    all_desc->type = ALL;

    LinkyBuffer *all = (LinkyBuffer *) add_new_page_to_notebook(self, all_desc);
    assert(NULL != all);

    // dummy data
    //append_linky_text_buffer(all, 1, 2, 3, "4");
    //append_linky_text_buffer(all, 1, 2, -1, "No valve");

    gtk_notebook_set_scrollable(&self->parent_instance, TRUE);
}

GType edsac_error_notebook_get_type(void) {
    static volatile gsize edsac_error_notebook_type_id_volatile = 0;
    if (g_once_init_enter(&edsac_error_notebook_type_id_volatile)) {
        static const GTypeInfo g_define_type_info = {
            sizeof(EdsacErrorNotebookClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) edsac_error_notebook_class_init,
            (GClassFinalizeFunc) NULL,
            NULL,
            sizeof(EdsacErrorNotebook),
            0,
            (GInstanceInitFunc) edsac_error_notebook_instance_init,
            NULL
        };

        GType edsac_error_notebook_type_id;
        edsac_error_notebook_type_id = g_type_register_static(gtk_notebook_get_type(), "EdsacErrorNotebook", &g_define_type_info, 0);
        g_once_init_leave(&edsac_error_notebook_type_id_volatile, edsac_error_notebook_type_id);
    }

    return edsac_error_notebook_type_id_volatile;
}

EdsacErrorNotebook *edsac_error_notebook_new(void) {
    return edsac_error_notebook_construct(EDSAC_TYPE_ERROR_NOTEBOOK);
}
