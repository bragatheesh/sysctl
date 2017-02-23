#include <malloc_heap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/inotify.h>
#include "sys_ctl.h"
#include "uthash.h"
#include <rte_hash.h>
#include <rte_common.h>
#include <rte_vect.h>
#include <rte_byteorder.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_string_fns.h>
#include <rte_cpuflags.h>

//struct command *hash_commands = NULL;

int
register_command(char* name, void* cb, struct flex_ctl* flex){
    /*here we will register the command and place in the flexctl hash*/
    int ret;
    struct command *c;
    size_t cmd_len;

    FILE* fp;
    long lsize;

    if(strlen(name) > (16)){
        printf("Error: name %s must be less than 16 chars\n", name);
        return -1;
    }

    /*ret = rte_hash_lookup_data(flex->hash, (const void*) name, (void**)c); //prev &s
      if(c != NULL){
      printf("Error: command %s has already been registered.\n", name);
      return -1;
      }*/


    HASH_FIND_STR(flex->hash_commands, name, c);  //Check if command is already present in hash
    if (c != NULL) {
        printf("Error: command %s has already been registered.\n", name);
        return -1;
    }

    c = malloc(sizeof(struct command));

    c->name = name;
    cmd_len = strlen(name) + strlen(flex->dir) + 2;
    c->dir = malloc(sizeof(char) *(strlen(name) + strlen(flex->dir) + 2));
    snprintf(c->dir, cmd_len,"%s/%s",flex->dir, name);
    c->id = 0;
    c->cb = cb;
    c->data = "No Data";
    HASH_ADD_KEYPTR(hh, flex->hash_commands, c->name, strlen(c->name), c);


    //open up flexpath.ctl and write register (command name) (file to point to)
    fp = fopen("/flexpath.ctl", "w+");
    if(fp < 0){
        printf("Error opening /flexpath.ctl\n");
        return -1;
    }

    if(fprintf(fp, "REGISTER,%s,%s", c->name, c->dir) < 0){
        printf("Error writing to /flexpath.ctl\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);

    return 0;
}


int
list_command(struct flex_ctl* flex){
    /*here we can list all the commands we have registered*/

    struct command *c;

    for(c = flex->hash_commands; c != NULL; c= c->hh.next){
        printf("name: %s, dir: %s, data: %s\n", c->name, c->dir, c->data);
    }	

    return 0;
}



void*
show(struct flex_ctl* flex, char* cmd_name){
    struct command *c;
    FILE* fp;     
    char* buffer;
    long size;
    HASH_FIND_STR(flex->hash_commands, cmd_name, c);         
    if(c == NULL){
        printf("Error: No such command %s\n", cmd_name);
        return NULL;
    }

    fp = fopen("/flexpath.ctl", "w+");
    if(fp < 0){
        printf("Error opening /flexpath.ctl\n");
        return -1;
    }

    if(fprintf(fp, "SHOW,%s", c->name) < 0){
        printf("Error writing to /flexpath.ctl\n");
        fclose(fp);
        return -1;
    }

    sleep(3);
    //check file again fro what flextctl wrote

    fclose(fp);

    return (void*)buffer;
}

int
set(struct flex_ctl* flex, char* cmd_name, int data){
    struct command *c;
    FILE* fp;     

    HASH_FIND_STR(flex->hash_commands, cmd_name, c);
    if(c == NULL){                         
        printf("Error: No such command %s\n", cmd_name);
        return -1;
    }

    fp = fopen(c->dir, "w");
    //write file code here
    fclose(fp);
}













/*
   int
   execute_command(char* name){
/*here we can either read or write to the dir*

int ret = 0;
struct command *c;
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
If a value does change, we will call the specified callback function*

int ret = 0;
struct command *c;

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
Upon detecting a change, we will call the callback function*
struct command* c = (struct command*) arg;

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
/*call calback function*
printf("%s was modifed\n", c->name);
if(c->cb != NULL)
c->cb(c);
}

pthread_exit(NULL);

return NULL;
}

int
monitor_command_wait(){
/*Here we will wait for all threads started by monitor_command_execute to finish*
struct command *c;

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
*/
void
callback_test(struct command *c){
    printf("Callback function of %s\n", c->name);
    return;
}

int
main(){
    char* cmd_name = "command1";
    void* cb = (void*)callback_test;

    char my_name[16];
    int ret;
    struct rte_hash* hash;

    struct flex_ctl* flex = malloc(sizeof(struct flex_ctl));


    struct rte_hash_parameters hash_params = {
        .name = NULL,
        .entries = (1024 * 1024 * 1),
        .key_len = sizeof(my_name),
        .hash_func_init_val = 0,
    };



    struct rte_hash_parameters hash_params_old = {
        .name = "temp",
        .entries = 9,
        .hash_func_init_val = 0,
        .key_len = (sizeof(char) * 16),
        .socket_id = 0
    };

    if(!rte_eal_has_hugepages()){
        printf("NO HUGEPAGES\n\n\n\n");
        return 0;
    }

    printf("numa nodes: %d MAX NODE: %d\n",malloc_get_numa_socket(),RTE_MAX_NUMA_NODES);

    hash_params.name = cmd_name;

    //hash = rte_hash_create(&hash_params);
    /*if(hash == NULL){
      printf("Error creating hash table for %s\n", cmd_name);
      return -1;
      }*/

    //flex->hash = hash;
    flex->hash_commands = NULL;
    flex->desc = "Manages all flexswitch directories";
    flex->base_dir = "/flex_ctl_";
    flex->PID = "0"; //temp for test
    flex->dir = malloc(strlen(flex->base_dir) + strlen(flex->PID) + 1);
    strcpy(flex->dir, flex->base_dir);
    strcat(flex->dir, flex->PID);
    flex->num_cmds = 0;


    ret = register_command(cmd_name, cb, flex);
    if(ret < 0){
        printf("Error registering command %s\n",cmd_name);
    }


    /*ret = register_command("net.ipv4.tcp_window_scaling", "/proc/sys/net/ipv4/tcp_window_scaling", 0, "0", NULL, hash);	
      if(ret < 0){
      printf("Error registering command %s\n",name);
      }*/
    list_command(flex);
    rte_hash_free(hash);	
    HASH_CLEAR(hh, flex->hash_commands);
    return 0;
}
