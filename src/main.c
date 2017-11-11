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

// functions

int main(int argc, char** argv) {
    gtk_init(&argc, &argv);

    EdsacErrorNotebook *error_notebook = edsac_error_notebook_new();
    edsac_error_notebook_say_hi(error_notebook);
    g_object_ref_sink(G_OBJECT(error_notebook));
    error_notebook = NULL;

    return EXIT_SUCCESS;
}
