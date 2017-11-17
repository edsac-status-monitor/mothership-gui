/*
 * Copyright 2017
 * GPL3 Licensed
 * sql.h
 * Functions relating to the database
 */

#ifndef SQL_H 
#define SQL_H

// link properly with C++
#ifdef _cplusplus
extern "C" {
#endif // _cplusplus

// includes
#include <stdbool.h>
#include <edsac_server.h> // libedsacnetworking
#include "EdsacErrorNotebook.h"
#include <glib.h>

typedef struct {
    char *message;
    unsigned int rack_no;
    unsigned int chassis_no;
    int valve_no;
} SearchResult;

// declarations
void free_search_result(gpointer res);

void init_database(const char* path);
void close_database(void);

// false on error
bool create_tables(void);

bool add_node(const unsigned int rack_no, const unsigned int chassis_no, const char* mac_address, const bool enabled, const char* config_path);
bool remove_node(const unsigned int rack_no, const unsigned int chassis_no);

bool add_error(const BufferItem *error);
bool remove_all_errors(void);

// returns a GList of SearchResults
GList *search_clickable(const Clickable *search);

// -1 on error
int count_clickable(const Clickable *search);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // SQL_H
