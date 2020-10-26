#include "common.h"

void print_map(){
    for(int y = 0; y < MAP_SIZE_Y; y++){
        for(int x = 0; x < MAP_SIZE_X; x++){
            printf("%d ",map_array[y][x]);
        }
        printf("\n");
    }
}

int client_init(void){  //typical TCP socket client implementation
    int sd = socket(AF_INET, SOCK_STREAM,0);
    struct sockaddr_in sockaddr_client;
    sockaddr_client.sin_family = AF_INET;
    sockaddr_client.sin_addr.s_addr =  inet_addr(SERVER_IP);
    sockaddr_client.sin_port = htons(SERVER_PORT);
    int res = connect(sd,(struct sockaddr*)&sockaddr_client,sizeof(sockaddr_client));
    if(res == -1)return -1;
    return sd;
}

int client_receive_map(int sd){
    if(read(sd,map_array,sizeof(char) * MAP_SIZE_X * MAP_SIZE_X) != MAP_SIZE_X*MAP_SIZE_Y) return -1; //tries to read the map from the server and writes it into memory
    print_map();
    return 0;
}

int client_send_booking(int sd){
    char buffer[BUFFER_SIZE];
    printf("Insert number of seats to book:");
    fgets(buffer, BUFFER_SIZE,stdin);
    int size = remove_newline_and_check_num(buffer);        //waits for a string from user input
    if(size == -1) return -1;                               //if it's not a number it errors out
    if(size > MAP_SIZE_X * MAP_SIZE_Y | size == 0){         //if it's zero or bigger than the map max coords, then it errors out
        printf("too many seats or zero\n");
        return -1;
    } 
    booking_order new_booking_order;                        //allocates booking_order struct
    new_booking_order.size = size;
    for(int i=0;i<size;i++){                                //for each seats requested, it asks x,y coords and checks if they are valid numbers and between the map coords, then it writes all this data to the new_booking_order struct
        printf("Insert COLUMN of seat number %d: ",i);
        memset(buffer,0,sizeof(char)*BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE,stdin);
        int x = remove_newline_and_check_num(buffer);
        if(x == -1 || x >= MAP_SIZE_X) return -1;

        printf("Insert ROW of seat number %d: ",i);
        memset(buffer,0,sizeof(char)*BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE,stdin);
        int y = remove_newline_and_check_num(buffer);
        if(y == -1||y>=MAP_SIZE_Y) return -1;

        new_booking_order.rows[i] = y;                  
        new_booking_order.cols[i] = x;
    }
    if(write(sd,&new_booking_order,sizeof(booking_order)) <=0) return -1;  //tries to send the entire new_booking_order struct to the server 

    memset(buffer,0,sizeof(char) * BUFFER_SIZE);
    if(read(sd,buffer,sizeof(char) * BUFFER_SIZE)<= 0) return -1;    //waits for the server answer, if it's zero it means it failed, otherwise the answer is its unique id

	if (strcmp(buffer,"0")==0) {
        printf("booking not accepted by server\n");
        return -1;
	}
    else{
        printf("booking accepted with id %s\n",buffer);
        return 0;
    }
}

int client_delete_booking(int sd){
    char buffer[BUFFER_SIZE];
    printf("Insert id of booking:");
    fgets(buffer, BUFFER_SIZE,stdin);   //waits for the id of the booking to be deleted
    int res = remove_newline_and_check_num(buffer); //checks if the id is not zero (RESERVED FOR SERVER INTEGRITY CHECKS) and it's a number
        if(res == -1) return -1;
        if(res == 0){
            printf("id not valid\n");
            return -1;
        }
    if(write(sd,buffer,strlen(buffer)+1) <= 0) return -1;   //writes id to the server
    
    memset(buffer,0,sizeof(char)*BUFFER_SIZE);
    if(read(sd,buffer,BUFFER_SIZE) <= 0) return -1;  //waits for the answer from the server, 0: server didnt find the order with that id, 1:server deleted that order
    if(strcmp(buffer,"0\n") == 0){
        printf("Id not accepted by server\n");
        return -1;
    } 
    if(strcmp(buffer,"1\n") == 0){
        printf("Id accepted by server\n");
        return 0;
    } 
    return -1;
}

int connect_and_execute(int* p_sd){
    char buffer[BUFFER_SIZE];
    fgets(buffer, BUFFER_SIZE, stdin);          //waits for a command from the user input
    *p_sd = client_init();                      //tries to connect to the server, blocking
    int sd = *p_sd;
    if(sd == -1) return -1;                     //if the server connection is not available, it errors out
    printf("--CONNECTION ESTABLISHED--\n");

    int res = remove_newline_and_check_num(buffer); //checks if the command is a number and between 1 and 3
    if(res == -1) return -1;
    if(res != 1 && res != 2 && res != 3){
        printf("id not valid\n");
        return -1;
    }

    if(write(sd,buffer,strlen(buffer)+1) <= 0){ //writes the command to the server 
        printf("error writing command to socket\n");
        return -1;
    }
	if (strcmp(buffer,"1\n")==0) {
        return client_receive_map(sd);  //if the command was "1" then we try to receive the map
	}
	if (strcmp(buffer,"2\n")==0) {
        return client_send_booking(sd); //if the command was "2" then we try to send a new booking 
	}
	if (strcmp(buffer,"3\n")==0) {
        return client_delete_booking(sd); //if the command was "3" then we try to delete a booking
	}

    printf("command invalid\n");  //in any other case there was a problem, better let the user know that
    return -1;
}

int main(int argc, char* argv[]){
    int server_descriptor = -1;

    while(1){ //loops forever
            printf("Usage:\n1: SHOW MAP\n2:BOOK SEATS\n3:DELETE BOOKING\n");
            printf("Command: ");
            int res = connect_and_execute(&server_descriptor);
            printf("--CONNECTION ENDED WITH RESULT %d--\n",res);
            close(server_descriptor);
        }
    }

