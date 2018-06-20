/*
    C ECHO client example using sockets
*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>    
 

#define CHUNK 1024 /* read 1024 bytes at a time */

void sendPassedFile(char *filename,  int sd, FILE* file ){

    char buf[CHUNK] = {'\0'};
    size_t nread;

    rewind(file);

    if (file != NULL ) {
        while ((nread = fread(buf, 1, sizeof buf, file)) > 0){

            send(sd, buf, sizeof buf, 0);

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

bool writeStrToServer(int sckt, const char *str)
{
    return writeDataToClient(sckt, str, strlen(str));
}

void splitString(const void *stri, const void *delimiter, char results[][1024] ){
    char *ptr;



    // initialisieren und ersten Abschnitt erstellen
    ptr = strtok(stri, delimiter);
    int idx = 0;

    while(ptr != NULL) {

        if(idx<=5){
            strcpy( results[idx], ptr);
        }
    



        // naechsten Abschnitt erstellen
        ptr = strtok(NULL, delimiter);
        idx++;
    }
}


int main(int argc , char *argv[])
{
    	int sock;
  	char cmd[1000] , server_reply[2000];
     	struct addrinfo hints, *res;

    	memset( &hints, 0, sizeof(hints) );	
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	//hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(argv[1], argv[2], &hints, &res);

	sock = socket( res->ai_family, res->ai_socktype, res->ai_protocol );

	printf("%s - %s", argv[1], argv[2]);
 
    //Connect to remote server
    if (connect(sock, res->ai_addr, res->ai_addrlen) )
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
     
    int receive = 1;
    bool run = true;
    //keep communicating with server
    while(run)
    {
     
        if(receive){ 
            //Receive a reply from the server
            if( recv(sock , server_reply , 2000 , 0) < 0)
            {
                puts("recv failed");
                break;
            }
             

            puts("Server reply: \r\n");
            puts(server_reply);
            receive = 0;
        }

        char p1[100];
        printf("Enter command:\r\n\r\n");
        scanf("%s%s" , cmd, p1);



        if(strncmp(cmd, "PUT", strlen("PUT") ) == 0 ){
            printf("in PUT:\r\n\r\n");
	    
           // receive = 1;

	  

           	 char command[100] = "PUT ";

		printf("%s", command );
          char *filename = p1;
            int fsize;

            printf("%s", filename );


             FILE* file = fopen(filename, "r");



	
            //get file size
            fseek(file, 0L, SEEK_END);
            fsize = ftell(file);

            char cfs[16];
            sprintf(cfs, "%d", fsize);
     

 	  // printf("%s-%d-%s\n",filename, fsize);


           strcat(command, filename);
            strcat(command, " ");
            strcat(command, cfs );
            strcat(command, "======" ); //seperate header from payload
            writeStrToServer(sock, command);
            sendPassedFile(filename, sock, file );
            printf("ready with sending file");
        }
        else if(strncmp(cmd, "GET", strlen("GET") ) == 0 ){
            char command[100] = "GET ";
            char *filename = p1;
            strcat(command, filename);
            writeStrToServer(sock, command);

            memset(server_reply, 0, 1024);

            int len = 0;
            char fullHeader[2*CHUNK] = "";
            char cpyfullHeader[2*CHUNK] = "";
            int bytesSubmitted = 0;
            int remainingBytes = 0;
            int gotFullHeader  = 0;
            int fileLength     = 0;
            int amountBytesToRead = CHUNK;

            while( (len = recv(sock , server_reply , amountBytesToRead , 0)) > 0){

                //printf("\n\nbytes read:%d submitted: %d\n", len, bytesSubmitted);
               
                


                //check if full header is arrived
                if(bytesSubmitted>=30 && !gotFullHeader){
                     gotFullHeader = 1;
                     char res[10][1024];
                     char resu[10][1024];
                     strcpy(cpyfullHeader, fullHeader);
                     //proceed extracting filesize
                     splitString(fullHeader, "\n", res);
                     splitString(cpyfullHeader, "======", resu);
                     //printf("-->>%s<<%d\n\n", resu[0], strlen(resu[0]));
                     fileLength = atoi(res[1]) + strlen(resu[0]) + strlen("======"); // file + header length + divider
                     remainingBytes = fileLength - bytesSubmitted;
                     //printf("got full header>>%d\n!<--", fileLength);
                }
                else{
                    strcat(fullHeader, server_reply);
                }


                if(remainingBytes<CHUNK && gotFullHeader){
                    amountBytesToRead = remainingBytes;
             
                    len = remainingBytes;
                }


                fwrite(server_reply,1,amountBytesToRead,stdout);
                

                bytesSubmitted+=len;
                remainingBytes-=len; 

                //printf("\n\nlength: %d --- <<<<bytes submitted:%d---remaing:%d\n\n", fileLength, bytesSubmitted, remainingBytes );

                if(bytesSubmitted==fileLength && fileLength != 0){
                   break;
                }

            }
            printf("\n\n\n" );
         
        }
        else if(strncmp(cmd, "QUIT", strlen("QUIT") ) == 0 ){
            run = false; //close socket
        }
        else if(strncmp(cmd, "LIST", strlen("LIST") ) == 0 ){
            char command[100] = "LIST";
            writeStrToServer(sock, command);
	    memset(server_reply, 0, 2000);
            recv(sock , server_reply , CHUNK , 0); 
            printf("%s",server_reply);
        }
        else{
            printf("No supported command! Please try again.");
        }
         


    }
     
    close(sock);
    return 0;

}



