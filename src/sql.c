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

    puts(query->str);
    bool ret = true;
    char *errstr = NULL;
    if (SQLITE_OK != sqlite3_exec(db, query->str, NULL, NULL, &errstr)) {
        puts(errstr);
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