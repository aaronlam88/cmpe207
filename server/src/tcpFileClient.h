#ifndef SYMBOL
#define SYMBOL value

#include "errexit.h"
#include "connectsock.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int sendFileRequest (int sock, char *buf);
int createFile (int sock, char *buf);
int getFile (int sock, char *buf, int fd);

#endif