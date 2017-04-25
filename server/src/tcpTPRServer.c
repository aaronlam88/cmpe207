/*********************************************************************/
/* Author: Aaron Lam                                                 */
/* Description: TCP server, thread per request                       */
/*********************************************************************/

/* tcpTPRServer.c - main */

#include <pthread.h>

#include "errexit.h"
#include "passivesock.h"
#include "mysql_connect.h"

#define BACKLOG 10

void getRole(MYSQL* conn,char* username, char* role) {
    const char* logger_format = "getRole [%s] %s\n";
    const char* prepare = "SELECT role FROM login WHERE username = '%s'";
    char query[BUFSIZ];
    sprintf(query, prepare, username);
    if(mysql_query(conn, query)) {
        logger(logger_format, "ERROR", mysql_error(conn));
        errexit("something wrong with the database");
    }
    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    strcpy(role, row[0]);
    mysql_free_result(result);
    return;
}

void commandHandler(int sock, MYSQL* conn, char* username, char* token) {
    const char* logger_format = "commandHandler [%s] %s";
    const char* last_msg = "Server ERROR, closing connection";

    char role[10];
    getRole(conn, username, role);

    int run_flag = 1;
    while(run_flag) {
        // get current working directory
        char cwd[BUFSIZ];
        memset(cwd, 0, BUFSIZ);
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            fprintf(stdout, "Current working dir: %s\n", cwd);
        }
        else {
            write(sock, last_msg, sizeof(last_msg));
            errexit("cannot getcwd");
        }


       char buf[BUFSIZ];
       memset(buf, 0, BUFSIZ);

        // get user loginKey
       if(read(sock, buf, strlen(token)) != strlen(token) || strcmp(buf, token) != 0) {
            const char* message = "log out! wrong key\0";
            write(sock, message, strlen(message));
            logger(logger_format, "ERROR", "wrong key from client!");
            return;
        } else {
            char* command = strtok(buf, " ");
            if(strcmp(command, "ls") == 0) {
                    // do ls
            }
            if(strcmp(command, "cd") == 0) {
                    // do cd
            }
            if(strcmp(command, "upload") == 0) {
                    // do upload
            }
            if(strcmp(command, "download") == 0) {
                    // do download
            }
            if(strcmp(command, "logout") == 0) {
                logger(logger_format, "INFO", "user logout");
                return;
            }
        }
    }
}

void getToken(char* token) {
    FILE* fp;
    char path[BUFSIZ];

    /* Open the command for reading. */
    fp = popen("date +%s | sha256sum | base64 | head -c 32 ; echo", "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    /* Read the output a line at a time - output it. */
    fgets(path, sizeof(path)-1, fp);
    strcpy(token, path);

    /* close */
    pclose(fp);

    return;
}

int loginHandler(int sock, MYSQL* conn, char* buf) {
    char* command = strtok(buf, " ");
    char token[33]; token[32] = '\0';

    const char* error = "not yet implent";
    if(strcmp(command, "login") == 0) {
        char* username = strtok(NULL, " ");
        char* password = strtok(NULL, " \n");

        if(login(conn, username, password) == 0) {
            write(sock, "LOGIN\0", 6);
            getToken(token);
            printf("%s\n", token);
            write(sock, token, 33);

            commandHandler(sock, conn, username, token);
        } else {
            write(sock, "FAIL\0", 5);
        }

        return 0;
    } else {
        printf("%s\n", error);
        write(sock, error, strlen(error));
        return -1;
    }
}

/**
 * handle_request: handle client request, call by pthread_create
 * Arguements:
 * input: storage ThreadData, need to be covert to ThreadData before use
 * Return:
 * return nothing. send file name and file content back to client
 * @author: Aaron Lam
 */
void *handle_request (void *input) {
    MYSQL* conn = NULL;
    conn = getConnect(conn);

    /* client socket */
    long *sock_pt = (long*) input;
    int sock = *sock_pt;

    /* message buffer; use default stdio BUFSIZ */
    char buf[BUFSIZ]; 
    memset(buf, 0, BUFSIZ);
    
    // getMessage(sock, buf);
    int count = read(sock, buf, BUFSIZ);
    if( count < 0) {
        printf("read error\n");
    }
    buf[count] = '\0';

    loginHandler(sock, conn, buf);

    close(sock);
    mysql_close(conn);
    pthread_exit(NULL);
}

/**
 * run_server: do server work - get file, and send file
 * use getFilePath() as blocking function before creating thread
 * Arguements:
 * sock     - socket descripter, current open socket
 * Return:
 * return nothing. Report error message (errno) if error
 * @author: Aaron Lam
 */
void run_server (int server_sock) {
    int sock; /* client socket */
    /* thread id */
    pthread_t thread;
    /* thread attribule */
    pthread_attr_t attr;

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    /* set detach state so main don't have to call pthread_join */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* while loop to keep server alive 
     * variables needed for a thread are created in while
     * no share variables between threads except socket
     */
    int run_flag = 1; 
    while (run_flag) {
        // run_flag = 0; /* for mem leak check */


        /* the from address of a client */
        struct sockaddr_in src_addr; 
        /* zero out the src_addr so it can be filled later */
        memset (&src_addr, 0, sizeof(src_addr)); 

        socklen_t socklen = sizeof(src_addr);

        sock = accept(server_sock, (struct sockaddr *)&src_addr, &socklen);

        /* create thread to handle request */
        if (pthread_create(&thread, &attr, handle_request, (void*) &sock) < 0) {
            printf("pthread_create(): %s\n", strerror(errno));
        }
    }

    /* destroy attribute to void memory leak */
    pthread_attr_destroy(&attr);

    /* main thread exit */
    pthread_exit(NULL);
}

/*------------------------------------------------------------------------
 * main - TCP Thread Per Request Server for file transfer service
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[]) {
    /* server socket */
    int sock;
    /* default port number */
    char *service = "9000"; 

    /* get command line input */
    switch (argc) {
        case 1:
        break;
        case 2:
        service = argv[1];
        break;
        default:
        errexit("usage: tcpTPRServer [port]\n");
    }

    /* get server socket */
    sock = passivesock(service, "tcp", BACKLOG);

    /* run server */
    run_server(sock);

}
