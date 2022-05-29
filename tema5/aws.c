#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libaio.h>
#include <errno.h>

#include "util.h"
#include "debug.h"
#include "sock_util.h"
#include "w_epoll.h"
#include "aws.h"
#include "http_parser.h"

#define ECHO_LISTEN_PORT		42424

char invalid_header[] = "HTTP/1.0 404 Not Found\r\n"
		"Date: Sun, 08 May 2011 09:26:16 GMT\r\n"
		"Last-Modified: Mon, 02 Aug 2010 17:55:28 GMT\r\n"
		"Accept-Ranges: bytes\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n";

char static_header[] = "HTTP/1.0 200 OK\r\n"
    "Date: Sun, 08 May 2011 09:26:16 GMT\r\n"
    "Server: Apache/2.2.9\r\n"
    "Last-Modified: Mon, 02 Aug 2010 17:55:28 GMT\r\n"
    "Accept-Ranges: bytes\r\n"
    "Vary: Accept-Encoding\r\n"
    "Connection: close\r\n"
    "Content-Type: text/html\r\n";

/* server socket file descriptor */
static int listenfd;

/* epoll file descriptor */
static int epollfd;

enum connection_state {
    STATE_RECEIVED_REQUEST,
	STATE_SENDING_REQUEST,
    STATE_SENT_REQUEST,
	STATE_CONNECTION_CLOSED
};

enum transfer_type {
    UNKNOWN,
	STATIC,
	DYNAMIC
};

/* structure acting as a connection handler */
struct connection {
	int sockfd;
	char recv_buffer[BUFSIZ];
	size_t recv_len;
	char send_buffer[BUFSIZ];
	off_t send_len;
	enum connection_state state;

    enum transfer_type transfer;
    int fd;
    off_t file_size;
    char path[BUFSIZ];
    size_t path_len;
};

size_t build_request_error_header(char request_error_header[BUFSIZ]) {
    strncpy(request_error_header, invalid_header, strlen(invalid_header));
    return strlen(request_error_header);
}

size_t build_request_static_header(char request_static_header[BUFSIZ], struct connection *conn) {
    conn->fd = open(conn->path + 1, O_RDONLY | O_NONBLOCK, 0644);
    if (conn->fd == -1) {
        strncpy(request_static_header, invalid_header, strlen(invalid_header));
        return strlen(invalid_header);
    }

    conn->file_size = lseek(conn->fd, 0L, SEEK_END);
    lseek(conn->fd, 0L, SEEK_SET);

    strncpy(request_static_header, static_header, strlen(static_header));
    sprintf(request_static_header + strlen(request_static_header), "Content-Length: %ld\r\n\r\n", conn->file_size);

    return strlen(request_static_header);
}

static int on_path_cb(http_parser *p, const char *buf, size_t len)
{   
    struct connection *conn = (struct connection *)p->data;
    memcpy(conn->path, buf, len);
    conn->path_len = len;

    if (strstr(conn->path, AWS_REL_STATIC_FOLDER)) {
        conn->transfer = STATIC;
    } else if (strstr(conn->path, AWS_REL_DYNAMIC_FOLDER)) {
        conn->transfer = DYNAMIC;
    }

	return 0;
}

static http_parser_settings settings_on_path = {
	/* on_message_begin */ 0,
	/* on_header_field */ 0,
	/* on_header_value */ 0,
	/* on_path */ on_path_cb,
	/* on_url */ 0,
	/* on_fragment */ 0,
	/* on_query_string */ 0,
	/* on_body */ 0,
	/* on_headers_complete */ 0,
	/* on_message_complete */ 0
};
/*
 * Initialize connection structure on given socket.
 */

static struct connection *connection_create(int sockfd)
{
	struct connection *conn = calloc(1, sizeof(*conn));

	DIE(conn == NULL, "malloc");

	conn->sockfd = sockfd;
    conn->transfer = UNKNOWN;

	return conn;
}

/*
 * Remove connection handler.
 */

static void connection_remove(struct connection *conn)
{
	close(conn->sockfd);
	conn->state = STATE_CONNECTION_CLOSED;
	free(conn);
}

/*
 * Handle a new connection request on the server socket.
 */

static void handle_new_connection(void)
{
	static int sockfd;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	struct connection *conn;
	int rc;

	/* accept new connection */
	sockfd = accept(listenfd, (SSA *) &addr, &addrlen);
	DIE(sockfd < 0, "accept");

	dlog(LOG_ERR, "Accepted connection from: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    
    int socket_flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, socket_flags | O_NONBLOCK);

	/* instantiate new connection handler */
	conn = connection_create(sockfd);

	/* add socket to epoll */
	rc = w_epoll_add_ptr_in(epollfd, sockfd, conn);
	DIE(rc < 0, "w_epoll_add_in");
}

/*
 * Receive message on socket.
 * Store message in recv_buffer in struct connection.
 */

static enum connection_state receive_message(struct connection *conn)
{
	ssize_t bytes_recv;
	int rc;
	char abuffer[64];

	rc = get_peer_address(conn->sockfd, abuffer, 64);
	if (rc < 0) {
		ERR("get_peer_address");
		goto remove_connection;
	}

    do {
        bytes_recv = recv(conn->sockfd, conn->recv_buffer + conn->recv_len, BUFSIZ, 0);
        conn->recv_len += bytes_recv;
    } while (errno != EAGAIN && bytes_recv != 0 && bytes_recv != -1);

    conn->state = STATE_RECEIVED_REQUEST;
    return STATE_RECEIVED_REQUEST;

remove_connection:
	rc = w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
	DIE(rc < 0, "w_epoll_remove_ptr");

	/* remove current connection */
	connection_remove(conn);

	return STATE_CONNECTION_CLOSED;
}

/*
 * Send message on socket.
 * Store message in send_buffer in struct connection.
 */

static enum connection_state send_remaining_message(struct connection *conn)
{
	int rc;
	char abuffer[64];

	rc = get_peer_address(conn->sockfd, abuffer, 64);
	if (rc < 0) {
		ERR("get_peer_address");
		goto remove_connection;
	}

    if (conn->transfer == STATIC && conn->state == STATE_SENDING_REQUEST) {
        do {
            rc = sendfile(conn->sockfd, conn->fd, &conn->send_len, conn->file_size - conn->send_len);
        } while (rc != -1 && errno != EAGAIN && conn->file_size != conn->send_len);

        if (conn->send_len != conn->file_size) {
            return STATE_SENDING_REQUEST;
        } else {
            conn->state = STATE_SENT_REQUEST;
            rc = w_epoll_update_ptr_in(epollfd, conn->sockfd, conn);
            DIE(rc < 0, "w_epoll_remove_ptr");

            rc = w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
            DIE(rc < 0, "w_epoll_remove_ptr");
            connection_remove(conn);
        }
    } 

	return STATE_SENT_REQUEST;

remove_connection:
	rc = w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
	DIE(rc < 0, "w_epoll_remove_ptr");

	/* remove current connection */
	connection_remove(conn);

	return STATE_CONNECTION_CLOSED;
}

/*
 * Handle a client request on a client connection.
 */

static void handle_client_request(struct connection *conn)
{
    int rc;
	size_t header_size;
	enum connection_state ret_state;

	ret_state = receive_message(conn);
    if (ret_state == STATE_RECEIVED_REQUEST) {
        http_parser request_parser;
        http_parser_init(&request_parser, HTTP_REQUEST);
        request_parser.data = conn;
        http_parser_execute(&request_parser, &settings_on_path, conn->recv_buffer, conn->recv_len);
        if (conn->transfer == UNKNOWN) {
            char request_error_header[BUFSIZ] = {0};
            header_size = build_request_error_header(request_error_header);

            send(conn->sockfd, request_error_header, header_size, 0);

            rc = w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
	        DIE(rc < 0, "w_epoll_remove_ptr");
            connection_remove(conn);
        } else if (conn->transfer == STATIC) {
            char request_static_header[BUFSIZ] = {0};
            header_size = build_request_static_header(request_static_header, conn);

            conn->state = STATE_SENDING_REQUEST;
            send(conn->sockfd, request_static_header, header_size, 0);

            do {
                rc = sendfile(conn->sockfd, conn->fd, &conn->send_len, conn->file_size - conn->send_len);
            } while (rc != -1 && errno != EAGAIN && conn->file_size != conn->send_len);

            if (conn->send_len != conn->file_size) {
                rc = w_epoll_add_ptr_out(epollfd, conn->sockfd, conn);
                DIE(rc < 0, "w_epoll_add_ptr_out");
            } else {
                conn->state = STATE_SENT_REQUEST;
                rc = w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
                DIE(rc < 0, "w_epoll_remove_ptr");
                connection_remove(conn);
            }
            
        } else {
            rc = w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
	        DIE(rc < 0, "w_epoll_remove_ptr");
            connection_remove(conn);
        }
    }
}

int main(void)
{
	int rc;

	epollfd = w_epoll_create();
	DIE(epollfd < 0, "w_epoll_create");

	listenfd = tcp_create_listener(AWS_LISTEN_PORT, DEFAULT_LISTEN_BACKLOG);
	DIE(listenfd < 0, "tcp_create_listener");

	rc = w_epoll_add_fd_in(epollfd, listenfd);
	DIE(rc < 0, "w_epoll_add_fd_in");

	dlog(LOG_INFO, "Server waiting for connections on port %d\n", AWS_LISTEN_PORT);

	/* server main loop */
	while (1) {
		struct epoll_event rev;

		/* wait for events */
		rc = w_epoll_wait_infinite(epollfd, &rev);
		DIE(rc < 0, "w_epoll_wait_infinite");

		if (rev.data.fd == listenfd) {
			dlog(LOG_DEBUG, "New connection\n");
			if (rev.events & EPOLLIN)
				handle_new_connection();
		} else {
			if (rev.events & EPOLLIN) {
				dlog(LOG_DEBUG, "New message\n");
				handle_client_request(rev.data.ptr);
			}
			if (rev.events & EPOLLOUT) {
				dlog(LOG_DEBUG, "Sending remaining message\n");
				send_remaining_message(rev.data.ptr);
			}
		}
	}

	return 0;
}
