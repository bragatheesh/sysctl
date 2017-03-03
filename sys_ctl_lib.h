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
#include <dirent.h>
#include <linux/inotify.h>
#include <sys/stat.h>

struct command{
	char* name;	            //name of our command, also used as a key for our hashtable
	char* dir;		        //name of the file we will be modifying along with absolute path from struct flexctl dir
	char* data;		        //holds the data that will be written (maybe not required?)
	FILE* fp;		        //holds file pointer to the file
	void (*cb)(void *);     //holds pointer to handler function
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

int daemon_pid = -1;

int set_daemon_pid(int pid){
    daemon_pid = pid;
    return 0;
}

int
get_pid_from_name(char* proc_name){
    //const char* proc_name = "flex_ctl";
    const char* directory = "/proc";
    size_t taskNameSize = 1024;
    char* taskName = calloc(1, taskNameSize);

    DIR* dir = opendir(directory);

    if (dir)
    {
        struct dirent* de = 0;

        while ((de = readdir(dir)) != 0)
        {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;

            int pid = -1;
            int res = sscanf(de->d_name, "%d", &pid);

            

            if (res == 1)
            {
                if(pid < 1000)
                    continue;
                // we have a valid pid

                // open the cmdline file to determine what's the name of the process running
                char cmdline_file[1024] = {0};
                sprintf(cmdline_file, "%s/%d/cmdline", directory, pid);

                FILE *cmdline = fopen(cmdline_file, "r");

                if (getline(&taskName, &taskNameSize, cmdline) > 0)
                {
                    // is it the process we care about?
                    if (strstr(taskName, proc_name) != 0)
                    {
                        fprintf(stdout, "A %s process, with PID %d, has been detected.\n", proc_name, pid);
                        fclose(cmdline);
                        closedir(dir);
                        set_daemon_pid(pid);
                        return pid;
                    }
                }
                fclose(cmdline);
            }
        }
        closedir(dir);
    }
    return -1;
}


int
register_command(char* cmd_buffer){
    int ret;
    size_t cmd_len;
    char* in_path = calloc(1, 100);
    char* out_path = calloc(1, 100);
    FILE* fp;
    int file_des;
    off_t file_size;
    struct stat stbuff;
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    sprintf(in_path, "/etc/init/flexctl/%d/flexctl_in.ctl", daemon_pid);
    sprintf(out_path, "/etc/init/flexctl/%d/flexctl_out.ctl", daemon_pid);
    //open up flexpath.ctl and write register (command name) (file to point to)

     int fd = inotify_init();
    if(fd < 0){
        printf("Failed to init inotify in list_command\n");
        return -1;
    }

    inotify_add_watch(fd, out_path, IN_MODIFY);
    int EVENT_SIZE = (sizeof(struct inotify_event));
    int EVENT_BUF_LEN = 1024 * (EVENT_SIZE + 16);
    char buffer[EVENT_BUF_LEN];
    

    file_des = open(in_path, O_RDWR);
    if(file_des < 0){
        printf("flexctl: couldn't open file %s", in_path);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file

    if(ftruncate(file_des, 0) == -1){
        printf("flexctl: couldn't truncate file %s", in_path);
        return -1;
    }

    if (write(file_des, cmd_buffer, strlen(cmd_buffer)) < 0){
        printf("Error writing to %s\n", in_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }

    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);
    
    int length = read(fd, buffer, EVENT_BUF_LEN);
    if(length < 0){
        printf("Error detecting changes in file from list_command\n");
        return -1;
    }

    fl.l_type = F_WRLCK;
    file_des = open(out_path, O_RDWR);
    if(file_des < 0){
        printf("flexctl: couldn't open file %s", out_path);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file
    if (fstat(file_des, &stbuff) != 0){
        printf("flexctl: couldn't stat file %s", out_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }
    int fsize = stbuff.st_size;

    char* rdbuff = calloc(1, fsize);
    if(!rdbuff){
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        printf("flexctl: Memory alloc failed for read file buffer\n");
        return -1;
    }

    if (read(file_des, rdbuff, fsize) < 0){
        printf("flexctl: File %s read failed\n", out_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        free(rdbuff);
        return -1;
    }

    if(!strcmp(rdbuff, "SUCCESS")){
        printf("%s Registered successfully\n", cmd_buffer);
        
    }
    else if(!strcmp(rdbuff, "FAIL_EXISTS")){
        printf("Error registering %s\nCommand already exists.\n", cmd_buffer);
    }
    
    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);
    free(rdbuff);


    return 0;
}

int
list_command(){
    /*here we can list all the commands we have registered*/
    int fd;
    int length;
    long fsize;
    char* rdbuff;
    char* in_path = calloc(1, 100);
    char* out_path = calloc(1, 100);
    FILE* fp;
    char* cmd = "LIST";
    int file_des;
    off_t file_size;
    struct stat stbuff;
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();
    
    sprintf(in_path, "/etc/init/flexctl/%d/flexctl_in.ctl", daemon_pid);
    sprintf(out_path, "/etc/init/flexctl/%d/flexctl_out.ctl", daemon_pid);

    fd = inotify_init();
    if(fd < 0){
        printf("Failed to init inotify in list_command\n");
        return -1;
    }

    inotify_add_watch(fd, out_path, IN_MODIFY);
    int EVENT_SIZE = (sizeof(struct inotify_event));
    int EVENT_BUF_LEN = 1024 * (EVENT_SIZE + 16);
    char buffer[EVENT_BUF_LEN];


    file_des = open(in_path, O_RDWR);
    if(file_des < 0){
        printf("flexctl: couldn't open file %s", in_path);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file

    if(ftruncate(file_des, 0) == -1){
        printf("flexctl: couldn't truncate file %s", in_path);
        return -1;
    }
    
    if(write(file_des, cmd, strlen(cmd)) < 0){
        printf("Error writing to %s\n", in_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }
    
    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);


    length = read(fd, buffer, EVENT_BUF_LEN);
    if(length < 0){
        printf("Error detecting changes in file from list_command\n");
        return -1;
    }

    fl.l_type = F_WRLCK;
    file_des = open(out_path, O_RDWR);
    if(file_des < 0){
        printf("flexctl: couldn't open file %s", out_path);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file
    if (fstat(file_des, &stbuff) != 0){
        printf("flexctl: couldn't stat file %s", out_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }
    fsize = stbuff.st_size;

    int ret = 0;
    rdbuff = calloc(1, fsize);
    if(!rdbuff){
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        printf("flexctl: Memory alloc failed for read file buffer\n");
        return -1;
    }

    if (read(file_des, rdbuff, fsize) < 0){
        printf("flexctl: File %s read failed\n", out_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        free(rdbuff);
        return -1;
    }
    
    printf("\n%s\n",rdbuff);
    
    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);
    free(rdbuff);
    return 0;
}



int
show(char* name){
    FILE* fp;     
    char* rdbuff;
    long fsize;
    char* in_path = calloc(1, 100);
    char* out_path = calloc(1, 100);
    char cmd[1024];
    int file_des;
    off_t file_size;
    struct stat stbuff;
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    sprintf(in_path, "/etc/init/flexctl/%d/flexctl_in.ctl", daemon_pid);
    sprintf(out_path, "/etc/init/flexctl/%d/flexctl_out.ctl", daemon_pid);

    int fd = inotify_init();
    if(fd < 0){
        printf("Failed to init inotify in list_command\n");
        return -1;
    }

    inotify_add_watch(fd, out_path, IN_MODIFY);
    int EVENT_SIZE = (sizeof(struct inotify_event));
    int EVENT_BUF_LEN = 1024 * (EVENT_SIZE + 16);
    char buffer[EVENT_BUF_LEN];


    file_des = open(in_path, O_RDWR);
    if(file_des < 0){
        printf("flexctl: couldn't open file %s", in_path);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file

    if(ftruncate(file_des, 0) == -1){
        printf("flexctl: couldn't truncate file %s", in_path);
        return -1;
    }

    bzero(cmd, 1024);
    if(sprintf(cmd, "SHOW,%s", name) < 0){
        printf("Error writing to %s\n", in_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
    }

    if(write(file_des, cmd, strlen(cmd)) < 0){
        printf("Error writing to %s\n", in_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }
    
    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);


    int length = read(fd, buffer, EVENT_BUF_LEN);
    if(length < 0){
        printf("Error detecting changes in file from list_command\n");
        return -1;
    }
    
    fl.l_type = F_WRLCK;
    file_des = open(out_path, O_RDWR);
    if(file_des < 0){
        printf("flexctl: couldn't open file %s", out_path);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file
    if (fstat(file_des, &stbuff) != 0){
        printf("flexctl: couldn't stat file %s", out_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }
    fsize = stbuff.st_size;

    rdbuff = calloc(1, fsize+1);
    if(!rdbuff){
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        printf("flexctl: Memory alloc failed for read file buffer\n");
        return -1;
    }

    if(read(file_des, rdbuff, fsize) < 0){
        printf("flexctl: File %s read failed\n", out_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        free(rdbuff);
        return -1;
    }

    printf("\n%s\n",rdbuff);
    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);
    free(rdbuff);
    return 0;
}

int
set(char* name, char* data){
    FILE* fp;     
    char* in_path = calloc(1, 100);
    char* out_path = calloc(1, 100);

    sprintf(in_path, "/etc/init/flexctl/%d/flexctl_in.ctl", daemon_pid);
    sprintf(out_path, "/etc/init/flexctl/%d/flexctl_out.ctl", daemon_pid);

    int file_des;
    off_t file_size;
    struct stat stbuff;
    struct flock fl;
    char buff[1024];
    
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    int fd = inotify_init();
    if(fd < 0){
        printf("Failed to init inotify in list_command\n");
        return -1;
    }

    inotify_add_watch(fd, out_path, IN_MODIFY);
    int EVENT_SIZE = (sizeof(struct inotify_event));
    int EVENT_BUF_LEN = 1024 * (EVENT_SIZE + 16);
    char buffer[EVENT_BUF_LEN];


    file_des = open(in_path, O_RDWR);
    if(file_des < 0){
        printf("Error opening %s\n", in_path);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file

    if(ftruncate(file_des, 0) == -1){
        printf("flexctl: couldn't truncate file %s", in_path);
        return -1;
    }

    bzero(buff, 1024);
    if(sprintf(buff, "SET,%s,%s",name, data) < 0){
        printf("Error writing to %s\n", in_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
    }
    
    if(write(file_des, buff, 1024) < 0){
        printf("Error writing to %s\n", in_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }

    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);
    
    int length = read(fd, buffer, EVENT_BUF_LEN);
    if(length < 0){
        printf("Error detecting changes in file from list_command\n");
        return -1;
    }

    fl.l_type = F_WRLCK;
    file_des = open(out_path, O_RDWR);
    if(file_des < 0){
        printf("flexctl: couldn't open file %s", out_path);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file
    if (fstat(file_des, &stbuff) != 0){
        printf("flexctl: couldn't stat file %s", out_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }
    int fsize = stbuff.st_size;

    char* rdbuff = calloc(1, fsize);
    if(!rdbuff){
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        printf("flexctl: Memory alloc failed for read file buffer\n");
        return -1;
    }

    if (read(file_des, rdbuff, fsize) < 0){
        printf("flexctl: File %s read failed\n", out_path);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        free(rdbuff);
        return -1;
    }

    if(!strcmp(rdbuff, "SUCCESS")){
        printf("%s set to %s successfully\n", name, data);  
    }
    else if(!strcmp(rdbuff, "FAIL_NOT_REGISTERED")){
        printf("Error: Command %s has not been registered\n", name);
    }
    else if(!strcmp(rdbuff, "FAIL_FILE_OPEN")){
        printf("Error: Could not open %s's file\n", name);
    }
    else if(!strcmp(rdbuff, "FAIL_FILE_WRITE")){
        printf("Error: Could not write to %s's file\n", name);
    }
    else{
        printf("Returned: %s\n", rdbuff);
    }
    
    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);
    free(rdbuff);
    return 0;
}


#endif /*__SYSCTL_H__ */
