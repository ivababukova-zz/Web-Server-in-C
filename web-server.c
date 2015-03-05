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
#include <stdbool.h>

#define MY_PORT 5000 // the port I am using

#define SMALL 1000
#define BIG 10000

void respond_400 (int conn_fd) {
  char buff [300];
  strcpy(buff,"HTTP/1.1 400 Bad Request\r\nConnection: keep-alive\r\n\r\n");
  strcat(buff,"<!DOCTYPE html><html><head><title>400</title></head>");
  strcat(buff,"<body><p>400 Bad Request</p></body></html>");
  write(conn_fd, buff, strlen(buff));
  close(conn_fd);
}

void respond_404 (int conn_fd) {
  char buff [300];
  strcpy(buff,"HTTP/1.1 404 File Not Found\r\nConnection: keep-alive\r\n\r\n");
  strcat(buff,"<!DOCTYPE html><html><head><title>404</title></head>");
  strcat(buff,"<body><p>404</p></body></html>");
  write(conn_fd, buff, strlen(buff));
  close(conn_fd);
}

void respond_500 (int conn_fd) {
  char buff [300];
  strcpy (buff, "HTTP/1.1 500\r\nConnection: keep-alive\r\n\r\n") ;
  strcat (buff, "<!DOCTYPE html><html><head><title>500</title></head>");
  strcat (buff, "<body><p>500</p></body></html>");
  write (conn_fd, buff, strlen(buff));
  close (conn_fd);
}

// allocate @length memory
char* newstr(int length){
   char* new_memory = (char*) calloc(length, sizeof(char));
   if (new_memory == NULL){
     perror("Error in calloc.");
     exit(1);
   }
   return new_memory;
}


bool write_message (const char *message,
                    int message_length,
                    int file_descriptor,
                    int repeat_if_error) {
    if(repeat_if_error <= 0){
        return false;
    }
    signed int total_written = 0;
    while(total_written < message_length) {
        ssize_t written_this_round = write (file_descriptor,
                        (const char *) &message[total_written],
                         sizeof(char) * (message_length - total_written)
                         );
        if (written_this_round != -1) {
            total_written += written_this_round;        
        }
        if (written_this_round == -1) {
            perror ("write error");
            return write_message (message + total_written,
                                  message_length - total_written,
                                  file_descriptor,
                                  repeat_if_error - 1);
        }
    }
    return true;
}

bool read_message ( char *message,
                    int message_length,
                    int file_descriptor,
                    int repeat_if_error) {
    if(repeat_if_error <= 0){
        return false;
    }
    signed int total_written = 0;
    while(total_written < message_length) {
        ssize_t written_this_round = read (file_descriptor,
                         (char *) &message[total_written],
                         sizeof(char) * (message_length - total_written)
                         );
        if (written_this_round != -1) {
            total_written += written_this_round;        
        }
        if (written_this_round == -1) {
            perror ("write error");
            return read_message (message + total_written,
                                  message_length - total_written,
                                  file_descriptor,
                                  repeat_if_error - 1);
        }
    }
    return true;
}

bool read_in_chunks (char* filename, int filestat_size, int conn_fd) {
    int BUFFER_SIZE = BIG;
    char buffer[BUFFER_SIZE];
    int size_left = filestat_size;
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        respond_404 (conn_fd);
        return false;
    }

    while (size_left > 0) {
        if (BUFFER_SIZE < size_left) {
            size_left = size_left - BUFFER_SIZE;
            BUFFER_SIZE = BUFFER_SIZE;
        } else {
            BUFFER_SIZE = size_left;
            size_left = size_left - size_left;
        }
        bool success = read_message(buffer, BUFFER_SIZE, fd, 10);
        
        success = write_message(buffer, BUFFER_SIZE, conn_fd, 10);
        if (!success){
            close(fd);
            return false;
        }
    }
    close(fd);
    return true;
}

int response_generator (int conn_fd, char *filename) {

    /* vars needed for finding the length of the file */
    struct stat filestat;
    int fd;
    char header_buff[BIG];
    char filesize[SMALL];
    bool success = ((fd=open(filename, O_RDONLY)) > -1);

    if (success) {
        fstat(fd, &filestat);
        close(fd);
    }
    else {
        perror("Error in measuring the size of the file.");
        respond_404 (conn_fd);
        return 1;
    }

    if (filestat.st_size > 0) {
        sprintf (filesize, "%zd", filestat.st_size);
    }

    if (filename == NULL || strcmp (filename, "400") == 0 ) {
        respond_400 (conn_fd);
        return 1;
    }

    if (strcmp (filename, "500") == 0) {
        respond_500 (conn_fd);
        return 1;
    }

    
 //   fp = fopen (filename, "r");


//    if (fp != NULL) {
        // put the file size to buffer
        sprintf (filesize, "%zd", filestat.st_size);
        strcpy (header_buff, "HTTP/1.1 200 OK\r\nContent-Length: ");

        /* write the content-length: */
        strcat (header_buff, filesize);
        strcat (header_buff, "\r\n");

        /* find the content-type: */
        if(strstr(filename, ".html")!=NULL||strstr(filename, ".hml")!= NULL){
            strcat (header_buff, "Content-Type: text/html\r\n");
        }
        else if (strstr (filename, ".txt") != NULL ) {
            strcat (header_buff, "Content-Type: text/plain\r\n");
        }
        else if(strstr(filename,".jpg")!= NULL||strstr(filename,".jpeg")!=NULL)
        {
            strcat (header_buff, "Content-Type: image/jpeg\r\n");
        }
        else if (strstr (filename, ".gif") != NULL ) {
            strcat (header_buff, "Content-Type: image/gif\r\n");
        } 
        else {
            strcat(header_buff,"Content-Type: application/octet-stream\r\n");
        }
        
        strcat (header_buff, "Connection: keep-alive\r\n\r\n");
        write_message (header_buff, strlen(header_buff), conn_fd, 5);

//        else {
           // fread (file_buff, sizeof(char), filestat.st_size, fp);
           // write_message (file_buff,filestat.st_size, conn_fd, 10);
            read_in_chunks (filename, filestat.st_size, conn_fd);
//        }
//    }
/*
    else {
        // I have measured the length of my 500.html file
        respond_500 (conn_fd);
        return -1;
    }        
*/
//    fclose (fp);
    close (conn_fd);
    return 0;
}

int checkHostName (char *hostname, char *hostname_req, char *buff) {
    char *host = strcasestr (buff, "host: ");
    if (host == NULL) { 
        return -1;
    }

    else {
        host += 6;
        char *start_host = host;
        char *stripped_host = strstr (start_host, ":5000\r\n");
        memcpy(hostname_req, start_host, stripped_host - start_host);
        if (strcmp (hostname_req, "localhost") == 0) {
            printf ("hostname is ok.\n");
            return 0;
        }
        if (strcmp (hostname_req, hostname) != 0) {
            strcat (hostname, ".dcs.gla.ac.uk");
            if (strcmp (hostname_req, hostname) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

/* returns the name of the file requested by the browser: */
int *request_parser (int clientsock) {
    char *filename = newstr (SMALL);
    char *get = newstr (SMALL);
    char *http = newstr (BIG);
    char *hostname = newstr (SMALL);
    char *hostname_req = newstr (SMALL);
    char *name = newstr (SMALL);
    char *buff = newstr (BIG);
    ssize_t r;
    
    gethostname(hostname, 100);
    
    r = read(clientsock, buff, BIG);
    if ( r == -1) {
            perror ("Error on RECV\n");
    }
    if (r > (unsigned int)strlen(buff)) {
        respond_500 (clientsock);
        perror ("this request is too big.\n");
    }
    if (r == 0) {
         
         return 0;
    }
 
    if (checkHostName (hostname, hostname_req, buff) == -1) {
         free(name);
         respond_400 (clientsock);
    }

    sscanf (buff, "%s %s %s", get, filename, http);
    
    if ( strcmp (get, "GET") == 0 ) {
        strcpy (name, &filename[1]);        
        free(filename);
        filename = name;
        response_generator (clientsock, filename);
    }

    else if (strcmp (get, "GET") != 0){
        free(name);
        respond_404 (clientsock);
    }

    free (filename);
    free (get);
    free (http);
    free (hostname);
    free (hostname_req);
    free (buff);
    return 0;

}

int sock;
void forever_accept () {
  struct sockaddr_in cliaddr;
  socklen_t cliaddrlen;
  cliaddrlen = sizeof(cliaddr);
  int cls;
  int i;

  while(1) {
    cls=accept(sock,(struct sockaddr*)&cliaddr, &cliaddrlen);
    if (cls != -1) {
        request_parser (cls);
    }
    else {
       perror ("Error on ACCEPT\n");
    }
  }
  pthread_exit(0);
}

int main () {
    int i = 0;
    int connNumb = 3;
    pthread_t ids[connNumb];
    
    for(i = 0; i < connNumb; i++){
        ids[i] = 0;    
    }

    int listens;
    struct sockaddr_in addr;
    
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
    // listen for and accept connections from browsers
    if ( (listens = listen (sock, SMALL)) == -1) {
        return -1;
    }
    // create threads in the thread pool:
    for(i = 0; i<connNumb; i++) {
        pthread_create (&ids[i], NULL, (void*(*)())&forever_accept, NULL); 
    }

    forever_accept();

    for(i = 0; i<connNumb; i++) {
        pthread_join (ids[i], NULL);
        //printf("%d\n", ids[i]);   
    }

    close (sock);
    return 0;
}
