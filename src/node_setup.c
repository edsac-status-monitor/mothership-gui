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
#include <libgen.h>

static const char subnet[] = "172.16";
static const char user[] = "pi"; // user used when logging in over ssh
static const char term[] = "/usr/bin/gnome-terminal --";

// command must not use single quotes
// obviously use with care
static bool run_as_root(const char *command) {
    assert(NULL != command);
    fprintf(stderr, "Running as root: %s\n", command);

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

// append text to file
// text shouldn't contain quotes
static char *append_to_file(const char *path, const char *text) {
    assert(NULL != path);
    assert(NULL != text);

    GString *command = g_string_new(NULL);
    assert(NULL != command);

    g_string_append_printf(command, "/bin/echo \"%s\" >> \"%s\"", text, path);
    
    char * ret = g_string_free(command, FALSE);

    return ret;
}

bool setup_node_network(const unsigned int rack_no, const unsigned int chassis_no, __attribute__((unused)) const char *mac_addr) {
    // remove this IP address and hostname from .ssh/known_hosts
    GString *known_hosts = g_string_new(NULL);
    assert(NULL != known_hosts);
    g_string_append_printf(known_hosts, "ssh-keygen -f ~/.ssh/known_hosts -R %s.%i.%i; ssh-keygen -f ~/.ssh/known_hosts -R node%i-%i;", subnet, rack_no, chassis_no, rack_no, chassis_no);

    int known_hosts_ret = system(known_hosts->str);
    g_string_free(known_hosts, TRUE);
    known_hosts = NULL;

    // this is just a precaution and won't be neccesary in most cases so ignore the error code so long as system() itself is not upset
    if (-1 == known_hosts_ret) {
        perror("system known_hosts");
        return false;
    }

    // add the node to /etc/hosts
    GString *hosts_str = g_string_new(NULL);
    assert(NULL != hosts_str);
    g_string_append_printf(hosts_str, "%s.%i.%i\tnode%i-%i", subnet, rack_no, chassis_no, rack_no, chassis_no);

    char *hosts_ret = append_to_file("/etc/hosts", hosts_str->str);
    g_string_free(hosts_str, TRUE);
    assert(NULL != hosts_ret);

    // add DHCP entry for the node
    GString *dhcp_str = g_string_new(NULL);
    assert(NULL != dhcp_str);
    g_string_append_printf(dhcp_str, "static_lease %s %s.%i.%i", mac_addr, subnet, rack_no, chassis_no);

    char *dhcp_ret = append_to_file("/etc/udhcpd.conf", dhcp_str->str);
    g_string_free(dhcp_str, TRUE);
    assert(NULL != dhcp_ret);

    // restart udhcpd
    const char restart[] = "/bin/systemctl restart udhcpd.service";

    // combine the commands
    GString *combined = g_string_new(hosts_ret);
    assert(NULL != combined);
    g_string_append_printf(combined, " && %s && %s", dhcp_ret, restart);

    bool ret = run_as_root(combined->str);

    g_string_free(combined, TRUE);
    g_free(hosts_ret);
    g_free(dhcp_ret);

    return ret;
}

// pretty dangerous function. Think carefully about the contents of file
static char *revert_file(const char *file, unsigned int rack_no, unsigned int chassis_no) {
    GString *command = g_string_new(NULL);
    assert(NULL != command);

    g_string_append_printf(command, "bash -c \"cat %s | grep -v %s.%i.%i > %s_tmp && mv %s_tmp %s\"", 
        file, subnet, rack_no, chassis_no, file, file, file);

    return g_string_free(command, FALSE);
}

void node_cleanup_network(const unsigned int rack_no, const unsigned int chassis_no) {
    char *hosts_str = revert_file("/etc/hosts", rack_no, chassis_no);
    assert(NULL != hosts_str);
    char *dhcp_str = revert_file("/etc/udhcpd.conf", rack_no, chassis_no);
    assert(NULL != dhcp_str);

    GString *combined = g_string_new(hosts_str);
    assert(NULL != combined);
    g_string_append_printf(combined, " && %s && /bin/systemctl restart udhcpd.service", dhcp_str);

    // ignore success code for usability: it does not matter that much if this cleanup succeeds
    run_as_root(combined->str);

    g_free(hosts_str);
    g_free(dhcp_str);
    g_string_free(combined, TRUE);
}

bool copy_file(const unsigned int rack_no, const unsigned int chassis_no, const char *src, const char *dest) {
    assert(NULL != src);
    assert(NULL != dest);

    GString *command = g_string_new(NULL);
    assert(NULL != command);

    g_string_printf(command, "%s /usr/bin/scp '%s' %s@%s.%i.%i:%s", term, src, user, subnet, rack_no, chassis_no, dest);

    int res = system(command->str);

    g_string_free(command, TRUE);

    if (EXIT_SUCCESS == res) {
        return true;
    }
    return false;
}

bool run_remote_command(const unsigned int rack_no, const unsigned int chassis_no, const char *cmd) {
    assert(NULL != cmd);

    GString *command = g_string_new(NULL);
    assert(NULL != command);

    g_string_printf(command, "%s /usr/bin/ssh %s@%s.%i.%i '%s'", term, user, subnet, rack_no, chassis_no, cmd);

    int res = system(command->str);
    g_string_free(command, TRUE);

    if (EXIT_SUCCESS == res) {
        return true;
    }
    return false;
}


bool copy_and_extract_archive(const unsigned int rack_no, const unsigned int chassis_no, const char *archive, const char *dest) {
    assert(NULL != archive);

    GString *mkdir = g_string_new(NULL);
    assert(NULL != mkdir);

    g_string_printf(mkdir, "mkdir -p %s", dest);
    run_remote_command(rack_no, chassis_no, mkdir->str);

    g_string_free(mkdir, TRUE);
    mkdir = NULL;

    if (!copy_file(rack_no, chassis_no, archive, dest)) {
        return false;
    }

    GString *archive_copy = g_string_new(archive);
    assert(NULL != archive_copy);

    const char *archive_name = basename(archive_copy->str);
    assert(NULL != archive_name);

    GString *command = g_string_new(NULL);
    assert(NULL != command);

    g_string_printf(command, "cd %s && tar -xf %s && rm %s", dest, archive_name, archive_name);

    bool ret = run_remote_command(rack_no, chassis_no, command->str);

    g_string_free(archive_copy, TRUE);
    g_string_free(command, TRUE);

    return ret;
}

bool setup_node_ssh(const unsigned int rack_no, const unsigned int chassis_no, const char *conf_archive) {
    return copy_and_extract_archive(rack_no, chassis_no, conf_archive, "/home/pi/edsac");
}
