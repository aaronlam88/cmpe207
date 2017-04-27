#ifndef TCP_FILE_SERVER_H
#define TCP_FILE_SERVER_H

#include "errexit.h"
#include "passivesock.h"

#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

void getFilePath (int sock, char *buf);

void sendFileName (int sock, char *buf);

void sendFile (int sock, char *buf);

#endif
