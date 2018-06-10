#define _GNU_SOURCE 1 // SPLICE_F_MORE | SPLICE_F_MOVE
#include <string.h>  
#include <stdlib.h>  
#include <unistd.h>
#include <stdio.h>  
#include <fcntl.h>
#include <assert.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h>  
#include <arpa/inet.h>

//最大连接数  
#define LISTENQ 3

//接收和发送的缓冲区大小  
#define BUFSIZE 4096  

//第一种方法：使用read, write，需要进行用户空间和内核空间的数据拷贝
void process_has_copy(int connfd)  
{  
    ssize_t n;  
    char buf[BUFSIZE]; 
    memset(buf, '\0', BUFSIZE); 
    while ((n = read(connfd, buf, BUFSIZE)) > 0)  
    {
        write(connfd, buf, n);
        memset(buf, '\0', BUFSIZE);
    }
}

//第二种方法：使用splice和管道，不需要进行用户空间和内核空间的数据拷贝
void process_no_copy(int connfd)  
{

    int pipefd[2];
    int ret = pipe(pipefd);
    while (1)
    {
        ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);
        ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);
    }
    close(connfd);
} 

int main(int argc, char *argv[])  
{  
    const char* ip = "10.108.36.99";
    int port = 9876;
 
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));  
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);  

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)  
        perror("socket");

    //解决端口重用问题
    int reuse = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopet error\n");
        return -1;
    }

    int ret = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)); 
    if (ret == -1) 
        perror("bind"); 
    
    //使套接字变为监听套接字
    ret = listen(sock, LISTENQ);   
    if (ret == -1)  
        perror("listen"); 

    char buff[BUFSIZE];
    memset(buff, '\0', sizeof(buff));
    while (1)  
    {
        struct sockaddr_in client_addr;
        socklen_t client_addrlen = sizeof(client_addr); 
        int connfd = accept(sock, (struct sockaddr *)&client_addr, &client_addrlen); 
        if (connfd == -1)
        {  
            perror("accept");   
        }

        //并发服务器，fork一个子进程来处理客户端请求 
        pid_t pid = fork();
        if (pid == 0)  
        {  
            printf("connection from %s, port %d\n", 
                inet_ntop(AF_INET, &client_addr.sin_addr, buff, sizeof(buff)), 
                ntohs(client_addr.sin_port));  
            memset(buff, '\0', sizeof(buff));
            close(sock);      //子进程不需要监听套接字  
            process_no_copy(connfd);  //子进程处理客户端请求  
            close(connfd);    //处理结束，关闭连接套接字  
            exit(0);          //处理结束，关闭子进程  
        } 
        else
        {
            close(connfd);    //父进程不需要连接套接字，直接关闭
        }
    }
}