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

static BufferItem *error(int rack_no, int chassis_no, const char* msg, MessageType type) {
    BufferItem *item = malloc(sizeof(BufferItem));
    assert(NULL != item);

    // IP address
    GString *address_string = g_string_new(NULL);
    g_string_sprintf(address_string, "127.0.%i.%i", rack_no, chassis_no);

    struct sockaddr *address = alloc_addr(address_string->str, /*arbitrary port number*/ 1234);
    assert(NULL != address);
    memcpy(&item->address, address->sa_data +2, sizeof(item->address));

    free(address);
    g_string_free(address_string, TRUE);

    // String Message & type
    switch (type) {
        case HARD_ERROR_VALVE:
            hardware_error_valve(&item->msg, /* arbitrary valve number */ 22, msg);
            break;
        case HARD_ERROR_OTHER:
            hardware_error_other(&item->msg, msg);
            break;
        case SOFT_ERROR:
            software_error(&item->msg, msg);
            break;
        default:
            free(item);
    }

    // recv_time
    item->recv_time = time(NULL);

    return item;
}

static void search_error(const unsigned int rack_no, const unsigned int chassis_no, const char *msg, MessageType type) {
    Clickable search;
    search.type = CHASSIS;
    search.rack_num = rack_no;
    search.chassis_num = chassis_no;
    GList *results = search_clickable(&search);
    assert(NULL != results);

    // only result (tests are written such that this will be true as it keeps things simple)
    assert(NULL == results->next);
    assert(NULL == results->prev);
    
    // check what we found
    SearchResult *res = results->data;
    assert(NULL != res);
    assert(NULL != res->message);
    assert(rack_no == res->rack_no);
    assert(chassis_no == res->chassis_no);

    GString *expected_msg = g_string_new(NULL);
    assert(NULL != expected_msg);
    switch (type) {
        case HARD_ERROR_VALVE:
            assert(22 == res->valve_no);
            // no break
        case HARD_ERROR_OTHER:
            g_string_sprintf(expected_msg, " Hardware Error: %s", msg);
            break;
        case SOFT_ERROR:
            g_string_printf(expected_msg, " Software Error: %s", msg);
            break;
        default:
            g_string_free(expected_msg, TRUE);
    }

    assert(0 > strcmp(expected_msg->str, res->message)); // res->msg includes expected_msg->str (it starts with the time)
    g_string_free(expected_msg, TRUE);
}

int main(void) {
    const char* correct_mac = "ff:ff:ff:ff:ff:ff";
    init_database(NULL); // NULL: memory only database
    assert(true == create_tables());

    // invalid mac address
    assert(false == add_node(0, 1, "not a mac address", false, "myconfig"));

    // null mac address
    assert(false == add_node(0, 1, NULL, false, "myconfig"));

    // null config path
    assert(false == add_node(0, 1, correct_mac, false, NULL));

    // malicious config path
    // if this wasn't fixed then the query would give a syntax error
    assert(true == add_node(0, 1, correct_mac, false, "\" lala sqlite will error. I could drop all your tables!!!"));
    assert(true == remove_node(0, 1));

    // add a valid nodes to use with the errors
    assert(true == add_node(0, 0, correct_mac, true, "myconfig"));
    assert(true == add_node(0, 1, correct_mac, true, "myconfig"));
    assert(true == add_node(0, 2, correct_mac, true, "myconfig"));

    // for count searching
    Clickable search;
    search.type = RACK;
    search.rack_num = 0;

    // there shouldn't be any errors in rack 0 to start with
    assert(0 == count_clickable(&search));

    // hard_error_valve error
    BufferItem *hard_error_valve = error(0, 0, "hard valve", HARD_ERROR_VALVE);
    assert(true == add_error(hard_error_valve));
    free(hard_error_valve); // we don't need to do a free_bufferitem because the string is static
    hard_error_valve = NULL;
    search_error(0, 0, "hard valve", HARD_ERROR_VALVE);

    assert(1 == count_clickable(&search));

    // hard_error_other error
    BufferItem *hard_error_other = error(0, 1, "hard other", HARD_ERROR_OTHER);
    assert(true == add_error(hard_error_other));
    free(hard_error_other);
    hard_error_other = NULL;
    search_error(0, 1, "hard other", HARD_ERROR_OTHER);

    assert(2 == count_clickable(&search));

    // software error
    BufferItem *soft_error = error(0, 2, "soft error", SOFT_ERROR);
    assert(true == add_error(soft_error));
    free(soft_error);
    soft_error = NULL;
    search_error(0, 2, "soft error", SOFT_ERROR);

    assert(3 == count_clickable(&search));

    // clean up
    assert(true == remove_all_errors());

    // check that all of those errors were removed
    assert(0 == count_clickable(&search));   

    // add errors back again on node 0, 0
    assert(true == add_error(error(0, 0, "", HARD_ERROR_VALVE)));
    assert(true == add_error(error(0, 0, "", HARD_ERROR_OTHER)));
    assert(true == add_error(error(0, 0, "", SOFT_ERROR)));
    Clickable node00_search;
    node00_search.type = CHASSIS;
    node00_search.rack_num = 0;
    node00_search.chassis_num = 0;
    assert(3 == count_clickable(&node00_search));

    // remove node 0, 0
    assert(true == remove_node(0, 0));

    // check that node 0, 0's errors were also removed
    assert(0 == count_clickable(&node00_search));
    
    assert(true == remove_node(0, 1));
    assert(true == remove_node(0, 2));
    close_database();
}
