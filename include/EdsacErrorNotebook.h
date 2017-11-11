/*
 * Copyright 2017
 * GPL3 Licensed
 * EdsacErrorNotebook.h
 * GObject Class Definition of EdsacErrorNotebook. Inheriting from GtkNotebook
 */

#ifndef EDSAC_ERROR_NOTEBOOK_H 
#define EDSAC_ERROR_NOTEBOOK_H 

// link properly with C++
#ifdef _cplusplus
extern "C" {
#endif // _cplusplus

// includes
#include <glib.h>
#include <gtk/gtk.h>

// GObject init
G_BEGIN_DECLS

// Macro definitions
#define EDSAC_TYPE_ERROR_NOTEBOOK (edsac_error_notebook_get_type())
#define EDSAC_ERROR_NOTEBOOK(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), EDSAC_TYPE_ERROR_NOTEBOOK, EdsacErrorNotebook))
#define EDSAC_ERROR_NOTEBOOK_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), EDSAC_TYPE_ERROR_NOTEBOOK, EdsacErrorNotebookClass))
#define EDSAC_IS_ERROR_NOTEBOOK(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), EDSAC_TYPE_ERROR_NOTEBOOK))
#define EDSAC_IS_ERROR_NOTEBOOK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), EDSAC_TYPE_ERROR_NOTEBOOK))
#define EDSAC_ERROR_NOTEBOOK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), EDSAC_TYPE_ERROR_NOTEBOOK, EdsacErrorNotebookClass))

// forward declaration
struct _EdsacErrorNotebookPrivate;

// Object
typedef struct {
    GtkNotebook parent_instance;
    struct _EdsacErrorNotebookPrivate *priv;
} EdsacErrorNotebook;

// Class
typedef struct {
    GtkNotebookClass parent_class;
} EdsacErrorNotebookClass;

// public methods
void edsac_error_notebook_say_hi(EdsacErrorNotebook *self);
EdsacErrorNotebook *edsac_error_notebook_new(void);

// boilerplate public methods
EdsacErrorNotebook *edsac_error_notebook_construct(GType object_type);
GType edsac_error_notebook_get_type(void) G_GNUC_CONST;

// GObject End
G_END_DECLS

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // EDSAC_ERROR_NOTEBOOK
