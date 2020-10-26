#include "common.h"

int next_free_id = 1;
FILE *database = NULL;
booking_order *array_booking_orders[MAP_SIZE_Y*MAP_SIZE_X];
int client_fd = -1;


int check_map_pos(int x, int y){
    return (map_array[y][x] == 1);
}

int write_map_pos(int x, int y){
    map_array[y][x] = 1;
}

void print_map(){
    for(int y = 0; y < MAP_SIZE_Y; y++){
        for(int x = 0; x < MAP_SIZE_X; x++){
            printf("%d ",map_array[y][x]);
        }
        printf("\n");
    }
}

void clear_map(){
    for(int y = 0; y < MAP_SIZE_Y; y++){
        for(int x = 0; x < MAP_SIZE_X; x++){
            map_array[y][x] = 0;
        }
    }
}

void init_db(){
    database = fopen("bookings.db", "r+");      //trying to open an already existing database
    if(database == NULL){                       //if it doesnt exists
        println("No database file found, creating a new one");
        database = fopen("bookings.db", "a+");  //creating a new database
        if(database == NULL) errorAndExit("Could not make new database");
        fseek(database, 0L, SEEK_SET);          //a+ has unexpected cursor position, setting it manually to start of file
        booking_order default_order;            
        default_order.id = 0;
        fwrite(&default_order, sizeof(booking_order), 1,database); //writing the first booking_order struct, for integrity checking purposes later on
        if(ferror(database)) errorAndExit("Could not write on database");
    }
    fseek(database, 0L, SEEK_SET);
}

void destroy_db(){
    fclose(database);
    if(remove("bookings.db") == -1) errorAndExit("error deleting db");
}


int read_db_to_memory(){
    fseek(database, 0L, SEEK_SET); //setting the database cursor to beginning
    int order_count = -1;
    while(1){                                                      //reading loop
        booking_order *t_booking = malloc(sizeof(booking_order));  //allocating memory for a booking_order struct
        if(t_booking == NULL) errorAndExit("error allocating memory");
        size_t byte_read = fread(t_booking, sizeof(booking_order),1,database); //reading 1 booking_order struct
        if(byte_read == 0) break;                                   //if we reached EOF then we have done reading
        else{
            if(order_count == -1 ) order_count =0;
            array_booking_orders[order_count] = t_booking;         //if a booking_order read is successful, i save it on memory
            order_count++;
        }
    }
    return order_count;
}

void update_map_from_db(){
    printf("--UPDATING MAP--\n");
    clear_map();                                //resetting all the map entries in memory
    int order_count = read_db_to_memory();      //getting all the booking_order structs from database to memory
    int id_zero_found = 0;                      //integrity checking flag
    for(int i = 0; i < order_count;i++){        //for each order we have saved in memory
        int id = array_booking_orders[i]->id;
        if(id == 0) id_zero_found = 1;          //if at least one order has id=0 then the database could potentially be good
        int size = array_booking_orders[i]->size;   
        int *rows = array_booking_orders[i]->rows;
        int *cols = array_booking_orders[i]->cols;
        printf("processing database order id %d\n",id); 
        if(id != 0) for(int i = 0; i < size;i++) write_map_pos(cols[i],rows[i]);    //if the id is not zero, then we update the map entries in memory
        next_free_id = MAX(id,next_free_id)+1;  //next_free_id is updated to always have a bigger value than the biggest id we currently have in memory, making it unique
    }
    if(id_zero_found == 0){                     //if there's no order with id=0, then databse is corrupted and needs to be deleted
        destroy_db(&database);
        errorAndExit("database failure, restart application");
    }
    printf("--END UPDATING MAP--\n");
}

void update_db(booking_order *new_booking_order){
    fseek(database,0,SEEK_END);
    if(fwrite(new_booking_order,sizeof(booking_order),1,database) <= 0) errorAndExit("database update failed");
}


int trywrite_booking_db(booking_order *booking_requested){
        int id = booking_requested->id;
        int size = booking_requested->size;
        int *rows = booking_requested->rows;
        int *cols = booking_requested->cols;
        printf("processing new order\n");
        for(int i=0; i < size;i++) if(check_map_pos(cols[i],rows[i])) return -1; //check if all the entries are valid and not already taken
        printf("all positions clear, giving id: %d\n",next_free_id);
        //for(int i=0;i < size; i++) write_map_pos(rows[i],cols[i]);
        booking_requested->id = next_free_id;                                    //assign this order the unique id
        update_db(booking_requested);                                            //updating database with the new entry
        return next_free_id;
}

int trydelete_booking_db(int target_id){
    int found = 0;
    if(target_id == 0) return 0;
    int order_count = read_db_to_memory();
    if(order_count == -1) errorAndExit("error db");

    for(int i = 0; i < order_count;i++){                        //for loop to check if we find an order with a particular id value
        if(array_booking_orders[i]->id == target_id){
            array_booking_orders[i]->id = -1;
            found = 1;
            printf("found order id %d\n",target_id);
            break;
        }
    }

    if(found == 1){                                            //if there is a match
        FILE* fp;
        fp = fopen("t_bookings.db", "w+");                      //create a new temporary database
        if(fp == NULL){
            if(fp == NULL) errorAndExit("Could not make new database");
        }
        fseek(fp, 0L, SEEK_SET);
        for(int i = 0; i < order_count; i++){
            if(array_booking_orders[i]->id != -1) if(fwrite(array_booking_orders[i], sizeof(booking_order),1,fp) <= 0) errorAndExit("Could not write new database"); //write the content of the original database but without the matched id
        }
        fclose(database);
        fclose(fp);
        remove("bookings.db");                                  //removing old database
        rename("t_bookings.db","bookings.db");                  //renaming the temp database to a new database
        init_db(&database);                                     //reopening the database
        return 1;
    }
    return 0;

}

int server_init(void){ //typical TCP socket implementation
    int sd = socket(AF_INET, SOCK_STREAM,0);
    if(sd == -1) errorAndExit("Socket initialization failed");
    struct sockaddr_in sockaddr_serv;
    sockaddr_serv.sin_family = AF_INET;
    sockaddr_serv.sin_addr.s_addr = INADDR_ANY;
    sockaddr_serv.sin_port = htons(SERVER_PORT);
    if(bind(sd,(struct sockaddr*)&sockaddr_serv,sizeof(sockaddr_serv)) == -1) errorAndExit("Socket initialization failed");
    if(listen(sd, 1) == -1) errorAndExit("Socket initialization failed");
    return sd;
}

int server_wait_for_connection(int sd){ //typical TCP socket connection implementation
    struct sockaddr_in client_sockaddr;
    socklen_t *sockkaddr_in_length = malloc(sizeof(struct sockaddr_in));
    int client_fd = accept(sd, (struct sockaddr*)&client_sockaddr, sockkaddr_in_length);
    if(client_fd == -1) return-1;
    return client_fd;
}

int server_send_map(int sd){    //sending the map array in one packet
    if (write(sd,map_array,sizeof(char) * MAP_SIZE_X * MAP_SIZE_Y) <= 0) return -1;
    return 0;
}

int server_receive_booking(int sd){
    booking_order new_booking_order; //allocating memory for a new booking order
    if(read(sd,&new_booking_order,sizeof(booking_order)) <= 0) return -1; //ifa booking_order struct is received
    int res = trywrite_booking_db(&new_booking_order); //tries to save this order, returns -1 if something went wrong or the id was already in use
    char buffer[BUFFER_SIZE];
    if(res == -1){
        printf("booking not accepted, sending 0\n");
        sprintf(buffer,"0");
        if(write(sd,buffer,2) <= 0) return -1; //answers to the client about the failed booking attempt
        return 0;
    }
    else{
        printf("booking accepted, sending id\n"); 
        sprintf(buffer,"%d",next_free_id);
        if(write(sd,buffer,strlen(buffer)+1) <= 0) return -1; //answers to the client about the order successful request with the unique id of the booking
        return 0;
    }
    return 0;

}

int server_delete_booking(int sd){
    char buffer[BUFFER_SIZE];
    if(read(sd,buffer,BUFFER_SIZE) <= 0) return -1; //waits for the client to send an id

    int num = remove_newline_and_check_num(buffer); 

    int result = 0;
    if(num == -1){
        result = 0;                                 //if the message received is not a number, it fails
    } 
    else{
        result = trydelete_booking_db(num);         //if the message received is a number, then tries to delete it from the database, result = 0 if no order matched, otherwise 1
    }
    memset(buffer,0,sizeof(char) * BUFFER_SIZE);
    sprintf(buffer,"%d\n",result);
    if(write(sd,buffer,BUFFER_SIZE) <= 0) return -1; //answers to the client with the result of the deleting operation
    return 0;
}

int server_read_execute_command(int sd){
    char string_command[BUFFER_SIZE];
    memset(string_command, 0, BUFFER_SIZE);
    
    if(read(sd,string_command,BUFFER_SIZE)<=0) return -1; //waits for a command from the client

	if (strcmp(string_command,"1\n")==0) {
        return server_send_map(sd);       //received 1: it will try to send the map contents to the client
	}
	if (strcmp(string_command,"2\n")==0) {
        return server_receive_booking(sd); //received 2: it will try to accept a new booking from the client
	}
	if (strcmp(string_command,"3\n")==0) {
        return server_delete_booking(sd); //received 3: it will try to delete a booking by requesting its id from the client
	}
	return -1;
}


int main(int argc, char* argv[]){
    int server_descriptor = -1;
    init_db(); //init database file
    update_map_from_db(); //reading database file and updating map in memory
    server_descriptor = server_init(); //initializing TCP server
    while(1){                    //looping forever
        update_map_from_db(); 
        print_map();
        printf("Waiting for connection:\n");
        client_fd = server_wait_for_connection(server_descriptor); //blocking until a client connects
        if(client_fd != 1){                                        //we have a connection!
            printf("--CONNECTION ESTABLISHED--\n");
            
            struct timeval timeout;                                 //setting up a timeout struct for this connection
            timeout.tv_sec = SERVER_TIMEOUT_S ;
            timeout.tv_usec = 0;
           // alarm(30);                                             //giving a 30 seconds timeout to this connection
            if (setsockopt (client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0)
                errorAndExit("setsockopt failed\n");

            if (setsockopt (client_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0)
                errorAndExit("setsockopt failed\n");

            int res = server_read_execute_command(client_fd);      //receive command and execute it
            close(client_fd);                                      //close the connection
            printf("--CONNECTION ENDED WITH RESULT %d--\n",res);
        }
    }
}

