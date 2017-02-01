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

typedef struct command{
	char* name;	//name of our command, also used as a key for our hashtable
	char* dir;		//holds the absolute directory
	int write;		//boolean, 1 for write, 0 for read
	char* data;		//holds the data that will be written
	int sock;		//holds the socket to the file dir (not used)
	FILE* fp;		//holds file pointer to the file
	void (*cb)(void *);		//holds pointer to callback function
	pthread_t id;		//holds pthread id for this command	
	UT_hash_handle hh;	//makes this structure hashable
}command;

int
register_command(char*, char*, int, char*, void*, struct rte_hash*);

int
list_command();

int
execute_command(char*);

pthread_t
monitor_command_init(char*);

void*
monitor_command_execute(void*);


#endif /*__SYSCTL_H__ */
