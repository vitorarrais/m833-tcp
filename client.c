#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERV_PORT "4990"
#define MAXDATASIZE 4096

int sendall(int s, char *buf, int *len) {
    int total = 0, bytesleft = *len, n;
    
    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    *len = total;
    return n==-1?-1:0;
}

int main(int argc, char *argv[]) {
    int conn_fd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *serv_addr;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    getaddrinfo(argv[1], SERV_PORT, &hints, &serv_addr);
    
    if ( (conn_fd = socket(serv_addr->ai_family, serv_addr->ai_socktype, serv_addr->ai_protocol)) == -1) {
        printf("Não foi possível criar socket.\n");
        return 0;
    }
    
    if ( (connect(conn_fd, serv_addr->ai_addr, serv_addr->ai_addrlen)) == -1 ) {
        printf("Não foi possível estabelecer conexão.\n");
        return 0;
    }
    
    while (1) {
        numbytes = recv(conn_fd, buf, MAXDATASIZE-1, 0);
        buf[numbytes] = '\0';
        
        printf("%s", buf);
        
        fgets(buf, MAXDATASIZE, stdin);
        
        int len = strlen(buf);
        buf[len-1] = '\0';
        sendall(conn_fd, buf, &len);
        
        if ( (buf[0] == 'E' || buf[0] == 'e') && buf[1] == '\0' ) break;
    }
    
    close(conn_fd);
    return 0;
}
