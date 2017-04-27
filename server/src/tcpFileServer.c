#include "tcpFileServer.h"
#include <stdio.h>

/**
 * getFilePath: get file path from client connected to current socket.
 * Arguements:
 * sock     - socket descripter, current open socket
 * buf      - buffer to storage message, using default BUFSIZ
 * Return:
 * the requested file path from client is save buf for later use
 * return nothing. Report error code (errno) if recvfrom() fail
 */
void getFilePath (int sock, char *buf) {
    int n = read(sock, buf, BUFSIZ);
    if (n < 0) {
        printf("read() error: %s\n", strerror(errno));
    }
    buf[n] = '\0';
    return;
}

/**
 * sendFileName: send file name back to client, act as connection check.
 * Arguements:
 * sock     - socket descripter, current open socket
 * buf      - buffer to storage message, using default BUFSIZ
 * Return:
 * return nothing. Report error code (errno) if sendto() fail
 */
void sendFileName (int sock, char *buf) {
    char filename[BUFSIZ];
    memset(filename, 0, BUFSIZ);
    strcpy(filename, basename(buf));
    filename[strlen(filename)] = '\0';

    int count = write(sock, filename, sizeof(filename));
    if (count < 0) {
        printf("write() error: %s\n", strerror(errno));
    }

}

/**
 * sendFile: open the requested file, then send file back to client.
 * Arguements:
 * sock     - socket descripter, current open socket
 * buf      - buffer to storage message, using default BUFSIZ
 * Return:
 * return nothing. Report error code (errno) if error
 */
void sendFile (int sock, char *buf) {
    printf("open file: %s\n", buf);
    int fd = open(buf, O_RDONLY); //open file identify by file path
    if ( fd == -1 ) { 
        write(sock, "\r\n", 2);
        printf("error openning file: '%s' '%s'\n", buf, strerror(errno));
    }

    printf("start sending...\n");
    int n;
    while ( (n = read(fd, buf, BUFSIZ)) > 0 ) {
        write(sock, buf, n);
    }
    sendto(sock, NULL, 0, 0, NULL, 0);
    
    close(fd);
    printf("close file\n");
    
    write(sock, "\r\n", 2);
}
