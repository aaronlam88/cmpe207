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

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void flushInputStream() {
    char c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void printMenu() {
    printf("********************************\n");
    printf("*  ls <filename>               *\n");
    printf("*  cd <dir>                    *\n");
    printf("*  upload <local_file_path>    *\n");
    printf("*  download <file_in cur_dir>  *\n");
    printf("*  print                       *\n");
    printf("*  logout                      *\n");
    printf("********************************\n");
}

void handleCommand(int sock, char* loginKey) {
    printMenu();

    char command[BUFSIZ];
    printf("> ");
    fgets(command, BUFSIZ, stdin);

    // run forever, unless logout
    while(strcmp(command, "logout") != 0) {
        if(strcmp(command, "printMenu") == 0) {
            printMenu();
        }
        else {
            // Always write the loginKey to server before sending command
            // this step is to authenticate the client
            // if loginKey doesn't match, command cannot be execute
            // if client send the wrong key, connect will be terminated by server
            write(sock, loginKey, strlen(loginKey));
            // make sure that command can be read by server
            write(sock, command, sizeof(command));

            // TODO handle return
            // base on the command, we may have different return message
            char buf[BUFSIZ];
            memset(buf, 0, BUFSIZ);
            while(read(sock, buf, BUFSIZ) > 0) {
                if(strcmp(buf, "\r\n") == 0) break;
                printf("%s\n", buf);
                memset(buf, 0, BUFSIZ);
            }
        }

        memset(command, 0, BUFSIZ);
        printf("> ");
        fgets(command, BUFSIZ, stdin);
        command[strlen(command)-1] = '\0';
    }

    return;
}

void login (int sock, char* loginKey) {
    int try;
    // try to login 5 times, after 5 time, shutdown client
    for(try = 0; try < 5; ++try) {
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
        write(sock,buf,strlen(buf));

        // get server response
        bzero(buf, strlen(buf));
        read(sock,buf,BUFSIZ);

        // if login success, get secret key from server
        if(strcmp(buf, "LOGIN") == 0) {
            bzero(buf, strlen(buf));
            read(sock, buf, BUFSIZ);

            strcpy(loginKey, buf);
            return;
        } else {
            printf("incorrect username or password\n");
        }
    }
    errexit("shutdown client after 5 try!");
}

void run_client(int sock) {
    // login key token, use to verify login status
    char loginKey[33];

    login(sock, loginKey);
    if(strlen(loginKey) != 33) {
        // login fail
        errexit("login fail!\n");
    } else {
        // if login success, handle user command
        handleCommand(sock, loginKey);
    }
    return;
}

void runn_client(int sock){

    char list[40];
    char buff[BUFSIZ];

    printf("enter ls to list your courses\n");
    scanf("%s",list);

    if(strcmp(list,"ls")){
        printf("type ls\n");

    }

    else{
        strcat(buff,"ls");


        write(sock,buff,strlen(buff));
        bzero(buff, strlen(buff));
        read(sock,buff,BUFSIZ);


        printf("your command : %s\n",buff);
        bzero(buff,BUFSIZ);

    }
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
        fprintf(stderr, "usage: tcpFileClient [host [port]]\n");
        return 1;
    }

    int sock = connectsock(host, service, "tcp");

    run_client(sock);
    return 0;
}