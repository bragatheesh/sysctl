#ifndef __SYSCTL_H__
#define __SYSCTL_H__

#include <stdio.h>
#include <stdlib.h>
#include "uthash.h"
#include <pthread.h>
#include <rte_jhash.h>
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

struct command{
	char* name;	            //name of our command, also used as a key for our hashtable
	char* dir;		        //name of the file we will be modifying along with absolute path from struct flexctl dir
	char* data;		        //holds the data that will be written (maybe not required?)
	FILE* fp;		        //holds file pointer to the file
	void (*cb)(void *);     //holds pointer to callback function on trigger of file change
	pthread_t id;           //holds pthread id for this command	
};

struct flex_ctl{
    struct rte_hash* hash;      // hash head
    char* desc;                 // description of this daemon 
    char* base_dir;             // base file location ex. /sys/proc/
    char* dir;                  // actual dir of ctl ex: /sys/proc/flex_ctl_PID
    int num_cmds;               // number of commands
    char* PID;                    // PID of daemon process, append this to dir ex: /sys/proc/flex_ctl_PID
    // func to print all the command that is availble under this hash


};

int
register_command(char* name, void* cb, struct flex_ctl* flex);

int
list_command();

int
execute_command(char* name);

pthread_t
monitor_command_init(char* name);

void*
monitor_command_execute(void* arg);


#endif /*__SYSCTL_H__ */
