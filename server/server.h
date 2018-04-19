#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
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

#define SERV_PORT "5990"
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
    char code[6];
} discipline;

// == INITIALIZE == //
void init_size(int *size);
void refresh_database( char **dsps, int size );

// == NETWORK == //
void communication(int cli_fd, char **dsps, char * buf, int size);
void update_next_class( int cli_fd, char * buf, char **dsps, int size );
void discipline_queries( int cli_fd, char * buf, char **dsps, int size );
int sendall(int s, char *buf, int *len, int timestamp);

// == SUPPORT FUNCTIONS == //
void find_discipline(char * code, char **dsps, int size, int *idx);
int is_discipline( char * code, char *dsps );
void print_discipline(discipline * dsp);

// == BUFFER FUNCTIONS == //
void discipline_to_buffer (char * buf, discipline * dsp);
void list_info_to_buffer (char * buf, int size);
void list_all_to_buffer (char * buf, int size);
void info_to_buffer (char * buf, discipline * dsp);
void syllabus_to_buffer (char * buf, discipline * dsp);
void next_class_to_buffer (char * buf, discipline * dsp);

// == TIME FUNCTIONS == //
void start_time();
int get_time();

// == DATABASE FUNCTIONS == //
void db_read_disciplines(discipline * dsps);
void db_read_discipline(discipline * dsp, int index);
void db_write_next_class( char * next_class, int index, int size );
