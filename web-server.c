#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLEN 1024 // maximum length of the buffer
#define MY_PORT 5000 // the port I am using
#define MAXFILENAME 30 // the max lenght of the name of the file requested

// don't put everything in main!
// functions that parse the request:

/*  
returns either of these depeding on the index value: 
    HTTP/1.1 404 Not Found
    HTTP/1.1 200 OK
    HTTP/1.1 400 Bad Request
    HTTP/1.1 500 Internal Server Error
*/
char * responseGenerator (int i, char *buff) {

    char *p;
 

    if (i == -1) {
        static char response[MAXFILENAME] = "404 Not Found\0";
        p = response;
    }

    if (i == 0) {
        static char response[MAXFILENAME] = "200 OK\0";
        p = response;
    }
    // if the server doesn't understand the request:
    if (i == 1) {
        static char response[MAXFILENAME] = "400 Bad Request\0";
        p = response;
    }
    // the server fails for a reason:
    if (i == 2) {
        static char response[MAXFILENAME] = "500 Internal Server Error\0";
        p = response;
    }
    
    printf ("the response is: %s\n", p );
    return p;
}

// returns the file requested by the browser: 
char * request_parser (char *buff) {

    /* process the request: */
    char *token = NULL;
    char *filename, *http;
    char get[15];

    filename = (char *)malloc(30);
    http = (char *)malloc(10);
    token = strtok(buff, "\r\n"); /* get the first token: */
/*
    while (token != NULL) {
        printf ("token: %s\n", token);
        token = strtok (NULL, "\r\n");
    }
*/

    while (token) {
        sscanf (token, "%s %s %s", get, filename, http);
        printf ("the first token: %s\n", get);

        if ( (strlen(get) == 3) && (get[0] == 'G') && (get[1] == 'E') && (get[2] == 'T') ) {
            strcpy (filename, &filename[1]);
            printf ("filename found here: %s\n", filename);
            break;
        }

/*
        if ( (strlen(get) == 5) && (get[0] == 'H') && (get[1] == 'o') ){
            printf ("found the host line of the request: %s\n", token);
            break;
            // call responseGenerator function to generate response
        }
*/
        else {
            token = strtok (NULL, "\r\n");
        }
    }
    printf("filename to be returned by parser function: %s\n", filename);
    free(http);
    return filename;
}

int main() {

    struct sockaddr_in addr;
    struct sockaddr_in cliaddr;
    int sock, clientsock, listens;
    socklen_t cliaddrlen;
    char buff[MAXLEN] = {0};
    char *p; // pointer to the buffer that contains the http request
    char *filerequest;  // pointer to the filename requested
    p = buff;
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

    // listen for and accept connections from browsers
    if ( (listens = listen (sock, 20)) == -1) {
        return -1;
    }
 
    cliaddrlen = sizeof(cliaddr);

    while(1) {
    // read():
        // accepts the connection, returns new file descriptor for the connection and cliaddr
        clientsock = accept(sock, (struct sockaddr*)&cliaddr, &cliaddrlen);
        if (clientsock == -1) {
            printf ("error: connection refused\n");
        }
        else {
            printf ("Connected\n");
        }

        if ( (recv(clientsock, buff, MAXLEN, 0)) == -1) {
            printf ("error: connection wasn't received\n");
        }   
  
        filerequest = request_parser (p);
        printf ("result from parsing: %s\n", filerequest);
        if (p == NULL) {
            printf ("internal server error\n");
        }
 
        int c;
        unsigned char data[MAXLEN], data2[MAXLEN];
        int newlen;

        /* TODO: write in the data buffer the response: */        
        strcat (data, "HTTP/1.1 200 OK \r\n");       
        fp = fopen(filerequest, "r");
        if (fp != NULL) {
            newlen = fread (data2, sizeof(char), MAXLEN, fp);
            data2[newlen + 1] = '\0';  
            strcat (data, data2); 
            fclose(fp);
            printf("Sending...\n");
        }

        else {
            printf("error while opening file.\n");
        }

        write (clientsock, data, newlen);
        //p = responseGenerator (2);
        //printf ("the response is: %s\n", p );
	    printf("writen the data\n");
        break;
    }
        close(clientsock);


    close(sock);
    return (0);
}

    // server should read and parse the request: retrieve the name of the file requested

// the server shoud check the value of the Host: HTTP header sent by the client to ensure it matches the current hostname -- gethostname()

// the host: header IS NOT in a fixed location

// the server should respond with appropriate HTTP headers, followed by the data, close the connection -- write (bqlqblqblqbq)

// test with simple file index.html
// the url given to the browser depends on the host being used


