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

int connect_TCP(char *addr, char *port);
int TCP_handshake(int sockfd, char* username);
int recv_msg(int sockfd);
int close_TCP(int sockfd);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

int main(int argc, char *argv[]) 
{
    char *hostname, *port, *username;
    int sock_fd;

    if (argc != 4) {
        fprintf(stderr, "usage: ./mp0client <hostname> <port> <username\n");
        exit(1);
    }

    hostname = malloc(sizeof(char));
    port = malloc(sizeof(char));
    username = malloc(sizeof(char));
    strcpy( hostname, argv[1]);
    strcpy( port, argv[2]);
    strcpy( username, argv[3]);

    if ((sock_fd = connect_TCP(hostname, port)) < 0) {
        exit(1);
    }
    
    // handshake
    if (TCP_handshake(sock_fd, username) < 0) {
        fprintf(stderr, "TCP handshake error\n");
        exit(1);
    }

    // receive 10 message
    for (int i = 0; i < 10; i++) {
        if (recv_msg(sock_fd) < 0) {
            fprintf(stderr, "recv message error\n");
            exit(1);
        }
    }

    // close connection
    if (close_TCP(sock_fd) < 0) {
        fprintf(stderr, "close connection error\n");
        exit(1);
    }

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

int TCP_handshake(int sockfd, char* username) 
{
    char msg[MAXSIZE];

    // write
    strcpy(msg, "HELO\n");
    if (write(sockfd, msg, strlen(msg)) <= 0) {
        perror("write error");
        return -1;
    }

    // wait for "100 - OK\n"
    if (read(sockfd, msg, MAXSIZE) <= 0) {
        perror("read error");
        return -1;
    }
    if (strncmp(msg, "100 - OK", 8) != 0) {
        perror("server reject");
        return -1;
    }

    // write
    sprintf(msg, "USERNAME %s\n", username);
    if (write(sockfd, msg, strlen(msg)) <= 0) {
        perror("write error");
        return -1;
    }

    // wait for "100 - OK\n"
    if (read(sockfd, msg, MAXSIZE) <= 0) {
        perror("read error");
        return -1;
    }
    if (strncmp(msg, "200", 3) != 0) {
        perror("username error");
        return -1;
    }

    return 1;
}

int recv_msg(int sockfd) 
{
    char msg[MAXSIZE];
    char *recv_msg;

    // write
    strcpy(msg, "RECV\n");
    if (write(sockfd, msg, strlen(msg)) <= 0) {
        perror("write error");
        return -1;
    }

    // wait for "100 - OK\n"
    if (read(sockfd, msg, MAXSIZE) < 0) {
        perror("read error");
        return -1;
    }
    if (strncmp(msg, "300", 3) != 0) {
        perror("sentence read error");
        return -1;
    }
    recv_msg = msg+12;
    printf("Received: ");
    while (*recv_msg != '\n') {
        printf("%c", *recv_msg);
        recv_msg++;
    }
    printf("\n");

    return 1;
}

int close_TCP(int sockfd) 
{
    char msg[MAXSIZE];

    // write
    strcpy(msg, "BYE\n");
    if (write(sockfd, msg, strlen(msg)) <= 0) {
        perror("write error");
        return -1;
    }

    // wait for "400\n"
    if (read(sockfd, msg, MAXSIZE) <= 0) {
        perror("read error");
        return -1;
    }
    if (strncmp(msg, "400", 3) != 0) {
        perror("BYE error");
        return -1;
    }
    close(sockfd);

    return 1;
}
