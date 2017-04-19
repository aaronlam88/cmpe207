/*------------------------------------------------------------------------
* connect.c - connect to mysql and run query
*------------------------------------------------------------------------
*/

// #include "mysql.h"
#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void logger (const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

int login(MYSQL* conn, char* username, char* password) {
    const char* logger_format = "login [%s] %s\n";

    MYSQL_RES *res;

    char* prepare = "SELECT login_function('%s', '%s')";
    char query[BUFSIZ];
    sprintf(query, prepare, username, password);

    if(mysql_query(conn, query)) {
        logger(logger_format, "ERROR", mysql_error(conn));
        return 0;
    } else {
        res = mysql_store_result(conn);
        unsigned long row_count = (unsigned long) mysql_num_rows(res);
        mysql_free_result(res);

        if(row_count == 1) {
            logger(logger_format, "INFO", "login success");
            return 1;
        } else {
            logger(logger_format, "INFO", "login fail");
            return 0;
        }
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

int main() {
    MYSQL *conn = mysql_init(NULL);

    char *server = "";
    char *user = "cmpe207";
    char *password = "cmpe207-Group8";
    char *database = "cmpe207";
    int port = 3306;

    /* Connect to database */
    if (!mysql_real_connect(conn, server, user, password, database, port, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    char username[40];
    printf("username: ");
    scanf("%s", username);

    char pass[40];
    strcpy(pass, getpass("password: "));

    login(conn, username, pass);

    do_query(conn, "SELECT * FROM login");

    /* close connection */
    mysql_close(conn);
}
