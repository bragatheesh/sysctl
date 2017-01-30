#include <stdio.h>
#include <stdlib.h>
#include "uthash.h"

typedef struct command{
	char* name;		//name of our command, also used as key for our hashtable
	char* dir;		//holds the absolute directory
	int write;		//boolean, 1 for write, 0 for read
	char* data;		//holds the read data or the data that will be written
	int sock;		//holds the socket to the dir (not_used)
	FILE* fp;		//holds file pointer to the file
	UT_hash_handle hh;	//makes this structure hashable
}command;

command *hash_commands = NULL;

int
register_command(char* name, char* dir, int write, char* data){
	/*here we will register the key and initialize a socket to 
	the respective directory*/
	
	command *c;

	HASH_FIND_STR(hash_commands, name, c);  //Check if command is already present in hash
	if (c != NULL) {
		printf("Error, command %s has already been registered.\n", name);
		return -1;
	}

	c = malloc(sizeof(command));

	c->name = name;
	c->dir = dir;
	c->write = write;
	c->data = data;

	HASH_ADD_KEYPTR(hh, hash_commands, c->name, strlen(c->name), c);
	
	return 0;
}

int
list_commands(){
	/*here we can list all the commands we have registered*/
	return 0;
}

int
execute_command(){
	/*here we can either read or write to the dir*/
	return 0;
}



int
main(){
	char* name = "net.ipv4.icmp_echo_ignore_all";
	char* dir = "/proc/sys/net/ipv4/icmp_echo_ignore_all";
	int write = 0;
	char* data = "";
	int ret;

	ret = register_command(name, dir, write, data);
	if(ret < 0){
		printf("Error registering command %s\n",name);
	}
	

	ret = register_command(name, dir, write, data);	
	if(ret < 0){
		printf("Error registering command %s\n",name);
	}

	unsigned int num_users;
	num_users = HASH_COUNT(hash_commands);
	printf("there are %u users\n", num_users);
	

	HASH_CLEAR(hh, hash_commands);

	return 0;
}
