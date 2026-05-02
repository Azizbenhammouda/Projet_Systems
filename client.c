#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 5000
#define NUM_QUESTIONS 3   

void recv_msg(int sock, char *buffer, int buf_size) {
    
    char size_str[16];
    int i = 0;

    while (i < (int)sizeof(size_str) - 1) {
        int r = recv(sock, &size_str[i], 1, 0);
        if (r <= 0) { buffer[0] = '\0'; return; }
        if (size_str[i] == '\n') break;
        i++;
    }
    size_str[i] = '\0';

    int size = atoi(size_str);

    
    if (size <= 0 || size >= buf_size) {
        buffer[0] = '\0';
        return;
    }

    
    int total = 0;
    while (total < size) {
        int r = recv(sock, buffer + total, size - total, 0);
        if (r <= 0) break;
        total += r;
    }

    buffer[total] = '\0';
}

int main() {
    int sock;
    struct sockaddr_in server_addr;

    char buffer[1024];
    char answer[10];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection failed\n");
        return 1;
    }

    printf("Connected! Waiting for other players...\n");

    
    for (int q = 0; q < NUM_QUESTIONS; q++) {
        recv_msg(sock, buffer, sizeof(buffer));
        printf("\n%s", buffer);
        fflush(stdout);
        scanf("%9s", answer);
        char upper = answer[0];
        if (upper >= 'a' && upper <= 'z') upper -= 32;
        send(sock, &upper, 1, 0);

        
        recv_msg(sock, buffer, sizeof(buffer));
        printf("\n%s", buffer);
    }

   
    recv_msg(sock, buffer, sizeof(buffer));
    printf("\n%s\n", buffer);

    close(sock);
    return 0;
}