#ifndef __FLEXCTL_H__
#define __FLEXCTL_H__

#include "uthash.h"

struct command{
    char* name;             //name of our command, also used as a key for our hashtable
    char* dir;              //name of the file we will be modifying along with absolute path from struct flexctl dir
    char* data;             //holds the data that will be written (maybe not required?)
    FILE* fp;               //holds file pointer to the file
    void (*cb)(void *);     //holds pointer to callback function on trigger of file change
    pthread_t id;           //holds pthread id for this command 
    UT_hash_handle hh;
};

struct flex_ctl{
    struct rte_hash* hash;      // hash head
    struct command* hash_commands;
    char* desc;                 // description of this daemon 
    char* base_dir;             // base file location ex. /sys/proc/
    char* dir;                  // actual dir of ctl ex: /sys/proc/flex_ctl_PID
    int num_cmds;               // number of commands
    char* PID;                    // PID of daemon process, append this to dir ex: /sys/proc/flex_ctl_PID
};


int
main(void);

int
register_command(char* buffer);

int
show(char* buffer);

int
set(char* buffer);

#endif /*__SYSCTL_H__ */
