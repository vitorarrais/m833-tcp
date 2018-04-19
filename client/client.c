#include "client.h"


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
    int conn_fd, numbytes, ptimeval, ctimeval;
    char buf[MAXDATASIZE], ptime[MAXDATASIZE];
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
        ctimeval = get_time();

        strcpy(ptime, (const char*) buf);

        buf[numbytes] = '\0';
        if ( buf[0] != 'N' ) {
            ptime[9] = '\0';
            ptimeval = atoi(ptime);

            printf("\n======= Tempo de comunicação =======\n");
            printf("tempo de total: %dµs\n", ctimeval);
            printf("tempo de comunicação: %dµs\n", ctimeval - ptimeval);
        }

        printf("%s", buf+9);

        fgets(buf, MAXDATASIZE, stdin);

        int len = strlen(buf);
        buf[len-1] = '\0';

        start_time();
        sendall(conn_fd, buf, &len);

        if ( (buf[0] == 'E' || buf[0] == 'e') && buf[1] == '\0' ) break;
    }

    close(conn_fd);
    return 0;
}

void start_time() {
    gettimeofday(&tv1, NULL);
}

int get_time() {
    gettimeofday(&tv2, NULL);
    timestamp = (tv2.tv_sec*1e6 + tv2.tv_usec) - (tv1.tv_sec*1e6 + tv1.tv_usec);
    return timestamp;
}