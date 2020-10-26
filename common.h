#define MAP_SIZE_X 3
#define MAP_SIZE_Y 3
#define SERVER_PORT 12345
#define SERVER_IP "127.0.0.1"
#define SERVER_TIMEOUT_S 30

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define BUFFER_SIZE 1024
#define MAX_SLOTS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

char map_array[MAP_SIZE_Y][MAP_SIZE_X];

typedef struct booking_order{
    int id;
    int size;
    int rows[MAP_SIZE_X];
    int cols[MAP_SIZE_Y];
}booking_order;


void errorAndExit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

void println(char *msg){
    printf("%s\n",msg);
}

int remove_newline_and_check_num(char *msg){
    char *t_msg;
    t_msg = malloc(sizeof(char) * (strlen(msg) + 1));
    if(t_msg == NULL) errorAndExit("error malloc");
    strcpy(t_msg,msg);
    if(t_msg[strlen(t_msg)-1]='\n') t_msg[strlen(t_msg)-1]='\0';
    char *t_t_msg = t_msg;
    while(*t_t_msg != '\0')
    {
        if(*t_t_msg < '0' || *t_t_msg > '9'){
            printf("not a number\n");
            return -1;
        }
        t_t_msg++;
    }
    return (int)strtol(t_msg, NULL, 10);
}


int print_strings_as_chars(char*msg){
    for(int i = 0; i < strlen(msg);i++) printf("%d",(int)msg[i]);
    printf("\n");
}
