#include <string.h>  
#include <stdlib.h> 
#include <stdio.h>  
#include <unistd.h>
#include <stdio.h> 
#include <sys/socket.h>  
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>  

#define BUFSIZE 4096  

void process(FILE* fp, int sock)  
{  
    printf("connect success! \n");  
    char send[BUFSIZE];
    char recv[BUFSIZE]; 
    memset(send, '\0', sizeof(send));
    memset(recv, '\0', sizeof(recv)); 
    while (fgets(send, BUFSIZE, fp) != NULL)  
    {  
        write(sock, send, strlen(send));  
        read(sock, recv, BUFSIZE);  
        fputs(recv, stdout);  
        memset(send, '\0', sizeof(send));
        memset(recv, '\0', sizeof(recv));   
    } 
}

int main(int argc, char **argv)  
{
    const char* ip = "10.108.xx.xx";
    int port = 9876;

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));  
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port); 


    int sock = socket(AF_INET, SOCK_STREAM, 0);  
    if (sock == -1)  
        perror("socket"); 
    int ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)); 
    if (sock == -1)  
        perror("socket");  

    process(stdin, sock);

    exit(0);  
}

