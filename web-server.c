#define _GNU_SOURCE // for strcasestr function

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

#define MAXLEN 2000000 // the max length of the buffer where I read in data
#define HEADER_LEN 10000 // max length of the responce headers
#define MY_PORT 5000 // the port I am using
#define MAXFILENAME 300 // the max lenght of the name of the file requested
#define SMALL 1000
#define BIG 100000
#define HUGE 1000000

// allocate @length memory
char* newstr(int length){
   char* new_memory = (char*) calloc(length, sizeof(char));
   if (new_memory == NULL){
     perror("Error in calloc.");
     exit(1);
   }
   return new_memory;
}


void respond_400 (int conn_fd) {
    char buff [300];
    strcpy (buff, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n") ;
    strcat (buff, "<!DOCTYPE html><html><head><title>400 Bad Request </title></head>");
    strcat (buff, "<body> <h1>400 Bad Request</h1><p>Something wrong with your request</p></body></html>");
    write (conn_fd, buff, strlen(buff));
    close (conn_fd);
}

void respond_404 (int conn_fd) {
    char buff [300];
    strcpy (buff, "HTTP/1.1 404 File Not Found\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n") ;
    strcat (buff, "<!DOCTYPE html><html><head><title>404 File Not Found</title></head>");
    strcat (buff, "<body> <h1>404 File Not Found</h1><p>The file requested doesn't exist.</p></body></html>");
    write (conn_fd, buff, strlen(buff));
    close (conn_fd);
}

void respond_500 (int conn_fd) {
    char buff [300];
    strcpy (buff, "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n") ;
    strcat (buff, "<!DOCTYPE html><html><head><title>500 Internal Server Error</title></head>");
    strcat (buff, "<body> <h1>500 Internal Server Error</h1></body></html>");
    write (conn_fd, buff, strlen(buff));
    close (conn_fd);
}

int response_generator (int conn_fd, char *filename) {

    /* vars needed for finding the length of the file */
    struct stat filestat;
    FILE *fp;
    int fd;
    char* header_buff = newstr(BIG);
    char* file_buff = newstr(HUGE);
    char filesize[7];//, name[30]; 

    if ( ((fd = open (filename, O_RDONLY)) < -1) || (fstat(fd, &filestat) < 0) ) {
        printf ("Error in measuring the size of the file.\n");
        respond_404 (conn_fd);
    }

    if (filename == NULL || strcmp (filename, "400") == 0 ) {
        respond_400 (conn_fd);
        return 1;
    }

    fp = fopen (filename, "r");
    if (fp == NULL) {
	    respond_404 (conn_fd);
        return 2;
    }

    else if (fp != NULL) {
        sprintf (filesize, "%zd", filestat.st_size); // put the file size to buffer
        strcpy (header_buff, "HTTP/1.1 200 OK\r\nContent-Length: ");

        /* write the content-length: */
        strcat (header_buff, filesize);
        strcat (header_buff, "\r\n");

        /* find the content-type: */
        if ( strstr(filename, ".html") != NULL || strstr(filename, ".hml") != NULL ) {
            strcat (header_buff, "Content-Type: text/html \r\n");
        }
        else if ( strstr(filename, ".txt") != NULL ) {
            strcat (header_buff, "Content-Type: text/plain \r\n");
        }
        else if ( strstr(filename, ".jpg") != NULL  || strstr(filename, ".jpeg") != NULL ) {
            strcat (header_buff, "Content-Type: image/jpeg \r\n");
        }
        else if ( strstr(filename, ".gif") != NULL ) {
            strcat (header_buff, "Content-Type: image/gif \r\n");
        } 
        else {
            strcat (header_buff, "Content-Type: application/octet-stream \r\n");
        }
        
        strcat (header_buff, "Connection: keep-alive\r\n\r\n");
        write (conn_fd, header_buff, strlen(header_buff));

        fread (file_buff, sizeof(char), filestat.st_size, fp);
        write (conn_fd, file_buff, filestat.st_size);
        free(file_buff);
        free(header_buff);
    }

    else {
        // I have measured the length of my 500.html file
        respond_500 (conn_fd);
        return -1;
    }        

    fclose (fp);
    close (conn_fd);
    return 0;
}

// int checkHostName (char *hostname, )

/* returns the name of the file requested by the browser: */
void *request_parser (int clientsock) {

    char *filename = newstr(SMALL);
    char *token = NULL;
    char *get = newstr(SMALL);
    char *http = newstr(BIG);
    char *hostname = newstr(SMALL);
    char *name = newstr(SMALL);
    char *buff = newstr(BIG);
    int r;
   // int clientsock = *(int *)args;

    gethostname(hostname, 100);

    if ( (r = read(clientsock, buff, BIG) ) == -1) {
            perror ("Error on RECV\n");
    }
    printf ("read value: %i\n", r);
    if (r == 0) {
        printf ("stopped requesting\n");
        return;
    }

    int i = 0;
    while (i < strlen (buff)) {
        printf ("%c", buff[i]);
        i++;
    }
 
    char *host = strcasestr (buff, "host: ");
    printf ("%s \n", host);
    if (1 == 2) {
        filename = "400"; // TODO check what error to return if there is missing host header
    }

    else {
        host += 6;
        char *start_host = host;

        char *stripped_host = strstr (start_host, "\r\n");
        printf ("%s \n", stripped_host);

        char hostnamereq[12000];
        memcpy(hostnamereq, start_host, stripped_host - start_host);  

        if (strcmp (hostnamereq, hostname) != 0) {
            strcat (hostname, ".dcs.gla.ac.uk:5000"); // check whether this hostname is used instead
            if (strcmp (hostnamereq, hostname) == 41) {
                filename = "400";
            }
        }
    }

    sscanf (buff, "%s %s %s", get, filename, http);
    if ( strcmp (get, "GET") == 0 ) {
        strcpy (name, &filename[1]);        
	    free(filename);
        filename = name;
    } else {
        free(name);
    }
    response_generator (clientsock, filename);
    free(filename);
    free(get);
    free(http);
    free(hostname);
    free(buff);
    return;
}


int main() {

    int connNumb = 10;
    int sock, clientsock, listens;
    struct sockaddr_in addr;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    pthread_t t1;

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
    if ( (listens = listen (sock, connNumb)) == -1) {
        return -1;
    }
 
    cliaddrlen = sizeof(cliaddr);
/*
    pthread *thread = malloc (sizeof (pthread_t)*connNumb); // pointer to the thread pool

    // create threads in the thread pool:
    int i;
    for (i = 0; i < connNumb; i++) {
        int ret = -1;
        ret = pthread_create (&thread[i], NULL, request_parser, NULL);
        if (ret != 0) {
            printf ("error occured while creating thread.\n");
            exit (1);
        }
    }
*/
    while(1) {
        // accepts the connection, returns new file descriptor for the connection and cliaddr
        if ( (clientsock = accept(sock, (struct sockaddr*)&cliaddr, &cliaddrlen)) == -1) {
            perror ("Error on ACCEPT\n");
        }   
        // start a new thread to process the network connection accepted:
        request_parser (clientsock);
        

/*
        if (pthread_create (&t1, NULL, request_parser, (void *)&clientsock)) {
            fprintf(stderr, "Error creating thread 1\n");
            return 1;
        }

        if (pthread_create (&t2, NULL, request_parser, (void *)&clientsock)) {
            fprintf(stderr, "Error creating thread 2\n");
            return 1;
        }
*/
    //    pthread_join(t1, NULL); /* wait for thread 1 to finish */
       // pthread_join(t2, NULL); /* wait for thread 1 to finish */
        printf ("Sending ...\n");

    }
    close (sock);
    return 0;
}



