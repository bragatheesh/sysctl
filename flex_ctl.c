//DAEMON

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <linux/inotify.h>
#include "flex_ctl.h"
#include <string.h>
#include <signal.h>
//#include <fcntl.h>

struct command* hash_commands = NULL;

int
register_command(char* fname, char* buffer){

    char *name;
    char *dir;
    char* token;
    char sep[] = ",";
    struct command* c;
    char* buff;
    int success = 1;

    token = strtok(buffer, sep);
    token = strtok(NULL, sep);
    name = (char*) calloc(strlen(token) + 1, sizeof(char));
    strcpy(name, token);
    token = strtok(NULL, sep);
    dir = (char*) calloc(strlen(token) + 1, sizeof(char));
    strcpy(dir, token);

    HASH_FIND_STR(hash_commands, name, c);  //Check if command is already present in hash
    if (c != NULL) {
        syslog(LOG_NOTICE,"flexctl: Error: command %s has already been registered.", name);
        success = 0;
    }

    c = malloc(sizeof(struct command));

    c->name = name;
    c->dir = dir;
    if(success)
        HASH_ADD_KEYPTR(hh, hash_commands, c->name, strlen(c->name), c);
    
    int file_des;
    off_t file_size;
    struct stat stbuff;
    struct flock fl;
    if(success){
        buff = (char*) calloc(strlen("SUCCESS") + 1, sizeof(char));
        strcpy(buff, "SUCCESS");
    }
    else{
        buff = (char*) calloc(strlen("FAIL_EXISTS") + 1, sizeof(char));
        strcpy(buff, "FAIL_EXISTS");
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    if (truncate(fname, 0) == -1){
        syslog(LOG_NOTICE, "flexctl: couldn't truncate file %s", fname);
        return -1;
    }

    file_des = open(fname, O_RDWR);
    if(file_des < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open file %s", fname);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file
    
    if(write(file_des, buff, strlen(buff)) < 0){
        syslog(LOG_NOTICE, "Error writing to %s\n", fname);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }

    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);

    syslog(LOG_NOTICE,"flexctl: %s %s", c->name, c->dir);
    return 0;
}


int
show(char* fname, char* buffer){
    char* name;
    char* token;
    char sep[] = ",";
    struct command* c;
    FILE* f; 
    FILE* fp;
    long fsize;
    char* rdbuff;

    token = strtok(buffer, sep);
    token = strtok(NULL, sep);
    name = (char*) calloc(strlen(token) + 1, sizeof(char));
    strcpy(name, token);

    HASH_FIND_STR(hash_commands, name, c);  //Check if command is already present in hash
    if (c == NULL) {
        syslog(LOG_NOTICE,"flexctl: Error: command %s has not been registered.", name);
        return -1;
    }

    f = fopen(c->dir, "rb");
    if(f < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open file %s",c->dir);
        return -1;
    }

    fseek(f, 0L, SEEK_END);
    fsize = ftell(f);
    rewind(f);

    if(fsize == 0){
        syslog(LOG_NOTICE, "flexctl: file size is zero");
        return -1;
    }

    rdbuff = (char*) calloc(fsize, sizeof(char));
    if(!rdbuff){
        fclose(f);
        syslog(LOG_NOTICE,"flexctl: Memory alloc failed for read file buffer");
        return -1;
    }

    if(1 != fread(rdbuff, fsize, 1 ,f)){
        fclose(f);
        syslog(LOG_NOTICE,"flexctl: File %s read failed", c->dir);
        //free(rdbuff);
        return -1;
    }

    fclose(f);

    int file_des;
    off_t file_size;
    struct stat stbuff;
    struct flock fl;
    
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    if (truncate(fname, 0) == -1){
        syslog(LOG_NOTICE, "flexctl: couldn't truncate file %s", fname);
        return -1;
    }

    file_des = open(fname, O_RDWR);
    if(file_des < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open file %s", fname);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file
    
    if(write(file_des, rdbuff, strlen(rdbuff)) < 0){
        syslog(LOG_NOTICE, "Error writing to %s\n", fname);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }

    //free(rdbuff);
    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);
    return 0;
}

int
set(char* fname, char* buffer){ //need to implement lock here
    char *name;
    char *data;
    char* token;
    char* buff;
    int success = 0;
    FILE* f;
    char sep[] = ",";
    struct command* c;

    token = strtok(buffer, sep);
    token = strtok(NULL, sep);
    name = (char*) calloc(strlen(token) + 1, sizeof(char));
    strcpy(name, token);
    token = strtok(NULL, sep);
    data = (char*) calloc(strlen(token) + 1, sizeof(char));
    strcpy(data, token);

    HASH_FIND_STR(hash_commands, name, c);  //Check if command is already present in hash
    if (c == NULL) {
        syslog(LOG_NOTICE,"flexctl: Error: command %s has not been registered.", name);
        success = -1;
    }

    if(success == 0){
        f = fopen(c->dir, "w+");
        if (f < 0){
            syslog(LOG_NOTICE, "flexctl: couldn't open file %s", c->dir);
            success = -2;
        }
        if(success == 0){
            syslog(LOG_NOTICE, "flexctl: Writing %s to %s", data, c->dir);
            if (fprintf(f, "%s", data) < 0){
                syslog(LOG_NOTICE, "Error writing to %s\n", c->dir);
                success = -3;
            }

            fclose(f);
        }
    }

    int file_des;
    off_t file_size;
    struct stat stbuff;
    struct flock fl;
    
    if(success == 0){
        buff = (char*) calloc(strlen("SUCCESS") + 1, sizeof(char));
        strcpy(buff, "SUCCESS");
    }
    else if(success == -1){
        buff = (char*) calloc(strlen("FAIL_NOT_REGISTERED") + 1, sizeof(char));
        strcpy(buff, "FAIL_NOT_REGISTERED");
    }
    else if(success == -2){
        buff = (char*) calloc(strlen("FAIL_FILE_OPEN") + 1, sizeof(char));
        strcpy(buff, "FAIL_FILE_OPEN");
    }
    else if(success == -3){
        buff = (char*) calloc(strlen("FAIL_FILE_WRITE") + 1, sizeof(char));
        strcpy(buff, "FAIL_FILE_WRITE");
    }
    
    
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    if (truncate(fname, 0) == -1){
        syslog(LOG_NOTICE, "flexctl: couldn't truncate file %s", fname);
    }

    file_des = open(fname, O_RDWR);
    if(file_des < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open file %s You will need to restart CLI.", fname);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file
    
    if(write(file_des, buff, strlen(buff)) < 0){
        syslog(LOG_NOTICE, "Error writing to %s You will need to restart CLI", fname);
        fl.l_type = F_UNLCK;
        fcntl(file_des, F_SETLK, &fl);
        close(file_des);
        return -1;
    }

    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);
    return 0;
}

int
list(char* fname){

    struct command *c;
    char buffer[1024];
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

    if (truncate(fname, 0) == -1){
        syslog(LOG_NOTICE, "flexctl: couldn't truncate file %s", fname);
        return -1;
    }

    file_des = open(fname, O_RDWR);
    if(file_des < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open file %s", fname);
        return -1;
    }

    fcntl(file_des, F_SETLKW, &fl); //lock file    

    for(c = hash_commands; c != NULL; c = c->hh.next){
        //bzero(buffer, 1024);
        syslog(LOG_NOTICE,"flexctl: name: %s, dir: %s", c->name, c->dir);
        int len = strlen("name: ") + strlen(c->name) + strlen(", dir: ") + strlen(c->dir) + strlen("\n");
        char *output = (char*) calloc(len + 1, sizeof(char));
        if(sprintf(output, "name: %s, dir: %s\n", c->name, c->dir) < 0){
        //if(sprintf(output, "name:, dir: \n") < 0){
            syslog(LOG_NOTICE, "flexctl: error printing to buffer in list");
            fl.l_type = F_UNLCK;
            fcntl(file_des, F_SETLK, &fl);
            close(file_des);
            return -1;
        }
        if(write(file_des, output, len) < 0){
            syslog(LOG_NOTICE, "Error writing to %s\n", fname);
            fl.l_type = F_UNLCK;
            fcntl(file_des, F_SETLK, &fl);
            close(file_des);
            return -1;
        }
    }
    fl.l_type = F_UNLCK;
    fcntl(file_des, F_SETLK, &fl);
    close(file_des);
    return 0;
}

void
file_handler(){
    int length, ret;
    long fsize;
    
    FILE* f;
    int file_des;
    off_t file_size;
    struct stat stbuff;
    struct flock fl;
    
    char* in_fname = "flexctl_in.ctl";
    char* out_fname = "flexctl_out.ctl";
    char* dpdk = "dpdk.ctl";


    f = fopen(in_fname, "a+");
    if(f < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open and/or create file %s", in_fname);
        exit(EXIT_FAILURE);
    }
    fclose(f);

    f = fopen(out_fname, "a+");
    if(f < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open and/or create file %s", out_fname);
        exit(EXIT_FAILURE);
    }
    fclose(f);

    int fd = inotify_init();
    if(fd < 0){
        syslog(LOG_NOTICE, "flexctl: failed to init inotify");
        exit(EXIT_FAILURE);
    }

    inotify_add_watch(fd, in_fname, IN_MODIFY);
    syslog(LOG_NOTICE, "flexctl: entering while loop for %s", in_fname);
    while (1) {

        char* rdbuff;

        int EVENT_SIZE = (sizeof(struct inotify_event));
        int EVENT_BUF_LEN = 1024 * (EVENT_SIZE + 16);
        char buffer[EVENT_BUF_LEN];

        length = read(fd, buffer, EVENT_BUF_LEN);
        if(length < 0){
            syslog(LOG_NOTICE, "flexct: error detecting change in file");
            continue;
        }
        else{
            
            fl.l_type = F_WRLCK;
            fl.l_whence = SEEK_SET;
            fl.l_start = 0;
            fl.l_len = 0;
            fl.l_pid = getpid();


            file_des = open(in_fname, O_RDWR);
            if(file_des < 0){
                syslog(LOG_NOTICE, "flexctl: couldn't open file %s", in_fname);
                continue;
            }

            fcntl(file_des, F_SETLKW, &fl); //lock file

            if(fstat(file_des, &stbuff) != 0){
                syslog(LOG_NOTICE, "flexctl: couldn't stat file %s", in_fname);
                fl.l_type = F_UNLCK;
                fcntl(file_des, F_SETLK, &fl);
                close(file_des);
                continue;
            }
            file_size = stbuff.st_size;
            
            syslog(LOG_NOTICE, "flexctl: fsize: %ld", file_size);

            if(!file_size){
                fl.l_type = F_UNLCK;
                fcntl(file_des, F_SETLK, &fl);
                close(file_des);
                continue;
            }

            rdbuff = (char*) calloc(file_size, sizeof(char));            
            if(!rdbuff){
                close(file_des);
                syslog(LOG_NOTICE,"flexctl: Memory alloc failed for read file buffer");
                if(hash_commands != NULL){
                    HASH_CLEAR(hh, hash_commands);
                }
                fl.l_type = F_UNLCK;
                fcntl(file_des, F_SETLK, &fl);
                close(file_des);
                exit(EXIT_FAILURE);
            }

            if(read(file_des, rdbuff, file_size) < 1){
                close(file_des);
                syslog(LOG_NOTICE,"flexctl: File read failed: %s", in_fname);
                //free(rdbuff);
                fl.l_type = F_UNLCK;
                fcntl(file_des, F_SETLK, &fl);
                close(file_des);
                continue;
            }

            syslog(LOG_NOTICE, "flexctl: buffer: %s", rdbuff);

            switch(rdbuff[0]){
                case 'R':
                    register_command(out_fname, rdbuff);
                    break;
                case 'S':
                    switch(rdbuff[1]){
                        case 'E':
                            set(out_fname, rdbuff);
                            break;
                        case 'H':
                            show(out_fname, rdbuff);
                            break;
                        default:
                            syslog(LOG_NOTICE, "flexctl: unknown command %s", rdbuff);
                    }
                    break;
                case 'L':
                    list(out_fname);
                    break;
                case 'E':
                    syslog(LOG_NOTICE, "flexctl: Exiting");
                    HASH_CLEAR(hh, hash_commands);
                    //free(rdbuff);
                    exit(EXIT_SUCCESS);
                    break;
                default:
                    syslog(LOG_NOTICE, "flexctl: unknown command %s", rdbuff);
                    break;
            }
            
            if (truncate(in_fname, 0) == -1){
                syslog(LOG_NOTICE, "flexctl: couldn't truncate file %s", in_fname);
            }

            fl.l_type = F_UNLCK;
            fcntl(file_des, F_SETLK, &fl);
            close(file_des);
            //free(rdbuff); 
            sleep(3);
        }

    }
}


void 
term(int signum){
    syslog(LOG_NOTICE, "flexctl: Caught SIGTERM");
    HASH_CLEAR(hh, hash_commands);
    exit(EXIT_SUCCESS);
}


int
//init_ctl(void){
main(void){
    /*pid_t pid, sid;

    pid = fork();
    if (pid < 0) {
        return -1;;
    }
    if (pid > 0) {
        printf("PID: %d\n", pid);
        syslog(LOG_NOTICE,"flexctl: PID: %d", pid);
        return pid;
    }

    umask(0);


    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }*/

    printf("PID: %d\n", getpid());
    int ret;
    char dir[23];
    bzero(dir, 23);
    sprintf(dir, "%d", getpid());

    if((chdir("/etc/init/flexctl")) < 0){
        if(mkdir("/etc/init/flexctl", 0777) < 0)
            exit(EXIT_FAILURE);
        
        if ((chdir("/etc/init/flexctl")) < 0) {
            exit(EXIT_FAILURE);
        }
    }

    if(chdir(dir) < 0){
        if(mkdir(dir, 0777) < 0)
            exit(EXIT_FAILURE);
        
        if ((chdir(dir)) < 0) {
            exit(EXIT_FAILURE);
        }
    }
    
    /*if((chdir(dir))< 0){
        syslog(LOG_NOTICE,"flexctl: could not change to dir /proc/%d", pid);
        exit(EXIT_FAILURE);
    }*/


    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    int length;
    pthread_t flexpath_thread, dpdk_thread;
    
    ret = pthread_create(&flexpath_thread, NULL, &file_handler, NULL);
    if(ret != 0){
        printf("Error creating thread\n");
        exit(EXIT_FAILURE);
    }

    pthread_join(flexpath_thread, NULL);
    HASH_CLEAR(hh, hash_commands);
    exit(EXIT_SUCCESS);
}
