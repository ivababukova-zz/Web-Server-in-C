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
#define MY_PORT 8080 // the port I am using
#define MAXFILENAME 30 // the max lenght of the name of the file requested

// don't put everything in main!
// functions that parse the request:

// 1. retrieve pointer to the name of the file requested:


// returns the file requested by the browser: 
char * parser (char *buff) {
    char *token = NULL;
    static char filename[MAXFILENAME];
    char *p;
    int i, j;

    token = strtok(buff, "\n");

    while (token) {
        if ( token[0] == 'G' && (2)<strlen(token)) {
            if (token[1] == 'E' && token[2] == 'T') { 
                i = 5;
                j = 0;
                while (token[i] != ' ') {
                    filename[j] = token[i];
                    j++;
                    i++;
                }
                filename[j] = '\0';
                break;
            }
        }
        token = strtok(NULL, "\n");
    }

    p = filename;
    // check whether the filename is correctly parsed: 
    return p;
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
        printf ("connected\n");
        int byte_count = recv(clientsock, buff, MAXLEN, 0);
        send(clientsock, buff, byte_count, 0);
        filerequest = parser (p);
        printf ("result from parsing: %s\n", filerequest);
        int c;
        fp = fopen(filerequest, "r");
        if (fp == NULL) {
            printf("error while opening file.\n");
        }
        

        while (1) {
            unsigned char data[256] = {0};
            int newlen;
            if (fp) {
                newlen = fread (data, sizeof(char), 256, fp);
                data[++newlen] = '\0';   
                fclose(fp);
            }
            printf("Sending...\n");
            write (clientsock, data, newlen);
            int i = 0;
            close(clientsock);
            }
        }

    close(sock);
    return (0);
}

    // server should read and parse the request: retrieve the name of the file requested

// the server shoud check the value of the Host: HTTP header sent by the client to ensure it matches the current hostname -- gethostname()

// the host: header IS NOT in a fixed location

// the server should respond with appropriate HTTP headers, followed by the data, close the connection -- write (bqlqblqblqbq)

// test with simple file index.html
// the url given to the browser depends on the host being used


