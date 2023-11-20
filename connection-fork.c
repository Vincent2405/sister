#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include "request.h"

int initConnectionHandler(void);


void handleConnection(int sockFD,int listenSockDF)
{   
    
    pid_t pid=fork();
    

    if (pid==0)
    {
        //execute als child
        printf("sockFD: %d\n",sockFD);
        FILE* rx;//eingabeFile
        rx=fdopen(sockFD,"r");
        if (rx==NULL)
        {
            perror("fehler bei öffnen vom stream\n");
            fclose(rx);
            close(sockFD);
            exit(EXIT_FAILURE);
        }
        
        int sock_copy = dup(sockFD);
        FILE *wx;
        wx = fdopen(sock_copy, "w");//ausgabeFile
        if (wx==NULL)
        {
            perror("fehler bei öffnen vom stream\n");
            fclose(wx);
            fclose(rx);
            close(sockFD);
            exit(EXIT_FAILURE);
        }

        handleRequest(rx,wx);

        //trenne verbindung
        close(sockFD);
        fclose(wx);
        fclose(rx);
        exit(EXIT_SUCCESS);
    }
    else if (pid > 0) 
    {
        //execute as parent
        close(sockFD); 
    }
    else {
        perror("fork failed\n");
        exit(EXIT_FAILURE);
    }

}


int initConnectionHandler(void)
{
    int ret=initRequestHandler();
    return ret;
}


