#include "/opt/lampp/include/mysql.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  MYSQL *conn;
  MYSQL_RES *res;
  MYSQL_ROW row;

  char *server = "";
  char *user = "root";
  char *password = "040188Lin!"; // set me first
  char *database = "cmpe226";
  int port = 3306;
  conn = mysql_init(NULL);

  /* Connect to database */
  if (!mysql_real_connect(conn, server, user, password, database, port, NULL, 0)) {
    fprintf(stderr, "%s\n", mysql_error(conn));
    exit(1);
  }

  /* send SQL query */
  if (mysql_query(conn, "show tables")) {
    fprintf(stderr, "%s\n", mysql_error(conn));
    exit(1);
  }
  res = mysql_use_result(conn);

  /* output table name */
  printf("MySQL Tables in mysql database:\n");
  while ((row = mysql_fetch_row(res)) != NULL)
    printf("%s \n", row[0]);

  /* close connection */
  mysql_free_result(res);
  mysql_close(conn);
}
