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
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wsign-conversion"
            ret = g_string_insert_c(ret, i, '"');
            #pragma GCC diagnostic pop
            assert(NULL != ret);
            i += 1; // don't loop forever
        }
    }

    return ret;
}

bool create_tables(void) {
    const char *table_create_sql = \
    "CREATE TABLE nodes(\
	    id INTEGER PRIMARY KEY NOT NULL UNIQUE,\
	    rack_no INTEGER NOT NULL,\
	    chassis_no INTEGER NOT NULL,\
	    mac_address TEXT NOT NULL,\
	    enabled INTEGER DEFAULT 1,\
	    config TEXT NOT NULL,\
	    UNIQUE(rack_no, chassis_no)\
    );\
    CREATE TABLE errors(\
	    id INTEGER PRIMARY KEY NOT NULL UNIQUE,\
	    node_id INTEGER NOT NULL,\
	    recv_time INTEGER NOT NULL,\
	    description TEXT NOT NULL,\
	    valve_no INTEGER DEFAULT -1,\
	    enabled INTEGER DEFAULT 1\
    );";

    char *errstr = NULL;
    if (SQLITE_OK != sqlite3_exec(db, table_create_sql, NULL, NULL, &errstr)) {
        puts(errstr);
        return false;
    }

    return true;
}

void init_database(const char *path) {
    bool new_db = true;
    if ((NULL != path) && (0 != strncmp("", path, 1))) {
        // check to see if the database already exists
        if (0 == access(path, F_OK)) {
            // file exists
            new_db = false;

            // check that we have read and write permissions on the file
            if (0 != access(path, W_OK | R_OK)) {
                fprintf(stderr, "I don't have permission to access database file %s\n", path);
                exit(EXIT_FAILURE);
            } else {
                printf("Using existing database at %s\n", path);
                // open database with sqlite
                assert(SQLITE_OK == sqlite3_open(path, &db));
            }
        } else {
            printf("Creating a new database at %s\n", path);
            // open database with sqlite
            assert(SQLITE_OK == sqlite3_open(path, &db));
        }
    } else {
        puts("Creating memory resident database");
        // open database with sqlite
        assert(SQLITE_OK == sqlite3_open(NULL, &db));
    }

    if (new_db) {
        create_tables();
    }
}

void close_database(void) {
    assert(SQLITE_OK == sqlite3_close(db));
}

bool add_node(const unsigned int rack_no, const unsigned int chassis_no, const char* mac_address, const bool enabled, const char* config_path) {
    if (!check_mac_address(mac_address)) {
        puts("mac");
        return false;
    }

    if (NULL == config_path) {
        puts("config");
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

    bool ret = true;
    char *errstr = NULL;
    if (SQLITE_OK != sqlite3_exec(db, query->str, NULL, NULL, &errstr)) {
        puts(errstr);
        ret = false;
    }

    g_string_free(query, TRUE);
    g_string_free(config_string, TRUE);

    return ret;
}

bool remove_node(const unsigned int rack_no, const unsigned int chassis_no) {
    // begin transaction
    GString *query = g_string_new("begin transaction;");
    assert(NULL != query);

    // delete all of the errors associated with this node
    g_string_append_printf(query,
        "DELETE FROM errors WHERE node_id IN \
            (SELECT DISTINCT id FROM nodes \
                WHERE rack_no = %i AND chassis_no = %i);", rack_no, chassis_no);
    
    // delete the node
    g_string_append_printf(query,
        "DELETE FROM nodes WHERE rack_no = %i AND chassis_no = %i;", rack_no, chassis_no);

    // commit transaction to the database
    g_string_append(query, "commit;");

    bool ret = true;
    char *errstr = NULL;
    if (SQLITE_OK != sqlite3_exec(db, query->str, NULL, NULL, &errstr)) {
        puts(errstr);
        ret = false;
    }

    g_string_free(query, TRUE);

    return ret;
}

bool remove_all_errors(void) {
    const char *query = "DELETE FROM errors;";

    bool ret = true;
    if (SQLITE_OK != sqlite3_exec(db, query, NULL, NULL, NULL)) {
        ret = false;
    }

    return ret;
}

static bool add_error_decoded(const uint32_t rack_no, const uint32_t chassis_no, const int valve_no, const time_t recv_time, const char *msg) {
    GString *query = g_string_new(NULL);
    assert(NULL != query);

    GString *msg_str = fix_string(msg);

    g_string_printf(query,
        "INSERT INTO errors(node_id, recv_time, description, enabled, valve_no) \
            SELECT nodes.id, %li, \"%s\", 1, %i \
                FROM nodes \
                WHERE nodes.rack_no = %i AND nodes.chassis_no = %i;", \
        recv_time, msg_str->str, valve_no, rack_no, chassis_no);

    bool ret = true;
    char *errmsg = NULL;
    if (SQLITE_OK != sqlite3_exec(db, query->str, NULL, NULL, &errmsg)) {
        puts(errmsg);
        ret = false;
    }

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
    const uint32_t rack_num_n = (addr & 0x0000FF00) << 16;
    const uint32_t chassis_num_n = (addr & 0x000000FF) << 24;

    // don't assume host endianness
    const uint32_t rack_num = ntohl(rack_num_n);
    const uint32_t chassis_num = ntohl(chassis_num_n);

    bool ret = false;

    struct tm *time = localtime(&error->recv_time);
    assert(NULL != time);
    char *time_str = asctime(time);
    time = NULL;
    g_free(time);
    assert(NULL != time_str);

    GString *error_msg = g_string_new(time_str);
    assert(NULL != error_msg);

    // remove the year and newline from the time string
    g_string_truncate(error_msg, error_msg->len - 5);

    switch (error->msg.type) {
        case HARD_ERROR_VALVE:
            g_string_append_printf(error_msg, "Hardware Error: %s", 
                error->msg.data.hardware_valve.message->str);

            ret = add_error_decoded(rack_num, chassis_num, 
                error->msg.data.hardware_valve.valve_no,
                error->recv_time, error_msg->str);
            break;

        case HARD_ERROR_OTHER:
            g_string_append_printf(error_msg, "Hardware Error: %s",
                error->msg.data.hardware_other.message->str);

            ret = add_error_decoded(rack_num, chassis_num, -1, 
                error->recv_time, error_msg->str);
            break;

        case SOFT_ERROR:
            g_string_append_printf(error_msg, "Software Error: %s",
                error->msg.data.software.message->str);

            ret = add_error_decoded(rack_num, chassis_num, -1,
                error->recv_time, error_msg->str);
            break;

        default:
            g_string_append_printf(error_msg, "Unknown Error Type: %i",
                error->msg.type);
            break;
    }

    g_string_free(error_msg, TRUE);
    return ret;
}

void free_search_result(gpointer res) {
    if (NULL == res) {
        return;
    }

    SearchResult *result = (SearchResult *) res;
    
    g_free(result->message);
    g_free(result);
}

static GString *clickable_query(const Clickable *search, const char* fields) {
    if (NULL == search) {
        return NULL;
    }

    // construct query
    GString *query = g_string_new("SELECT"); 
    assert(NULL != query);
    g_string_append_printf(query, " %s \
                    FROM errors \
                    INNER JOIN nodes \
                    ON errors.node_id = nodes.id \
                    WHERE nodes.enabled = 1 AND errors.enabled = 1 ", fields); 

    switch(search->type) {
        case ALL:
            break;
        case RACK:
            g_string_append_printf(query, "AND nodes.rack_no = %i", search->rack_num); 
            break;
        case CHASSIS:
            g_string_append_printf(query, "AND nodes.rack_no = %i AND nodes.chassis_no = %i", search->rack_num, search->chassis_num);
            break;
        case VALVE:
            g_string_append_printf(query, "AND nodes.rack_no = %i AND nodes.chassis_no = %i AND errors.valve_no = %i", search->rack_num, search->chassis_num, search->valve_num);
            break;
        default:
            g_string_free(query, TRUE);
            g_print("I don't know how to search for that!\n");
            return NULL;
    }

    return query;
}

GList *search_clickable(const Clickable *search) {
    GString *query = clickable_query(search, "errors.description, nodes.rack_no, nodes.chassis_no, errors.valve_no");
    if (NULL == query) {
        return NULL;
    }
    g_string_append(query, " ORDER BY errors.recv_time;");

    GList *results = NULL; // empty list

    sqlite3_stmt *statement = NULL;
    if (SQLITE_OK != sqlite3_prepare_v2(db, query->str, -1, &statement, NULL)) {
        g_string_free(query, TRUE);
        return NULL;
    }
    g_string_free(query, TRUE);

    int status = SQLITE_ERROR;
    do {
        status = sqlite3_step(statement);
        if (SQLITE_DONE == status) {
            break;
        } else if (SQLITE_ROW != status) {
            assert(SQLITE_OK == sqlite3_finalize(statement));
            puts("Bad sqlite3_step");
            g_list_free_full(results, free_search_result);
            return NULL;
        }
        // status == SQL_ROW so get the data
        SearchResult *res = malloc(sizeof(SearchResult));
        assert(NULL != res);

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpointer-sign"
        res->message = strdup(sqlite3_column_text(statement, 0));
        #pragma GCC diagnostic pop
        assert(NULL != res->message);

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wsign-conversion"
        res->rack_no = sqlite3_column_int(statement, 1);
        res->chassis_no = sqlite3_column_int(statement, 2);
        #pragma GCC diagnostic pop
        res->valve_no = sqlite3_column_int(statement, 3);

        results = g_list_append(results, res);
    } while (true);

    assert(SQLITE_OK == sqlite3_finalize(statement));

    return results;
}    

int count_clickable(const Clickable *search) {
    GString *query = clickable_query(search, "Count(*)");
    if (NULL == query) {
        return -1;
    }
    g_string_append_c(query, ';');

    sqlite3_stmt *statement = NULL;
    if (SQLITE_OK != sqlite3_prepare_v2(db, query->str, -1, &statement, NULL)) {
        g_string_free(query, TRUE);
        return -1;
    }
    g_string_free(query, TRUE);

    // there should only be one row
    if (SQLITE_ROW != sqlite3_step(statement)) {
        assert(SQLITE_OK == sqlite3_finalize(statement));
        return -1;
    }

    int count = sqlite3_column_int(statement, 0);
    assert(SQLITE_OK == sqlite3_finalize(statement));

    return count;
}

GList *list_racks(void) {
    const char* query = "SELECT DISTINCT rack_no FROM nodes;";

    GList *results = NULL; // empty list

    sqlite3_stmt *statement = NULL;
    if (SQLITE_OK != sqlite3_prepare_v2(db, query, -1, &statement, NULL)) {
        puts("Error constructing list_racks query");
        return NULL;
    }

    int status = SQLITE_ERROR;
    do {
        status = sqlite3_step(statement);
        if (SQLITE_DONE == status) {
            break;
        } else if (SQLITE_ROW != status) {
            assert(SQLITE_OK == sqlite3_finalize(statement));
            puts("Bad sqlite3_step list_racks");
            g_list_free(results);
            return NULL;
        }
        // status == SQL_ROW so get data
        gpointer item = (gpointer) sqlite3_column_int64(statement, 0);
        results = g_list_append(results, item);
    } while(true);

    assert(SQLITE_OK == sqlite3_finalize(statement));

    return results;
}

GList *list_chassis_by_rack(const unsigned int rack_no) {
    GString* query = g_string_new(NULL);
    g_string_printf(query, "SELECT DISTINCT chassis_no FROM nodes WHERE rack_no = %i;", rack_no);

    GList *results = NULL; // empty list

    sqlite3_stmt *statement = NULL;
    if (SQLITE_OK != sqlite3_prepare_v2(db, query->str, -1, &statement, NULL)) {
        g_string_free(query, TRUE);
        puts("Error constructing list_racks query");
        return NULL;
    }
    g_string_free(query, TRUE);

    int status = SQLITE_ERROR;
    do {
        status = sqlite3_step(statement);
        if (SQLITE_DONE == status) {
            break;
        } else if (SQLITE_ROW != status) {
            assert(SQLITE_OK == sqlite3_finalize(statement));
            puts("Bad sqlite3_step list_racks");
            g_list_free(results);
            return NULL;
        }
        // status == SQL_ROW so get data
        gpointer item = (gpointer) sqlite3_column_int64(statement, 0);
        results = g_list_append(results, item);
    } while(true);

    assert(SQLITE_OK == sqlite3_finalize(statement));

    return results;
}