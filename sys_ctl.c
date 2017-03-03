//CLI

#include <malloc_heap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/inotify.h>
#include "sys_ctl_lib.h"
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
#include <dirent.h>


int
main(int argc, char** argv){

    char* cmd_name;
    char* data;
    char* dir;
    char* line;
    char* tmp_line;
    char* token;
    char sep[] = ",";
    size_t len = 0;
    ssize_t read;
    FILE* fp;

    if(argc < 2){
        printf("USAGE:\n./sys_ctl [command] [name] [dir/data]\n"
                "REGISTER syntax: REGISTER [dir]\n"
                "SET syntax: SET [command_name] [data]\n"
                "SHOW syntax: SHOW [command_name]\n"
                "LIST syntax: LIST\n");
        return 0;
    }

    //if(get_pid_from_name("/home/snaproute/workspace/sysctlbckup/sysctl/a.out") == -1){
    if(get_pid_from_name("./a.out") == -1){
        printf("flexctl daemon not found\n");
        return -1;
    }

    if(!(strcmp("FILE", argv[1]))){
        printf("File specified\n");
        if(argc != 3){
            printf("FILE syntax: FILE file_name\n");
            return -1;
        }
        fp = fopen(argv[2], "rb");
        if(fp == NULL){
            printf("Error opening file %s\n", argv[2]);
            return -1;
        }
    }

    while ((read = getline(&line, &len, fp)) != -1){
        strtok(line, "\n");
        tmp_line = calloc(1, strlen(line)+ 1);
        strcpy(tmp_line, line);
        token = strtok(line, sep);
        sleep(1);
        if (!(strcmp("REGISTER", token))){
            printf("Register\n");
            token = strtok(NULL, sep);
            if(token == NULL){
                printf("REGISTER syntax: REGISTER,[cmd_name],[path],[cb]");
                return -1;
            }

            register_command(tmp_line);
            continue;
        }

        else if (!(strcmp("SET", token))){
            printf("Set\n");

            token = strtok(NULL, sep);
            if(token == NULL){
                printf("SET syntax: SET [command_name] [data]\n");
                return -1;
            }

            cmd_name = calloc(1, strlen(token));
            strcpy(cmd_name, token);
            
            token = strtok(NULL, sep);
            if(token == NULL){
                printf("SET syntax: SET [command_name] [data]\n");
                return -1;
            }

            data = calloc(1, strlen(token));
            strcpy(data, token);
            
            set(cmd_name, data);
            continue;
        }

        else if (!(strcmp("SHOW", token))){
            printf("Show\n");

            token = strtok(NULL, sep);
            if (token == NULL){
                printf("SHOW syntax: SHOW [command_name]\n");
                return -1;
            }
            cmd_name = calloc(1, strlen(token));
            strcpy(cmd_name, token);
            show(cmd_name);
            continue;
        }
        
        else if (!(strcmp("LIST", token))){
            printf("List\n");
            list_command();
            continue;
        }

        else{
            printf("USAGE:\n./sys_ctl [command] [name] [dir/data]\n"
                   "REGISTER syntax: REGISTER [command_name] [dir]\n"
                   "SET syntax: SET [command_name] [data]\n"
                   "SHOW syntax: SHOW [command_name]\n"
                   "LIST syntax: LIST\n");
            return 0;
        }
    }

    return 0;
}
