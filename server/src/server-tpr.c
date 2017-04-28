/*********************************************************************/
/* Author: Aaron Lam                                                 */
/* Description: TCP server, thread per request                       */
/*********************************************************************/

/* tcpTPRServer.c - main */

#include <pthread.h>

#include "errexit.h"
#include "passivesock.h"
#include "mysql_connect.h"
#include "fileClient.h"
#include "fileServer.h"

#define BACKLOG 10

void sendMsgOver(int sock) {
    printf("OVER\n");
    write(sock, "\0\0\0\0", 4);
}

void do_upload(int sock) {
    char buf[BUFSIZ];
    int fd;

    if (sendFileRequest(sock, buf) == -1) {
        printf("sendFileRequest() error: %s\n", strerror(errno));
        return;
    }
    if ((fd = createFile(sock, buf)) == -1){
        printf("createFile() error: %s\n", strerror(errno));
        return;
    }
    if (getFile(sock, buf, fd) == -1) {
        printf("getFile() error: %s\n", strerror(errno));
        return;
    }
}

void do_download(int sock, struct sockaddr_in src_addr, socklen_t socklen) {
    char buf[BUFSIZ];
    memset (buf, 0, BUFSIZ);

    getFilePath(sock, buf, &src_addr, &socklen);
    sendFileName(sock, buf, src_addr, socklen);
    sendFile(sock, buf, src_addr, socklen);

    return;
}

void do_cd (int sock, char* command, struct sockaddr_in src_addr, socklen_t socklen) {
    const char* logger_format = "do_cd [%s] %s\n";

    // change directory
    if( chdir(strtok(NULL, " \n")) == -1) {
        logger(logger_format, "ERROR", "cannot do_cd");
    }
    char cwd[BUFSIZ];
    // get current directory
    getcwd(cwd, sizeof(cwd));
    // send current directory back to client
    write(sock, cwd, strlen(cwd));
    
    return;
}

void do_ls (int sock, char* command, struct sockaddr_in src_addr, socklen_t socklen) {
    const char* logger_format = "do_ls [%s] %s\n";

    char ls[BUFSIZ];
    memset(ls, 0, BUFSIZ);
    strcpy(ls, "/bin/ls -la ");

    if((command = strtok(NULL, "\n"))) {
        strcat(ls, command);
    }
    logger(logger_format, "COMMAND", ls);

    // user file and popen to get output of linux command
    FILE *fp;
    fp = popen(ls, "r");
    // if error
    if(fp == NULL) {
        logger(logger_format, "ERROR", "cannot do_ls");
    }
    
    // get the output of ls and send it back to client
    char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ);
    while(fgets(buf, BUFSIZ, fp) != NULL) {
        write(sock, buf, strlen(buf));
        memset(buf, 0, BUFSIZ);
    }
    pclose(fp);
    return;
}

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


/**
 * commandHandler: handle user command, check if user has the security token
 * compare the security token generated for this session with the user's token
 * if security token match: 
 *      get user command and call the correct function to handle it
 * if no sercurity token || token doesn't match:
 *      send login fail message back to client
 *      terminate this session
 *
 * Arguements:
 *      int sock:       current open socket use to talk to connected client
 *      MYSQL* conn:    current open connection to MySQL database
 *      char* username: the client's username
 *      char* token:    the security token generated for this session
 * Return:
 *      return 0 if login success; -1 if fail
 * @author: Aaron Lam
 */
void commandHandler(int sock, MYSQL* conn, char* username, const char* token) {
    char logger_format[BUFSIZ];
    sprintf(logger_format, "thread %lx %s" , pthread_self(), "commandHandler [%s] '%s'\n");
    const char* last_msg = "Server ERROR, closing connection";

    // must have but not use variable
    struct sockaddr_in src_addr;
    memset (&src_addr, 0, sizeof(src_addr));
    socklen_t socklen = sizeof(src_addr);

    char role[10];
    getRole(conn, username, role);
    logger(logger_format, "INFO", role);

    chdir("courses");

    int run_flag = 1;
    while(run_flag) {
        // get current working directory
        char cwd[BUFSIZ];
        memset(cwd, 0, BUFSIZ);

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            logger(logger_format, "CWD", cwd);
        }
        else {
            write(sock, last_msg, sizeof(last_msg));
            errexit("cannot getcwd");
        }

        // get user loginKey
        char buf[BUFSIZ];
        memset(buf, 0, BUFSIZ);
        read(sock, buf, 33);

        printf("token: %s", token);
        printf("key: %s", buf);

        // check user loginKey
        if(strncmp(buf, token, 32) != 0) {
            logger(logger_format, "ERROR", "wrong key from client!");
            const char* message = "log out! wrong key\0";
            write(sock, message, strlen(message));

            return;
        }

        logger(logger_format, "INFO", "loginKey check out");

        // get user command from client
        memset(buf, 0, BUFSIZ);
        int count = read(sock, buf, BUFSIZ);

        // user send an emtpy command
        if(count == 0 || strlen(buf) == 0) {
            logger(logger_format, "DEBUG", "empty command");
            sendMsgOver(sock);
            continue;
        }
        logger(logger_format, "COMMAND", buf);

        //=============================
        // handle logout
        if(strncmp(buf, "logout", 6) == 0) {
            logger(logger_format, "INFO", "user logout");
            return;
        }

        // parse input, and handle command accordingly
        char* command = strtok(buf, " \n");

        //=============================
        // do ls -- list all file in current directory
        if(strcmp(command, "ls") == 0) {
            do_ls(sock, command, src_addr, socklen);

            // send message over
            sendMsgOver(sock);
            continue;
        }
        //=============================
        // do cd -- change directory to 
        if(strcmp(command, "cd") == 0) {
            do_cd(sock, command, src_addr, socklen);

            // send message over
            sendMsgOver(sock);
            continue;
        }
        //=============================
        // do mkdir -- create new directory
        if(strcmp(command, "mkdir") == 0) {
            char mkdir[BUFSIZ];
            memset(mkdir, 0, BUFSIZ);
            sprintf(mkdir, "mkdir %s", strtok(NULL, "\n"));
            system(mkdir);
            
            // send message over
            sendMsgOver(sock);
            continue;
        }
        //=============================
        // do rm -- remove file or directory
        // only rm file from current directory
        if(strcmp(command, "rm") == 0) {
            char rm[BUFSIZ];
            memset(rm, 0, BUFSIZ);
            sprintf(rm, "rm -rf %s/%s", cwd, strtok(NULL, "\n"));
            system(rm);
            
            // send message over
            sendMsgOver(sock);
            continue;
        }
        //=============================
        // handle upload
        // user want to upload file to server
        // get file from user and save it
        if(strcmp(command, "upload") == 0) {
            do_download(sock, src_addr, socklen);

            // send message over
            sendMsgOver(sock);
            continue;
        }
        //=============================
        // handle download
        // user want to download a file from server
        // send the file back to client
        if(strcmp(command, "download") == 0) {
            do_download(sock, src_addr, socklen);
            char* end = "\0";
            write(sock, end, sizeof(end));
            // send message over
            sendMsgOver(sock);
            continue;
        }
        //=============================
        // command not handle
        // send message over
        sendMsgOver(sock);
    }
    // end while

    return;
}

/**
 * getToken: generate a (highly) unquie 32 character token
 * 
 * Arguements:
 *      char* token: should be char[33] use to store the return of this function
 * Return:
 *      void
 * @author: Aaron Lam
 */
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

/**
 * loginHandler: handle user login, check if username and passwork is valid
 * if login success: 
 *      call getToken() to get security token
 *      send login success messge back to client, along with security token
 *      call commandHandler() to handle user command
 * if login fail:
 *      send login fail message back to client
 *
 * Arguements:
 *      int sock: current open socket use to talk to connected client
 *      MYSQL* conn: current open connection to MySQL database
 *      char* buf: the buffer that storage the user input
 * Return:
 *      return 0 if login success; -1 if fail
 * @author: Aaron Lam
 */
int loginHandler(int sock, MYSQL* conn, char* buf) {
    if(buf == NULL || strlen(buf) == 0) {
        return -1;
    }

    char* command = strtok(buf, " ");
    char token[33];

    const char* error = "not yet implent";
    if(strcmp(command, "login") == 0) {
        char* username = strtok(NULL, " ");
        char* password = strtok(NULL, " \n");

        if(login(conn, username, password) == 0) {
            write(sock, "LOGIN\0", 6);
            getToken(token);
            token[33] = '\0';
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
    const char* logger_format = "thread %02lx [%s] %s\n";
    logger(logger_format, pthread_self(), "INFO", "start");
    MYSQL* conn = NULL;
    conn = getConnect(conn);

    /* client socket */
    long *sock_pt = (long*) input;
    int sock = *sock_pt;

    // message buffer; use default stdio BUFSIZ
    char buf[BUFSIZ]; 
    memset(buf, 0, BUFSIZ);
    
    // getMessage(sock, buf);
    int count = read(sock, buf, BUFSIZ);
    if( count < 0) {
        printf("read error\n");
    }
    buf[count] = '\0';

    loginHandler(sock, conn, buf);

    logger(logger_format, pthread_self(), "INFO", "stop");

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
    const char* logger_format = "server [%s] %s\n";
    logger(logger_format, "INFO", "start");
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

    logger(logger_format, "INFO", "stop");
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
        errexit("usage: server [port]\n");
    }

    /* get server socket */
    sock = passivesock(service, "tcp", BACKLOG);

    /* run server */
    run_server(sock);

}
