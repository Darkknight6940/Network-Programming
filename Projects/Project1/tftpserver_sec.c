/*
    Name: Alseny Sylla
    Course: Network Programming
    Assignment 1: TFTP Server Implementation
*/
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>



#define BUF_LEN 4096
#define TIMEOUT_SERV 10
#define RECEIVE_ATTEMPTS 10


void sig_chld(int){
    pid_t process;
    while(1){
        int success = (process = waitpid(-1, NULL, WNOHANG));
        if(success <= 0){
            break;
        }
    }
}


void sig_alarm(int);



void sig_int(int){
    signal(SIGINT, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGINT, sig_int);
    signal(SIGALRM, sig_alarm);
    alarm(10);
}

void sig_alarm(int){
    alarm(10);
}


enum opcode
 {
     RRQ=1,
     WRQ,
     DATA,
     ACK,
     ERROR
};

/* WE will create Typedef Union (for Packet Messages) that will have a collection of struct for:

    1) Packet Defintion (Read or Write)
    2) Error Packet Defintion for error message
    3) ACKNOWLEDGEMENT packet Definition
    4) Data Packet Definiton

    This will make things easier and neat
*/
typedef union 
{

    //1) Packet Definition (We will name requestType)
    struct
    {
        uint16_t opcode;
        char filenameMode[514];

    }requestType;

    //2) Error Packet Definition
            //t
    struct
    {
        uint16_t opcode;
        char errorString[512];
        uint16_t codeError;

    }error;

    uint16_t opcode;

    //3)  ACKNOWLEDGEMENT packet Definition
    struct 
    {
        uint16_t opcode;
        uint16_t blockNumber;
        
    }ack;

    //4) Data Packet Definition
    struct 
    {
        
        uint16_t opcode;
        uint16_t blockNumber;
        char data[512];

    }data;


}packetMessage;




/*
    An ACK packet is sent with an opcode of block number
*/
ssize_t send_acknowledgement_packet (int socket_file_descriptor, uint16_t blockNumber,
    struct sockaddr_in * sock_inf, socklen_t sock_len)
{
    packetMessage message;

    message.ack.opcode = htons(ACK);
    message.ack.blockNumber = htons(blockNumber);

    ssize_t return_len;

    if((return_len = sendto(socket_file_descriptor, &message, sizeof(message.ack),0,
        (struct sockaddr *) sock_inf, sock_len)) < 0)
    {
        perror("sendto was unsuccessful\n");
    }

    return return_len;
}


//error message is sent  with the opcode, error code, the error message 
//and a 0 byte.
ssize_t errorMessage (int socket_file_descriptor, int error_code, const char* error_message,
                    struct sockaddr_in **sock_inf, socklen_t sock_len )
{

    packetMessage message;

    message.opcode = htons(ERROR);

    //Assign the error message to to the error message from the struct using strncpy function
    size_t size_errorString= sizeof(message.error.errorString);
    strncpy(message.error.errorString, error_message, size_errorString );

    // ass the error code to the codeError from the struct
    message.error.codeError = error_code;

    ssize_t return_len;

    if((return_len= sendto(socket_file_descriptor, &message, strlen(error_message) + 4, 0,
        (struct sockaddr*) * sock_inf, sock_len)) < 0)
    {
        perror("SendTo was Unsuccessful\n");
    }

    return return_len;

}



/*WRQ: this function receives each WRQ and generates a socket endpoint so that
                the file can be written/transferred from the client to the server
*/

void THE_WRQ (struct sockaddr_in *sock_inf, packetMessage *message, int buf_len)
{
    //We must support the "octet" mode
    char *buffer = message->requestType.filenameMode;

    char *fileMode = strchr(buffer, '\0') +1;

    if(strcasecmp(fileMode, "octet")!=0)
    {
        perror("Invalid octet file mode\n");
        exit(1);
    }

    //TFTP uses UDP so let's check for protocol
    struct protoent *com_protocol= NULL;
    if((com_protocol = getprotobyname("udp"))==0)
    {
        fprintf(stderr, "Protocol error." );
        exit(1);
    }



    /*
        Timeout is set to 10. no packets/requests received, a timeout happens
    */
    struct timeval tv;

    tv.tv_sec = TIMEOUT_SERV;
    tv.tv_usec = 0;


    //socket endpoint is now created

    int socket_file_descriptor;
    if((socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, com_protocol->p_proto)) == -1)
    {
        perror("socket server failed to be created.\n");
        exit(1);
    }

    if(setsockopt(socket_file_descriptor, SOL_SOCKET, SO_RCVTIMEO, & tv, sizeof(tv)) < 0)
    {
        perror("setsockopt is unsuccessful\n");
        exit(1);
    }


    //now let's open the file to write the data

    FILE *fd;
    fd = fopen(buffer, "w");
    //make sure the file was succesffully open
    if(fd == NULL)
    {   
        perror("file couldn't be opened");
        errorMessage(socket_file_descriptor, errno, strerror(errno), &sock_inf,
            sizeof(*sock_inf));
        exit(1);
    }


    uint16_t blockNum = 0;
    ssize_t current_len;

    //ACK sent to host A with source = B's TID, destination = A's TID
    current_len = send_acknowledgement_packet(socket_file_descriptor, blockNum, sock_inf, sizeof(*sock_inf));

    if(current_len < 0){
        perror("ack was unsuccessful");
        exit(1);  
    }

    bool done = false;

    while(!done)
    {
        //cliend will send data up to 10 times
        //received attempts is 10, so let's set the count to 10.

        int count = RECEIVE_ATTEMPTS;
        //let's keep sending data as long as the count hasn't reached attempts
        while(count > 0)
        {
            ssize_t rec_return;
            socklen_t holder = sizeof(*sock_inf);
            if((rec_return = recvfrom(socket_file_descriptor, message, sizeof(*message), 0, 
                (struct sockaddr *) sock_inf, &holder )) < 0 && errno != EAGAIN)
            {
                perror(" received from invalid (error)\n");
            }

            if(rec_return >= 4){
                count = -1;
                current_len = rec_return;
            }

            if(count != -1)
            {   //Then we know the message size is not correct
                if(rec_return >= 0 && rec_return < 4)
                {
                    perror("Message size isn't correct\n");
                    const char *req_mess = "Request is incorrect";
                    errorMessage(socket_file_descriptor, 0, req_mess, &sock_inf, sizeof(*sock_inf));
                    exit(1);
                }

                if(errno == EAGAIN)
                {
                    printf("Errno, File Transfer is not successful.\n");
                    exit(1);
                }

                //"Host B sends ACK to Host A"

                current_len = send_acknowledgement_packet(socket_file_descriptor, blockNum, sock_inf, sizeof(*sock_inf));

                if(current_len < 0)
                {
                    perror("unsuccessful ack..");
                    exit(1);
                }

            }

            --count;
        }

        //Here count is either equal 0 or less than 0

        if(count == 0)
        {
            printf("File Transfer Timed Out.\n");
            exit(1);
        }

        ++blockNum;

        /* For incorrect opcode or block Number, error messages need to be displayed*/
        if(ntohs(message->ack.blockNumber) != blockNum)
        {
            printf("Warninf: ACK: Block number is not valid.\n");
            const char *m =  "Invalid block number for acknowledgement..";
            errorMessage(socket_file_descriptor, 0, m, &sock_inf, sizeof(*sock_inf));
            exit(1);
        }

        if(ntohs(message->opcode) != DATA)
        {
             printf("ACK: Message error %s %i\n", inet_ntoa(sock_inf->sin_addr), sock_inf->sin_port);
             const char *m =  "invalid message/packet";
             errorMessage(socket_file_descriptor, 0, m, &sock_inf, sizeof(*sock_inf));
             exit(1);
        }

        if(ntohs(message->opcode) == ERROR)
        {
            printf("MESSAGE ERROR %s %i", inet_ntoa(sock_inf->sin_addr), sock_inf->sin_port);
            exit(1);
        }

        if(sizeof(message->data) > current_len)
        {
            done = true;
        }

        message->data.data[512]= '\0';
        current_len = fwrite(message->data.data, 1, current_len-4, fd);
        if(current_len < 0)
        {
            perror("Writing data on the file was not successful");
            exit(1);
        }


        //AN "ACK" is sent from Host B to Host A
        current_len = send_acknowledgement_packet(socket_file_descriptor, blockNum, sock_inf, sizeof(*sock_inf));
        if(current_len < 0){
            perror("Unsuccessful acknowledgement..");
            exit(1);
        }
      

    }

    fclose(fd);
    close(socket_file_descriptor); 

}

void THE_RRQ(struct sockaddr_in *sock_inf, packetMessage *message, int buf_len)
{
     //We must support the "octet" mode
    char *buffer = message->requestType.filenameMode;

    char *fileMode = strchr(buffer, '\0') +1;

    if(strcasecmp(fileMode, "octet")!=0)
    {
        perror("Invalid octet file mode\n");
        exit(1);
    }

    //TFTP uses UDP so let's check for protocol
    struct protoent *com_protocol= NULL;
    if((com_protocol = getprotobyname("udp"))==0)
    {
        fprintf(stderr, "Protocol error." );
        exit(1);
    }



    /*
        Timeout is set to 10. no packets/requests received, a timeout happens
    */
    struct timeval tv;

    tv.tv_sec = TIMEOUT_SERV;
    tv.tv_usec = 0;


    //socket endpoint is now created

    int socket_file_descriptor;
    if((socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, com_protocol->p_proto)) == -1)
    {
        perror("socket server failed to be created.\n");
        exit(1);
    }

    if(setsockopt(socket_file_descriptor, SOL_SOCKET, SO_RCVTIMEO, & tv, sizeof(tv)) < 0)
    {
        perror("setsockopt is unsuccessful\n");
        exit(1);
    }


    FILE *fd;
    fd = fopen(buffer, "r");
    //make sure the file was succesffully open
    if(fd == NULL)
    {   
        perror("file couldn't be opened");
        errorMessage(socket_file_descriptor, errno, strerror(errno), &sock_inf,
            sizeof(*sock_inf));
        exit(1);
    }

    uint16_t blockNum = 0;
    ssize_t current_len;

    bool done = false;

    while(!done)
    {
        //bocks of 512 of data is read
        char dataRead[512];
        current_len = fread(dataRead,1,sizeof(dataRead), fd);
        dataRead[current_len] = '\0';
        ++blockNum;

        //last block number is any # of bytes less than 512
        if(current_len < 512)
        {
            done  = true;
        }


         
        int count = RECEIVE_ATTEMPTS;
        while(count > 0)
        {   
            packetMessage mess;
            ssize_t numberSent;
            

            mess.opcode = htons(DATA);
            mess.data.blockNumber = htons(blockNum);
            memcpy(mess.data.data, dataRead, current_len);
            printf("DATA READ: %s\n",dataRead);

            if((numberSent = sendto(socket_file_descriptor,  &mess, 4 + current_len, 
                0, (struct sockaddr *) sock_inf, sizeof(*sock_inf))) == 0){
                perror("send is not successful\n");
                exit(1);
            }

            if(numberSent < 0){
                perror("Transfer was not successful.");
                exit(1);
            }



            ssize_t rec_return;
            socklen_t holder = sizeof(*sock_inf);
            if( (rec_return = recvfrom(socket_file_descriptor, message, sizeof(*message), 
                0, (struct sockaddr *) sock_inf,  &holder)) < 0 && errno != EAGAIN)
            {
                perror("Receive from error\n");
            }

            if(rec_return >= 4){
                count = -1;
            }


            if(count != -1){
                if(rec_return < 4  && rec_return >=0){
                    const char *req_s = "request was not valid.";
                    errorMessage(socket_file_descriptor, 0, req_s, &sock_inf, sizeof(sock_inf));
                    exit(1);
                }

                if(errno == EAGAIN){
                   
                    printf("ERRNO,File transfer unsuccessful.\n");
                    exit(1);
                }
            }
        

            --count;
        }

        if(count == 0){
            printf("File transfer timed out.\n");
            exit(1);
        }




         /*If the received
        message struct has an incorrect opcode or
        incorrect block number, error packets are sent or error messages
        are displayed.*/

        if(ntohs(message->opcode) == ERROR){
            printf("MESSAGE ERROR %s %i", inet_ntoa(sock_inf->sin_addr), sock_inf->sin_port);
            exit(1);
        }

        if(ntohs(message->opcode) != ACK){
             printf("ACK: message error %s %i", inet_ntoa(sock_inf->sin_addr), sock_inf->sin_port);
             const char *mess =  "Invalid packet";
             errorMessage(socket_file_descriptor, 0, mess, &sock_inf, sizeof(sock_inf));
             exit(1);
        }

        if(ntohs(message->ack.blockNumber) != blockNum){
            printf("ACK: Block number is not valid.\n");
            const char *mess =  "Invalid block number for ack";
            errorMessage(socket_file_descriptor, 0, mess, &sock_inf, sizeof(*sock_inf));
            exit(1);
        }


    

      

    }

    fclose(fd);
    close(socket_file_descriptor);





}


int main(int argc, char *argv[]){


   
    struct sigaction act;
  
   
    
    act.sa_handler = sig_alarm;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGALRM, &act, NULL);


    act.sa_handler = sig_chld;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, NULL);

    ssize_t n;
    unsigned short int opcode;
    uint16_t * opcode_ptr;
    struct sockaddr_in sock_info;
    
    
    socklen_t sockaddr_len;
    int serverSocket;
    
    sockaddr_len = sizeof(sock_info);


  
    struct servent *serventReturn = NULL;
    if((serventReturn = getservbyname("tftp", "udp")) == 0){
        perror("GET SERVER BY NAME ERROR.\n");
        exit(1);
    }

    
    memset(&sock_info, 0, sockaddr_len);


   

    sock_info.sin_addr.s_addr = htonl(INADDR_ANY);


        
    sock_info.sin_port = htons(serventReturn->s_port);
    sock_info.sin_family = PF_INET;
    

  
   
    if((serverSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(-1);
    }
    
    if(bind(serverSocket, (struct sockaddr *)&sock_info, sockaddr_len) < 0) {
        perror("bind");
        exit(-1);
    }



    getsockname(serverSocket, (struct sockaddr *)&sock_info, &sockaddr_len);
    printf("%d\n", ntohs(sock_info.sin_port));

    packetMessage mymessage;
    while(1) 
    {
    intr_recv:

    
          struct sockaddr_in client_sock;
          socklen_t slen = sizeof(client_sock);
         

    
      
        n = recvfrom(serverSocket, &mymessage, sizeof(mymessage), 0,
                     (struct sockaddr *)&client_sock, &slen);
       
        if(n < 0) 
        {
            if(errno == EINTR) goto intr_recv;
            perror("recvfrom");
            exit(-1);
        }
       
        uint16_t opcode_tmp = -1;
        opcode_ptr = &opcode_tmp;
        *opcode_ptr = mymessage.opcode;
        opcode = ntohs(*opcode_ptr);
        printf("ACTUAL OPCODE %d\n", opcode);
        if(opcode != RRQ && opcode != WRQ)
         {
            *opcode_ptr = htons(ERROR);
            *(opcode_ptr + 1) = htons(4);
        intr_send:
            n = sendto(serverSocket, &mymessage, 5, 0,
                       (struct sockaddr *)&sock_info, sockaddr_len);
            if(n < 0) {
                if(h_errno == EAI_INTR) goto intr_send;
                perror("sendto");
                exit(-1);
            }
        }
        else {
            if(fork() == 0) 
            {

                close(serverSocket);


              
            
                if(opcode == WRQ) THE_WRQ(&client_sock, &mymessage, BUF_LEN);

                  if(opcode == RRQ) THE_RRQ(&client_sock, &mymessage, BUF_LEN );
               
                break;
            }
            else 
            {
            }
        }
    }
    

    
    
    
    

    return 0;
}



