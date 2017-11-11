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

// declarations

// private object data
typedef struct _EdsacErrorNotebookPrivate {
    gchar *test_member_data;
} EdsacErrorNotebookPrivate;

static gpointer edsac_error_notebook_parent_class = NULL;
#define EDSAC_ERROR_NOTEBOOK_GET_PRIVATE(_o) (G_TYPE_INSTANCE_GET_PRIVATE((_o), EDSAC_TYPE_ERROR_NOTEBOOK, EdsacErrorNotebookPrivate))

// functions

// just a test to see if everything is working
void edsac_error_notebook_say_hi(EdsacErrorNotebook *self) {
    assert(NULL != self);
    g_print(self->priv->test_member_data);
}

// internal GObject stuff 
EdsacErrorNotebook *edsac_error_notebook_construct(GType object_type) {
    EdsacErrorNotebook *self = (EdsacErrorNotebook *) g_object_new(object_type, NULL);
    return self;
}

// DESTROY PRIVATE MEMBER DATA HERE
static void edsac_error_notebook_finalize(GObject *obj) {
    EdsacErrorNotebook *self = EDSAC_ERROR_NOTEBOOK(obj);

    g_free(self->priv->test_member_data);
    self->priv->test_member_data = NULL;

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

    self->priv->test_member_data = g_strdup("Hello World!\n");
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
