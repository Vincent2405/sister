
#include <stdio.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "connection.h"
#include "request.h"
#include "cmdline.h"
#include <signal.h>
#include <sys/wait.h>

void sigchld_handler(int signum) {
    // warten auf alle beendeten Kindprozesse
    // nicht stoppen(WNOHHANG) 
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char const *argv[])
{
    int default_port=1337;
    int ret3;
    
    //read input
    ret3 = cmdlineInit(argc, (char**)argv);
    if (ret3 != 0) {
        perror("error");
        return -1;
    }


    const char *path = cmdlineGetValueForKey("wwwpath");
    if (path == NULL) {
        fprintf(stderr, "error: No wwwpath specified.\n");
        return -1;
    }


    int port;

    const char* port_str = cmdlineGetValueForKey("port");
    if (port_str != NULL) {
        char *endptr;
        errno = 0;

        port = strtol(port_str, &endptr, 10);
        if (errno != 0)
        {
            perror("error");
            return -1;
        }
        if (endptr == port_str || *endptr != '\0') {
            fprintf(stderr, "error: Invalid port.\n");
            return -1;
        }
    } else {
        port = default_port;
    }

    printf("port: %d\n",port);
    printf("wwwpath: %s\n",path);

    if(initConnectionHandler()!=0)
    {
        //fehler
        exit(EXIT_FAILURE);
        return 0;
    }

    struct sockaddr_in6 addr = {
        .sin6_family = AF_INET6,
        .sin6_port = htons(port),
        .sin6_addr = in6addr_any,
    };
    
    int listenSock=socket( AF_INET6, SOCK_STREAM,0);
    if (listenSock==-1)
    {
        fprintf(stderr,"socket konnte nicht ersatellt werden!\n");
        exit(EXIT_FAILURE);
    }
    
    int flag = 1;
    int ret=setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if (ret!=0)
    {
        perror("setsock Fehler");
        exit(EXIT_FAILURE);
    }

    int ret2=bind(listenSock,((struct sockaddr*)&addr),sizeof(addr)); //binde addr zum socket
    if (ret2!=0)
    {
        exit(EXIT_FAILURE);
    }
    

    if(listen(listenSock,SOMAXCONN)!=0)
    {
        printf("listen failed\n");
        return -1;
        //fehler
    }
    printf("listen succesful\n");
    int acceptSockets=1; 
    struct sigaction sa;

    // Signalhandler einrichten
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction err\n");
        exit(1);
    }   
    while (acceptSockets)
    {
        struct sockaddr_in6 clientAddr;
        memset(&clientAddr,0,sizeof(struct sockaddr_in6));
        socklen_t len=sizeof(clientAddr); 
        int newConnectionFD=accept(listenSock,(struct sockaddr*)&clientAddr,&len);
        setsockopt(newConnectionFD, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
        //printf("%d \n",ntohs(*clientAddr.sin6_addr.__in6_u.__u6_addr16));
        handleConnection(newConnectionFD,listenSock);//handle new connnecions
    }

    return 0;
}
