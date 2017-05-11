/*********************************************************************/
/* Author: Aaron Lam                                                 */
/* Description: TCP File Client, connect to TCP file server          */
/*     get file path from user, then send request to server          */
/*     get file name return from server, create file with file name  */
/*     get file content from server and write to local file          */
/*********************************************************************/


/* tcpFileClient.c - main */

#include "errexit.h"
#include "connectsock.h"
#include "fileClient.h"
#include "fileServer.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void flushInputStream() {
    char c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void printMenu() {
    printf("********************************\n");
    printf("*  ls [filename]               *\n");
    printf("*  cd [dir]                    *\n");
    printf("*  upload                      *\n");
    printf("*  download                    *\n");
    printf("*  print                       *\n");
    printf("*  logout                      *\n");
    printf("********************************\n");
}

void do_download(int sock) {
    char buf[BUFSIZ];
    int fd;

    if (sendFileRequest(sock, buf) == -1) {
        printf("sendFileRequest() error: %s\n", strerror(errno));
    }

    if ((fd = createFile(sock, buf)) == -1) {
        printf("createFile() error: %s\n", strerror(errno));
    }

    if (getFile(sock, buf, fd) == -1) {
        printf("getFile() error: %s\n", strerror(errno));
    }
}

void do_upload(int sock) {
    char buf[BUFSIZ]; /* message buffer; use default stdio BUFSIZ */
    memset (buf, 0, BUFSIZ);

    printf("File path: ");
    fgets(buf, BUFSIZ, stdin);
    buf[strlen(buf) - 1] = '\0';

    printf("open file: %s\n", buf);
    FILE *fp = fopen(buf, "r"); //open file identify by file path
    if ( fp == NULL ) {
        write(sock, "\0", 1);
        printf("error openning file: '%s' '%s'\n", buf, strerror(errno));
    } else {
        write(sock, buf, strlen(buf) + 1);
        struct sockaddr_in src_addr;
        sendFile(sock, buf, src_addr, 0);
    }
    fclose(fp);
    return;
}

void handleCommand(int sock, char* loginKey) {
    printMenu();

    char command[BUFSIZ];
    printf("> ");
    fgets(command, BUFSIZ, stdin);
    // drop the '\n' at the end of command
    command[strlen(command) - 1] = '\0';

    // run forever, unless logout
    while (strncmp(command, "logout", 6) != 0) {
        // handle print first since it don't require resources
        if (strncmp(command, "print", 5) == 0) {
            printMenu();
            continue;
        }

        // skip empty command
        if (strlen(command) == 0) {
            goto SKIP;
        }

        char buf[BUFSIZ];
        memset(buf, 0, BUFSIZ);
        // Always write the loginKey to server before sending command
        // this step is to authenticate the client
        // if loginKey doesn't match, command cannot be execute
        // if client send the wrong key, connect will be terminated by server
        write(sock, loginKey, strlen(loginKey));

        // make sure that command can be read by server
        write(sock, command, strlen(command));

        // base on the command, we may have different return message
        if (strncmp(command, "download", 8) == 0) {
            do_download(sock);
        } else if (strncmp(command, "upload", 6) == 0) {
            do_upload(sock);
        } else {
            int n;
            while ( (n = recvfrom(sock, buf, 1, 0, NULL, NULL)) > 0) {
                if (buf == NULL || buf[0] == '\0') {
                    break;
                }

                printf("%s", buf);
                memset(buf, 0, BUFSIZ);
            }
            printf("\n");
        }

    // SKIP label
    SKIP:
        memset(command, 0, BUFSIZ);
        printf("> ");
        fgets(command, BUFSIZ, stdin);
        command[strlen(command) - 1] = '\0';
    }
    // gracefully shutdown client, notify server
    write(sock, loginKey, strlen(loginKey));
    write(sock, "logout", strlen("logout"));

    return;
}

void login (int sock) {
    // login key token, use to verify login status
    char loginKey[33];

    char username[40]; bzero(username, 40);
    char password[40]; bzero(password, 40);

    char buf[BUFSIZ]; bzero(buf, BUFSIZ);
    // get username
    printf("username: ");
    scanf("%s", username);
    // get password
    strcpy(password, getpass("password: "));
    flushInputStream();

    // construct login command
    sprintf(buf, "login %s %s\n", username, password);

    // send login command to server
    write(sock, buf, strlen(buf));

    // get server response
    bzero(buf, strlen(buf));
    read(sock, buf, BUFSIZ);

    // if login success, get secret key from server
    if (strcmp(buf, "LOGIN") == 0) {
        bzero(loginKey, 33);
        read(sock, loginKey, 33);
        printf("loginKey: %s\n", loginKey);
        handleCommand(sock, loginKey);
        return;
    } else {
        errexit("wrong username or password!\n");
    }
}

void run_client(int sock) {
    login(sock);
}


/*------------------------------------------------------------------------
* main - TCP File Client for TCP File Service
*------------------------------------------------------------------------
*/
int main(int argc, char *argv[]) {
    char *host = "localhost"; /* host to use if none supplied */
    char *service = "9000"; /* default service name */

    switch (argc) {
    case 1:
        host = "localhost";
        break;
    case 3:
        service = argv[2];
    /* FALL THROUGH */
    case 2:
        host = argv[1];
        break;
    default:
        fprintf(stderr, "usage: client [host [port]]\n");
        return 1;
    }

    int sock = connectsock(host, service, "tcp");

    run_client(sock);
    return 0;
}
