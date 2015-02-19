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

#define MAXLEN 200000
#define MY_PORT 5000 // the port I am using
#define MAXFILENAME 30 // the max lenght of the name of the file requested



/* find the content type of the file requested */
char *find_content_type (char *filename) {
    char *p;  // pointer to the type found
    int i;
    char buf1[MAXFILENAME]; // used to store the extension of the file
    char buf2[MAXFILENAME];
    
    p = (char *)malloc(30);
    strcpy(buf1, filename);
    printf("name of file requested: %s \n", buf1);

    /* find the extension: */
    for (i = 0; i<strlen(buf1); i++) {
        if ( buf1[i] == '.' ) {
            strcpy(buf2, &buf1[i]);
        }
    }
    /* find the type: */
    if ( strcmp(buf2, ".html") == 0 || strcmp (buf2, ".hml") == 0) {
        strcpy (buf2, "Content-Type: text/html \r\n");
    }

    else if ( strcmp(buf2, ".txt") == 0) {
        strcpy (buf2, "Content-Type: text/plain \r\n");
    }

    else if ( strcmp(buf2, ".jpg") == 0 || strcmp (buf2, ".jpeg") == 0) {
        strcpy (buf2, "Content-Type: image/jpeg \r\n");
    }

    else if ( strcmp(buf2, ".gif") == 0) {
        strcpy (buf2, "Content-Type: image/gif \r\n");
    }
    
    else {
        strcpy (buf2, "Content-Type: application/octet-stream \r\n");
    }

    p = buf2;
    printf ("content-type: %s\n", p);
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
char * response_generator (char *filename) {

    char *p; // pointer to the whole response
    char *content_type; // pointer to the content type
    /* vars needed for finding the length of the file */
    struct stat filestat;
    FILE *fp;
    int fd;
    off_t size;
    char filesize[6]; 

    if ( ((fd = open (filename, O_RDONLY)) < -1) || (fstat(fd, &filestat) < 0) ) {
        perror ("Error in measuring the size of the file");
    }
    char data[filestat.st_size + 150], data2[filestat.st_size + 40], data3[filestat.st_size];

    if (filename == NULL || strcmp (filename,"404") == 0) {
        // I have measured the length of my 400.html file
        strcpy (data, "HTTP/1.1 400 Bad Request\r\nContent-Length: 327\r\nContent-Type: text/html\r\n");
        fp = fopen ("400index.html", "r");
    }


    printf ("***************size of file: %d\n", filestat.st_size);
    close (fd);

    sprintf (filesize, "%d", filestat.st_size); // put the file size of buffer, so we can add it to the response header

    fp = fopen (filename, "r");
    if (fp == NULL) {
        // I have measured the length of my 404.html file
        strcpy (data, "HTTP/1.1 404 Not Found\r\nContent-Length: 165\r\nContent-Type: text/html\r\n");
        fp = fopen ("404index.html", "r");
    }

    else if (fp != NULL) {
        strcpy (data, "HTTP/1.1 200 OK\r\nContent-Length: ");
        /* content-length: */
        strcat (data, filesize);
        strcat (data, "\r\n");
        /* content-type: */
        strcpy (data2, find_content_type (filename));
        printf ("content-type: %s\n", find_content_type (filename));
	    strcat (data, data2);
    }

    else {
        // I have measured the length of my 500.html file
        strcpy (data, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 190\r\nContent-Type: text/html\r\n");
        fp = fopen ("500index.html", "r");
    }
        

    strcat (data, "Connection: keep-alive\r\n\n");

    /* read the file in a data buffer: */
    fread (data3, sizeof(char), filestat.st_size + 150, fp);
    strcat (data, data3);   // seg fault here
    printf ("hello\n");
    data[strlen(data)] = '\0';


    p = data;
    fclose(fp);
    return p;
}



/* returns the name of the file requested by the browser: */
char * request_parser (char *buff) {

    char *filename;
    char *token = NULL;
    char get[15], http[10];
    char hostname[40];

    filename = (char *)malloc(30);

    // TODO: check the hostname.dcs.gla.ac.uk as well
    /* first check the hostname, because if it is wrong then we don't need to do any more calculations */
    if (gethostname (hostname, sizeof hostname) != 0) {
        filename = "404";
        return filename;
    }
    printf ("+++++++%s++++++++\n", hostname);

    token = strtok(buff, "\r\n"); /* get the first token: */
    sscanf (token, "%s %s %s", get, filename, http);

    if ( (strlen(get) == 3) && (get[0] == 'G') && (get[1] == 'E') && (get[2] == 'T') ) {
        strcpy (filename, &filename[1]);
        printf("filename to be returned by parser function: %s\n", filename);        
        return filename;
    }

    return "404";
}



int main() {

    int sock, clientsock, listens, newlen;
    char buff[MAXLEN] = {0};
    char *buff_p, *response_p;
    struct sockaddr_in addr;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;

    buff_p = buff;
    FILE *fp;

    // create a TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0); 
    if (sock == -1) {
        return -1; 
    }

    /* inititalize address, port structure */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MY_PORT);
    addr.sin_addr.s_addr = htonl (INADDR_ANY);

    // bind the socket to port 8080
    if (bind (sock, (struct sockaddr *) &addr, sizeof(addr) ) == -1 ) {
        return -1;
    }

    // listen for and accept connections from browsers, max 20 connections can wait
    if ( (listens = listen (sock, 20)) == -1) {
        return -1;
    }
 
    cliaddrlen = sizeof(cliaddr);

    while(1) {
        // char response_buff[MAXLEN];

        // accepts the connection, returns new file descriptor for the connection and cliaddr
        if ( (clientsock = accept(sock, (struct sockaddr*)&cliaddr, &cliaddrlen)) == -1) {
            perror ("Error on ACCEPT\n");
        }
        if ( (recv(clientsock, buff, MAXLEN, 0)) == -1) {
            perror ("Error on RECV\n");
        }   

        printf ("\n****************print the request:******************\n");
        int j = 0;
        while (buff[j] != '\0') {
            printf ("%c", buff[j]);
            j++;
        }
        
        response_p = response_generator (request_parser (buff_p));
        // strcpy (response_buff, response_p);
        newlen = strlen (response_p);
/*
        // check what is going to be sent:
        int i;
        for (i = 0; i<newlen; i++) {
            printf ("%c", response_buff[i]);
        }
*/
        write (clientsock, response_p, newlen);
        printf("Sending1...\n");
    }

    close(clientsock);
    close(sock);
    return 0;
}



