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
    int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;

    bool mode_put = false;
    FILE *opened_file;
    int fsize;

    struct addrinfo hints, *res;
    memset( &hints, 0, sizeof(hints) );

      
    char buffer[1025];  //data buffer of 1K
      
    //set of socket descriptors
    fd_set readfds;
      
    //a message
    char *message = "RNP Server v1.0 \r\n";
  
    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
    }
      
    //create a master socket
    /*if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }*/
 

 
         hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_STREAM;
        //hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;



        getaddrinfo(NULL, "8888", &hints, &res);

       master_socket = socket( res->ai_family, res->ai_socktype, res->ai_protocol );


    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }



 
    //type of socket created
    address.sin_family = AF_INET6;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
 

printf("test");
     
    //bind the socket to localhost port 8888
    if (bind(master_socket, res->ai_addr, res->ai_addrlen )  <  0 ) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);
     
    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
      
    //accept the incoming connection
    addrlen = sizeof(address);
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
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
          
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
        
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
                     
                    break;
                }
            }
        }
          
        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i];
              
            if (FD_ISSET( sd , &readfds)) 
            {

                strcpy(buffer, ""); //clear buffer
                     

                //Check if it was for closing , and also read the incoming message
                if ((valread = recv( sd , buffer, CHUNK, 0)) == 0) //len = recv(sd, buf, CHUNK, 0)) > 0) )
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      
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

                       

                        printf("in GET(old:%s):\n", res[1]);

                        splitString(buffer, " ", res);
                        printf("after split:\n");

                        writeStrToClient( sd, "200 OK\n" );

                        printf("after write:\n");

                        sendPassedFile(res[1], sd);

                        printf("after send:\n");

                    }
                    else if( strncmp(buffer, "PUT", strlen("PUT") ) == 0 ){
                        buffer[valread] = '\0';
                        mode_put = true;

                        char buf2[10240] = "";
                        strcpy(buf2, buffer);

                        splitString(buffer, " ", res);

                        opened_file = fopen(res[1], "w");
                        fsize = atoi(res[2]);

                        splitString(buf2, "======", resu);

                        
                        strcpy( buffer, resu[1] );

                        if(strlen(buffer) == 0){
                            valread = 0;
                        }

                    }
                    else if( strncmp(buffer, "LIST", strlen("LIST") ) == 0 ){

                            printf("in LIST:");
			int j;
                        for(j=0; j<=max_clients; j++ ){
                            if(client_socket[j] != 0){
                                char clients[16];
                                struct sockaddr_in cli;
                                int cli_len;
                                getpeername(client_socket[j] , (struct sockaddr*)&cli , (socklen_t*)&cli_len);

                                sprintf( clients, "Client(sd:%d): ip %s , port %d \n", client_socket[j], inet_ntoa(cli.sin_addr) , ntohs(cli.sin_port));
                                
                                //sprintf(clients, "%d\n", client_socket[j]);

                                printf("sd:%s\n", clients);
                                writeStrToClient(sd, clients);
                            }
                        }
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

                            writeStrToClient( sd, "200 OK\r\n" );
                        }
                    }

                    
                }
            }
        }
    }
      
    return 0;
} 
