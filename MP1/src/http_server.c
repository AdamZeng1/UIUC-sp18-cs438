#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define BUF_SIZE 8096

/**
 * Global Variables
 */

/**
 * Function declaration
 */
void write_line(int sockfd, char *line);
void *get_in_addr(struct sockaddr *sa);
int setup_listener(char *port);
void handle_connection(int sockfd);
void http_404(int sockfd);
void http_400(int sockfd);
void http_accept(int sockfd, FILE *file);

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: ./http_server <port>\n");
        exit(1);
    }
    char *PORT = strdup(argv[1]);

    /* Varaibles for connection */
    int listener, newfd;
    listener = setup_listener(PORT);

    /* Listen */
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    puts("Waiting for incoming connections...");

    /* Looping to accept new connection */
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    pid_t child_pid;

    while (1) {
        addrlen = sizeof(struct sockaddr_in);
        newfd = accept(listener, (struct sockaddr *)&clientaddr, (socklen_t*)&addrlen);
        if (newfd < 0)
            perror("server: accept failed");

        child_pid = fork();
        if (child_pid < 0) { /* fork error */
            perror("server: fork error");
        } else if (child_pid == 0) { /* child process */
            char *clientIP_l = (char*)malloc(INET_ADDRSTRLEN);
            inet_ntop(AF_INET, get_in_addr((struct sockaddr*)&clientaddr), clientIP_l, INET6_ADDRSTRLEN);
            fprintf(stdout, "new connection from %s\n", clientIP_l);
            handle_connection(newfd);
        }
        /* parent process */
        close(newfd); 
    }


    return 0;
}

void write_line(int sockfd, char *line) {
    if (write(sockfd, line, strlen(line)) < 0) {
        perror("write failed");
    }
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int setup_listener(char *PORT) {
    int listener;
    int rv;
    struct addrinfo hints, *ai, *p;

    /* Binding the socket */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        /* Use the (SO_REUSEADDR | SO_REUSEPORT ) socket option */
        int yes=1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }

    // if we enter this part means we have a fail on bind()
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai); // all done with this
    return listener;
}

void http_404(int sockfd) {
    write_line(sockfd, (char*)"HTTP/1.1 404 Not Found\r\n");
}

void http_400(int sockfd) {
    write_line(sockfd, (char*)"HTTP/1.1 400 Bad Request\r\n");
}

void http_accept(int sockfd, FILE *file) {
    write_line(sockfd, (char*)"HTTP/1.0 200 OK\r\n");
    write_line(sockfd, (char*)"Content-Type: text/html\r\n\r\n");

    char c;
    while ((c = fgetc(file)) && (c != EOF)) {
        write(sockfd, &c, 1);
    }
}

void handle_connection(int sockfd) {
    char msg_buf[BUF_SIZE];
    int ret = read(sockfd, msg_buf, BUF_SIZE);
    if (ret == 0 || ret == -1) {
        fprintf(stderr, "request receive error\n");
        http_400(sockfd);
        exit(3);
    }
    /* Set '\0' to end of msg_buf */
    if (ret > 0 && ret < BUF_SIZE)
        msg_buf[ret] = '\0';
    else
        msg_buf[0] = '\0';

    /* set '\0' to the end of GET.... */
    for (int i = 0; i < ret; i++) {
        if (msg_buf[i] == '\r' || msg_buf[i+1] == '\n') {
            msg_buf[i] = '\0';
        }
    }

    /* Only handle http GET */
    if (strncmp(msg_buf,"GET ",4) && strncmp(msg_buf,"get ",4)) {
        http_400(sockfd);
        exit(3);
    }

    /* Get file name */
    char filename[1000];
    char *ptr = msg_buf + 5;
    int i = 0;
    while (*ptr != ' ') {
        filename[i] = *ptr;
        ptr++;
        i++;
    }
    filename[i] = '\0';
    printf("request %s\n", filename);

    /* Open file */
    FILE *file = fopen(filename, "r");
    if (!file) {
        http_404(sockfd);
        exit(3);
    } else {
        http_accept(sockfd, file);
    }

    close(sockfd);
}
