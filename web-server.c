#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXLEN 2000000
#define HEADER_LEN 10000
#define MY_PORT 5000 // the port I am using
#define MAXFILENAME 300 // the max lenght of the name of the file requested



/* find the content type of the file requested */
char *find_content_type (char *filename) {
    char *p;  // pointer to the type found
    p = (char *) malloc(30*sizeof(char));
    
    /* find the type: */
    if ( strstr(filename, ".html") != NULL || strstr(filename, ".hml") != NULL ) {
        strcpy (p, "Content-Type: text/html \r\n");
    }
    else if ( strstr(filename, ".txt") != NULL ) {
        strcpy (p, "Content-Type: text/plain \r\n");
    }
    else if ( strstr(filename, ".jpg") != NULL  || strstr(filename, ".jpeg") != NULL ) {
        strcpy (p, "Content-Type: image/jpeg \r\n");
    }
    else if ( strstr(filename, ".gif") != NULL ) {
        strcpy (p, "Content-Type: image/gif \r\n");
    } 
    else {
        strcpy (p, "Content-Type: application/octet-stream \r\n");
    }
    //return "Content-type: image/jpeg\r\n";
    return p;
}



/*  
formates the response 
either of these depeding on the index value: 
    HTTP/1.1 404 Not Found + content type + connection + 404 file made by me
    HTTP/1.1 200 OK + content type + connection header + the file wanted
    HTTP/1.1 400 Bad Request + content type + connection + 400 file made by me
    HTTP/1.1 500 Internal Server Error + content type + connection + 500 file made by me
*/
void response_generator (int conn_fd, char *filename) {

    /* vars needed for finding the length of the file */
    struct stat filestat;
    FILE *fp;
    int fd;
    char header_buff [HEADER_LEN];
    char file_buff [MAXLEN];
    char filesize[7];//, name[30]; 

    //strcpy (name, filename);
    //printf ("*********2******filename: %s\n", filename);
    if ( ((fd = open (filename, O_RDONLY)) < -1) || (fstat(fd, &filestat) < 0) ) {
        printf ("Error in measuring the size of the file");
    }
/*
    else {
        printf ("*********3******filename: %s %s\n", filename, name);
        char data[filestat.st_size + 150], data2[filestat.st_size + 40], data3[filestat.st_size];
    }
*/
    //printf ("*********4******filename: %s\n", filename);
    if (filename == NULL) {
        // I have measured the length of my 400.html file
        strcpy (header_buff, "HTTP/1.1 400 Bad Request\r\nContent-Length: 327\r\nContent-Type: text/html\r\n");
        fp = fopen ("400index.html", "r");
    }

    printf ("filename: %s\n", filename);
    sprintf (filesize, "%zd", filestat.st_size); // put the file size of buffer, so we can add it to the response header
    printf ("size of file: %s\n", filesize);

    fp = fopen (filename, "r");
    if (fp == NULL) {
	printf ("fp is null or filename = 404\n");
        // I have measured the length of my 404.html file
        strcpy (header_buff, "HTTP/1.1 404 Not Found\r\nContent-Length: 165\r\nContent-Type: text/html\r\n");
        fp = fopen ("404index.html", "r");
    }

    else if (fp != NULL) {
        strcpy (header_buff, "HTTP/1.1 200 OK\r\nContent-Length: ");
        /* content-length: */
        strcat (header_buff, filesize);
        strcat (header_buff, "\r\n");
        /* content-type: */
        strcat (header_buff, find_content_type (filename));
        printf ("%s\n", find_content_type (filename));
    }

    else {
        // I have measured the length of my 500.html file
        strcpy (header_buff, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 190\r\nContent-Type: text/html\r\n");
        fp = fopen ("500index.html", "r");
    }
        

    strcat (header_buff, "Connection: keep-alive\r\n\r\n");
    printf ("\n****************print the request:******************\n");
    int i = 0;
    while (header_buff [i]) {
        printf ("%c", header_buff[i]);
        i ++;
    }

    write (conn_fd, header_buff, strlen(header_buff));
    fread (file_buff, sizeof(char), filestat.st_size + 1, fp);
    fclose(fp);

    printf ("\n****************print the request:******************\n");
    i = 0;
    /* read the file in a data buffer: */
/*
    while (i < filestat.st_size ) {
        printf ("%c", file_buff [i]);
        i ++;
    }
*/
    write (conn_fd, file_buff, filestat.st_size);
    close (conn_fd);
}



/* returns the name of the file requested by the browser: */
void request_parser (int clientsock) {

    char *filename;
    char *token = NULL;
    char get[15], http[10];
    char hostname[40];
    char name[10];
    char buff [MAXLEN];

    filename = (char *)malloc(30);

    if ( (recv(clientsock, buff, MAXLEN, 0)) == -1) {
            perror ("Error on RECV\n");
    }

    // TODO: check the hostname.dcs.gla.ac.uk as well
    /* first check the hostname, because if it is wrong then we don't need to do any more calculations */
    if (gethostname (hostname, sizeof hostname) != 0) {
        filename = "404";
    }

    token = strtok(buff, "\r\n"); /* get the first token: */
    sscanf (token, "%s %s %s", get, filename, http);

    if ( (get[0] == 'G') && (get[1] == 'E') && (get[2] == 'T') ) {
        strcpy (name, &filename[1]);        
	    filename = name;
    }
    else {
        filename = "404";
    }
    response_generator (clientsock, filename);
}



int main() {

    int sock, clientsock, listens;
    struct sockaddr_in addr;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;

    // create a TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0); 
    if (sock == -1) {
        return -1; 
    }

    /* inititalize address, port structure */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MY_PORT);
    addr.sin_addr.s_addr = htonl (INADDR_ANY);
    
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    
    // bind the socket to port 8080
    if (bind (sock, (struct sockaddr *) &addr, sizeof(addr) ) == -1 ) {
        return 7;
    }
    // listen for and accept connections from browsers, max 20 connections can wait
    if ( (listens = listen (sock, 20)) == -1) {
        return -1;
    }
 
    cliaddrlen = sizeof(cliaddr);

    while(1) {
        // accepts the connection, returns new file descriptor for the connection and cliaddr
        if ( (clientsock = accept(sock, (struct sockaddr*)&cliaddr, &cliaddrlen)) == -1) {
            perror ("Error on ACCEPT\n");
        }   
        request_parser (clientsock);
        printf ("Sending1...\n");
        close (clientsock);
    }  
    close (clientsock);
    close (sock);
    return 0;
}



