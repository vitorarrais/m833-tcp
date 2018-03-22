#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>

#define SERV_PORT "4990"
#define NUMDISCIPLINES 10
#define MAXSIZE 4096
#define INTRO "Bem vindo ao sistema de consultas de disciplinas.\nFaça login para acessar a aplicação."
#define PRIMARY_MENU "\n======= Menu Principal =======\nSelecione a opção desejada:\n1 - Listar códigos das disciplinas.\n2 - Listar informações das disciplinas.\n3 - Informações sobre disciplina.\n4 - Ementa da disciplina.\n5 - Comentário sobre próxima aula.\nE - Encerrar conexão.\ninput: "
#define PROFESSOR_MENU "\n======= Menu Principal =======\nSelecione a opção desejada:\n1 - Listar códigos das disciplinas.\n2 - Listar informações das disciplinas.\n3 - Informações sobre disciplina.\n4 - Ementa da disciplina.\n5 - Comentário sobre próxima aula.\n6 - Escrever comentário da próxima aula.\nE - Encerrar conexão.\ninput: "
#define SECONDARY_MENU "\n======= Menu Secundário =======\n1 - Menu principal.\nE - Encerrar Conexão.\ninput: "
#define USER_MENU "\n======= Tipo de Usuário =======\n1 - Aluno.\n2 - Professor.\ninput: "

typedef struct {
    char title[50];
    char syllabus[1024];
    char classroom[10];
    char next_class[250];
    short time;
    char code[5];
} discipline;

// == NETWORK == //
void communication(int cli_fd, char * buf, discipline * dsps );
void update_next_class( int cli_fd, char * buf, discipline * dsps, int size );
void discipline_queries( int cli_fd, char * buf, discipline * dsps, int size );
int sendall(int s, char *buf, int *len);

// == MODIFY DISCIPLINE == //
void init_discipline(discipline * dsp,  char * title, char * syllabus, char * classroom, char * next_class, short time, char * code);
void set_next_class(discipline *dsp, char * next_class);

// == SUPPORT FUNCTIONS == //
discipline* find_discipline(char * code, discipline * dsps, int size);
int is_discipline( char * code, discipline * dsp );
void print_discipline(discipline * dsp);

// == BUFFER FUNCTIONS == //
void discipline_to_buffer (char * buf, discipline * dsp);
void list_info_to_buffer (char * buf, discipline * dsps, int size);
void list_all_to_buffer (char * buf, discipline * dsps, int size);
void info_to_buffer (char * buf, discipline * dsp);
void syllabus_to_buffer (char * buf, discipline * dsp);
void next_class_to_buffer (char * buf, discipline * dsp);

// == TIME FUNCTIONS == //
void start_time();
int get_time();

// = time variables =
struct timeval tv1, tv2;

// == MAIN == //
int main(int argc, char **argv) {
    
    // = network variables =
    int conn_fd, cli_fd;
    pid_t cli_pid;
    socklen_t cli_len;
    struct addrinfo hints, *serv_addr;
    struct sockaddr cli_addr;
    
    // = logic variables =
    discipline dsps[NUMDISCIPLINES];
    char buf[MAXSIZE];

    
    // == INIT ==
    init_discipline(&dsps[0], "Laboratório Circuitos Digitais", "Metodologia de projeto digital. Técnicas de projeto usando lógica programável.", "CC305", "Introdução a sinais em VHDL.", 16, "MC613" );
    init_discipline(&dsps[1], "Cálculo I", "Intervalos e desigualdades. Funções. Limites. Continuidade. Derivada e diferencial. Integral.", "CB01", "Definição de integrais com limites infinitos.", 8, "MA111" );
    init_discipline(&dsps[2], "Programação de Redes de Computadores", "Programação utilizando diferentes tecnologias de comunicação: sockets, TCP e UDP, e chamada de método remoto.", "CC303", "Aprensentação serviços cliente/servidor utilizando protocolo TCP", 10, "MC832" );
    init_discipline(&dsps[3], "Mecânica Geral", "Revisão de matrizes e cálculo vetorial. Mecânica Newtoniana. Oscilações lineares.", "CB010", "Resolução de funções variáveis com a velocidade.", 8, "F315" );
    init_discipline(&dsps[4], "Projeto e Análise de Algoritmos III", "Tratamento de Problemas NP-difíceis.", "CB02", "Aplicações de algoritmos branch-and-bound.", 14, "MC658" );
    
    
    // == NETWORKING ==
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    getaddrinfo(NULL, SERV_PORT, &hints, &serv_addr);
    
    conn_fd = socket(serv_addr->ai_family, serv_addr->ai_socktype, serv_addr->ai_protocol);
    
    bind(conn_fd, serv_addr->ai_addr, serv_addr->ai_addrlen);
    
    listen(conn_fd, 10);
    printf("server: waiting for connections...\n");
//    printf("pinto\n");
    
    while(1) {
        cli_len = sizeof(cli_addr);
        cli_fd = accept(conn_fd, (struct sockaddr *) &cli_addr, &cli_len);
        if ( !fork() ) {
            struct sockaddr_in *sin = (struct sockaddr_in *) &cli_addr;
            printf("server(#%d): connected to %s:(%d)\n", getpid(), inet_ntoa(sin->sin_addr), sin->sin_port);
            close(conn_fd);
            communication(cli_fd, buf, dsps);
            close(cli_fd);
            return 0;
        }
        close(cli_fd);
    }
}

// == NETWORK == //
void communication(int cli_fd, char * buf, discipline * dsps ) {
    int len, size = 5, user = 0, numbytes;
    
    strcpy(buf, INTRO );
    strcat(buf, USER_MENU );
    
    len = strlen(buf);
    sendall(cli_fd, buf, &len);
    
    while( 1 ) {
        numbytes = recv(cli_fd, buf, MAXSIZE-1, 0);
    
        if ( buf[0] == '2' ) {
            strcpy(buf, "Senha de acesso: " );
            len = strlen(buf);
            sendall(cli_fd, buf, &len);
            
            numbytes = recv(cli_fd, buf, MAXSIZE-1, 0);
            buf[numbytes] = '\0';
            if ( strcmp(buf, "admin") == 0 ){
                user = 1;
                break;
            }
            else {
                strcpy(buf, "\nSenha incorreta." );
                strcat(buf, USER_MENU );
                len = strlen(buf);
                sendall(cli_fd, buf, &len);
            }
        }
        else {
            break;
        }
    }
    
    while (1) {
        int end = 0, timer;
        
        if ( user == 0) strcpy(buf, PRIMARY_MENU );
        else strcpy(buf, PROFESSOR_MENU );
        
        len = strlen(buf);
        sendall(cli_fd, buf, &len);
        
        numbytes = recv(cli_fd, buf, MAXSIZE-1, 0);
        
        switch (buf[0]) {
            case '1':
                start_time();
                list_info_to_buffer( buf, dsps, size);
                printf("server(#%d): query - list all codes, time: %dµs\n", getpid(), get_time());
                len = strlen(buf);
                sendall(cli_fd, buf, &len);
                break;
            case '2':
                start_time();
                list_all_to_buffer( buf, dsps, size);
                printf("server(#%d): query - list all disciplines, time: %dµs\n", getpid(), get_time());
                len = strlen(buf);
                sendall(cli_fd, buf, &len);
                break;
            case '3':
            case '4':
            case '5':
                discipline_queries(cli_fd, buf, dsps, size );
                break;
            case '6':
                if (user == 1) update_next_class( cli_fd, buf, dsps, size );
                break;
            case 'E':
            case 'e':
                end = 1;
                break;
        }
        
        recv(cli_fd, buf, MAXSIZE-1, 0);
        if ( buf[0] == 'E' || buf[0] == 'e' ) end = 1;
        
        if (end == 1) {
            break;
        }
    }
}

void update_next_class( int cli_fd, char * buf, discipline * dsps, int size ) {
    int len, numbytes;
    strcpy(buf, "\n======= Alterar comentário próxima aula =======\nDigite o código da disciplina: ");
    
    len = strlen(buf);
    sendall(cli_fd, buf, &len);
    
    numbytes = recv(cli_fd, buf, MAXSIZE-1, 0);
    
    buf[numbytes] = '\0';
    for ( int i = 0; i < numbytes; i++ ) buf[i] = toupper(buf[i]);
    discipline *ret = find_discipline(buf, dsps, size);
    
    if ( ret != NULL ) {
        strcpy(buf, "Insira comentário (250): ");
        len = strlen(buf);
        sendall(cli_fd, buf, &len);
        
        numbytes = recv(cli_fd, buf, MAXSIZE-1, 0);
        buf[numbytes] = '\0';
        
        start_time();
        set_next_class( ret, buf);
        printf("server(#%d): query - update next class commentary, time: %dµs\n", getpid(), get_time());
        strcpy(buf, "\nComentário alterado.");
        strcat(buf, SECONDARY_MENU);
    }
    else {
        strcpy(buf, "\nDisciplina não encontrada.");
        strcat(buf, SECONDARY_MENU);
    }
    
    len = strlen(buf);
    sendall(cli_fd, buf, &len);
}

void discipline_queries( int cli_fd, char * buf, discipline * dsps, int size ) {
    int len;
    char option = buf[0], *message;
    if ( option == '3') {
        strcpy(buf, "\n======= Informações sobre disciplina =======\nDigite o código da disciplina: ");
    }
    if ( option == '4') {
        strcpy(buf, "\n======= Ementa da disciplina =======\nDigite o código da disciplina: ");
    }
    if ( option == '5') {
        strcpy(buf, "\n======= Comentário sobre próxima aula =======\nDigite o código da disciplina: ");
    }
    len = strlen(buf);
    sendall(cli_fd, buf, &len);

    int numbytes = recv(cli_fd, buf, MAXSIZE-1, 0);
    
    buf[numbytes] = '\0';
    for ( int i = 0; i < numbytes; i++ ) buf[i] = toupper(buf[i]);
    discipline *ret = find_discipline(buf, dsps, size);
    
    if ( ret != NULL ) {
        if ( option == '3') {
            start_time();
            discipline_to_buffer(buf, ret);
            printf("server(#%d): query - get discipline info, time: %dµs\n", getpid(), get_time());
            strcat(buf, SECONDARY_MENU);
        }
        if ( option == '4') {
            start_time();
            syllabus_to_buffer(buf, ret);
            printf("server(#%d): query - get discipline info, time: %dµs\n", getpid(), get_time());
        }
        if ( option == '5') {
            start_time();
            next_class_to_buffer(buf, ret);
            printf("server(#%d): query - get discipline next class commentary, time: %dµs\n", getpid(), get_time());
        }
    }
    else {
        strcpy(buf, "\nDisciplina não encontrada.");
        strcat(buf, SECONDARY_MENU);
    }

    len = strlen(buf);
    sendall(cli_fd, buf, &len);
    
}

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

// == MODIFY DISCIPLINE == //
void init_discipline(discipline * dsp,  char * title, char * syllabus, char * classroom, char * next_class, short time, char * code) {
    strcpy(dsp->title, title);
    strcpy(dsp->syllabus, syllabus);
    strcpy(dsp->classroom, classroom);
    strcpy(dsp->next_class, next_class);
    strcpy(dsp->code, code);
    dsp->time = time;
}

void set_next_class(discipline *dsp, char * next_class) {
    strcpy(dsp->next_class, next_class);
}

// == SUPPORT FUNCTIONS == //
void print_discipline(discipline * dsp) {
    printf("%-16s %s \n", "Código:", dsp->code);
    printf("%-15s %s \n", "Disciplina:", dsp->title);
    printf("%-15s %s \n", "Ementa:", dsp->syllabus);
    printf("%-15s %s \n", "Sala:", dsp->classroom);
    printf("%-16s %dhs \n", "Horário:", dsp->time);
    printf("%-16s %s \n", "Próxima aula:", dsp->next_class);
}

discipline* find_discipline(char * code, discipline* dsps, int size) {
    for ( int i = 0; i < size; i++ ) {
        if ( is_discipline(code, &dsps[i] ) ) return &dsps[i];
    }
    return NULL;
}

int is_discipline( char * code, discipline * dsp ) {
    if ( strcmp(code,dsp->code) == 0 ) return 1;
    else return 0;
}

// == BUFFER FUNCTIONS == //
void list_all_to_buffer (char * buf, discipline * dsps, int size) {
    char tmp[MAXSIZE];
    strcpy(buf,"\n======= Códigos das disciplinas =======");
    for ( int i = 0; i < size; i++ ) {
        sprintf(tmp, "\n== #%d == \n", (i+1));
        strcat(buf,tmp);
        discipline_to_buffer (tmp, &dsps[i]);
        strcat(buf,tmp);
    }
    strcat(buf, SECONDARY_MENU);
}

void list_info_to_buffer (char * buf, discipline * dsps, int size) {
    char tmp[MAXSIZE];
    strcpy(buf,"\n======= Informações das disciplinas =======");
    for ( int i = 0; i < size; i++ ) {
        sprintf(tmp, "\n== #%d == \n", (i+1));
        strcat(buf,tmp);
        info_to_buffer (tmp, &dsps[i]);
        strcat(buf,tmp);
    }
    strcat(buf, SECONDARY_MENU);
}

void discipline_to_buffer (char * buf, discipline * dsp) {
    sprintf(buf, "%-16s %s \n%-15s %s \n%-15s %s \n%-15s %s \n%-16s %dhs \n%-16s %s \n",
            "Código:", dsp->code,
            "Disciplina:", dsp->title,
            "Ementa:", dsp->syllabus,
            "Sala:", dsp->classroom,
            "Horário:", dsp->time,
            "Próxima aula:", dsp->next_class);
}

void info_to_buffer (char * buf, discipline * dsp) {
    sprintf(buf, "%-16s %s \n%-15s %s \n", "Código:", dsp->code, "Disciplina:", dsp->title);
}

void syllabus_to_buffer (char * buf, discipline * dsp) {
    sprintf(buf, "%-15s %s \n%-15s %s \n", "Disciplina:", dsp->title, "Ementa:", dsp->syllabus);
    strcat(buf, SECONDARY_MENU);
}

void next_class_to_buffer (char * buf, discipline * dsp) {
    sprintf(buf, "%-15s %s \n%-16s %s \n", "Disciplina:", dsp->title, "Próxima aula:", dsp->next_class);
    strcat(buf, SECONDARY_MENU);
}


void start_time() {
    gettimeofday(&tv1, NULL);
}

int get_time() {
    gettimeofday(&tv2, NULL);
    return tv2.tv_usec-tv1.tv_usec;
}

