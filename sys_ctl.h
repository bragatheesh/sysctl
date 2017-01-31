#ifndef __SYSCTL_H__
#define __SYSCTL_H__

#include <stdio.h>
#include <stdlib.h>
#include "uthash.h"
#include <fcntl.h>

typedef struct command{
	char* name;	//name of our command, also used as a key for our hashtable
	char* dir;		//holds the absolute directory
	int write;		//boolean, 1 for write, 0 for read
	char* data;		//holds the data that will be written
	int sock;		//holds the socket to the file dir (not used)
	FILE* fp;		//holds file pointer to the file
	void* cb;		//holds pointer to callback function
	UT_hash_handle hh;	//makes this structure hashable
}command;

int
register_command(char*, char*, int, char*);

int
list_command();

int
execute_command(char*);

int
monitor_command(char*);


#endif /*__SYSCTL_H__ */
