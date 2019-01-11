
Assignment: 3

Team Members: 
	Sylla, Alseny (syllaa)-syllaa@rpi.edu
	Qianjun Chen (Chenq8)-chenq8@rpi.edu

Once a client connects to our server, they should tell us their name. If the command is not correct, they will be disconnected right away. If the command format is not correct, they will be messaged as well. If the name is not correct format, they will be notified as well. 

LIST command allows the users know all the channels. If the channel is specified, all the users in that channel will be listed

JOIN command allows the users to join a group. if that group does not exist, it will be created

PART command allows the users to disconnect from groups. if the group name is not specified, all the groups that the user is in will be disconnected. 

OPERATOR command allows the user to be the operator if the user put in the correct password. 

KICK command is only for operator. The user will be first check whether that is an operator. If is, the specified user will be removed from the specified group. 

PRIVMSG is the command that allows user to talk to certain person or certain group. If the message is empty, nothing will be sent out. 

Both ctrl+c and QUIT command can disconnect a user safely. Everything about that user will be removed. 

* During testing, the program compile and run normally on mac terminal. No matter we use clang++ or g++, there is no error or warning at all. 

*But on a windows computer, if we use g++, it compile and run normally but with warning. If we use clang++, there will be errors. 

** we also implement for extra credit for supporting both ipv4 and ipv6. 


