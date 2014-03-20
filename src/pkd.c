/*
 * pkd.c -- a sample public-key daemon using libssh
 *
 * Uses public key authentication to establish an
 * exec channel and echo back "hello" to the user.
 *
 * (c) 2014 Jon Simons
 */

#include <errno.h>
#include <stdio.h>

#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <libssh/server.h>

static void usage()
{
    printf("Usage: pkd [ssh-rsa|ssh-dss|ecdsa] <path-to-hostkey>\n");
}

static int init_libssh()
{
    int rc = ssh_threads_set_callbacks(ssh_threads_get_pthread());
    if (rc != SSH_OK) {
        goto out;
    }

    rc = ssh_init();
    if (rc != 0) {
        rc = SSH_ERROR;
        goto out;
    }

out:
    return (rc == SSH_OK) ? 0 : 1;
}

static void cleanup_libssh()
{
    int rc = ssh_finalize();
    if (rc != 0) {
        fprintf(stderr, "ssh_finalize: %d\n", rc);
    }
}

static int server_fd = -1;

static int init_server_fd(short port)
{
    int rc = 0;
    int yes = 1;
    struct sockaddr_in addr;

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        rc = -1;
        goto out;
    }

    rc = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (rc != 0) {
        goto outclose;
    }

    memset(&addr, 0x0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    rc = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc != 0) {
        goto outclose;
    }

    rc = listen(server_fd, 128);
    if (rc == 0) {
        goto out;
    }

outclose:
    close(server_fd);
    server_fd = -1;
out:
    return rc;
}

static int accept_fd()
{
    int fd = -1;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    do {
        fd = accept(server_fd, (struct sockaddr *) &addr, &len);
    } while ((fd < 0) && (errno == EINTR));

    return fd;
}

static int eof_received = 0;

static void pkd_eof(ssh_session session,
                    ssh_channel channel,
                    void *userdata)
{
    printf("pkd_eof\n");
    eof_received = 1;
}

static int close_received = 0;

static void pkd_chan_close(ssh_session session,
                           ssh_channel channel,
                           void *userdata)
{
    printf("pkd_chan_close\n");
    close_received = 1;
}

static int req_exec_received = 0;

static int pkd_req_exec(ssh_session s,
                        ssh_channel c,
                        const char *cmd,
                        void *userdata)
{
    /* assumes pubkey authentication has already succeeded */
    printf("pkd_req_exec\n");
    req_exec_received = 1;
    return 0;
}

static struct ssh_channel_callbacks_struct pkd_channel_cb = {
    .channel_eof_function = pkd_eof,
    .channel_close_function = pkd_chan_close,
    .channel_exec_request_function = pkd_req_exec,
};

static int pkd_auth_pubkey_cb(ssh_session s,
                              const char *user,
                              ssh_key key,
                              char state,
                              void *userdata)
{
    printf("pkd_auth_pubkey_cb state: %d\n", state);
    if ((state == SSH_PUBLICKEY_STATE_NONE) ||
        (state == SSH_PUBLICKEY_STATE_VALID)) {
        return SSH_AUTH_SUCCESS;
    }

    return SSH_AUTH_DENIED;
}

static int pkd_service_request_cb(ssh_session session,
                                  const char *service,
                                  void *userdata)
{
    printf("pkd_service_request_cb: %s\n", service);
    return (0 == (strcmp(service, "ssh-userauth"))) ? 0 : -1;
}

static ssh_channel pkd_channel_openreq_cb(ssh_session s,
                                          void *userdata)
{
    ssh_channel c = NULL;
    ssh_channel *out = (ssh_channel *) userdata;

    /* assumes pubkey authentication has already succeeded */
    printf("pkd_channel_openreq_cb\n");

    c = ssh_channel_new(s);
    if (c == NULL) {
        fprintf(stderr, "ssh_channel_new: %s\n", ssh_get_error(s));
        return NULL;
    }

    ssh_callbacks_init(&pkd_channel_cb);
    pkd_channel_cb.userdata = userdata;
    if (ssh_set_channel_callbacks(c, &pkd_channel_cb) != SSH_OK) {
        fprintf(stderr, "ssh_set_channel_callbacks: %s\n", ssh_get_error(s));
        ssh_channel_free(c);
        c = NULL;
    }

    *out = c;

    return c;
}

static struct ssh_server_callbacks_struct pkd_server_cb = {
    .auth_pubkey_function = pkd_auth_pubkey_cb,
    .service_request_function = pkd_service_request_cb,
    .channel_open_request_session_function = pkd_channel_openreq_cb,
};

static int exec_hello(int fd,
                      const char *hostkeyalgo,
                      const char *hostkeypath)
{
    int rc = -1;
    ssh_bind b = NULL;
    ssh_session s = NULL;
    ssh_event e = NULL;
    ssh_channel c = NULL;
    int level = SSH_LOG_FUNCTIONS;

    eof_received = 0;
    close_received  = 0;
    req_exec_received = 0;

    b = ssh_bind_new();
    if (b == NULL) {
        fprintf(stderr, "ssh_bind_new\n");
        goto out;
    }

    enum ssh_bind_options_e opts;
    if (strcmp("ssh-rsa", hostkeyalgo) == 0) {
        opts = SSH_BIND_OPTIONS_RSAKEY;
    } else if (strcmp("ssh-dss", hostkeyalgo) == 0) {
        opts = SSH_BIND_OPTIONS_DSAKEY;
    } else if (strcmp("ecdsa", hostkeyalgo) == 0) {
        opts = SSH_BIND_OPTIONS_ECDSAKEY;
    } else {
        fprintf(stderr, "unknown algorithm: %s\n", hostkeyalgo);
        rc = -1;
        goto out;
    }

    rc = ssh_bind_options_set(b, opts, hostkeypath);
    if (rc != 0) {
        fprintf(stderr, "ssh_bind_options_set: %s\n", ssh_get_error(b));
        goto out;
    }

    rc = ssh_bind_options_set(b, SSH_BIND_OPTIONS_LOG_VERBOSITY, &level);
    if (rc != 0) {
        fprintf(stderr, "ssh_bind_options_set log verbosity: %s\n",
                        ssh_get_error(b));
        goto out;
    }

    s = ssh_new();
    if (s == NULL) {
        fprintf(stderr, "ssh_new\n");
        goto out;
    }

    /* ssh_bind_accept loads host key as side-effect */
    rc = ssh_bind_accept_fd(b, s, fd);
    if (rc != SSH_OK) {
        fprintf(stderr, "ssh_bind_accept_fd: %s\n", ssh_get_error(b));
        goto out;
    }

    /* accept only publickey-based auth */
    ssh_set_auth_methods(s, SSH_AUTH_METHOD_PUBLICKEY);

    /* initialize callbacks */
    ssh_callbacks_init(&pkd_server_cb);
    pkd_server_cb.userdata = &c;
    rc = ssh_set_server_callbacks(s, &pkd_server_cb);
    if (rc != SSH_OK) {
        fprintf(stderr, "ssh_set_server_callbacks: %s\n", ssh_get_error(s));
        goto out;
    }

    /* first do key exchange */
    rc = ssh_handle_key_exchange(s);
    if (rc != SSH_OK) {
        fprintf(stderr, "ssh_handle_key_exchange: %s\n", ssh_get_error(s));
        goto out;
    }

    /* setup and pump event to carry out exec channel */
    e = ssh_event_new();
    if (e == NULL) {
        fprintf(stderr, "ssh_event_new\n");
        goto out;
    }

    rc = ssh_event_add_session(e, s);
    if (rc != SSH_OK) {
        fprintf(stderr, "ssh_event_add_session\n");
        goto out;
    }

    /* poll until exec channel established */
    while ((rc != SSH_ERROR) && (req_exec_received == 0)) {
        rc = ssh_event_dopoll(e, -1 /* infinite timeout */);
    }

    if (rc == SSH_ERROR) {
        fprintf(stderr, "ssh_event_dopoll\n");
        goto out;
    } else if (c == NULL) {
        fprintf(stderr, "poll loop exited but exec channel not ready\n");
        rc = -1;
        goto out;
    }

    rc = ssh_channel_write(c, "hello\n", 6);
    if (rc != 6) {
        fprintf(stderr, "ssh_channel_write partial (%d)\n", rc);
    }

    rc = ssh_channel_request_send_exit_status(c, 0);
    if (rc != SSH_OK) {
        fprintf(stderr, "ssh_channel_request_send_exit_status: %s\n",
                        ssh_get_error(s));
        goto out;
    }

    rc = ssh_channel_send_eof(c);
    if (rc != SSH_OK) {
        fprintf(stderr, "ssh_channel_send_eof: %s\n",
                        ssh_get_error(s));
        goto out;
    }

    rc = ssh_channel_close(c);
    if (rc != SSH_OK) {
        fprintf(stderr, "ssh_channel_close: %s\n",
                        ssh_get_error(s));
        goto out;
    }

    while (!ssh_channel_is_eof(c) && (eof_received == 0)) {
        rc = ssh_event_dopoll(e, 1000 /* milliseconds */);
        if (rc == SSH_ERROR) {
            fprintf(stderr, "ssh_event_dopoll for remote eof: %s\n",
                            ssh_get_error(s));
            break;
        } else {
            rc = 0;
        }
    }

    while (!ssh_channel_is_closed(c) && (close_received == 0)) {
        rc = ssh_event_dopoll(e, 1000 /* milliseconds */);
        if (rc == SSH_ERROR) {
            fprintf(stderr, "ssh_event_dopoll for remote eof: %s\n",
                            ssh_get_error(s));
            break;
        } else {
            rc = 0;
        }
    }

out:
    if (c != NULL) {
        ssh_channel_free(c);
    }
    if (e != NULL) {
        ssh_event_remove_session(e, s);
        ssh_event_free(e);
    }
    if (s != NULL) {
        ssh_disconnect(s);
        ssh_free(s);
    }
    if (b != NULL) {
        ssh_bind_free(b);
    }
    return rc;
}

int main(int argc, char **argv)
{
    int fd = 0;
    int rc = 0;

    if (argc != 3) {
        usage();
        rc = 1;
        goto out;
    }

    rc = init_libssh();
    if (rc != 0) {
        fprintf(stderr, "init_libssh: %d\n", rc);
        goto out;
    }

    rc = init_server_fd(1234);
    if (rc != 0) {
        fprintf(stderr, "init_server_fd: %d\n", rc);
        goto outlib;
    }

    while (1) {
        fd = accept_fd();
        if (fd < 0) {
            fprintf(stderr, "accept_fd");
            rc = -1;
            break;
        }

        rc = exec_hello(fd, argv[1], argv[2]);
        if (rc != 0) {
            fprintf(stderr, "exec_hello: %d\n", rc);
            break;
        }
    }

    close(server_fd);

outlib:
    cleanup_libssh();
out:
    return rc;
}
