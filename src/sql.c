/*
 * Copyright 2017
 * GPL3 Licensed
 * sql.c
 * Functions relating to database interaction
 */

// includes
#include "config.h"
#include "sql.h"
#include <stdio.h>
#include <assert.h>
#include <sqlite3.h>
#include <pthread.h>
#include <glib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define DEFAULT_DB_PATH "./mothership.db"

static pthread_mutex_t db_mutex;
static sqlite3 *db = NULL;

// functions

// checks that str is a valid mac address
static bool check_mac_address(const char* str) {
    if (NULL == str) {
        return false;
    }

    // format should be "ff:ff:ff:ff:ff:ff"
    if (17 != strnlen(str, 17)) {
        return false;
    }

    // first two hex digits
    if ((FALSE == g_ascii_isxdigit(str[0])) || (FALSE == g_ascii_isxdigit(str[1]))) {
        return false;
    }

    // 5 lots of :ff
    for (int i = 2; i < 14; i+=3) {
        if ((':' != str[i + 0]) || (FALSE == g_ascii_isxdigit(str[i + 1])) || (FALSE == g_ascii_isxdigit(str[i + 2]))) {
            return false;
        }
    }

    return true;
}

// removes quotes from strings so that they can go in queries safely
static GString *fix_string(const char* str) {
    GString *ret = g_string_new(str);
    assert(NULL != ret);

    for (gsize i = 0; i < ret->len; i++) {
        if ('"' == ret->str[i]) {
            #pragma GCC diagnostic ignored "-Wsign-conversion"
            ret = g_string_insert_c(ret, i, '"');
            assert(NULL != ret);
            i += 1; // don't loop forever
        }
    }

    return ret;
}

void init_database(void) {
    assert(SQLITE_OK == sqlite3_open(DEFAULT_DB_PATH, &db));
    assert(0 == pthread_mutex_init(&db_mutex, NULL));
}

void close_database(void) {
    assert(0 == pthread_mutex_destroy(&db_mutex));
    assert(SQLITE_OK == sqlite3_close(db));
}

bool add_node(const unsigned int rack_no, const unsigned int chassis_no, const char* mac_address, const bool enabled, const char* config_path) {
    if (!check_mac_address(mac_address)) {
        return false;
    }

    if (NULL == config_path) {
        return false;
    }

    GString *config_string = fix_string(config_path);

    gint enabled_int;
    if (enabled) {
        enabled_int = 1;
    } else {
        enabled_int = 0;
    }

    GString *query = g_string_new(NULL);
    assert(NULL != query);

    g_string_printf(query,
        "INSERT into nodes(rack_no, chassis_no, mac_address, enabled, config) VALUES(%i, %i, \"%s\", %i, \"%s\");", 
        rack_no, chassis_no, mac_address, enabled_int, config_string->str);

    assert(0 == pthread_mutex_lock(&db_mutex));

    bool ret = true;
    char *errstr = NULL;
    if (SQLITE_OK != sqlite3_exec(db, query->str, NULL, NULL, &errstr)) {
        ret = false;
    }

    assert(0 == pthread_mutex_unlock(&db_mutex));
    g_string_free(query, TRUE);
    g_string_free(config_string, TRUE);

    return ret;
}

bool remove_node(const unsigned int rack_no, const unsigned int chassis_no) {
    GString *query = g_string_new(NULL);
    assert(NULL != query);

    g_string_printf(query,
        "DELETE FROM nodes WHERE rack_no = %i AND chassis_no = %i;", 
        rack_no, chassis_no);

    assert(0 == pthread_mutex_lock(&db_mutex));

    bool ret = true;
    if (SQLITE_OK != sqlite3_exec(db, query->str, NULL, NULL, NULL)) {
        ret = false;
    }

    assert(0 == pthread_mutex_unlock(&db_mutex));
    g_string_free(query, TRUE);

    return ret;
}

bool remove_all_errors(void) {
    const char *query = "DELETE FROM errors;";

    assert(0 == pthread_mutex_lock(&db_mutex));

    bool ret = true;
    if (SQLITE_OK != sqlite3_exec(db, query, NULL, NULL, NULL)) {
        ret = false;
    }

    assert(0 == pthread_mutex_unlock(&db_mutex));

    return ret;
}

static int find_node_callback(void *ret, int argc, char** argv, __attribute__((unused)) char **col_name) {
    if ((NULL == ret) || (NULL == argv)) {
        return 1;
    } else if (NULL == *argv) {
        return 1;
    } else if (1 != argc) {
        return 1;
    }

    // assume we get a valid number from sqlite
    int * ret_int = (int *) ret;
    *ret_int = atoi(argv[0]);

    return 0;
}

// attempts to find the node id we were asked for
// if it is not found then -1 is returned
static int find_node(const unsigned int rack_no, const unsigned int chassis_no) {
    GString *query = g_string_new(NULL);
    assert(NULL != query);

    printf("find node rack %x chassis %x\n", rack_no, chassis_no);

    g_string_printf(query,
        "SELECT id FROM nodes WHERE rack_no=%i AND chassis_no=%i;", rack_no, chassis_no);

    assert(0 == pthread_mutex_lock(&db_mutex));

    int ret = -1;
    if (SQLITE_OK != sqlite3_exec(db, query->str, find_node_callback, &ret, NULL)) {
        ret = -1;
    }

    assert(0 == pthread_mutex_unlock(&db_mutex));
    g_string_free(query, TRUE);

    return ret;
}

static bool add_error_decoded(const uint32_t rack_no, const uint32_t chassis_no, const int valve_no, const time_t recv_time, const char *msg) {
    GString *query = g_string_new(NULL);
    assert(NULL != query);

    GString *msg_str = fix_string(msg);

    const int node_id = find_node(rack_no, chassis_no);
    if (-1 == node_id) {
        return false;
    }

    g_string_printf(query,
        "INSERT INTO errors(node_id, recv_time, description, enabled, valve_no) VALUES(%i, %li, \"%s\", 1, %i);", node_id, recv_time, msg_str->str, valve_no);

    assert(0 == pthread_mutex_lock(&db_mutex));

    bool ret = true;
    if (SQLITE_OK != sqlite3_exec(db, query->str, NULL, NULL, NULL)) {
        ret = false;
    }

    assert(0 == pthread_mutex_unlock(&db_mutex));

    g_string_free(query, TRUE);
    g_string_free(msg_str, TRUE);

    return ret;
}

bool add_error(const BufferItem *error) {
    if (NULL == error) {
        return false;
    }

    // "Network" is big endian. We don't know what the host is
    uint32_t addr = htonl(error->address.s_addr);
    printf("%x\n", addr);
    const uint32_t rack_num_n = (addr & 0x0000FF00) << 16;
    const uint32_t chassis_num_n = (addr & 0x000000FF) << 24;

    // don't assume host endianness
    const uint32_t rack_num = ntohl(rack_num_n);
    const uint32_t chassis_num = ntohl(chassis_num_n);

    bool ret = false;
    GString *error_msg = g_string_new(NULL);
    assert(NULL != error_msg);

    switch (error->msg.type) {
        case HARD_ERROR_VALVE:
            g_string_sprintf(error_msg, "Hardware Error: %s", 
                error->msg.data.hardware_valve.message->str);

            ret = add_error_decoded(rack_num, chassis_num, 
                error->msg.data.hardware_valve.valve_no,
                error->recv_time, error_msg->str);
            break;

        case HARD_ERROR_OTHER:
            g_string_sprintf(error_msg, "Hardware Error: %s",
                error->msg.data.hardware_other.message->str);

            ret = add_error_decoded(rack_num, chassis_num, -1, 
                error->recv_time, error_msg->str);
            break;

        case SOFT_ERROR:
            g_string_sprintf(error_msg, "Software Error: %s",
                error->msg.data.software.message->str);

            ret = add_error_decoded(rack_num, chassis_num, -1,
                error->recv_time, error_msg->str);
            break;

        default:
            g_string_sprintf(error_msg, "Unknown Error Type: %i",
                error->msg.type);
            break;
    }

    g_string_free(error_msg, TRUE);
    return ret;
}

