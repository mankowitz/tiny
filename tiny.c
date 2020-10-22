/* $begin tinymain */
/*
 * tiny.c - proxy based on tiny.c
 */
#include "csapp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define HTTPPORT 80

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);

void server()
{
    //open a file
    FILE *html_data;
    html_data = fopen("proxy.c", "r");

    char response_data[1024];
    fgets(response_data, 1024, html_data);

    char http_header[2048] = "HTTP/1.0 200 OK\r\n\n";
    strcat(http_header, response_data);

    //create a socket
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    //define the address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8001);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    listen(server_socket, 5);

    int client_socket;
    while (1)
    {
        client_socket = accept(server_socket, NULL, NULL);
        send(client_socket, http_header, sizeof(http_header), 0);
        close(client_socket);
    }
}

void client(int port, char *ip_name, char *page, char *query, int fd)
{
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // This is getting uglier and uglier......
    struct hostent *he;
    he = gethostbyname(ip_name);
    if (he == NULL)
    {
        printf("what knind fo name is %s?", ip_name);
    }
    struct in_addr **addr_list;
    addr_list = (struct in_addr **)he->h_addr_list;
    printf("%s translated to  %s\n", ip_name, inet_ntoa(*addr_list[0]));

    //connect to an addresss
    struct sockaddr_in remote_address;
    remote_address.sin_family = AF_INET;
    //  remote_address.sin_port = htons(HTTPPORT);
    remote_address.sin_port = htons(port);

    // this could be much cleaner, like maybe remote_address.sin_addr.s_addr = addr_list[0]
    inet_aton(inet_ntoa(*addr_list[0]), &remote_address.sin_addr.s_addr);

    //connect to socket:
    connect(client_socket, (struct sockaddr *)&remote_address, sizeof(remote_address));
    printf("connected\n");

    char get[1024];
    sprintf(get, "GET /%s%s HTTP/1.0", page, query);
    char host[1024];
    char *user_agent = "User-Agent: Mankrosoft Internet Imploder (compatible: with_nothing)";
    sprintf(host, "Host: %s", ip_name);
    char *accept_lang = "Accept-Language: en-us";
    char *connection_close = "Connection: close";

    char request[8192];
    sprintf(request, "%s\r\n%s\r\n%s\r\n%s\r\n\r\n", get, host, accept_lang, connection_close);

    printf("sending req\n%s", request);

    send(client_socket, request, sizeof(request), 0);

    char response[10000];
    recv(client_socket, &response, sizeof(response), 0);

    Rio_writen(fd, response, strlen(response)); //line:netp:servestatic:endserve

    printf("aint xlosed yet\n");
    close(client_socket);
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);  //line:netp:tiny:doit
        Close(connfd); //line:netp:tiny:close
    }
}
/* $end tinymain */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) //line:netp:doit:readrequest
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version); //line:netp:doit:parserequest
    if (strcasecmp(method, "GET"))
    { //line:netp:doit:beginrequesterr
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    } //line:netp:doit:endrequesterr
      //    read_requesthdrs(&rio);                              //line:netp:doit:readrequesthdrs

    /* Parse URI from GET request */

    char verb[100];
    char address[100];
    int port = 80;
    char ip[100];
    char page[100];
    char query[100];
    sscanf(uri, "%[^:]://%[^/]/%[^?]%s", verb, address, page, query);
    if (strstr(address, ":"))
    {
        sscanf(address, "%[^:]:%d", ip, &port);
    }
    else
    {
        port = 80;
        strcpy(ip, address);
    }
    printf("verb = \"%s\"\n", verb);
    printf("ip = \"%s\"\n", ip);
    printf("port = \"%d\"\n", port);
    printf("page = \"%s\"\n", page);
    printf("query = \"%s\"\n", query);

    client(port, ip, page, query, fd);

    printf("are you here yet?\n");

    return;

    if (stat(filename, &sbuf) < 0)
    { //line:netp:doit:beginnotfound
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    } //line:netp:doit:endnotfound

    if (is_static)
    { /* Serve static content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
        { //line:netp:doit:readable
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size); //line:netp:doit:servestatic
    }
    else
    { /* Serve dynamic content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
        { //line:netp:doit:executable
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs); //line:netp:doit:servedynamic
    }
}
/* $end doit */

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while (strcmp(buf, "\r\n"))
    { //line:netp:readhdrs:checkterm
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if (!strstr(uri, "cgi-bin"))
    { /* Static content */                 //line:netp:parseuri:isstatic
        strcpy(cgiargs, "");               //line:netp:parseuri:clearcgi
        strcpy(filename, ".");             //line:netp:parseuri:beginconvert1
        strcat(filename, uri);             //line:netp:parseuri:endconvert1
        if (uri[strlen(uri) - 1] == '/')   //line:netp:parseuri:slashcheck
            strcat(filename, "home.html"); //line:netp:parseuri:appenddefault
        return 1;
    }
    else
    { /* Dynamic content */    //line:netp:parseuri:isdynamic
        ptr = index(uri, '?'); //line:netp:parseuri:beginextract
        if (ptr)
        {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        }
        else
            strcpy(cgiargs, ""); //line:netp:parseuri:endextract
        strcpy(filename, ".");   //line:netp:parseuri:beginconvert2
        strcat(filename, uri);   //line:netp:parseuri:endconvert2
        return 0;
    }
}
/* $end parse_uri */

/*
 * serve_static - copy a file back to the client 
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);    //line:netp:servestatic:getfiletype
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); //line:netp:servestatic:beginserve
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf)); //line:netp:servestatic:endserve
    printf("Response headers:\n");
    printf("%s", buf);

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);                        //line:netp:servestatic:open
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); //line:netp:servestatic:mmap
    Close(srcfd);                                               //line:netp:servestatic:close
    Rio_writen(fd, srcp, filesize);                             //line:netp:servestatic:write
    Munmap(srcp, filesize);                                     //line:netp:servestatic:munmap
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = {NULL};

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0)
    { /* Child */ //line:netp:servedynamic:fork
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);                         //line:netp:servedynamic:setenv
        Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */    //line:netp:servedynamic:dup2
        Execve(filename, emptylist, environ); /* Run CGI program */ //line:netp:servedynamic:execve
    }
    Wait(NULL); /* Parent waits for and reaps child */ //line:netp:servedynamic:wait
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor="
                  "ffffff"
                  ">\r\n",
            body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */

int hostname_to_ip(char *hostname, char *ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if ((he = gethostbyname(hostname)) == NULL)
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **)he->h_addr_list;

    for (i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strcpy(ip, inet_ntoa(*addr_list[i]));
        return 0;
    }

    return 1;
}