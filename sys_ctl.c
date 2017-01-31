#include <stdio.h>
#include <stdlib.h>
#include "uthash.h"
#include "sys_ctl.h"
#include <fcntl.h>

/*typedef struct command{
	char* name;		//name of our command, also used as key for our hashtable
	char* dir;		//holds the absolute directory
	int write;		//boolean, 1 for write, 0 for read
	char* data;		//holds the read data or the data that will be written
	int sock;		//holds the socket to the dir (not_used)
	FILE* fp;		//holds file pointer to the file
	void* cb;		//hold pointer to callback function
	UT_hash_handle hh;	//makes this structure hashable
}command;*/

command *hash_commands = NULL;

int
register_command(char* name, char* dir, int write, char* data){
	/*here we will register the key and initialize a socket to 
	the respective directory*/
	
	command *c;

	HASH_FIND_STR(hash_commands, name, c);  //Check if command is already present in hash
	if (c != NULL) {
		printf("Error: command %s has already been registered.\n", name);
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
list_command(){
	/*here we can list all the commands we have registered*/
	
	command *c;

	for(c = hash_commands; c != NULL; c= c->hh.next){
		printf("name: %s, dir: %s, write: %d, data: %s\n", c->name, c->dir, c->write, c->data);
	}	
	
	return 0;
}

int
execute_command(char* name){
	/*here we can either read or write to the dir*/
	
	int ret = 0;
	command *c;
	char line[1];
	
	HASH_FIND_STR(hash_commands, name, c);
	
	if(c == NULL){
		printf("Error: No such command %s\n", name);
		return -1;
	}

	if(c->write == 0){ //read
		c->fp = fopen(c->dir, "r");
		if(c->fp == NULL){
			printf("Error opening file to read: %s\n", c->dir);
			return -1;
		}

		while(fread(line, sizeof(line), 1, c->fp) == 1){
			printf("%s", line);
		}
	
		fclose(c->fp);
	}

	else{ //write
		c->fp = fopen(c->dir, "w");
		if(c->fp == NULL){
			printf("Error opening file to write: %s\n", c->dir);
			return -1;
		}

		ret = fwrite(c->data, 1, sizeof(c->data), c->fp);
		if(ret != sizeof(c->data)){
			printf("Error: size of data written does not match size of data\n");
		}

	}

	return 0;
}

int
monitor_command(char* name){
	/*Here we will monitor a file to see if its value changes.
	If a value does change, we will call the specified callback function*/
	
	return 0;
}

int
main(){
	char* name = "net.ipv4.icmp_echo_ignore_all";
	char* dir = "/proc/sys/net/ipv4/icmp_echo_ignore_all";
	int write = 1;
	char* data = "1\n";
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
	printf("there are %u commands\n", num_users);

	list_command();	

	ret = execute_command(name);
	if(ret < 0){
		printf("Error executing command: %s\n", name);
	}
	
	HASH_CLEAR(hh, hash_commands);

	return 0;
}
