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
      isoperator = false;
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
    Channel(std::string cname){
      users = std::list<User>();
      channel_name = cname;
    }

    std::string get_cname() const{
      return channel_name;
    }

    void add_user(std::string username){
      users.push_back(User(username));
    }

    int users_num(){
      return users.size();
    }

	bool remove_user(std::string username){
      std::list<User>::iterator it = std::find(users.begin(), users.end(), User(username));
      if(it!=users.end()){
        users.erase(it);
        return true;
      }


      return false;
    }
	
    bool user_in(std::string username){
      std::list<User>::iterator it = std::find(users.begin(), users.end(), User(username));
      if(it != users.end()){
        return true;
      }

      return false;
	}

    const std::list<User>& get_users() const{
      return users;
    }

    ~Channel(){

    }

  private:
    std::list<User> users;
    std::string channel_name;


};



//Commands or Classes to Implement:
/*
	Channel, USER, LIST, JOIN, PART, OPERATOR, KICK, PRIVMSG, QUIT
*/


int main(int argc, char* argv[])
{
	
	return EXIT_SUCCESS;
}
