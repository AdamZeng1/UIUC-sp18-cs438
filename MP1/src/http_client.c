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

#define BUF_SIZE 1024

/**
 * Global Variables
 */
char url[2500], file_path[2500];
int port;
FILE *output;

/**
 * Function declaration
 */

void *get_in_addr(struct sockaddr *sa);
int connect_TCP(char *addr, char *port);
void parse_url(char *input);
void send_http_request(int sockfd);
void write_line(int sockfd, char *line);
void http_client(int sockfd, char *input);
int readline (int fd, char* ptr, int maxlen);

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: ./http_client <url>\n");
        exit(1);
    }
    int sock_fd;
    char port_a[8];

    parse_url(argv[1]);
    sprintf(port_a, "%d", port);
    if ((sock_fd = connect_TCP(url, port_a)) < 0) {
        exit(1);
    }
    output = fopen("output", "w");

    send_http_request(sock_fd);
    char buff[BUF_SIZE];
    int redirect_flag = 0;
    int write_flag = 0;
    while (readline(sock_fd, buff, BUF_SIZE) > 0) {
        if (!redirect_flag) {
            if (strncmp(buff, "HTTP/1.1 301", 12) == 0 || strncmp(buff, "HTTP/1.0 301", 12) == 0) {
                redirect_flag = 1;
                printf("%s", buff);
            } else {
                if (write_flag) {
                    printf("%s", buff);
                    fprintf(output, "%s", buff);
                } else {
                    printf("%s", buff);
                }
                if (!write_flag && strncmp(buff, "\r\n", 2) == 0) {
                    write_flag = 1;
                }
            }
        } else {
            if (strncmp(buff, "Location", 8) == 0) {
                printf("%s", buff);
                int n = 0;
                int i = 10;
                char new_url[2000];
                while (buff[i] != '\r' && buff[i] != '\n') {
                    new_url[n] = buff[i];
                    i++;
                    n++;
                }
                new_url[n] = '\0';
                http_client(sock_fd, new_url);
                break;
            } else {
                printf("%s", buff);
            }
        }
    }


    close(sock_fd);
    fclose(output);

    return 0;
}

void http_client(int sock_fd, char *input) {
    parse_url(input);
    char port_a[20];
    sprintf(port_a, "%d", port);
    if ((sock_fd = connect_TCP(url, port_a)) < 0) {
        exit(1);
    }

    send_http_request(sock_fd);
    char buff[BUF_SIZE];
    int redirect_flag = 0;
    int write_flag = 0;
    while (readline(sock_fd, buff, BUF_SIZE) > 0) {
        if (!redirect_flag) {
            if (strncmp(buff, "HTTP/1.1 301", 12) == 0 || strncmp(buff, "HTTP/1.0 301", 12) == 0) {
                redirect_flag = 1;
                if (write_flag) {
                    printf("%s", buff);
                    fprintf(output, "%s", buff);
                } else {
                    printf("%s", buff);
                }
                if (!write_flag && strncmp(buff, "\r\n", 2) == 0) {
                    write_flag = 1;
                }
            } else {
            }
        } else {
            if (strncmp(buff, "Location", 8) == 0) {
                printf("%s", buff);
                int n = 0;
                int i = 10;
                char new_url[2000];
                while (buff[i] != '\r' && buff[i] != '\n') {
                    new_url[n] = buff[i];
                    i++;
                    n++;
                }
                new_url[n] = '\0';
                http_client(sock_fd, new_url);
                break;
            } else {
                printf("%s", buff);
            }
        }
    }
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void write_line(int sockfd, char *line) {
    if (write(sockfd, line, strlen(line) + 1) < 0) {
        perror("write failed");
    }
}

int readline (int fd, char* ptr, int maxlen) {
    int n, rc;
    char c;
    for (n = 1; n < maxlen; n++) {
        if ((rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n') {
                break;
            }
        } else if (rc == 0) {
            if (n == 1)
                return 0;
            else
                break;      // EOF, no data read
        } else
            return(-1);     // EOF, some data was read
    }

    *ptr = 0;
    return(n);
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
    *url = '\0';
    *file_path = '/';
    file_path[1] = '\0';
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

    if (strlen(file_path) == 1) {
        strcpy(file_path, "");
    }

    if (strncmp(url, "http", 4) == 0) {
        int i = 0;
        for(; i < (int)strlen(url)-7; i++) {
            url[i] = url[i + 7];
        }
        url[i] = '\0';
    }

}

void send_http_request(int sockfd) 
{
    //sockfd = 1;
    char tmp[3000];
    sprintf(tmp, "GET /%s HTTP/1.0\r\n\r\n", file_path);
    write_line(sockfd, tmp);
    //sprintf(tmp, "Host: %s:%d\r\n", url, port);
    //write_line(sockfd, tmp);
    //write_line(sockfd, "Accept: */*\r\n");
    //write_line(sockfd, "Keep-Alive: 300\r\n");
    //write_line(sockfd, "Connection: Keep-Alive\r\n\r\n");
    //free(tmp);
}
