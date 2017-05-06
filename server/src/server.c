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

/**
 * sendMsgOver: send message over signal back to client
 *      this functiion should be call at the end of every messages from server to client
 * 
 * Arguements:
 *      int sock: current open socket, use to write query result back to client
 * Return:
 *      return nothing
 * @author: Aaron Lam
 */
void sendMsgOver(int sock) {
    printf("\n=======================================\n");
    write(sock, "\0", 1);
}

/**
 * do_upload: handle upload command from client
 *      get file from client and save it to current directory
 * 
 * Arguements:
 *      int sock     : current open socket, use to write query result back to client
 * Return:
 *      return nothing
 * @author: Aaron Lam
 */
void do_upload(int sock) {
    char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ);

    // get file name
    read(sock, buf, BUFSIZ);

    // create or overwrite file with file name
    printf("createFile: %s\n", buf);
    mode_t mode = 0666;
    int fd = open(buf, O_WRONLY | O_CREAT, mode);

    // current client will not expect error message from upload
    // therefore, not error message is sent back to client
    // only log error to stdout
    if ( fd == -1 ) {
        printf("open() error: %s\n", strerror(errno));
        return;
    }
    if (getFile(sock, buf, fd) == -1) {
        printf("getFile() error: %s\n", strerror(errno));
        return;
    }
    return;
}

/**
 * do_download: handle download command from client
 *      send the requested file back to client
 * 
 * Arguements:
 *      int sock     : current open socket, use to write query result back to client
 *      struct sockaddr_in src_addr & socklen_t socklen are needed for functions call
 Return:
 *      return nothing
 * @author: Aaron Lam
 */
void do_download(int sock, struct sockaddr_in src_addr, socklen_t socklen) {
    char buf[BUFSIZ];
    memset (buf, 0, BUFSIZ);

    // get file name
    read(sock, buf, BUFSIZ);

    // only allow download from current directory
    // therefore only get base name from client, no full path will be read
    char filename[BUFSIZ];
    memset(filename, 0, BUFSIZ);
    strcpy(filename, basename(buf));

    // get file with filename in current directory and send it back to user
    // current version of client is not expected any error message from server
    // therefore, only termniate sending file when error occur and log error to stdout
    if(strlen(filename) != strlen(buf)) {
        write(sock, "", 0);
        printf("%s %s\n", "do_download [ERROR] ", buf);
        return;
    }
    // send file to client
    sendFile(sock, filename, src_addr, socklen);

    return;
}


/**
 * do_cd: handle cd command from client, call linux cd and behave like linux cd
 *      cd will do security check, cd will not allow user to move out of their designated directory
 *      if users move out of their designated directory, cd will move them to base directory
 *
 * Arguements:
 *      int sock                : current open socket, use to write query result back to client
 *      char* command           : cd command
 *      char* role              : current user role
 *      char* crs_dir           : server course directory (base directory)
 *      char courses[10][255]   : courses directory, use for security check
 * Return:
 *      return nothing
 *      write current directory back to client
 * @author: Aaron Lam
 */
void do_cd (int sock, char* command, char* role, char* crs_dir, char courses[10][255]) {
    const char* logger_format = "do_cd [%s] %s\n";

    // change directory
    chdir(command);
    char cwd[BUFSIZ];
    // get current directory
    getcwd(cwd, sizeof(cwd));



    // create test dir user for in security check
    char test[BUFSIZ];
    strcpy(test, crs_dir);
    strcat(test, "/");

    int allow = 0; // false

    //==============security check==============
    if (strncmp(role, "admin", 5) == 0) {
        // no restriction
        allow = 1;
        logger(logger_format, "INFO", "run as admin");
    } else if (strncmp(role, "teacher", 7) == 0) {
        // class restriction
        logger(logger_format, "INFO", "run as teacher");
        for (int i = 0; i < 10 && courses[i][0] != '\0'; ++i) {
            strcat(test, courses[i]);
            if ((strncmp(test, cwd, strlen(test))) == 0) {
                allow = 1;
                break;
            }
        }
    } else if (strncmp(role, "student", 7) == 0) {
        // student folder restrictino
        logger(logger_format, "INFO", "run as student");
        for (int i = 0; i < 10 && courses[i][0] != '\0'; ++i) {
            strcat(test, courses[i]);
            if ((strncmp(test, cwd, strlen(test))) == 0) {
                allow = 1;
                break;
            }
        }
    } else {
        // unknown role, for future use
        // set to no use for now
        logger(logger_format, "DEBUG", "unknown role");
    }
    //=========================================

    if (strcmp(cwd, crs_dir) == 0) {
        // user move back to courses dir allow
        allow = 1;
    }
    if (allow) {
        // send current directory back to client
        write(sock, cwd, strlen(cwd));
    } else {
        // move user back to courses dir
        chdir(crs_dir);
        const char* msg = "you don't have the right to access this directory";
        write(sock, msg, strlen(msg));
    }

    return;
}

/**
 * do_ls: handle ls command from client, call linux ls and behave like linux ls with -la option
 *
 * Arguements:
 *      int sock        : current open socket, use to write query result back to client
 *      char* command   : ls command
 * Return:
 *      return nothing
 *      write ls -la result back to client
 * @author: Aaron Lam
 */
void do_ls (int sock, char* command) {
    const char* logger_format = "do_ls [%s] %s\n";

    char ls[BUFSIZ];
    memset(ls, 0, BUFSIZ);
    strcpy(ls, "/bin/ls -la ");
    if (command) {
        strcat(ls, command);
    }

    logger(logger_format, "COMMAND", ls);

    // user file and popen to get output of linux command
    FILE *fp;
    fp = popen(ls, "r");
    // if error
    if (fp == NULL) {
        logger(logger_format, "ERROR", "cannot do_ls");
    }

    // get the output of ls and send it back to client
    char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ);
    while (fgets(buf, BUFSIZ, fp) != NULL) {
        write(sock, buf, strlen(buf));
        memset(buf, 0, BUFSIZ);
    }
    pclose(fp);
    return;
}

/**
 * do_sql: handle sql command from client, limit the query to cmpe207 database
 *
 * Arguements:
 *      int sock        : current open socket, use to write query result back to client
 *      MYSQL* conn     : current open connection to MySQL database
 *      char* query     : the query get from client and will be perform
 * Return:
 *      return nothing
 *      write query result back to client
 * @author: Aaron Lam
 */
void do_sql(int sock, MYSQL* conn, char* query) {
    const char* logger_format = "do_sql [%s] %s\n";
    const char* prepare = "mysql cmpe207 -t -u cmpe207 -pcmpe207-Group8 --execute \"%s\"";
    char buf[BUFSIZ];
    sprintf(buf, prepare, query);
    printf("query: %s\n", buf);

    // user file and popen to get output of linux command
    FILE *fp;
    fp = popen(buf, "r");
    // if error
    if (fp == NULL) {
        logger(logger_format, "ERROR", "cannot do_ls");
    }

    // get the output of ls and send it back to client
    memset(buf, 0, BUFSIZ);
    while (fgets(buf, BUFSIZ, fp) != NULL) {
        write(sock, buf, strlen(buf));
        memset(buf, 0, BUFSIZ);
    }
    pclose(fp);

    return;
}


/**
 * getRole: get the user's role from database
 *
 * Arguements:
 *      MYSQL* conn:    current open connection to MySQL database
 *      char* username: the client's username
 *      char* role:     get the role from database, and return to caller
 * Return:
 *      return nothing
 * @author: Aaron Lam
 */
void getRole(MYSQL* conn, char* username, char* role) {
    const char* logger_format = "getRole [%s] %s\n";
    const char* prepare = "SELECT role FROM login WHERE username = '%s'";
    char query[BUFSIZ];
    sprintf(query, prepare, username);
    if (mysql_query(conn, query)) {
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
 * getCourses: get courses that user currently involve in
 *              max number of courses is 10
 *              max length of course name is 255
 *
 * Arguements:
 *      MYSQL* conn             : current open connection to MySQL database
 *      char* username          : the client's username
 *      char courses[10][255]   : the return value to caller
 * Return:
 *      return nothing
 * @author: Aaron Lam
 */
void getCourses(MYSQL* conn, char* username, char courses[10][255]) {
    const char* logger_format = "getCourses [%s] %s\n";
    const char* prepare = "SELECT coursename FROM course WHERE username = '%s'";

    char query[BUFSIZ];
    sprintf(query, prepare, username);
    if (mysql_query(conn, query)) {
        logger(logger_format, "ERROR", mysql_error(conn));
        errexit("something wrong with the database\n");
    }
    MYSQL_RES *result = mysql_store_result(conn);

    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)) && i < 10) {
        strcpy(courses[i++], row[0]);
    }
    mysql_free_result(result);
    return;
}


/**
 * commandHandler: handle user command, check if user has the security token
 * compare the security token generated for this session with the user's token
 *
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
    // logger message format
    char logger_format[BUFSIZ];
    sprintf(logger_format, "Thread [%lx] %s" , pthread_self(), "commandHandler [%s] '%s'\n");

    // user try to run admin command warning
    char warning[BUFSIZ];
    sprintf(warning, "Attemp to run admin command, %s your action has been recorded!", username);

    // last words from server before it shutdown
    const char* last_msg = "Server ERROR, closing connection";

    // must have but not use variable
    struct sockaddr_in src_addr;
    memset (&src_addr, 0, sizeof(src_addr));
    socklen_t socklen = sizeof(src_addr);

    // get the user role
    char role[10];
    getRole(conn, username, role);
    logger(logger_format, "ROLE", role);

    // get the user folder
    // max number of courses a person can invole is 10
    // max length of course name is 255
    char courses[10][255];
    for (int i = 0; i < 10; ++i) {
        memset(courses[i], 0, 255);
    }
    getCourses(conn, username, courses);
    for (int i = 0; i < 10 && courses[i][0] != '\0'; ++i) {
        printf("%s\n", courses[i]);
    }

    // change working directory to ./courses
    chdir("./courses");

    // get current courses directory
    char crs_dir[255];
    getcwd(crs_dir, sizeof(crs_dir));

    int run_flag = 1;
    while (run_flag) {
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

        // logger(logger_format, "TOKEN", token);
        // logger(logger_format, "KEY", buf);

        // check user loginKey
        if (strncmp(buf, token, 32) != 0) {
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
        if (count == 0 || strlen(buf) == 0) {
            logger(logger_format, "DEBUG", "empty command");
            sendMsgOver(sock);
            continue;
        }
        logger(logger_format, "COMMAND", buf);

        //=============================
        // handle logout
        if (strncmp(buf, "logout", 6) == 0) {
            logger(logger_format, "INFO", "user logout");
            return;
        }

        // parse input, and handle command accordingly
        char* command = strtok(buf, " \n");

        //=============================
        // do ls -- list all file in current directory
        if (strcmp(command, "ls") == 0) {
            command = strtok(NULL, "\n\0");

            //==============security check==============
            // no need to check for ls
            //=========================================
            do_ls(sock, command);

            // send message over
            sendMsgOver(sock);
            continue;
        }
        //=============================
        // do cd -- change directory to
        if (strcmp(command, "cd") == 0) {
            command = strtok(NULL, " \n\0");

            //==============security check==============
            // let do_cd function do the checking
            //=========================================
            do_cd(sock, command, role, crs_dir, courses);

            // send message over
            sendMsgOver(sock);
            continue;
        }
        //=============================
        // do mkdir -- create new directory
        if (strcmp(command, "mkdir") == 0) {

            //==============security check==============
            if (strncmp(role, "admin", 5) != 0) {
                logger(logger_format, "WARING: mkdir", username);
                write(sock, warning, strlen(warning));
                sendMsgOver(sock);
                continue;
            }
            //=========================================

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
        if (strcmp(command, "rm") == 0) {

            //==============security check==============
            if (strncmp(role, "admin", 5) != 0) {
                logger(logger_format, "WARING: rm", username);
                write(sock, warning, strlen(warning));
                sendMsgOver(sock);
                continue;
            }
            //=========================================

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
        if (strcmp(command, "upload") == 0) {

            //==============security check==============
            // only allow upload to current folder
            // do_cd should only allow user to get into the right folder
            //=========================================

            do_upload(sock);

            // send message over
            sendMsgOver(sock);
            continue;
        }
        //=============================
        // handle download
        // user want to download a file from server
        // send the file back to client
        if (strcmp(command, "download") == 0) {

            //==============security check==============
            // only allow download from current folder
            // do_cd should only allow user to get into the right folder
            //=========================================

            do_download(sock, src_addr, socklen);

            // send message over
            sendMsgOver(sock);
            continue;
        }
        //=============================
        // handle sql query
        // user want to download a file from server
        // send the file back to client
        if (strcmp(command, "sql") == 0) {

            //==============security check==============
            if (strncmp(role, "admin", 5) != 0) {
                logger(logger_format, "WARING: sql", username);
                write(sock, warning, strlen(warning));
                sendMsgOver(sock);
                continue;
            }
            //=========================================
            command = strtok(NULL, "\n\0");
            logger(logger_format, "SQL", command);
            do_sql(sock, conn, command);
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
    fp = popen("shuf -i 1-2147483646 -n 1 | sha256sum | base64 | head -c 32 ; echo", "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    /* Read the output a line at a time - output it. */
    fgets(path, sizeof(path) - 1, fp);
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
    if (buf == NULL || strlen(buf) == 0) {
        return -1;
    }

    char* command = strtok(buf, " ");
    char token[33];

    const char* error = "not yet implent";
    if (strcmp(command, "login") == 0) {
        char* username = strtok(NULL, " ");
        char* password = strtok(NULL, " \n");

        if (login(conn, username, password) == 0) {
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
    if ( count < 0) {
        printf("read error\n");
    } else {
        buf[count] = '\0';
        loginHandler(sock, conn, buf);
    }

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
