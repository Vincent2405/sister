#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#define PORT 7370
#define MAX_USER_NAME 20
#define ROLE_TUTOR 0
#define ROLE_STUDENT 1
#define ROLE_UNKNOWN -1
// vorgegeben Strukturen:
typedef struct client {
    FILE *rx, *tx;
    struct client *next;
} client_t;

struct fifo {
    client_t *head;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
};
// vorgegeben Funktionen:
static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void usage(const char *name) {
    fprintf(stderr, "USAGE: %s <tutor-names...>\n", name);
}
// Globale Variablen:
static void fifo_put(struct fifo *f, client_t *c);
static client_t * client_create(int fd);
static void client_destroy(client_t *c);
static client_t *fifo_get(struct fifo *f);
static void *worker(void *tutors);


static struct fifo fifo_accept = {
    .head = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .condition = PTHREAD_COND_INITIALIZER,
};
static struct fifo fifo_students = {
    .head = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .condition = PTHREAD_COND_INITIALIZER,
};
int main(int argc, char *argv[])
{
    // Argumente prüfen und Prozesszustand initialisieren
    char* tuts[argc];
    for (int i = 0; i < argc-1; i++)
    {
        tuts[i]=strdup(argv[i+1]);    
    }
    //Null terminierend
    tuts[argc]=NULL;
    
    int sockFd=socket(AF_INET6,SOCK_STREAM,0);
    if (sockFd<0)
    {
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in6 s6;
    memset(&s6, 0, sizeof(s6));
    s6.sin6_port=htons(PORT); 
    s6.sin6_family=AF_INET6;
    s6.sin6_addr=in6addr_any;

    int ret=bind(sockFd,(struct sockaddr*)&s6,sizeof(s6));
    if (ret!=0)
    {
        exit(EXIT_FAILURE);
    }
    
    if(listen(sockFd,10)!=0)
    {
        exit(EXIT_FAILURE);
    }

    // Arbeiterfäden starten
    pthread_t pid;
    int ret2=pthread_create(&pid,NULL,worker,tuts);
    if (ret2!=0)
    {
        die("tete");
        exit(EXIT_FAILURE);
    }

    
    // Socket öffnen
    while (1)
    {
        int clientFD=accept(sockFd,NULL,0);
        if (clientFD<0)
        {
            continue;
        }
        else
        {
            client_t *c=client_create(clientFD);
            if (c!=NULL)
            {
                fifo_put(&fifo_accept,c);
            }
        }
        
    }

    close(sockFd);
    for (int i = 0; i < argc-1; i++)
    {
        free(tuts[i]);
    }
    exit(EXIT_SUCCESS);

} 
static int exchange_line(FILE *rx, FILE *tx)
{
    int buffersize=100;
    char buffer[buffersize];
    if (fgets(buffer,buffersize,rx)==NULL)
    {
        perror("fehler beim einlesen");
        return -1;
    }
    
    if (fputs(buffer,tx)==EOF)
    {
        perror("fehler beim schreiben");
        return -1;
    }
    fflush(rx);
    return 0;
    
}
static client_t * client_create(int fd)
{
    struct client* c=malloc(sizeof(struct client));
    if (c==NULL)
    {
        perror("malloc failure");
        return NULL;
    }
    c->rx=fdopen(fd,"r");
    if (c->rx==NULL)
    {
        close(fd);
        free(c);
        perror("konnte File nicht öffnen");
        return NULL;
    }
    
    c->tx=fdopen(fd,"w");
    if (c->rx==NULL)
    {
        fclose(c->rx);
        free(c);
        close(fd);
        perror("konnte File nicht öffnen");
        return NULL;
    }
    return c;


}
static void client_destroy(client_t *c)
{
    if (c==NULL)
    {
        if (c->rx!=NULL)
        {
            fclose(c->rx);
        }
        if (c->tx!=NULL)
        {
            fclose(c->tx);
        }
        free(c);   
    }
}
static void fifo_put(struct fifo *f, client_t *c)
{
    pthread_mutex_lock(&f->mutex);
    if (f==NULL||c==NULL)
    {
        f->head=c;
        
    }
    else
    {
        client_t* currrent=f->head;
        while (currrent->next!=NULL)
        {
            currrent=currrent->next;
        }
        //current am ende der liste
        c->next=NULL;
        currrent->next=c;
    }
    pthread_cond_signal(&f->condition);
    pthread_mutex_unlock(&f->mutex);

}
static client_t *fifo_get(struct fifo *f)
{
    pthread_mutex_lock(&f->mutex);
    while (f->head==NULL)
    {
        //warte passiv
        int ret= pthread_cond_wait(&f->condition, &f->mutex);
    }
    
    client_t* retClient;
    retClient=f->head; 
    f->head=f->head->next;
    
    pthread_mutex_unlock(&f->mutex);
    return retClient;


}
static int role(FILE *rx, const char *tutors[])
{
    // Nutzername einlesen
    int buffersize=MAX_USER_NAME+2;//\0\n
    char buffer[buffersize];
    if(fgets(buffer,buffersize,rx)!=NULL)
    {
        return ROLE_UNKNOWN;
    }
    // Nutzername prüfen
    char* curr=(char*)tutors[0];
    int i=0;
    while(curr!=NULL)
    {
        if (strcmp(buffer,curr)==0)
        {
            return ROLE_TUTOR;
        }
        i++;
        curr=(char*)tutors[i];
    }
    return ROLE_STUDENT;
} 
static void *worker(void *tutors) {
    while (1)
    {
        client_t *c=fifo_get(&fifo_accept);
        if (c==NULL)
        {
            continue;
        }
        
        int r;
        r=role(c->rx,tutors);
        if (r==ROLE_UNKNOWN)
        {
            client_destroy(c);
            return NULL;
        }
        else if(r==ROLE_STUDENT)
        {
            fifo_put(&fifo_students,c);
        }
        else if (r==ROLE_TUTOR)
        {
            //warte auf verbundung;
            client_t* s=fifo_get(&fifo_students);
            if (s==NULL)
            {
                return NULL;
            }
            if(exchange_line(s->rx,s->tx)!=0)
            {
                return NULL;
            };
        }
    }
    
    
}

