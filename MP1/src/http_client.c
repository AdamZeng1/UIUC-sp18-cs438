#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define MAXSIZE 512

/* 
 * Global Variables
 */
char *url, *file_path;
int port;

/*
 * Function declaration
 */

void *get_in_addr(struct sockaddr *sa);
int connect_TCP(char *addr, char *port);
void parse_url(char *input);

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: ./http_client <url>\n");
        exit(1);
    }

    parse_url(argv[1]);



    free(url);
    free(file_path);
    return 0;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int connect_TCP(char *addr, char *port) 
{
    int sock_fd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char add_s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sock_fd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), add_s, sizeof add_s);
    freeaddrinfo(servinfo);

    return sock_fd;
}

void parse_url(char *input) 
{
    url = malloc(sizeof(char) * 2500);
    file_path = malloc(sizeof(char) * 2500);
    *url = '\0';
    *file_path = '\0';
    port = 80;

    int flag = 0;
    int http_flag = 0;
    int http_slash = 0;

    char *ptr_i = input;
    char *ptr = url;
    while (*ptr_i) {
        if (flag == 0) {
            if (*ptr_i == ':') {
                if (strncmp(url, "http", 4) == 0 && http_flag == 0) {
                    http_flag = 1;
                    *ptr = *ptr_i;
                    ptr++;
                } else {
                    flag = 1;
                    port = 0;
                    *ptr = '\0';
                }
            } else if (*ptr_i == '/') {
                if (strncmp(url, "http", 4) == 0 && http_slash < 2) {
                    http_slash++;
                    *ptr = *ptr_i;
                    ptr++;
                } else {
                    *ptr = '\0';
                    ptr = file_path;
                }
            } else {
                *ptr = *ptr_i;
                ptr++;
            }
        } else if (flag == 1) {
            if (*ptr_i == '/') {
                flag = 2;
                ptr = file_path;
            } else {
                port = port * 10 + *ptr_i - '0';
            }
        } else {
            *ptr = *ptr_i;
            ptr++;
        }
        ptr_i++;
    }

    if (strlen(file_path) == 0) {
        strcpy(file_path, "/");
    }

    printf("%s\n%d\n%s\n", url, port, file_path);
}
