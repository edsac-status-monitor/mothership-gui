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

// functions

int main(void) {
    const unsigned int test_num = 1876142908;
    const char* correct_mac = "ff:ff:ff:ff:ff:ff";
    init_database();

    // add and remove a valid node
    assert(true == add_node(test_num, test_num, correct_mac, true, "myconfig"));
    assert(true == remove_node(test_num, test_num));

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

    close_database();
}
