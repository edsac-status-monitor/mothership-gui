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
    bool enabled;
    int id;
} SearchResult;

typedef struct {
    unsigned int rack_no;
    unsigned int chassis_no;
} NodeIdentifier;

// declarations
bool check_mac_address(const char* str);

void free_search_result(gpointer res);

void init_database(const char* path);
void close_database(void);

// get the fields we want out of the IP v4 address (xxx.xxx.rack_no.chassis_no)
NodeIdentifier *parse_ip_address(const struct in_addr *address);

// only effects things which search on clickables
void set_show_disabled(bool new_val);
bool get_show_disabled(void);

bool add_node(const unsigned int rack_no, const unsigned int chassis_no, const char* mac_address, const bool enabled, const char* config_path);
bool remove_node(const unsigned int rack_no, const unsigned int chassis_no);
bool node_exists(const int rack_no, const int chassis_no);

bool add_error(const BufferItem *error);
bool add_error_decoded(const uint32_t rack_no, const uint32_t chassis_no, const int valve_no, const time_t recv_time, const char *msg);
bool remove_all_errors(void);

// returns a GList of SearchResults
GList *search_clickable(const Clickable *search);

// GList of unsigned int
GList *list_racks(void);

// GList of chassis numbers
GList *list_chassis_by_rack(const uintptr_t rack_no);

// GList of NodeIdentifiers
GSList *list_nodes(void);

bool node_toggle_disabled(const unsigned long int rack_no, const unsigned long int chassis_no);
bool error_toggle_disabled(const uintptr_t id);

// -1 on error
int count_clickable(const Clickable *search);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // SQL_H
