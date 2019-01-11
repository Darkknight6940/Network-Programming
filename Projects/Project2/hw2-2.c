//
//  hw2.c
//  
//
//  Created by QianJun Chen, Alseny Sylla on 10/14/18.
//
#include <ctype.h>
#include <stdio.h> 
#include <string.h>    //strlen
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>    //close
#include <arpa/inet.h> //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO macros
#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>

using namespace std;

#define MAX_CLIENT 5
#define TRUE 1 
#define FALSE 0 


// compare words and find whether characters
// are placed correctly.
int compare(char* cli_word, char* word, int* cor, int* cor_place, int length){
    int correct[length];        // list of record whether that letter is found in guess
    int c;
    for(c=0; c < length; c++){
        correct[c] = 0;
    }
    
    int cor_count = 0;        // count of correct letters
    int c_p = 0;              // count of correct placement of letter
    int i, j;
    for(i = 0; i< length; i++){
        if(toupper(cli_word[i]) == toupper(word[i])){
            c_p++;
        }
        for(j = 0; j < length; j++){
            if(toupper( cli_word[i]) == toupper(word[j]) && correct[j] == 0){
                correct[j] = 1;
                cor_count++;
                break;
            }
        }
    }
    *cor = cor_count;
    *cor_place = c_p;
    
    // if everything is correct return 1
    if(cor_count == length && c_p == length){
        return 1;
    }
    return 0;
}

// check whether the cliend put in words with correct length,
// if it's correct.
int correct_length(int length, char* cli_word){
    if(strlen(cli_word) != length){
        return 0;
    }
    return 1;
}

// structure for storing names
struct Client{
    char* name;
};

int main(int argc, char *argv[]){
    
    if (argc != 3) {
        std::cerr << "Usage:\n " << argv[0] << " infile-students outfile-grades\n";
        return 1;
    }
    std::ifstream in_str(argv[1]);
    
    string temp;
    vector<string> dic;
    
    while(in_str>>temp){
        dic.push_back(temp);
    }
    
    
    srand(time(NULL));
    int random_word_index = rand()%dic.size();
    
    std::string random_word = dic[random_word_index ];
    
    int length = random_word.size();
    char word[length];
    strcpy(word, random_word.c_str());
    int cor, cor_place;
    
    int opt = TRUE; 
    int master_socket , addrlen , new_socket , client_socket[MAX_CLIENT] , 
     activity, i , valread , sd; 
    int max_sd;
    
    // initiate a list of client name as empty string
    struct Client client_name[MAX_CLIENT];
    for (i = 0; i < MAX_CLIENT; i++){
        struct Client temp;
        temp.name = (char*)"";
        client_name[i] = temp;
    }
    
    int current_client = 0;    // count number of clients
    
    char* ask_name = (char*)"What's your name? ";
    struct sockaddr_in address;
    
    char buffer[1025]; // guess word buffer
        
    // set of socket descriptors
    fd_set readfds; 

    // initialise all client_socket[] to 0 so not checked
    for (i = 0; i < MAX_CLIENT; i++) 
    { 
        client_socket[i] = 0; 
    } 


    // create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 

    // set master socket to allow multiple connections ,
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
        sizeof(opt)) < 0 ) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 

    // type of socket created
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(0);

    // bind socket
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(master_socket, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else
        printf("port number %d\n", ntohs(sin.sin_port));
    

    // maximum of 5 clients
    if (listen(master_socket, 5) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    }

    while(TRUE)
    {
        //clear the socket set 
        FD_ZERO(&readfds); 

        //add master socket to set 
        FD_SET(master_socket, &readfds); 
        max_sd = master_socket; 

            //add child sockets to set 
        for ( i = 0 ; i < MAX_CLIENT ; i++) 
        { 
            //socket descriptor 
            sd = client_socket[i]; 
                
            //if valid socket descriptor then add to read list 
            if(sd > 0) 
                FD_SET( sd , &readfds); 
                
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd) 
                max_sd = sd; 
        } 

        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL); 
    
        if ((activity < 0) && (errno!=EINTR)) 
        { 
            printf("Error with select system call"); 
        }
        
        // new client comes
        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
            { 
                perror("Unsuccessful accept system call"); 
                exit(EXIT_FAILURE); 
            }
            
            printf("successful client connected\n");
            
            char name[125];    // name buffer
            int flag = 1;   // for loop termination and continuing
            while(flag){
                if( send(new_socket, ask_name, strlen(ask_name), 0) != strlen(ask_name) )
                {
                    perror("send");
                }
                // get the name
                int namelen ;
                if((namelen = recv(new_socket, name, 125, 0)) < 0){
                    perror("name error");
                }
                //check whether the name exists
                name[namelen-1] = '\0';
                for(i = 0; i < MAX_CLIENT; i++){
                    if(!strcmp(client_name[i].name,name)){
                        flag = 0;
                        break;
                    }
                }
                if(flag == 0){
                    flag = 1;
                }else{
                    flag = 0;
                }
            }
            
            for (i = 0; i < MAX_CLIENT; i++)
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    current_client++;
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                    char users_connect[1024];
                    sprintf(users_connect, "%d players are currently playing and the length of secret word is %d.\n", current_client, length);
                    send(new_socket, users_connect, strlen(users_connect), 0);
                    struct Client new_client;
                    
                    char* namecpy = (char *) malloc(125*sizeof(char));
                    strcpy(namecpy, name);
                    new_client.name = namecpy;
                    client_name[i] = new_client;
                    break;
                }
            }
            

        }
        for (i = 0; i < MAX_CLIENT; i++)
        {
            sd = client_socket[i];
            
            if (FD_ISSET( sd , &readfds))
            {
                //Check if it was for closing , and also read the
                //incoming message
                if ((valread = read( sd , buffer, 1024)) <= 0)
                {
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                    current_client--;
                    struct Client empty;
                    empty.name = (char*)"";
                    client_name[i] = empty;
                }
                
                else
                {
                    
                    buffer[valread-1] = '\0';
                    //compare word length
                    if(!correct_length(length, buffer)){
                        char* wrong = (char*)"Wrong length of word.\n";
                        send(sd, wrong, strlen(wrong), 0);
                    }else if( !compare(buffer, word,&cor, &cor_place, length)){  // compare words
                        int j;
                        for(j = 0; j<MAX_CLIENT; j++){
                            
                            char wrongword[1024];
                            sprintf(wrongword, "%s guessed %s: %d letter(s) were correct and %d letter(s) were correctly placed.\n", client_name[i].name, buffer, cor, cor_place);
                            send(client_socket[j], wrongword, strlen(wrongword), 0);
                        }
                    }else{   //if guess is correct
                        int j;
                        for(j = 0; j<MAX_CLIENT; j++){
                            char wrongword[1024];
                            sprintf(wrongword, "%s has correctly guessed %s\n", client_name[i].name, buffer);
                            send(client_socket[j], wrongword, strlen(wrongword), 0);
                            close(client_socket[j]);
                        }
                        return 0;
                        
                    }
                    
                }
            }
        }
        
    }
    
}
