/*
Alseny Sylla
Qianjun Chen
*/

#include <iostream>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <openssl/sha.h>
#include <sys/types.h>
#include <thread>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cmath>
#include <mutex>
#include <sstream>
#include <string.h>
#include <cassert>
#include <map>
#include <list>
#include <stdio.h>
#include <pthread.h>

#include "sha1.hpp"

//---------------------------------------------------------------------

unsigned int indexValue(int distance)
{
    return (log(distance)/log(2));
}


//Helper function for solving the XOR metric
static inline unsigned int value(char c)
{
    if (c >= '0' && c <= '9') { return c - '0';      }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
    return -1;
}

//this function returns the distance between two hexademical string 
//and return the distance between them in integer using xor
unsigned int str_xor(std::string const & s1, std::string const & s2)
{
    //assert(s1.length() == s2.length());

    static char const alphabet[] = "0123456789abcdef";

    std::string result;
    result.reserve(s1.length());

    for (std::size_t i = 0; i != s1.length(); ++i)
    { 
        unsigned int v = value(s1[i]) ^ value(s2[i]);

        assert(v < sizeof alphabet);

        result.push_back(alphabet[v]);
    }
    unsigned int x;   
    std::stringstream ss;
    ss << std::hex << result;
    ss >> x;
    return x;
}

// Node class for storing in the k bucket
class Node{
public:
    Node(char* in_name, int in_port, std::string in_id){
        name = in_name;
        port = in_port;
        id = in_id;
    }
    
    const char* get_name() const{
        return name;
    }

    const int get_port() const{
        return port;
    }
    
    const std::string get_id() const{
        return id;
    }

    unsigned int distance(std::string const & node2_id)
    {
        return str_xor(id, node2_id);
    }
    
    ~Node(){
        
    }
    
    
private:
    char* name;
    int port;
    std::string id;
};

bool operator==(const Node &first, const Node &second){
    if(first.get_id()== second.get_id()){
        if(first.get_port() == second.get_port()){
            if(first.get_id() == second.get_id()){
                return true;
            }
        }
    }
    return false;
}

// structure for the argument use in thread
struct nodestruct{
    char* name;
    int port;
    std::string id;
    int sock;
};


typedef std::map<int, std::list<Node> > bucketMap;
typedef bucketMap::const_iterator c_bItr;
typedef bucketMap::iterator bItr;
typedef std::map<int,int> dataMap;
typedef dataMap::iterator d_itr;

// global variable that store all the information and k bucket
std::map<int, std::list<Node> > k_buckets;
std::map<std::string, int> data;
int value_k;

// set up address detailed
void setServerDetails(struct sockaddr_in &server, std::string ip, int port)
{
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip.c_str());
    server.sin_port = htons(port);
}

// set up duration
void setDuration(struct timeval &timer)
{
    timer.tv_sec = 3;
    timer.tv_usec = 0;
}


// see if a peer is still connected
bool isLoggedIn(std::string ip, int port)
{   
    struct sockaddr_in NodeToConnectTo;
    socklen_t l = sizeof(NodeToConnectTo);

    //let's set the socket info for that node
    setServerDetails(NodeToConnectTo, ip, port);

    //set timer for socket
    struct timeval timer;
    setDuration(timer);

    //create a UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM,0);

    if(sock < 0)
    {
        perror("Socket failed in isLoggedIn Function\n");
        exit(-1);
    }

    //let's set the timer we created for this socket
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,(char*)&timer, sizeof(struct timeval));

    char msg[] = "isLoggedIn";

    sendto(sock, msg, strlen(msg),0,(struct sockaddr*)&NodeToConnectTo,l);

    char resp[5];

    int len = recvfrom(sock, resp, 2, 0, (struct sockaddr*)&NodeToConnectTo, &l);

    close(sock);

    if(len >= 0)
        return true;
    else
        return false;
}

// function that listen on request from others
void *listenOthers(void * nself){
    
    // get the ip port and id of self
    struct nodestruct *node = (struct nodestruct *) nself;
    char* in_ip = node->name;
    int in_port = node->port;
    std::string in_id = node->id;
    Node n_self(in_ip, in_port, in_id);
    int sock = node->sock;
    
    char buff[1024];
    int recv;
    while(true){
        // store address info of others
        struct sockaddr_in RecvAddr;
        socklen_t recvaddr_len = sizeof(RecvAddr);
        recv = recvfrom (sock, buff, 1024, 0, (struct sockaddr *) &RecvAddr, &recvaddr_len);
    	if (recv == 0){
    		perror("No messages are available");
    		exit(0);
    	}
        
        char re_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(RecvAddr.sin_addr), re_ip, INET_ADDRSTRLEN);
        int re_port = RecvAddr.sin_port;

        buff[recv] = '\0';
        const char sep[2] = " ";
        std::string mess(buff);
        std::cout<< mess<<std::endl;
        
        // parse buffer get request
        char *token = strtok(buff, sep);
        if(token != NULL){
            if(token[strlen(token)-1]=='\n'){
                token[strlen(token)-1] = '\0';
            }
        }
        std::string temp(token);
        std::string first = temp;

        if( first == "HELLO"){
            // obtain other arguments
            std::string send_name = "";
            std::string send_id = "";
            if((token = strtok(NULL, sep)) != NULL){
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1]='\0';
                }
                std::string temp(token);
                send_name = temp;
            }

            if((token = strtok(NULL, sep)) != NULL){
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1]='\0';
                }
                std::string temp(token);
                send_id = temp;
            }
            
            
            // send this message back
            std::string sendm = "MYID " + in_id+"\n";
            int err = sendto(sock, sendm.c_str(), sizeof(sendm), 0, (struct sockaddr *) &RecvAddr, sizeof(RecvAddr));
            if(err == 0){
                perror("fail in sendto");
                exit(0);
            }
            
            // store node
            //put into correct k bucket
            unsigned int distance = n_self.distance(send_id);
            //use xor function get the distance
            //calculate i
            int i=indexValue(distance);
            // put into k bucket
            std::map<int, std::list<Node> >::iterator it;
            it = k_buckets.find(i);
            std::list<Node> temp;
            Node newnode(re_ip, re_port, send_id);
            if (it != k_buckets.end()){
                temp = k_buckets[i];
                temp.push_front(newnode);
                k_buckets.insert(std::make_pair(i,temp));
            }else{
                temp.push_front(newnode);
                k_buckets.insert(std::make_pair(i,temp));
            }

            
            
        }else if(first == "MYID"){
            std::string tar_id;
            if((token = strtok(NULL, sep)) != NULL){
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1]='\0';
                }
                std::string temp(token);
                tar_id = temp;
            }
            
            //put into correct k bucket
            int distance =n_self.distance(tar_id);
            
            //calculate i
            int i = indexValue(distance);

            // put into k bucket
            std::map<int, std::list<Node> >::iterator it;
            it = k_buckets.find(i);
            std::list<Node> temp;
            Node newnode(re_ip, re_port, tar_id);
            if (it != k_buckets.end()){
                temp = k_buckets[i];
                temp.push_front(newnode);
                k_buckets.insert(std::make_pair(i,temp));
            }else{
                temp.push_front(newnode);
                k_buckets.insert(std::make_pair(i,temp));
            }

        }else if(first == "FIND_NODE"){
            int found = 0;     // flag for terminate or continue the loop
            std::string tar_id = "";
            std::string t = "";
            if((token = strtok(NULL, sep)) != NULL){
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1]='\0';
                }
                std::string temp(token);
                tar_id = temp;
            }

            // xor function to calculate the distance
            unsigned int distance = n_self.distance(tar_id);
            std::string someip=std::string(in_ip);
            std::string message = "NODE "+someip+" "+std::to_string(in_port)+" "+in_id+"\n";
            if(distance == 0){
                int err = sendto(sock, message.c_str(),sizeof(message) , 0, (struct sockaddr*) &RecvAddr, sizeof (RecvAddr));
                if(err == 0){
                    perror("failed in send");
                    exit(0);
                }
                found = 1;
            }
            
            
            // find correct i value for k bucket
            int i = indexValue(distance);
            
            
            // access correct k bucket
            std::map<int, std::list<Node> >::iterator it;
            it = k_buckets.find(i);
            std::list<Node> temp;
            if (found == 0){
                if(it != k_buckets.end()){
                   temp = k_buckets[i]; 
                }
                std::list<Node>::iterator n_it;
                
                // loop that loop through nodes
                for(n_it =  temp.begin(); n_it != temp.end(); n_it++){
                    if (n_it->get_id() ==  tar_id){
                        someip=std::string(n_it->get_name());
                        std::string message = "NODE "+ someip+" "+std::to_string(n_it->get_port())+" "+tar_id+"\n";
                        int err = sendto(sock, message.c_str(),sizeof(message) , 0, (struct sockaddr*) &RecvAddr, sizeof (RecvAddr));
                        if(err == 0){
                            perror("failed in send");
                            exit(0);
                        }
                        n_it = temp.end();
                        found = 1;
                    }
                }
                // if not found, loop through and send k closest nodes back
                if(found == 0){
                    int count;
                    n_it =  temp.begin();
                    for(count = 0; count < value_k; count++){
                        if(n_it == temp.end()&& it == k_buckets.end()){
                            it = k_buckets.begin();
                            n_it = it->second.begin();
                        }
                        if(n_it == temp.end()&& it != k_buckets.end()){
                            it++;
                            n_it = it->second.begin();
                        }
                        someip=std::string(n_it->get_name());
                        std::string message = "NODE "+ someip+" "+ std::to_string(n_it->get_port())+" "+n_it->get_id()+"\n";
                        int err = sendto(sock, message.c_str(),sizeof(message) , 0, (struct sockaddr*) &RecvAddr, sizeof (RecvAddr));
                        if(err == 0){
                            perror("failed in send");
                            exit(0);
                        }
                        n_it++;
                    }
                
                }
            }
            
        }else if (first == "FIND_DATA"){
            std::string key = "";
            std::string t = "";
            if((token = strtok(NULL, sep)) != NULL){
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1]='\0';
                }
                std::string temp(token);
                key = temp;
            }
            
            //find locally first
            std::map<std::string, int>::iterator it = data.find(key);
            if(it != data.end()){
                std::string message = "VALUE "+ in_id+" "+key+" "+std::to_string(data[key])+"\n";
                int err = sendto(sock, message.c_str(),sizeof(message) , 0, (struct sockaddr*) &RecvAddr, sizeof (RecvAddr));
                    if(err == 0){
                        perror("failed in send");
                        exit(0);
                    }
            }else{
                 // xor function to calculate the distance
                int distance =n_self.distance(key);
                
                // find correct i value for k bucket
                int i = indexValue(distance);
                
                // access correct k bucket
                std::map<int, std::list<Node> >::iterator it;
                it = k_buckets.find(i);
                std::list<Node> temp;
                
                temp = k_buckets[i];
                std::list<Node>::iterator n_it;
                
                // loop through and send back k closest nodes
                int count;
                n_it =  temp.begin();
                for(count = 0; count < value_k; count++){
                   if(n_it == temp.end()&& it == k_buckets.end()){
                        it = k_buckets.begin();
                        n_it = it->second.begin();
                    }
                    if(n_it == temp.end()&& it != k_buckets.end()){
                        it++;
                        n_it = it->second.begin();
                    }
                    std::string someip = std::string(n_it->get_name());
                    std::string message = "NODE "+ someip+" "+ std::to_string(n_it->get_port())+" "+n_it->get_id()+"\n";
                    int err = sendto(sock, message.c_str(),sizeof(message) , 0, (struct sockaddr*) &RecvAddr, sizeof (RecvAddr));
                    if(err == 0){
                        perror("failed in send");
                        exit(0);
                    }
                    n_it++;
                }
                
            }    
            
           
            

        }else if (first == "STORE"){
            // obtain key and value first
            std::string key = 0;
            std::string t = "";
            if((token = strtok(NULL, sep)) != NULL){
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1]='\0';
                }
                std::string temp(token);
                key = temp;
            }
            
            int value = 0;
            if((token = strtok(NULL, sep)) != NULL){
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1]='\0';
                }
                std::string temp(token);
                t = temp;
            }
            value = std::stoi(t);
            
            // store the value inside our data
            data[key] = value;
            
        }else if(first == "QUIT"){
            // obtain id
            std::string quit_id = "";
            if((token = strtok(NULL, sep)) != NULL){
                if(token[strlen(token)-1] == '\n'){
                    token[strlen(token)-1]='\0';
                }
                std::string temp(token);
                quit_id = temp;
            }

            // use xor function to find k bucket

            unsigned int distance = n_self.distance(quit_id);
            int i = indexValue(distance);
            
            // remove the it from the k bucket
            Node n_remove(re_ip, re_port, quit_id);
            std::list<Node> temp = k_buckets[i];
            temp.remove(n_remove);
        }else{
            std::cout << "wrong request"<<std::endl;
        }
        
        
        
    }
}

// function that return the closest node in the list to the target node
Node closestNode(std::list<Node>& mylist, Node ob)
{
    int min = 1000;
    Node target("",0, "");

    for(std::list<Node>::iterator itr =mylist.begin(); itr != mylist.end(); ++itr)
    {
        int difference = ob.distance(itr->get_id());
        
        //int difference = std::abs();
        if(difference < min && difference < 1000 )
        {
            min = difference;
            target = *itr;
        }
    }
    return target;
}

//this function splits a node into a vector
std::vector<std::string> split(std::string str, std::string token){
    std::vector<std::string>result;
    while(str.size()){
        unsigned int index = str.find(token);
        if(index!=std::string::npos){
            result.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)result.push_back(str);
        }else{
            result.push_back(str);
            str = "";
        }
    }
    return result;
}

void *takeCommand(void * nself){
    struct nodestruct *node = (struct nodestruct *) nself;
    char* in_ip = node->name;
    int in_port = node->port;
    std::string in_id = node->id;
    Node n_self(in_ip, in_port, in_id);
    int fd = node->sock;

    //NOw let's start getting input from command

    while(true)
    {
        std::string command;
        std::cin >> command;


        if(command == "CONNECT")
        {
            std::string ip_addr;
            std::string port_number;

            std::cin >> ip_addr;
            std::cin >> port_number;

            int portNumber = std::stoi(port_number);
            

            //Let's check that the node we are trying to connect to 
            // is still in the connected (responding)
            //if(isLoggedIn(ip_addr, portNumber )== true)
            //{
               
                //Let's set the socket details for this node
                struct sockaddr_in NodeToConnectTo;

                socklen_t l = sizeof(NodeToConnectTo);

                setServerDetails(NodeToConnectTo, ip_addr, portNumber);

                //let's create a message that store the send name and send nodeID
                std::string s = std::to_string(n_self.get_port());
                std::string tem = std::string("HELLO")+ " " +n_self.get_name()+" "+ s;

                char* msg = const_cast<char*>(tem.c_str());

                if(sendto(fd, msg, strlen(msg), 0, (struct sockaddr*) &NodeToConnectTo, l)==-1)
                {
                    std::cout << "lol sento didnt work for connect" << std::endl;
                    perror("lol error: sendto message in connect");
                    exit(-1);
                }
                //close(sock);
            //}
            /*else
            {
                std::cout << "No active node on this ip and port" << std::endl;
                
            }*/



        }

         else if(command == "FIND_NODE")
        {
            std::string targetNodeId;

            //Store the id of the node we are searching in variable targetNodeId
            std::cin >> targetNodeId;

            //get the distance in order to i (bucket) the node is stored in. 
            unsigned int distance = n_self.distance(targetNodeId);

            //find the correct bucket (i) the node is stored in. 
            int nodeLocation_i = indexValue(distance);

            /*Note: variable nodeLocaiton_i represents the key in the bucketmap.*/

            //let's find the key (the bucket) using map iterator
            bItr itr = k_buckets.find(nodeLocation_i);

             

            //now let's send a message to the closest node id
            if(itr!= k_buckets.end())
            {
                //this means it has found the bucket 

                //closestNode is a helper that returns the closest node
                //in a specific bucket
                std::list<Node> tempBucket = itr->second;

                for(unsigned int j =0; j < value_k; j++)
                {

                
                    Node myClosestNode = closestNode(tempBucket, n_self);
                    tempBucket.remove(myClosestNode);

                    //Let's send this node FIND_NODE <node ID>

                    struct sockaddr_in NodeToConnectTo;
                    socklen_t l = sizeof(NodeToConnectTo);
                    setServerDetails(NodeToConnectTo, myClosestNode.get_name(), myClosestNode.get_port());
                    
                    std::string message = std::string("FIND_NODE") + " " + targetNodeId;
                    char* msg = const_cast<char*>(message.c_str());
                    if(sendto(fd, msg, strlen(msg), 0, (struct sockaddr*) &NodeToConnectTo, l)==-1)
                    {
                        std::cout << "lol sento didnt work" << std::endl;
                        perror("lol error: sendto ");
                        exit(-1);
                    }


                    //socket is still open
                    for(unsigned int i = 0; i < value_k; i++)
                    {

                        /*this node will send either one node and k nodes*/
                        char responseMessage[1024];
                        int len;
                        if((len= recvfrom(fd, responseMessage, 1024,0,(struct sockaddr*)&NodeToConnectTo, &l))== -1)
                        {
                            std::cout << "the message to send in findnode failed" << std::endl;
                            perror("error");
                            exit(-1);
                        }

                        responseMessage[len] = '\0';

                        std::string entry = responseMessage;
                        //entry will be in the form " NODE <remote node name> <remote port> <remote ID>"
                        std::vector<std::string> responseV = split(entry," ");

                        //check if the returned node is the correct one
                        if(responseV[3]== targetNodeId && i==0)
                        {
                            std::cout << "found the target node" << std::endl;
                            //close(sock);
                            break;
                        }
                        else
                        {
                            int port_num = std::stoi(responseV[2]);
                            char* tempad = const_cast<char*>(responseV[1].c_str());
                            Node tempNode(tempad,port_num,responseV[3]);
                            tempBucket.push_back(tempNode);
                        }


                    }

                }




            }




        }

       

       else if(command == "FIND_DATA")
        {
            std::string key;

            std::cin >> key;
           




            //get the distance in order to i (bucket) the node is stored in. 
            unsigned int distance = n_self.distance(key);

            //find the correct bucket (i) the node is stored in. 
            int nodeLocation_i = indexValue(distance);

            /*Note: variable nodeLocaiton_i represents the key in the bucketmap.*/

            //let's find the key (the bucket) using map iterator
            bItr itr = k_buckets.find(nodeLocation_i);

             

            //now let's send a message to the closest node id
            if(itr!= k_buckets.end())
            {
                //this means it has found the bucket 

                //closestNode is a helper that returns the closest node
                //in a specific bucket
                std::list<Node> tempBucket = itr->second;

                for(unsigned int j =0; j < value_k; j++)
                {

                    int min = 1000;

                    Node myClosestNode("",0, "");

                    for(std::list<Node>::iterator itr =tempBucket.begin(); itr != tempBucket.end(); ++itr)
                    {
                        int difference = itr->distance(key);
                        if(difference < min && difference < 1000 )
                        {
                            min = difference;
                            myClosestNode = *itr;
                        }
                    }

                    //you are already visited this node. remove it now

                    tempBucket.remove(myClosestNode);

                    //Let's send this node FIND_NODE <node ID>
                    struct sockaddr_in NodeToConnectTo;
                    socklen_t l = sizeof(NodeToConnectTo);

                    setServerDetails(NodeToConnectTo, myClosestNode.get_name(), myClosestNode.get_port());
                    
                    std::string message = std::string("FIND_DATA") + " " + key;
                    char* msg = const_cast<char*>(message.c_str());
                    if(sendto(fd, msg, strlen(msg), 0, (struct sockaddr*) &NodeToConnectTo, l)==-1)
                    {
                        std::cout << "lol sento didnt work" << std::endl;
                        perror("lol error: sendto ");
                        exit(-1);
                    }


                    //socket is still open
                    for(unsigned int i = 0; i < value_k; i++)
                    {
                        
                        /*this node will send either one node and k nodes*/
                        char responseMessage[1024];
                        int len;
                        if((len= recvfrom(fd, responseMessage, 1024,0,(struct sockaddr*)&NodeToConnectTo, &l))== -1)
                        {
                            std::cout << "the message to send in findKeye failed" << std::endl;
                            perror("error");
                            exit(-1);
                        }

                        responseMessage[len] = '\0';

                        std::string entry = responseMessage;
                        //entry will be in the form " NODE <remote node name> <remote port> <remote ID>"
                        std::vector<std::string> responseV = split(entry," ");

                        //check if the returned node is the correct one
                        if(responseV[0]== "VALUE")
                        {
                            std::cout << "received " << responseV[3] << " from " << myClosestNode.get_id() << std::endl;
                            //close(sock);
                            break;
                        }
                        else
                        {
                            int port_num = std::stoi(responseV[2]);
                            char* tempad = const_cast<char*>(responseV[1].c_str());
                            Node tempNode(tempad,port_num,responseV[3]);
                            tempBucket.push_back(tempNode);
                        }


                    }

                }




            }



        }
        else if(command == "STORE")
        {
            std::string key;
            int value;
            std::cin >> key;
            std::cin >> value;

            

             //get the distance in order to i (bucket) the node is stored in. 
            unsigned int distance = n_self.distance(key);

            //find the correct bucket (i) the node is stored in. 
            int nodeLocation_i = indexValue(distance);

            /*Note: variable nodeLocaiton_i represents the key in the bucketmap.*/

            //let's find the key (the bucket) using map iterator
            bItr itr = k_buckets.find(nodeLocation_i);

             

            //now let's send a message to the closest node id
            if(itr!= k_buckets.end())
            {
                //this means it has found the bucket 

                //closestNode is a helper that returns the closest node
                //in a specific bucket
                std::list<Node> tempBucket = itr->second;

                
                

                int min = 1000;
                // find the closest node
                Node myClosestNode("",0, "");
                
                for(std::list<Node>::iterator itr =tempBucket.begin(); itr != tempBucket.end(); ++itr)
                {
                    int difference = itr->distance(key);
                    if(difference < min && difference < 1000 )
                    {
                        min = difference;
                        myClosestNode = *itr;
                    }

                }
                
                // set up address info of receiver
                struct sockaddr_in NodeToConnectTo;
                socklen_t l = sizeof(NodeToConnectTo);

                setServerDetails(NodeToConnectTo, myClosestNode.get_name(), myClosestNode.get_port());
                    
                // send store request to the receiver
                std::string message = std::string("STORE") + " " + key + std::to_string(value);
                char* msg = const_cast<char*>(message.c_str());
                if(sendto(fd, msg, strlen(msg), 0, (struct sockaddr*) &NodeToConnectTo, l)==-1)
                {
                    std::cout << "lol sento didnt work" << std::endl;
                    perror("lol error: sendto ");
                    exit(-1);
                }


            }

        }
       
        else if(command == "QUIT")
        {
            //this node should send a message to all its nodes in the kbuckets
            
            for(c_bItr itr = k_buckets.begin(); itr!= k_buckets.end(); ++itr)
            {
                std::list<Node> mValues = itr->second;
                std::list<Node>::const_iterator v_itr;
                for(v_itr = mValues.begin(); v_itr!= mValues.end(); v_itr++)
                {
                    if(isLoggedIn(v_itr->get_name(), v_itr->get_port())==true)
                    {
                        
                        // set up address info of receiver
                        struct sockaddr_in NodeToConnectTo;
                        socklen_t l = sizeof(NodeToConnectTo);

                        setServerDetails(NodeToConnectTo, v_itr->get_name(), v_itr->get_port());

                        std::string s = std::to_string(n_self.get_port());
                        std::string tem = std::string("QUIT")+ " " +n_self.get_id();

                        char* msg = const_cast<char*>(tem.c_str());
                        
                        // sent quit notification to all connected nodes
                        if(sendto(fd, msg, strlen(msg), 0, (struct sockaddr*) &NodeToConnectTo, l)==-1)
                        {
                            std::cout << "lol sento didnt work for connect" << std::endl;
                            perror("lol error: sendto message in connect");
                            exit(-1);
                        }
                        close(fd);

                    }
                }
            }

        }



        else
        {
            std::string invalid_command = "Invalid command.\n";
            std::cerr << invalid_command << std::endl;
        }

    }   

}


int main(int argc, char* argv[]){
    if(argc != 5){
        std::cerr << "Not enough arguments\n";
        return 1;
    }
    pthread_t threads[2];
    char* in_ip = argv[1];
    std::string in_seed = argv[3];
    value_k = std::stoi(argv[4]);
    
    class SHA1 ob;

    std::string in_id = ob.nodeId_str(in_seed);
    
    
    int in_port = 0;
    in_port = std::stoi(argv[2]);
    
    struct nodestruct nself ;
    nself.id = in_id;
    nself.name = in_ip;
    nself.port = in_port;
    
    int sock;
    struct sockaddr_in saddr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sock < 0){
        perror("socket failed");
        exit(EXIT_FAILURE); 
    }
    saddr.sin_addr.s_addr = inet_addr(in_ip);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(in_port);
    if( bind(sock, (struct sockaddr *)&saddr, sizeof(saddr))< 0){
        perror("binding failed");
        exit(EXIT_FAILURE);
    }
    nself.sock = sock;
    
    
    int rc = pthread_create(&threads[0], NULL, &listenOthers, (void*)&nself);
    if (rc) {
        std::cout << "Error:unable to create thread," << rc << std::endl;
        exit(-1);
    }
    
    pthread_create(&threads[1], NULL, &takeCommand, (void*)&nself);
    if (rc) {
        std::cout << "Error:unable to create thread," << rc << std::endl;
        exit(-1);
    }
    
    pthread_exit(NULL);
    return 0;
    

}