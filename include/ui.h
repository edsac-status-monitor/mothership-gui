/*
 * Copyright 2017
 * GPL3 Licensed
 * ui.h
 * UI stuff other than EdsacErrorNotebook
 */

#ifndef UI_H 
#define UI_H

// link properly with C++
#ifdef _cplusplus
extern "C" {
#endif // _cplusplus

// includes
#include <glib.h>

// declarations
int start_ui(int *argc, char ***argv, gpointer timer_id);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // UI_H
