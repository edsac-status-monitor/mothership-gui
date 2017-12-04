/*
 * Copyright 2017
 * GPL3 Licensed
 * node_setup.c
 * Functions relating to the setup of nodes
 */

// includes
#include "config.h"
#include "node_setup.h"
#include <glib.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static const char subnet[] = "172.16";

// command must not use single quotes
// obviously use with care
static bool run_as_root(const char *command) {
    assert(NULL != command);
    fprintf(stderr, "Running %s as root\n", command);

    // pkexec requires GNOME
    GString *exec = g_string_new("/usr/bin/pkexec --user root /bin/bash -c ");
    assert(NULL != exec);

    g_string_append_printf(exec, "'%s'", command);

    int ret = system(exec->str);

    g_string_free(exec, TRUE);

    if (EXIT_SUCCESS == ret) {
        return true;
    }

    perror("run_as_root");
    return false;
}

// append text to file as root
// obviously use with care
// text shouldn't contain quotes
static bool append_to_file_root(const char *path, const char *text) {
    assert(NULL != path);
    assert(NULL != text);

    GString *command = g_string_new(NULL);
    assert(NULL != command);

    g_string_append_printf(command, "/bin/echo \"%s\" >> \"%s\"", text, path);
    
    bool ret = run_as_root(command->str);
    g_string_free(command, TRUE);

    return ret;
}

bool setup_node(const unsigned int rack_no, const unsigned int chassis_no, __attribute__((unused)) const char *mac_addr) {
    // add the node to /etc/hosts
    GString *hosts_str = g_string_new(NULL);
    assert(NULL != hosts_str);
    g_string_append_printf(hosts_str, "%s.%i.%i\tnode%i-%i", subnet, rack_no, chassis_no, rack_no, chassis_no);

    bool hosts_ret = append_to_file_root("/etc/hosts", hosts_str->str);
    g_string_free(hosts_str, TRUE);
    if (!hosts_ret) {
        return false;
    }

    // add DHCP entry for the node
    GString *dhcp_str = g_string_new(NULL);
    assert(NULL != dhcp_str);
    g_string_append_printf(dhcp_str, "static_lease %s %s.%i.%i", mac_addr, subnet, rack_no, chassis_no);

    bool dhcp_ret = append_to_file_root("/etc/udhcpd.conf", dhcp_str->str);
    g_string_free(dhcp_str, TRUE);
    if (!dhcp_ret) {
        return false;
    }

    // restart udhcpd
    return run_as_root("/bin/systemctl restart udhcpd.service");
}

// pretty dangerous function. Think carefully about the contents of file
static void revert_file(const char *file, unsigned int rack_no, unsigned int chassis_no) {
    GString *command = g_string_new(NULL);
    assert(NULL != command);

    g_string_append_printf(command, "bash -c \"cat %s | grep -v %s.%i.%i > %s_tmp && mv %s_tmp %s\"", 
        file, subnet, rack_no, chassis_no, file, file, file);

    // don't assert result because the user might cancel 
    run_as_root(command->str);
}

void node_cleanup(const unsigned int rack_no, const unsigned int chassis_no) {
    revert_file("/etc/hosts", rack_no, chassis_no);
    revert_file("/etc/udhcpd.conf", rack_no, chassis_no);
    run_as_root("/bin/systemctl restart udhcpd.service");
}

// functions

