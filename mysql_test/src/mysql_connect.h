#ifndef MYSQL_CONNECT_H_
#define MYSQL_CONNECT_H_
/*------------------------------------------------------------------------
* connect.c - connect to mysql and run query
*------------------------------------------------------------------------
*/

// #include "mysql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

void logger (const char* format, ...);
MYSQL* getConnect (MYSQL* conn);
int login (MYSQL* conn, char* username, char* password);
int do_query (MYSQL* conn, const char* query);


#endif