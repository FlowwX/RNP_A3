/**
    Handle multiple socket connections with select and fd_set on Linux
*/
  
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <stdbool.h>    
#include <time.h>
#include <sys/stat.h>
#include <netdb.h>
#include <dirent.h> 


  
#define TRUE   1
#define FALSE  0
#define PORT 8888
#define CHUNK 1024 /* read 1024 bytes at a time */





bool writeDataToClient(int sckt, const void *data, int datalen)
{
    const char *pdata = (const char*) data;

    while (datalen > 0){
        int numSent = send(sckt, pdata, datalen, 0);
        if (numSent <= 0){
            if (numSent == 0){
                printf("The client was not written to: disconnected\n");
            } else {
                perror("The client was not written to");
            }
            return false;
        }
        pdata += numSent;
        datalen -= numSent;
    }

    return true;
}

bool writeStrToClient(int sckt, const char *str)
{
    return writeDataToClient(sckt, str, strlen(str));
}



void sendPassedFile(char *filename,  int sd){

    char buf[CHUNK] = {'\0'};
    FILE *file;
    size_t nread;






    file = fopen(filename, "r");

    printf("after open(%s)\n", filename);
            //get file size
    fseek(file, 0L, SEEK_END);
    int fsize = ftell(file);
    rewind(file);

     printf("after size determining\n");

 
    char cfs[16];
    sprintf(cfs, "%d\n", fsize);
    char last_modified[100] = "";
    struct stat b;
    if (!stat(file, &b)) {
        strftime(last_modified, 100, "%d/%m/%Y %H:%M:%S\n", localtime( &b.st_ctime));
        printf("\nLast modified date and time = %s\n", last_modified);
    }
    else{
        strcpy(last_modified, "not available");
    }


    //build header
    writeStrToClient(sd, cfs);
    writeStrToClient(sd, "last_modify:");
    writeStrToClient(sd, last_modified);
    writeStrToClient(sd, "\n");
    writeStrToClient(sd, "======\n");
    

    printf("after header\n");

    if (file != NULL ) {
        while ((nread = fread(buf, 1, sizeof buf, file)) > 0){

            send(sd, buf, nread, 0);
            printf("%d\n", nread);

        }

        if (ferror(file)) {
            printf("ERROR in reading file: %s\n", filename);
        }
        fclose(file);
    }
    else{
        printf("Couldn't open file: %s\n", filename);
    }
}


void splitString(const void *stri, const void *delimiter, char results[][10240] ){
    char *ptr;

    



    // initialisieren und ersten Abschnitt erstellen
    ptr = strtok(stri, delimiter);
    
    int idx = 0;

    while(ptr != NULL) {

        if(idx<=5){
            strcpy( results[idx], ptr);
            printf("->>>%s\n", results[idx]);
            memset(ptr,0,strlen(ptr)); // reset string
        }
    



        // naechsten Abschnitt erstellen
        ptr = strtok(NULL, delimiter);

        idx++;
    }
}




 
int main(int argc , char *argv[])
{
    int opt = TRUE;
    int32_t master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    int amount_of_clients = 0;

    bool mode_put = false;
    FILE *opened_file;

    int fsize, fsize_header;
    int yes=1;
    int rv;
    struct addrinfo hints, *res, *p;


      
    char buffer[1025];  //data buffer of 1K
      
    //set of socket descriptors
    fd_set readfds;
      
    //a message
    char *message = "RNP Server v1.0 \r\n\r\ncommands: \r\n\t - LIST \r\n\t - PUT <filename> \r\n\t - GET <filename>\r\n\t - QUIT \r\n!Important: This program reads everytime two strings as input, so you have\r\nenter always 2 strings and continue with enter-button.\r\n\r\n";
  
    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
    }

 


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP




    if ((rv = getaddrinfo(NULL, "8888", &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }


    // loop through all the results and bind to the first we can
    for(p = res; p != NULL; p = p->ai_next) {
        if ((master_socket = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(master_socket, p->ai_addr, p->ai_addrlen) == -1) {
            close(master_socket);
            perror("server: bind");
            continue;
        }

        break;
    }


 void getPortAndIP(int sd, char* outstr){

    socklen_t len;
    struct sockaddr_storage addr;
    char ipstr[INET6_ADDRSTRLEN];
    int port;

    int s = sd;
    len = sizeof addr;
    getpeername(s, (struct sockaddr*)&addr, &len);

    // deal with both IPv4 and IPv6:
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    } else { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
    }

    sprintf(outstr, "ip:%s on port:%d", ipstr, port);
 }


void readServerDir(char* output){
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            char fib[300] = "";
            sprintf(fib, "\t- %s\n", dir->d_name);
            strcat(output, fib);
        }
        closedir(d);
    }
}



 

 


     

    printf("Listener on port %d \n", PORT);
     
    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
      

    puts("Waiting for connections ...");
     
    while(TRUE) 
    {
        //clear the socket set
        FD_ZERO(&readfds);
  
        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
         
        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++) 
        {
            //socket descriptor
            sd = client_socket[i];
             
            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);
             
            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }
  
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    
        if ((activity < 0) && (errno!=EINTR)) 
        {
            printf("select error");
        }
          
        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) 
        {

            sin_size = sizeof their_addr;
            new_socket = accept(master_socket, (struct sockaddr *)&their_addr, &sin_size);
            if (new_socket == -1) {
                perror("accept");
                continue;
            }

            char outp[100] = "";
            getPortAndIP(new_socket,outp);
            //inform user of socket number - used in send and receive commands
            printf("client with %s connects\n" , outp );
        
            //send new connection greeting message
            if( send(new_socket, message, strlen(message), 0) != strlen(message) ) 
            {
                perror("send");
            }
              
            puts("Welcome message sent successfully");
              
            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) 
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                    amount_of_clients++;
                    break;

                }
            }


        }
          
        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++) 
        {

            sd = client_socket[i];

             //if(amount_of_clients>1)  {while(1); }
            if (FD_ISSET( sd , &readfds)) 
            {
            

                memset(buffer,0, CHUNK); //clear buffer
                     

                //Check if it was for closing , and also read the incoming message
                if ((valread = recv( sd , buffer, CHUNK, 0)) == 0) //len = recv(sd, buf, CHUNK, 0)) > 0) )
                {
                  

                    char outp[100] = "";
                    getPortAndIP(sd,outp);
                    printf("Cient(ID:%d) with %s disconnects\n",i, outp);
                      
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                   
                }
                  
                //process client requests
                else
                {
 
                    char res[10][10240];
                    char resu[10][10240];
                    memset(res,0,sizeof(res));
                 
                    
                    //do all operations here
                    if( strncmp(buffer, "GET", strlen("GET") ) == 0 ){

                       

        

                        splitString(buffer, " ", res);
        

                        writeStrToClient( sd, "200 OK\n" );

      

                        sendPassedFile(res[1], sd);

      

                    }
                    else if( strncmp(buffer, "PUT", strlen("PUT") ) == 0 ){
                        buffer[valread] = '\0';
                        mode_put = true;

                        char buf2[10240] = "";
                        strcpy(buf2, buffer);

                        splitString(buffer, " ", res);

                        opened_file = fopen(res[1], "w");
                        fsize = atoi(res[2]);
                        fsize_header = fsize;

                        splitString(buf2, "======", resu);

                        
                        strcpy( buffer, resu[1] );

                        if(strlen(buffer) == 0){
                            valread = 0;
                        }

                    }
                    else if( strncmp(buffer, "LIST", strlen("LIST") ) == 0 ){

                        char repsonse[4*1024];
                        char payload[3*1024];
                        memset(repsonse,0,sizeof repsonse);
                        memset(payload,0,sizeof payload);
  

      

                        strcat(payload, "\t###CONNECTED CLIENTS:\n");
                        int j;
                        for(j=0; j<=max_clients; j++ ){
                            if(client_socket[j] != 0){
                                
                                char buf[100] = "";
                                char out[100] = "";
                                getPortAndIP(client_socket[j],buf);
                                sprintf(out,"\t- Client with ID:%d, %s\n", j, buf);

                                strcat(payload, out);
                            }
                        }

                        char files[100] = "";
                        readServerDir(files);

                        strcat(payload, "\n\t###CONTENT OF SERVER FOLDER:\n");
                        strcat(payload, files);

                        char len[10] = "";
                        sprintf(len,"%d\n",strlen(payload));
                        strcat(repsonse, "OK\n");
                        strcat(repsonse, len);
                        strcat(repsonse, "======\n");
                        strcat(repsonse,payload);

                        writeDataToClient(sd, repsonse, sizeof repsonse);

                    }


                    //handle mods
                    if( mode_put && (fsize > 0) ) {

                        if(fsize<=valread){
                            valread = fsize;
                        }

                        fsize-=valread;
                        fwrite(buffer, sizeof(char), valread, opened_file);

                        if(fsize <= 0){
                            fclose(opened_file);
                            mode_put = false;

                            char ip[32] = "";
                            char ip_ret[64] = "";
                            char size_ret[16] = "";
                            char resp[1024] = "";

                            getPortAndIP(master_socket, ip);
                            sprintf(ip_ret, "send from %s\n", ip);
                            sprintf(size_ret, "size:%d bytes\n", fsize_header);

                            strcat(resp, "\n\nOK\n");
                            strcat(resp, ip_ret);
                            strcat(resp, size_ret);
                            strcat(resp, "======\n");

                            writeDataToClient( sd, resp, sizeof resp );
                        }
                    }

                    
                }
            }
        }
    }
      
    return 0;
} 
