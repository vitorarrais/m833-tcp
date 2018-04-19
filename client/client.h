#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERV_PORT "5990"
#define MAXDATASIZE 4096

struct timeval tv1, tv2;
int timestamp;

int sendall(int s, char *buf, int *len);
void start_time();
int get_time();
