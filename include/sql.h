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

// declarations
void init_database(void);
void close_database(void);

bool add_node(const unsigned int rack_no, const unsigned int chassis_no, const char* mac_address, const bool enabled, const char* config_path);
bool remove_node(const unsigned int rack_no, const unsigned int chassis_no);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // SQL_H
