
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "i4httools.h"
#include "cmdline.h"

char* checkRequest(char*);
int sendHtmlFileToOutputStream(const char*, FILE*);

void handleRequest(FILE* rx,FILE* wx)
{

    //lies http anfragezeile
    char currentline[8192];
    fgets(currentline,sizeof(currentline),rx);
    printf("anfrageZeile: %s\n",currentline);
    
    //check Request
    char* relPath=checkRequest(currentline);
    if (relPath==NULL)
    {
        //sende fehlermeldung an server
        httpBadRequest(wx);
        fclose(wx);
        fclose(rx);
        exit(EXIT_FAILURE);
        return;
    }
    
    printf("relpath: %s\n",relPath);

    char* absPath=(char*)cmdlineGetValueForKey("wwwpath");
    char fullpath[strlen(absPath)+strlen(relPath)+1];
    snprintf(fullpath, sizeof(fullpath), "%s%s", absPath, relPath);

    printf("fullpath: %s\n",fullpath);

    if(checkPath(fullpath)==-1)
    {
        
        //return error massage to server
        httpForbidden(wx,relPath);
        fclose(wx);
        fclose(rx);

        exit(EXIT_FAILURE);
        return;
        
    }
    else
    {
        int r=sendHtmlFileToOutputStream(fullpath,wx);
        if (r==0)
        {
            printf("succsessful\n");
            fclose(wx);
            fclose(rx);
            exit(EXIT_SUCCESS);
        }
        else if (r==ENOENT)
        {
            httpNotFound(wx,relPath);
            fclose(wx);
            fclose(rx);
            exit(EXIT_FAILURE);
        }
        else
        {
            httpInternalServerError(wx,relPath);
            fclose(wx);
            fclose(rx);
            exit(EXIT_FAILURE);
        }
    }
    
}

int sendHtmlFileToOutputStream(const char* filename, FILE* wx) {
    errno=0;
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Fehler beim Öffnen der Datei");
        //return not found
        return errno;
    }
    char buffer[1024];
    size_t bytesRead;
    httpOK(wx);

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        fwrite(buffer, 1, bytesRead, wx);
    }
    
    fclose(file);
    return 0;

}

char* checkRequest(char* requestLine)
{
    char* path;
    char* token=strtok(requestLine," ");
    if (strcmp(token,"GET")!=0)
    {
        fprintf(stderr,"fehler bei GET");
        return NULL;
    }
    path = strtok(NULL," ");

    token=strtok(NULL,"\r\n");
    //check ob Version = HTTP/1.0 or HTTP/1.1
    if((strcmp(token,"HTTP/1.0")==0) || (strcmp(token,"HTTP/1.1")==0))
    {
        return path;
    }
    fprintf(stderr,"fehler bei HTTP version: %s.",token);
    return NULL;
    
}
int initRequestHandler(void)
{
    return 0;
}

