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

// declarations
void init_database(void);
void close_database(void);

bool add_node(const unsigned int rack_no, const unsigned int chassis_no, const char* mac_address, const bool enabled, const char* config_path);
bool remove_node(const unsigned int rack_no, const unsigned int chassis_no);

bool add_error(const BufferItem *error);
bool remove_all_errors(void);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // SQL_H
