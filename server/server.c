#include "server.h"

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
    int size;
    char ** dsps;
    char buf[MAXSIZE];

    // == INIT ==
    init_size( &size );

    dsps = malloc(size * sizeof(char *));
    for (int i = 0; i < size; i++)
        dsps[i] = malloc(6 * sizeof(char));

    refresh_database( dsps, size );

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

    while(1) {
        cli_len = sizeof(cli_addr);
        cli_fd = accept(conn_fd, (struct sockaddr *) &cli_addr, &cli_len);
        if ( !fork() ) {
            struct sockaddr_in *sin = (struct sockaddr_in *) &cli_addr;
            printf("server(#%d): connected to %s:(%d)\n", getpid(), inet_ntoa(sin->sin_addr), sin->sin_port);
            close(conn_fd);
            communication(cli_fd, dsps, buf, size);
            close(cli_fd);
            return 0;
        }
        close(cli_fd);
    }

    return 0;
}



// == INITIALIZE == //
void init_size(int *size) {
    char tmp[50];

    struct flock fl = {F_RDLCK, SEEK_SET, 0, 0, 0 };
    fl.l_pid = getpid();

    FILE* fd = fopen("../database/database.dat", "r");
    fcntl(fileno(fd), F_SETLKW, &fl);

    fscanf(fd, "%d\n", size);

    fl.l_type = F_UNLCK;
    fcntl(fileno(fd), F_SETLKW, &fl);
    fclose(fd);
}

void refresh_database( char **dsps, int size ) {
    discipline all[size];
    db_read_disciplines(all);
    for ( int i = 0; i < size; i++ ) strcpy(dsps[i],all[i].code);
}


// == NETWORK == //
void communication(int cli_fd, char **dsps, char * buf, int size) {
    int len, user = 0, numbytes;

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
                list_info_to_buffer( buf, size);
                printf("server(#%d): query - list all codes, time: %dµs\n", getpid(), get_time());
                len = strlen(buf);
                sendall(cli_fd, buf, &len);
                break;
            case '2':
                start_time();
                list_all_to_buffer( buf, size);
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

void update_next_class( int cli_fd, char * buf, char **dsps, int size ) {
    int len, numbytes, id;

    refresh_database( dsps, size );

    strcpy(buf, "\n======= Alterar comentário próxima aula =======\nDigite o código da disciplina: ");

    len = strlen(buf);
    sendall(cli_fd, buf, &len);

    numbytes = recv(cli_fd, buf, MAXSIZE-1, 0);

    buf[numbytes] = '\0';
    for ( int i = 0; i < numbytes; i++ ) buf[i] = toupper(buf[i]);
    find_discipline(buf, dsps, size, &id);

    if ( id != -1 ) {
        strcpy(buf, "Insira comentário (250): ");
        len = strlen(buf);
        sendall(cli_fd, buf, &len);

        numbytes = recv(cli_fd, buf, MAXSIZE-1, 0);
        buf[numbytes] = '\0';

        start_time();
        db_write_next_class(buf, id, size);
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

void discipline_queries( int cli_fd, char * buf, char **dsps, int size ) {
    int len, id;
    char option = buf[0], *message;
    discipline dsp;

    refresh_database( dsps, size );

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
    find_discipline(buf, dsps, size, &id);

    if ( id != -1 ) {
        if ( option == '3') {
            start_time();
            db_read_discipline(&dsp, id);
            discipline_to_buffer(buf, &dsp);
            printf("server(#%d): query - get discipline info, time: %dµs\n", getpid(), get_time());
            strcat(buf, SECONDARY_MENU);
        }
        if ( option == '4') {
            start_time();
            db_read_discipline(&dsp, id);
            syllabus_to_buffer(buf, &dsp);
            printf("server(#%d): query - get discipline info, time: %dµs\n", getpid(), get_time());
        }
        if ( option == '5') {
            start_time();
            db_read_discipline(&dsp, id);
            next_class_to_buffer(buf, &dsp);
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

// == SUPPORT FUNCTIONS == //
void print_discipline(discipline * dsp) {
    printf("%-16s %s \n", "Código:", dsp->code);
    printf("%-15s %s \n", "Disciplina:", dsp->title);
    printf("%-15s %s \n", "Ementa:", dsp->syllabus);
    printf("%-15s %s \n", "Sala:", dsp->classroom);
    printf("%-16s %dhs \n", "Horário:", dsp->time);
    printf("%-16s %s \n", "Próxima aula:", dsp->next_class);
}

void find_discipline(char * code, char **dsps, int size, int *idx) {
    for ( int i = 0; i < size; i++ ) {
        if ( is_discipline(code, dsps[i]) == 0 ) {
            *idx = i;
            return;
        }
    }
    *idx = -1;
}

int is_discipline( char * code, char *dsp ) {
    for ( int i = 0; i < 5; i++ ) {
        if ( code[i] != dsp[i] ) {
            return 1;
        }
    }
    return 0;
}

// == BUFFER FUNCTIONS == //
void list_all_to_buffer (char * buf, int size) {
    discipline all[size];
    db_read_disciplines( all );
    char tmp[MAXSIZE];
    strcpy(buf,"\n======= Códigos das disciplinas =======");
    for ( int i = 0; i < size; i++ ) {
        sprintf(tmp, "\n== #%d == \n", (i+1));
        strcat(buf,tmp);
        discipline_to_buffer (tmp, &all[i]);
        strcat(buf,tmp);
    }
    strcat(buf, SECONDARY_MENU);
}

void list_info_to_buffer (char * buf, int size) {
    discipline all[size];
    db_read_disciplines( all );
    char tmp[MAXSIZE];
    strcpy(buf,"\n======= Informações das disciplinas =======");
    for ( int i = 0; i < size; i++ ) {
        sprintf(tmp, "\n== #%d == \n", (i+1));
        strcat(buf,tmp);
        info_to_buffer (tmp, &all[i]);
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

// == DATABASE FUNCTIONS == //

//char title[50];
//char syllabus[1024];
//char classroom[10];
//char next_class[250];
//short time;
//char code[6];

void db_read_disciplines( discipline * dsps) {
    struct flock fl = {F_RDLCK, SEEK_SET, 0, 0, 0 };
    fl.l_pid = getpid();
    int N;

    FILE* fd = fopen("../database/database.dat", "r");
    fcntl(fileno(fd), F_SETLKW, &fl);

    fscanf(fd, "%d\n", &N);

    for ( int i = 0; i < N; i++ )
        fscanf(fd, "%59[^;]; %1023[^;]; %9[^;]; %249[^;]; %5[^;]; %hd\n", dsps[i].title, dsps[i].syllabus, dsps[i].classroom, dsps[i].next_class, dsps[i].code, &dsps[i].time);

    fl.l_type = F_UNLCK;
    fcntl(fileno(fd), F_SETLKW, &fl);
    fclose(fd);
}
void db_read_discipline(discipline * dsp, int index) {
    struct flock fl = {F_RDLCK, SEEK_SET, 0, 0, 0 };
    fl.l_pid = getpid();
    int N;

    FILE* fd = fopen("../database/database.dat", "r");
    fcntl(fileno(fd), F_SETLKW, &fl);

    fscanf(fd, "%d\n", &N);

    for ( int i = 0; i < index+1; i++ )
        fscanf(fd, "%59[^;]; %1023[^;]; %9[^;]; %249[^;]; %5[^;]; %hd\n", dsp->title, dsp->syllabus, dsp->classroom, dsp->next_class, dsp->code, &dsp->time);

    fl.l_type = F_UNLCK;
    fcntl(fileno(fd), F_SETLKW, &fl);
    fclose(fd);

}
void db_write_next_class( char * next_class, int index, int size ) {
    char filename[40] = "../database/database.dat", replica[40] = "../database/replica.dat";
    int N;

    discipline dsps[size];
    db_read_disciplines( dsps );
    strcpy(dsps[index].next_class, next_class);

    struct flock fl = {F_WRLCK, SEEK_SET, 0, 0, 0 };
    fl.l_pid = getpid();

    FILE* fd = fopen(filename, "w");
    fcntl(fileno(fd), F_SETLKW, &fl);
    fclose(fd);

    FILE* fd2 = fopen(replica, "w");

    fprintf(fd2, "%d\n", size);
    for( int i = 0; i < size; i++ ) {
        fprintf(fd2, "%s; %s; %s; %s; %s; %d\n", dsps[i].title, dsps[i].syllabus, dsps[i].classroom, dsps[i].next_class, dsps[i].code, dsps[i].time);
    }
    fclose(fd2);

    remove(filename);
    rename(replica, filename);

    fl.l_type = F_UNLCK;
    fcntl(fileno(fd), F_SETLKW, &fl);
}


// == TIME FUNCTIONS == //

void start_time() {
    gettimeofday(&tv1, NULL);
}

int get_time() {
    gettimeofday(&tv2, NULL);
    return tv2.tv_usec-tv1.tv_usec;
}
