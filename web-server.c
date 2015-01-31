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


#define MAXLEN 1024 // maximum length of the buffer
#define MY_PORT 8080 // the port I am using


// don't put everything in main!
// functions that parse the request:

// 1. retrieve pointer to the name of the file requested:


int main(int argc, char *argv[]) {

    struct sockaddr_in addr;
    struct sockaddr_in cliaddr;
    int sock, listens, connsocket;
    socklen_t cliaddrlen;
    char buff[MAXLEN];

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
 
    // accepting request from the browser:
    cliaddrlen = sizeof(cliaddr);

    connsocket = accept (sock, (struct sockaddr*) &cliaddr, &cliaddrlen);
    if (connsocket == -1 ) {
        return -1;
    }

    // read():
    while (1) {
        int clientfd;
    
        clientfd = accept(sock, (struct sockaddr*)&cliaddr, &cliaddrlen);
        printf ("connected\n");

        send(clientfd, buff, recv(clientfd, buff, MAXLEN, 0), 0);

        close(clientfd);
    }

    close(sock);
    return 0;
}

    // server should read and parse the request: retrieve the name of the file requested

// the server shoud check the value of the Host: HTTP header sent by the client to ensure it matches the current hostname -- gethostname()

// the host: header IS NOT in a fixed location

// the server should respond with appropriate HTTP headers, followed by the data, close the connection -- write (bqlqblqblqbq)

// test with simple file index.html
// the url given to the browser depends on the host being used


