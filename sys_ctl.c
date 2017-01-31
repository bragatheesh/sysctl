#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/inotify.h>
#include "sys_ctl.h"
#include "uthash.h"

command *hash_commands = NULL;

int
register_command(char* name, char* dir, int write, char* data, void* cb){
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
	c->id = NULL;
	c->cb = cb;
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

pthread_t
monitor_command_init(char* name){
	/*Here we will monitor a file to see if its value changes.
	If a value does change, we will call the specified callback function*/
	
	int ret = 0;
	command *c;

	HASH_FIND_STR(hash_commands, name, c);
	if(c == NULL){
		printf("Error: No such command %s\n", name);
		return -1;
	}
	
	ret = pthread_create(&(c->id), NULL, &monitor_command_execute, (void*)c);
	if(ret != 0){
		printf("Error creating thread for %s\n", c->name);
		return -1;
	}

	return c->id;
}


void*
monitor_command_execute(void* arg){
	/*Here we will monitor the specified file for any changes
	 Upon detecting a change, we will call the callback function*/
	command* c = (command*) arg;

	int length, i = 0;
      	int fd;
	int wd;
	int EVENT_SIZE = ( sizeof (struct inotify_event) );
	int EVENT_BUF_LEN = 1024 * (EVENT_SIZE + 16);
  	char buffer[EVENT_BUF_LEN];

        fd = inotify_init();
	if ( fd < 0 ) {
		printf("Error: failed to init inotify");
		pthread_exit(NULL);
	}

	wd = inotify_add_watch( fd, c->dir, IN_MODIFY);

	length = read( fd, buffer, EVENT_BUF_LEN );

	if(length < 0){
		printf("Error detecting change in file\n");
		pthread_exit(NULL);
	}
	else{
	/*call calback function*/
		printf("%s was modifed\n", c->name);
		if(c->cb != NULL)
			c->cb(c);
	}

	pthread_exit(NULL);

	return NULL;
}

int
monitor_command_wait(){
	/*Here we will wait for all threads started by monitor_command_execute to finish*/
	command *c;

        for(c = hash_commands; c != NULL; c= c->hh.next){
		if(c->id == NULL)
			continue;
		else{
			pthread_join(c->id, NULL);
			//printf("Thread: %s joined.\n", c->name);
		}
	}

	return 0;
}

void
callback_test(command *c){
	printf("Callback function of %s\n", c->name);
	return;
}

int
main(){
	char* name = "net.ipv4.icmp_echo_ignore_all";
	char* dir = "/proc/sys/net/ipv4/icmp_echo_ignore_all";
	int write = 1;
	char* data = "1\n";
	int ret;

	ret = register_command(name, dir, write, data,(void*)callback_test);
	if(ret < 0){
		printf("Error registering command %s\n",name);
	}
	

	ret = register_command("net.ipv4.tcp_window_scaling", "/proc/sys/net/ipv4/tcp_window_scaling", 0, "0", NULL);	
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

	ret = monitor_command_init(name);
	if(ret == NULL){
		printf("Error in monitor init for command: %s\n", name);
	}

        ret = monitor_command_init("net.ipv4.tcp_window_scaling");
        if(ret == NULL){
                printf("Error in monitor init for command: %s\n", name);
        }
	
	else{
		monitor_command_wait();
	}
	
	HASH_CLEAR(hh, hash_commands);

	return 0;
}
