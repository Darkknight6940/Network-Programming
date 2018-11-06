
/*
 Assignment 3
 Team members:
 Sylla, Alseny
 Qianjun Chen
 */
#include <poll.h>
#include <sys/uio.h>
#include <netdb.h>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <list>
#include <arpa/inet.h>
#include <vector>
#include <utility>
#include <map>
#include <string>
#include <regex>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>

using namespace std;


class User{
public:
    User(std::string username){
        name = username;
        isOperator = false;
    }
    
    const std::string get_name() const{
        return name;
    }
    
    void makeOperator(){
        isOperator = true;
    }
    
    bool is_operator(){
        return isOperator;
    }
    
    ~User(){
        
    }
    
    
private:
    std::string name;
    bool isOperator;
    
};

class Channel{
public:
    //copy constructor for the channel class.
    Channel(std::string cname){
        users = std::list<User>();
        channel_name = cname;
    }
    //name of the current channel.
    std::string get_cname() const{
        return channel_name;
    }
    //add a user to the channel
    void add_user(std::string username){
        users.push_back(User(username));
    }
    
    //Function that returns the total number of users in the channel
    int users_num(){
        return users.size();
    }
    /*
     if a user is this channel, this function will remove them
     and return true. Otherwise false
     */
    bool remove_user(std::string username){
        std::list<User>::iterator it = std::find(users.begin(), users.end(), User(username));
        if(it!=users.end()){
            users.erase(it);
            return true;
        }
        
        
        return false;
    }
    
    //Boolean to verify whether a user is this channel
    bool user_in(std::string username){
        std::list<User>::iterator it = std::find(users.begin(), users.end(), User(username));
        if(it != users.end()){
            return true;
        }
        
        return false;
    }
    
    //list containing all current activer users
    const std::list<User>& get_users() const{
        return users;
    }
    
    ~Channel(){
        
    }
    
private:
    std::list<User> users;
    std::string channel_name;
    
    
};

/*
 Just in case users have the same nickname
 create user equal operator to compare
 */
bool operator==(const User &first, const User &second){
    if(first.get_name()== second.get_name()){
        return true;
    }
    return false;
}

bool operator == (const Channel &c1, const Channel &c2){
    return c1.get_cname() == c2.get_cname();
}





//Global variable to store all the channels in a list
//Global variable to store all the channels in a list
std::vector<Channel> curr_channels;
std::vector<int> client_socks;
std::vector<User> curr_users;
std::map<int, std::string> names;
std::vector<std::vector<std::string> > commands;

void remove_client(int fd, int client_i){
    for (int i = 0 ; i < client_i ; i++ )
    {
        if ( fd == client_socks[i] )
        {
            int k;
            for ( k = i ; k < client_i - 1 ; k++ )
            {
                client_socks[k] = client_socks[k+1];
            }
            break;
        }
    }
}

//LIst is a function that returns a string displaying all the channels
// or users in a specific channels.
// Then we will send that string to the socket.
std::string LIST (const std::string &second_command)
{
    std::string send_string = "";//will return this string
    //if second_command is empty then will return all the channels.
    if (second_command == "")
    {
        //No channel specified
        send_string += "There are currently " + std::to_string(curr_channels.size()) + " channels." + "\n";
        for(int i = 0; i < curr_channels.size(); i++)
        {
            send_string += ("* " + curr_channels[i].get_cname())+ "\n";
        }
        
    }
    else
    {
        //Channel is specified so print all the users
        int channel_location  = -1;
        for( int i=0; i < curr_channels.size(); i++){
            if(curr_channels[i].get_cname()== second_command){
                //we found the channel in all channels.
                channel_location = i;
            }
        }
        if (channel_location != -1)
        {
            send_string += "There are currently " + std::to_string(curr_channels[channel_location].users_num()) + " members.\n";
            send_string += second_command + " members: ";
            
            std::list<User>::const_iterator users_itr;
            users_itr = curr_channels[channel_location].get_users().begin();
            for(int u = 0; u < curr_channels[channel_location].users_num(); u++)
            {
                send_string += + " " + users_itr->get_name();
                users_itr++;
            }
            send_string += "\n";
            
        }
        else
        {
            send_string += "There are currently " + std::to_string(curr_channels.size()) + " channels." + "\n";
        }
    }
    
    
    return send_string;
    
}




std::string OPERATOR (const std::string &second_command, int client_fd, const std::string& operator_password)
{
    /*
        Passwords should be at most 20 characters and
        should adhere to the following regular expression: [a-zA-Z][_0-9a-zA-Z]*
    */
    if(!std::regex_match(second_command, std::regex("[a-zA-Z][_0-9a-zA-Z]*")))
    {
        //this implies that it's invalid password. not the right format
        return std::string("Operators not permitted.\n");
    }
    else
    {
        if(second_command !=  operator_password)//didn't get the password right
        {
            return std::string("Invalid OPERATOR command. \n");
        }
        else
        {
            //passwords match 
            //we look for the user and we make him the operator
            for(unsigned int i =0; i < curr_users.size(); i++)
            {
                if (curr_users[i].get_name()== names[client_fd])
                {
                    curr_users[i].makeOperator();
                }
            }
            return std::string("OPERATOR status bestowed.\n");
        }
    }
}





int main(int argc, char* argv[])
{
    
    
    
    /*
     We should not accept no mome than three argument.
     first one is the .out
     second one in case it the optional flag --optpass=<password> is supplied
     */
    if(argc != 1 && argc != 2)
    {
        std::cerr << "ERROR: Invalid number of command line arguments" << std::endl;
        std::cerr << "Usage: Example: cri.out --opt-pass=netprog" << std::endl;
        return EXIT_FAILURE;
    }
    
    std::string operator_password =""; //will store password supplied
    
    if(argc ==2)
    {
        std::string s =argv[1];
        std::string delimiter = "=";
        std::string token = s.substr(s.find(delimiter)+1, s.size());
        
        operator_password = token;
    }
    
    
    //CREATE THE SOCKET LISTENER FOR CONNECTION AS TCP SOCKET
    int sock;
    /*using IPV6 Internet protocols (AF_INET6) for the domain
     ~using SOCK_STREAM for sequenced, reliable, two way, connection-based byte streams.
     ~0 is used normally as the  single protocol.
     ~on success, a file descriptor for new socket is retuned. on error, -1 is returned*/
    if((sock = socket( AF_INET6, SOCK_STREAM, 0)) == -1){
        perror("Unsuccessful socket() systems call");
        return EXIT_FAILURE;
    }
    //Let's also make sure the file descriptor of the socket is not less than 0.
    if(sock < 0)
    {
        perror("file descritor of the socket < 0. See Socket()");
        return EXIT_FAILURE;
    }
    
    
    //CREATE OPTION (setsockopt()) TO MANIPULATE OPTIONS for our socket
    int setOptions = 1;
    /*SQL_SOCKET is used to manipulate options at the sockets API level
     SO_REUSEADDR is used to reuse the ip address and port and coninuing the connection
     The last two arguments are used to access option values for setsockopt()
     On success, 0 is returned and On error -1*/
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char * ) &setOptions, sizeof(setOptions)) < 0){
        perror("setsockopt err");
        exit(EXIT_FAILURE);
    }
    
    //Using sockaddr_in6 structure since it is bigger than generic sockaddr. All address
    //types can be stored safely
    struct sockaddr_in6 server; //server  side
    struct sockaddr_in6 client; //client side
    
    server.sin6_family = AF_INET6;
    server.sin6_addr   = in6addr_any;
    
    //Randomly chosen port
    unsigned short port = 8888;
    
    /* htons() is host-to-network-short for marshalling */
    /* Internet is "big endian"; Intel is "little endian" */
    server.sin6_port = htons( port );
    int len = sizeof( server );
    
    
    //associate the server socket endpoint with a particular port
    if ( bind( sock, (struct sockaddr *)&server, len ) < 0 )
    {
        perror( "bindind (bind() server socket endpoint couldn't associate with port" );
        exit( EXIT_FAILURE );
    }
    
    //new addded
    if (listen(sock, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }else{
        printf("Port number is %d\n", port);
    }
    
    int addrlen, new_socket, i, received, ready;
    int cliend_i = 0;
    char buff[1024];
    std::string invalid_com = "";
    
    //set of socket descriptors
    fd_set readfds;
    
    while(true){
        //clear the socket set
        FD_ZERO(&readfds);
        
        //add master socket to set
        FD_SET(sock, &readfds);
        
        for(i = 0; i< cliend_i; i++){
            FD_SET(client_socks[i], &readfds);
        }
        
        ready = select( FD_SETSIZE, &readfds, NULL, NULL, NULL );
        if((ready < 0)&& (errno != EINTR)){
            printf("Error with select");
        }
        
        //new client connects
        if(FD_ISSET(sock, &readfds)){
            if((new_socket = accept(sock, (struct sockaddr *)&client, (socklen_t*)&addrlen))< 0){
                perror("Unsuccessful accept call");
                exit(EXIT_FAILURE);
            }
            client_socks.push_back(new_socket);
            commands.push_back(std::vector<std::string>());
            cliend_i++;
        }
        
        for(i = 0; i < cliend_i ; i++){
            int client_fd = client_socks[i];
            if(FD_ISSET(client_fd, &readfds)){
                
                if((received = recv(client_fd, buff, 1024, 0))<0){
                    perror("error with recv");
                }
                else if(received == 0){
                    printf("Client %d closed connection.\n", client_fd);
                    close(client_fd);
                    
                    //here should be function that remove a client from list
                    remove_client(client_fd, cliend_i);
                    commands.erase(commands.begin()+i);
                    cliend_i--;
                }
                else{
                    //received packet
                    buff[received] = '\0';
                    
                    const char sep[2] = " ";
                    std::string origion(buff);
                    std::string first = "";
                    std::string second = "";
                    
                    char *token = strtok(buff, sep);
                    
                    if(token != NULL){
                        if(token[strlen(token)-1]=='\n'){
                            token[strlen(token)-1] = '\0';
                        }
                    }
                    
                    //parsing
                    if((token != NULL && std::string(origion).size() != std::string(token).size())
                       ||(token != NULL && std::string(token)== "PART")){
                        std::string temp(token);
                        first = temp;
                        
                        if(commands[i].size() == 0 && strncmp(first.c_str(), "USER", 4) != 0){
                            invalid_com = "Invalid command, please identify yourself with USER.\n";
                            if((received = send(client_fd, invalid_com.c_str(), invalid_com.size(), 0)) != invalid_com.size()){
                                perror("send()\n");
                            }
                            close(client_fd);
                            //here should be function that remove a client from list
                            remove_client(client_fd, cliend_i);
                            cliend_i--;
                            commands.erase(commands.begin()+i);
                            continue;
                        }
                        commands[i].push_back(first);
                        
                        if((token = strtok(NULL, sep)) != NULL){
                            if(token[strlen(token)-1] == '\n'){
                                token[strlen(token)-1]='\0';
                            }
                            std::string temp(token);
                            second = temp;
                        }
                        
                    }
                    else{
                        invalid_com = "Invalid command.\n";
                        if((received = send(client_fd, invalid_com.c_str(), invalid_com.size(), 0)) != invalid_com.size()){
                            perror("send()");
                        }
                        close(client_fd);
                        
                        
                        remove_client(client_fd, cliend_i);
                        
                        
                        
                        cliend_i--;
                        commands.erase(commands.begin()+i);
                        continue;
                    }
                    
                    
                    //USER
                    if(strncmp(first.c_str(), "USER", 4)==0){
                        if(second.size() > 20 || !std::regex_match(second, std::regex("[a-zA-Z][_0-9a-zA-Z]*"))){
                            std::string message = "Invalid user name.\n";
                            if((received=send(client_fd, message.c_str(), message.size(), 0))!= message.size() ){
                                perror("send()");
                            }
                        }else{
                            User newu(second);
                            names.insert(std::make_pair(client_socks[i],second));
                            curr_users.push_back(newu);
                            std::string message = "Welcome, "+second+".\n";
                            if((received = send(client_fd, message.c_str(), message.size(), 0))!= message.size()){
                                perror("send()");
                            }
                        }
                        
                    }
                    
                    
                    //LIST
                    else if(strncmp(first.c_str(), "LIST", 4)== 0){
                        
                        std::string send_string = LIST(second);
                        received = send (client_fd, send_string.c_str(), send_string.size(),0);
                        if(received != send_string.size())
                        {
                            perror("send failed in LIST");
                        }
                        
                        
                    }
                    
                    
                    //JOIN
                    else if(strncmp(first.c_str(), "JOIN", 4)== 0){

                        
                        //std::string send_string = JOIN(second);
                        

                        std::string message = "";
                        if(second.size() > 20 || !std::regex_match(second, std::regex("#[a-zA-Z][_0-9a-zA-Z]*"))){
                            message = "Invalid channel name.\n";
                            received = send (client_fd, message.c_str(), message.size(), 0);
                            if(received != message.size()){
                                perror("send failed in JOIN");
                            }
                        }else{
                            Channel new_chanel(second);
                            std::vector<Channel>::iterator it = std::find(curr_channels.begin(), curr_channels.end(), new_chanel);
                            if(it == curr_channels.end()){
                                new_chanel.add_user(names[client_socks[i]]);
                                curr_channels.push_back(new_chanel);
                            }else{
                                it->add_user(names[client_socks[i]]);
                                for(std::map<int,std::string>::iterator it2 = names.begin(); it2 != names.end(); it2++){
                                    if(it->user_in(it2->second) && it2->second != names[client_socks[i]]){
                                        std::string notification = second+"> "+names[client_socks[i]]+" joined the channel.\n";
                                        if((received = send(it2->first, notification.c_str(), notification.size(), 0))!= notification.size()){
                                            perror("send() failed in join");
                                        }
                                        
                                    }
                                }
                            }
                            message = "Joined channel "+second+"\n";
                            received = send (client_fd, message.c_str(), message.size(), 0);
                            if(received != message.size()){
                                perror("send failed in JOIN");
                            }
                        }
                    }
                    
                    
                    //PART
                    else if(strncmp(first.c_str(), "PART", 4)==0){
                       
                        
                        
                        

                        //std::cout<<"WITHIN THIS BRANCH.."<<std::endl;
                        
                        std::string message = "";
                        if(second == ""){
                            for(int k = 0; k < curr_channels.size(); k++){
                                curr_channels[k].remove_user(names[client_fd]);
                            }
                        }else{
                            std::vector<Channel>::iterator it = std::find(curr_channels.begin(), curr_channels.end(), Channel(second));
                            if(it == curr_channels.end()){
                                message= "Channel "+second+" does not exist.\n";
                                received = send (client_fd, message.c_str(), message.size(), 0);
                                if(received != message.size()){
                                    perror("send failed in PART");
                                }
                            }else{
                                bool removal = it->remove_user(names[client_fd]);
                                if(!removal){
                                    message = "You are not currently in " + second+".\n";
                                    received = send(client_fd, message.c_str(), message.size(), 0);
                                    if(received != message.size()){
                                        perror("send() failed in PART");
                                    }
                                }else{
                                    message = second+"> "+names[client_fd]+" left the channel.\n";
                                    for(std::map<int, std::string>::iterator it2 = names.begin(); it2!=names.end(); it2++){
                                        if(it->user_in(it2->second)){
                                            received = send(it2->first, message.c_str(), message.size(), 0);
                                            if(received != message.size()){
                                                perror("send() failed in PART");
                                            }
                                        }
                                        
                                    }
                                    received =send(client_fd, message.c_str(), message.size(), 0);
                                    if(received != message.size()){
                                        perror("send() failed in PART");
                                    }
                                    
                                    
                                }
                                
                                
                            }
                            
                            
                            
                        }
                    }
                    
                    //OPERATOR
                    else if(strncmp(first.c_str(), "OPERATOR", 8)==0){
                        std::string send_string = OPERATOR(second, client_fd, operator_password);
                        received = send(client_fd, send_string.c_str(), send_string.size(),0);
                        if(received != send_string.size())
                        {
                            perror("send() failed in OPERATOR");
                        }
                        
                        
                    }
                    
                    
                    //KICK
                    else if(strncmp(first.c_str(), "KICK", 4)==0){
                        //first we find the operator
                        int  user_curr_loc = -1;
                        for(unsigned int j=0; j < curr_users.size(); j++)
                        {
                            if(curr_users[j] == User(names[client_fd]))
                            {
                                user_curr_loc = j;
                                break;
                            }
                        }
                        if(user_curr_loc != -1 && curr_users[user_curr_loc].is_operator())
                        {
                            //We found the user and he is the operator

                            //let's find the channels
                            int chanel_curr_loc = -1;
                            for(unsigned int j =0; j < curr_channels.size(); j++)
                            {
                                if (curr_channels[j] == Channel(second))
                                {
                                    chanel_curr_loc = j;
                                    break;
                                }
                            }
                            if(chanel_curr_loc != -1)
                            {
                               char* third = strtok(NULL, " ");
                               if(third != NULL)//this refers to the user that will be kicked out
                               {
                                    if(third[strlen(third)-1]=='\n')
                                    {
                                        third[strlen(third)-1]='\0';
                                    }

                                    for(std::map<int, std::string>::iterator name_itr = names.begin(); name_itr != names.end(); name_itr++)
                                    {
                                        if(curr_channels[chanel_curr_loc].user_in(name_itr->second))
                                        {
                                            std::string send_string = second + ">" + third + " has been kicked from the channel.\n";
                                            received = send(name_itr->first, send_string.c_str(), send_string.size(), 0);
                                            if(received != send_string.size())
                                            {
                                                perror("send() failed in KICK");
                                            }

                                        }
                                    }

                                   curr_channels[chanel_curr_loc].remove_user(std::string(third)); 
                               } 
                            }
                        }
                        else
                        {
                            if(!curr_users[user_curr_loc].is_operator())
                            {
                                std::string send_string = "Operator privileges are not granted.\n";
                                received = send(client_fd, send_string.c_str(), send_string.size(),0);
                                if(received != send_string.size())
                                {
                                    perror("send() failed in Kick");
                                }
                            }
                            else
                            {
                                //At this point, if he is the operator, then that means we couldn't find 
                                //the channel
                                std::string send_string = "Channel not found. \n";
                                received = send(client_fd, send_string.c_str(), send_string.size(),0);
                                if(received != send_string.size())
                                {
                                    perror("send failed, in KICK");
                                }
                            }
                        }

                    }
                    
                    
                    
                    //PRIVMSG
                    else if(strncmp(first.c_str(), "PRIVMSG", 7)==0){
                        std::string message = "";
                        if(second == ""){
                            message = "Invalid command\n";
                            received = send(client_fd, message.c_str(), message.size(), 0);
                            if(received != message.size()){
                                perror("send() failed in PRIVMSG");
                            }
                        }else{
                            std::string third = "";
                            std::stringstream t(origion);
                            std::string tempf, temps;
                            t>>tempf;
                            t>>temps;
                            std::string curr_word;
                            while(t>>curr_word){
                                third += curr_word+" ";
                            }
                            if(second[0] == '#'){
                                if(third != ""){
                                    if(third.size() <= 512){
                                        std::vector<Channel>::iterator it = std::find(curr_channels.begin(), curr_channels.end(), second);
                                        if(it != curr_channels.end()){
                                            message = second+"> "+names[client_fd]+": "+third+"\n";
                                            for(std::map<int, std::string>::iterator it2 = names.begin(); it2!= names.end(); it2++){
                                                if(it->user_in(it2->second)){
                                                    received = send(it2->first, message.c_str(), message.size(), 0);
                                                    if(received != message.size()){
                                                        perror("send() failed in PRIVMSG");
                                                    }
                                                }
                                                
                                            }
                                        }
                                    }
                                }
                                
                            }else{
                                if(third != ""){
                                    if(third.size() <= 512){
                                        message = names[client_fd]+": "+third+"\n";
                                        for(std::map<int, std::string>::iterator it = names.begin(); it != names.end(); it++){
                                            if(it->second == second){
                                                received = send(it->first, message.c_str(), message.size(), 0);
                                                if(received != message.size()){
                                                    perror("send() failed in PRIVMSG");
                                                }
                                            }
                                            
                                        }
                                    }
                                }
                                
                            }
                        }
                    }
                    
                    
                    
                    //QUIT
                    else if(strncmp(first.c_str(), "QUIT", 4)==0){
                        names.erase(client_fd);
                        close(client_fd);
                        for(int j = 0; j< curr_users.size(); j++){
                            curr_users.erase(curr_users.begin()+j);
                        }
                        for(int k = 0; k< curr_channels.size(); k++){
                            curr_channels[k].remove_user(names[client_fd]);
                        }
                        //here should be function that remove a client from list
                        remove_client(client_fd, cliend_i);
                        cliend_i--;
                        commands.erase(commands.begin()+i);
                    }
                    
                    
                    
                    
                }
                
                
            }
            
            
            
            
            
        }
        
        
        
        
    }
    
    
    close(sock);
    return EXIT_SUCCESS;
}

