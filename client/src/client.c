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


void run_client(int sock) {
    char username[40];
    char buf[BUFSIZ];
    //int n_read;
    //char buf_ls[BUFSIZ];
    //char list[40];
    printf("username: ");
    scanf("%s", username);

    char pass[40];
    bzero(buf, BUFSIZ);
    strcpy(pass, getpass("password: "));

    strcat(buf,"login ");
    strcat(buf, username);
    strcat(buf," ");
    strcat(buf,pass);

    write(sock,buf,strlen(buf));
    bzero(buf, strlen(buf));
    read(sock,buf,BUFSIZ);

    
    	printf("%s\n",buf);
    	bzero(buf,BUFSIZ);
    

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
    int sock1 = connectsock(host, service, "tcp");

    run_client(sock);
    runn_client(sock1);
    return 0;
}
