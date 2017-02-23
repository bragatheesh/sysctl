//CLI

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


int
register_command(char* name, char* dir){
    int ret;
    size_t cmd_len;

    FILE* fp;

    //open up flexpath.ctl and write register (command name) (file to point to)
    fp = fopen("/flexpath.ctl", "w+");
    if(fp < 0){
        printf("Error opening /flexpath.ctl\n");
        return -1;
    }

    if(fprintf(fp, "REGISTER,%s,%s", name, dir) < 0){
        printf("Error writing to /flexpath.ctl\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int
list_command(){
    /*here we can list all the commands we have registered*/
    long fsize;
    char* rdbuff;
    FILE* fp;
    
    fp = fopen("/flexpath.ctl", "w+");
    if(fp < 0){
        printf("Error opening /flexpath.ctl\n");
        return -1;
    }

    if(fprintf(fp, "LIST") < 0){
        printf("Error writing to /flexpath.ctl\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);

    sleep(2);

    fp = fopen("/flexpath.ctl", "rb");
    if(fp < 0){ 
        printf("Error opening /flexpath.ctl\n");
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    fsize = ftell(fp);
    rewind(fp);

    rdbuff = calloc(1, fsize+1);
    if(!rdbuff){
        fclose(fp);
        printf("flexctl: Memory alloc failed for read file buffer\n");
        return -1;
    }

    if(1 != fread(rdbuff, fsize, 1 ,fp)){
        fclose(fp);
        printf("flexctl: File /flexpath.ctl read failed\n");
        free(rdbuff);
        return -1;
    }
    printf("\n%s\n",rdbuff);
    fclose(fp);

    return 0;
}



int
show(char* name){
    FILE* fp;     
    char* rdbuff;
    long fsize;

    fp = fopen("/flexpath.ctl", "w+");
    if(fp < 0){
        printf("Error opening /flexpath.ctl\n");
        return -1;
    }

    if(fprintf(fp, "SHOW,%s", name) < 0){
        printf("Error writing to /flexpath.ctl\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    sleep(1);
    
    fp = fopen("/flexpath.ctl", "r");
    if(fp < 0){
        printf("Error opening /flexpath.ctl\n");
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    fsize = ftell(fp);
    rewind(fp);

    rdbuff = calloc(1, fsize+1);
    if(!rdbuff){
        fclose(fp);
        printf("flexctl: Memory alloc failed for read file buffer\n");
        return -1;
    }

    if(1 != fread(rdbuff, fsize, 1 ,fp)){
        fclose(fp);
        printf("flexctl: File /flexpath.ctl read failed\n");
        free(rdbuff);
        return -1;
    }
    printf("\n%s\n",rdbuff);
    fclose(fp);
    free(rdbuff);
    return 0;
}

int
set(char* name, char* data){
    FILE* fp;     
    fp = fopen("/flexpath.ctl", "w+");
    if(fp < 0){
        printf("Error opening /flexpath.ctl\n");
        return -1;
    }

    if(fprintf(fp, "SET,%s,%s",name, data) < 0){
        printf("Error writing to /flexpath.ctl\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
}


int
main(int argc, char** argv){

    char* cmd_name;
    char* data;
    char* dir;

    if(argc < 2){
        printf("USAGE:\n./sys_ctl [command] [name] [dir/data]\n"
                "REGISTER syntax: REGISTER [command_name] [dir]\n"
                "SET syntax: SET [command_name] [data]\n"
                "SHOW syntax: SHOW [command_name]\n"
                "LIST syntax: LIST\n");
        return 0;
    }

    if(!(strcmp("REGISTER",argv[1]))){
        printf("Register\n");
        if(argc != 4){
            printf("REGISTER syntax: REGISTER [dir]\n");
            return -1;
        }

        dir = calloc(1, strlen(argv[2]));
        strcpy(dir, argv[2]);
        register_command(dir);
        
        return 0;       
    }
    else if(!(strcmp("SET", argv[1]))){
        printf("Set\n");
        if(argc != 4){
            printf("SET syntax: SET [command_name] [data]\n");
            return -1;
        }
        cmd_name = calloc(1, strlen(argv[2]));
        strcpy(cmd_name, argv[2]);
        data = calloc(1, strlen(argv[3]));
        strcpy(data, argv[3]);
        set(cmd_name, data);
        return 0;
    }
    else if(!(strcmp("SHOW", argv[1]))){
        printf("Show\n");
        if(argc != 3){
            printf("SHOW syntax: SHOW [command_name]\n");
            return -1;
        }
        cmd_name = calloc(1, strlen(argv[2]));
        strcpy(cmd_name, argv[2]);
        show(cmd_name);
        return 0;
    }
    else if(!(strcmp("LIST", argv[1]))){
        printf("List\n");
        if(argc != 2){
            printf("LIST syntax: LIST\n");
            return -1;
        }
        list_command();
        return 0;
    }

    else{
        printf("USAGE:\n./sys_ctl [command] [name] [dir/data]\n"
                "REGISTER syntax: REGISTER [command_name] [dir]\n"
                "SET syntax: SET [command_name] [data]\n"
                "SHOW syntax: SHOW [command_name]\n"
                "LIST syntax: LIST\n");
        return 0;
    }

    return 0;
}
