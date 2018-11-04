/*
 Assignment 3
 Team members:
	Sylla, Alseny
	Qianjun Chen 
*/
#include <poll.h>
#include <sys/uio.h>
#include <unistd.h>
#include   <netdb.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
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
#include <sys/select.h>      /* <===== */

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




//Global variable to store all the channels in a list
std::vector<Channel> curr_channels;


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
  unsigned short port = 8128;

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

	
	return EXIT_SUCCESS;
}

