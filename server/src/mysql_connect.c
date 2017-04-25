/*------------------------------------------------------------------------
* connect.c - connect to mysql and run query
*------------------------------------------------------------------------
*/

// #include "mysql.h"
#include "mysql_connect.h"


void logger (const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

MYSQL* getConnect (MYSQL *conn) {
    const char* logger_format = "getConnect [%s] %s\n";

    conn = mysql_init(NULL);

    char *server = "";
    char *user = "cmpe207";
    char *password = "cmpe207-Group8";
    char *database = "cmpe207";
    int port = 3306;

    /* Connect to database */
    if (!mysql_real_connect(conn, server, user, password, database, port, NULL, 0)) {
        logger(logger_format, "ERROR", mysql_error(conn));
        exit(1);
    }

    logger(logger_format, "INFO", "connected");
    return conn;
}

int login(MYSQL* conn, char* username, char* password) {
    const char* logger_format = "login [%s] %s\n";

    char* prepare = "SELECT login_function('%s', '%s')";
    char query[BUFSIZ];
    sprintf(query, prepare, username, password);

    if(mysql_query(conn, query)) {
        logger(logger_format, "ERROR", mysql_error(conn));
        return -1;
    } 

    MYSQL_RES *result = mysql_store_result(conn);

    MYSQL_ROW row = mysql_fetch_row(result);

    if(atoi(row[0]) == 1) {
        logger(logger_format, "INFO", "login success");
        return 0;
    } else {
        logger(logger_format, "INFO", "invalid username or password");
        return -1;
    }
}

int do_query (MYSQL* conn, const char* query) {
    const char* logger_format = "do_query [%s] %s\n";

    logger(logger_format, "INFO: try query", query);


    mysql_query(conn, query);

    MYSQL_RES *result = mysql_store_result(conn);

    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        for(int i = 0; i < num_fields; i++) { 
            printf("%s ", row[i] ? row[i] : "NULL");
        } 
        printf("\n"); 
    }

    mysql_free_result(result);

    return 0;
}


// /*Unit test*/
// int main() {
//     MYSQL *conn = getConnect(conn);

//     char username[40];
//     printf("username: ");
//     scanf("%s", username);

//     char pass[40];
//     strcpy(pass, getpass("password: "));

//     login(conn, username, pass);

//     // do_query(conn, "SELECT * FROM login");

//     /* close connection */
//     mysql_close(conn);
// }
