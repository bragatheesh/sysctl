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

struct command* hash_commands = NULL;

int
register_command(char* buffer){

    char *name;
    char *dir;
    char* token;
    char sep[] = ",";
    struct command* c;

    token = strtok(buffer, sep);
    token = strtok(NULL, sep);
    name = calloc(1,strlen(token)+1);
    strcpy(name, token);
    token = strtok(NULL, sep);
    dir = calloc(1,strlen(token)+1);
    strcpy(dir, token);

    HASH_FIND_STR(hash_commands, name, c);  //Check if command is already present in hash
    if (c != NULL) {
        syslog(LOG_NOTICE,"flexctl: Error: command %s has already been registered.", name);
        return -1;
    }

    c = malloc(sizeof(struct command));

    c->name = name;
    c->dir = dir;

    HASH_ADD_KEYPTR(hh, hash_commands, c->name, strlen(c->name), c);

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
    name = calloc(1, strlen(token)+1);
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

    rdbuff = calloc(1, fsize+1);
    if(!rdbuff){
        fclose(f);
        syslog(LOG_NOTICE,"flexctl: Memory alloc failed for read file buffer");
        return -1;
    }

    if(1 != fread(rdbuff, fsize, 1 ,f)){
        fclose(f);
        syslog(LOG_NOTICE,"flexctl: File %s read failed", c->dir);
        free(rdbuff);
        return -1;
    }

    fp = fopen(fname, "w+");
    if(fp < 0){
        syslog(LOG_NOTICE, "flexctl: File %s open failed", fname);
        fclose(f);
        return -1;
    }

    if(fprintf(fp, "%s", rdbuff) < 0){
        printf("Error writing to %s\n", fname);
        fclose(f);
        fclose(fp);
        return -1;
    }

    syslog(LOG_NOTICE,"%s", rdbuff);
    free(rdbuff);
    fclose(f);
    fclose(fp);
    return 0;
}

int
set(char* buffer){
    char *name;
    char *data;
    char* token;
    FILE* f;
    char sep[] = ",";
    struct command* c;

    token = strtok(buffer, sep);
    token = strtok(NULL, sep);
    name = calloc(1,strlen(token)+1);
    strcpy(name, token);
    token = strtok(NULL, sep);
    data = calloc(1,strlen(token)+1);
    strcpy(data, token);

    HASH_FIND_STR(hash_commands, name, c);  //Check if command is already present in hash
    if (c == NULL) {
        syslog(LOG_NOTICE,"flexctl: Error: command %s has not been registered.", name);
        return -1;
    }

    f = fopen(c->dir, "w+");
    if(f < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open file %s",c->dir);
        return -1;
    }
    syslog(LOG_NOTICE, "flexctl: Writing %s to %s", data, c->dir);
    if(fprintf(f, "%s", data) < 0){
        printf("Error writing to %s\n", c->dir);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

int
list(char* fname){

    struct command *c;
    FILE* fp;

    fp = fopen(fname, "w+");
    if(fp < 0){
        syslog(LOG_NOTICE, "flexctl: File %s open failed", fname);
        return -1;
    }

    for(c = hash_commands; c != NULL; c= c->hh.next){
        syslog(LOG_NOTICE,"name: %s, dir: %s", c->name, c->dir);
        if(fprintf(fp, "name: %s, dir: %s\n", c->name, c->dir) < 0){
            printf("Error writing to %s\n", fname);
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}

void
file_handler(void* file_name){
    int length, ret;
    long fsize;
    char* rdbuff;
    FILE* f;
    char* fname = calloc(1, strlen((char*)file_name) + 1);
    strcpy(fname, (char*)file_name);

    int fd = inotify_init();
    if(fd < 0){
        syslog(LOG_NOTICE, "flexctl: failed to init inotify");
        exit(EXIT_FAILURE);
    }

    inotify_add_watch(fd, fname, IN_MODIFY);
    syslog(LOG_NOTICE, "flexctl: entering while loop for %s", fname);
    while (1) {

        int EVENT_SIZE = (sizeof(struct inotify_event));
        int EVENT_BUF_LEN = 1024 * (EVENT_SIZE + 16);
        char buffer[EVENT_BUF_LEN];

        length = read(fd, buffer, EVENT_BUF_LEN);
        if(length < 0){
            syslog(LOG_NOTICE, "flexct: error detecting change in file");
            if(hash_commands != NULL){
                HASH_CLEAR(hh, hash_commands);
            }
            exit(EXIT_FAILURE);
        }
        else{
            f = fopen(fname, "rb");
            if(f < 0){
                syslog(LOG_NOTICE, "flexctl: couldn't open file %s", fname);
                if(hash_commands != NULL){
                    HASH_CLEAR(hh, hash_commands);
                }
                exit(EXIT_FAILURE);
            }

            fseek(f, 0L, SEEK_END);
            fsize = ftell(f);
            rewind(f);

            syslog(LOG_NOTICE, "flexctl: fsize: %ld", fsize);

            if(!fsize){
                continue;
            }

            rdbuff = calloc(1, fsize+1);            
            if(!rdbuff){
                fclose(f);
                syslog(LOG_NOTICE,"flexctl: Memory alloc failed for read file buffer");
                if(hash_commands != NULL){
                    HASH_CLEAR(hh, hash_commands);
                }
                exit(EXIT_FAILURE);
            }

            if(1 != fread(rdbuff, fsize, 1 ,f)){
                fclose(f);
                syslog(LOG_NOTICE,"flexctl: File read failed: %s", rdbuff);
                free(rdbuff);
                if(hash_commands != NULL){
                    HASH_CLEAR(hh, hash_commands);
                }
                exit(EXIT_FAILURE);
            }

            fclose(f);

            syslog(LOG_NOTICE, "flexctl: buffer: %s", rdbuff);

            switch(rdbuff[0]){
                case 'R':
                    register_command(rdbuff);
                    break;
                case 'S':
                    switch(rdbuff[1]){
                        case 'E':
                            set(rdbuff);
                            break;
                        case 'H':
                            show(fname, rdbuff);
                            break;
                        default:
                            syslog(LOG_NOTICE, "flexctl: unknown command %s", rdbuff);
                    }
                    break;
                case 'L':
                    list(fname);
                    break;
                case 'E':
                    syslog(LOG_NOTICE, "flexctl: Exiting");
                    HASH_CLEAR(hh, hash_commands);
                    free(rdbuff);
                    exit(EXIT_SUCCESS);
                    break;
                default:
                    syslog(LOG_NOTICE, "flexctl: unknown command %s", rdbuff);
                    break;
            }

            free(rdbuff); 
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
init_ctl(void){
    pid_t pid, sid;

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
    }

    char dir[100];
    sprintf(dir, "/flex_%d", getpid());
    if ((chdir(dir)) < 0) {
        mkdir(dir, 0777);
        if ((chdir(dir)) < 0) {
            exit(EXIT_FAILURE);
        }
    }


    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    int length, ret;
    pthread_t flexpath_thread, dpdk_thread;
    long fsize;
    char* rdbuff;
    char* flex = "flexpath.ctl";
    char* dpdk = "dpdk.ctl";
    FILE* f;
    f = fopen(flex, "a+");
    if(f < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open and/or create file flexpath.ctl");
        exit(EXIT_FAILURE);
    }
    fclose(f);
/*
    f = fopen(dpdk, "a+");
    if(f < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open and/or create file dpdk.ctl");
        exit(EXIT_FAILURE);
    }
    fclose(f);*/
    ret = pthread_create(&flexpath_thread, NULL, &file_handler, (void*)flex);
    if(ret != 0){
        printf("Error creating thread for %s\n", "flexpath.ctl");
        exit(EXIT_FAILURE);
    }
/*
    ret = pthread_create(&dpdk_thread, NULL, &file_handler, (void*)dpdk);
    if(ret != 0){
        printf("Error creating thread for %s\n", "dpdk.ctl");
        exit(EXIT_FAILURE);
    }*/
    pthread_join(flexpath_thread, NULL);
    //pthread_join(dpdk_thread, NULL);
    HASH_CLEAR(hh, hash_commands);
    exit(EXIT_SUCCESS);
}
