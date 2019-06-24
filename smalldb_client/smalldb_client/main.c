//
//  main.cpp
//  smalldb_client
//
//  Created by Young-il Seo on 21/06/2019.
//  Copyright © 2019 Young-il Seo. All rights reserved.
//
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MAXLINE 1024

int main(int argc, char **argv) {
    struct sockaddr_in serveraddr;
    int server_sockfd;
    int client_len;
    char buf[MAXLINE];
    
    if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("error : ");
        return 1;
    }
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serveraddr.sin_port = htons(4000);
    
    client_len = sizeof(serveraddr);
    
    if(connect(server_sockfd, (struct sockaddr*)&serveraddr, client_len) == -1) {
        perror("connect error : ");
        return 1;
    }
    memset(buf, 0x00, MAXLINE);
    read(0, buf, MAXLINE);
    if(write(server_sockfd, buf, MAXLINE) <= 0) {
        perror("write error : ");
        return 1;
    }
    memset(buf, 0x00, MAXLINE);
    if(read(server_sockfd, buf, MAXLINE) <= 0) {
        perror("read error: ");
        return 1;
    }
    close(server_sockfd);
    printf("server:%s\n", buf);
    return 0;
}
