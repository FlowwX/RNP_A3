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
 
int main(int argc , char *argv[])
{
    	int sock;
  	char message[1000] , server_reply[2000];
     	struct addrinfo hints, *res;

    	memset( &hints, 0, sizeof(hints) );	
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

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
     
    //keep communicating with server
    while(1)
    {
        printf("Enter message : ");
        scanf("%s" , message);
         
        //Send some data
        if( send(sock , message , strlen(message) , 0) < 0)
        {
            puts("Send failed");
            return 1;
        }
         
        //Receive a reply from the server
        if( recv(sock , server_reply , 2000 , 0) < 0)
        {
            puts("recv failed");
            break;
        }
         
        puts("Server reply :");
        puts(server_reply);
    }
     
    close(sock);
    return 0;
}
