#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

static int http_connect(char *host, int port);
static int http_send(int sock,char *fmt,...);

static int http_connect(char *host, int port)
{
        int white_sock;
        struct hostent * site;
        struct sockaddr_in me;
        site = gethostbyname(host);
        if(site == NULL) return -2;
        white_sock = socket(AF_INET,SOCK_STREAM, 0);
        if(white_sock < 0) return -1;
        memset(&me, 0, sizeof(struct sockaddr_in));
        memcpy(&me.sin_addr, site->h_addr_list[0], site->h_length);
        me.sin_family = AF_INET;
        me.sin_port = htons(port);
        return (connect(white_sock, (struct sockaddr *)&me, sizeof(struct sockaddr)) < 0) ? -1 : white_sock;
}

static int http_send(int sock,char *fmt,...)
{
        char BUF[1024];
        va_list argptr;
        va_start(argptr, fmt);
        vsprintf(BUF,fmt, argptr);
        va_end(argptr);
        return send(sock, BUF, strlen(BUF), 0);
}


int http_get(char *host, int port, char * query)
{
        int black_sock;

        black_sock = http_connect(host, port);
        if (black_sock < 0) return;

        http_send(black_sock, "GET %s HTTP/1.1\r\n", query, 10);
        http_send(black_sock, "Host: %s\r\n", host, 10);
        http_send(black_sock, "Connection: close\r\n", 10);
        http_send(black_sock, "\r\n", 10);

        return black_sock;
}

int http_post(char *host, int port, char * query, char * data)
{
        int black_sock;
        int len = 0;

        black_sock = http_connect(host, port);
        if (black_sock < 0) return;
        len = strlen(data);
        http_send(black_sock, "POST %s HTTP/1.1\r\n", query, 10);
        http_send(black_sock, "Host: %s\r\n", host, 10);
        http_send(black_sock, "Content-Length: %d\r\n", len, 10);
        http_send(black_sock, "Connection: close\r\n", 10);
        http_send(black_sock, "\r\n", 10);
        http_send(black_sock, "%s", data, 10);

        return black_sock;
}
