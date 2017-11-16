/*
 * Copyright 2017
 * GPL3 Licensed
 * sql-test.c
 * Tests for sql.c
 */

// includes
#include "config.h"
#include "sql.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <edsac_representation.h>
#include <glib.h>
#include <string.h>
#include <time.h>
#include <edsac_arguments.h>

// functions

int main(void) {
    const unsigned int test_num = 248;
    const char* correct_mac = "ff:ff:ff:ff:ff:ff";
    init_database(NULL); // memory only database
    assert(true == create_tables());

    // invalid mac address
    assert(false == add_node(test_num, test_num, "not a mac address", false, "myconfig"));

    // null mac address
    assert(false == add_node(test_num, test_num, NULL, false, "myconfig"));

    // null config path
    assert(false == add_node(test_num, test_num, correct_mac, false, NULL));

    // malicious config path
    // if this wasn't fixed then the query would give a syntax error
    assert(true == add_node(test_num, test_num, correct_mac, false, "\" lala sqlite will error. I could drop all your tables!!!"));
    assert(true == remove_node(test_num, test_num));

    // add a valid node to use with the errors
    assert(true == add_node(test_num, test_num, correct_mac, true, "myconfig"));

    // add an error
    BufferItem item;

    // IP address
    GString *address_string = g_string_new(NULL);
    g_string_sprintf(address_string, "127.0.%i.%i", test_num, test_num);

    struct sockaddr *address = alloc_addr(address_string->str, 1234);
    memcpy(&item.address, address->sa_data +2, sizeof(item.address));

    free(address);
    g_string_free(address_string, TRUE);

    // Message
    hardware_error_valve(&item.msg, 22, "test");

    // recv_time
    item.recv_time = time(NULL);

    // create an error from this BufferItem
    assert(true == add_error(&item));

    // search for this error
    Clickable search;
    search.type = RACK;
    search.rack_num = test_num;
    GList *results = search_clickable(&search);
    assert(NULL != results);
    
    // the result is what we just added
    SearchResult *res = results->data;
    assert(NULL != res);
    assert(NULL != res->message);
    puts(res->message);
    assert(0 > strcmp("Hardware Error: test", res->message));
    assert(test_num == res->rack_no);
    assert(test_num == res->chassis_no);
    assert(22 != res->rack_no);

    // only result
    assert(NULL == results->next);
    assert(NULL == results->prev);

    // clean up
    assert(true == remove_all_errors());

    // test that we can't find the error we just removed
    assert(NULL == search_clickable(&search));

    assert(true == remove_node(test_num, test_num));
    close_database();
}
