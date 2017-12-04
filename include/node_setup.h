/*
 * Copyright 2017
 * GPL3 Licensed
 * node_setup.h
 * Functions relating to the automatic set up of nodes
 */

#ifndef NODE_SETUP_H 
#define NODE_SETUP_H

// link properly with C++
#ifdef _cplusplus
extern "C" {
#endif // _cplusplus

// includes
#include <stdbool.h>

// declarations

// attempt to set up a node specified by the arguments. These arguments are assumed to be valid.
// returns success
bool setup_node_network(const unsigned int rack_no, const unsigned int chassis_no, const char *mac_addr);

// attempt to revert the changes made in setup_node
void node_cleanup_network(const unsigned int rack_no, const unsigned int chassis_no);

// copy file to node using ssh
bool copy_file(const unsigned int rack_no, const unsigned int chassis_no, const char *src, const char *dest);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // NODE_SETUP_H
