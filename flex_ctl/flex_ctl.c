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

struct command* hash_commands = NULL;

int
register_command(char* buffer){

    char *name = calloc(1,strlen(buffer)+1);
    char *dir = calloc(1,strlen(buffer)+1);
    char* token;
    char sep[] = ",";
    struct command* c;

    token = strtok(buffer, sep);
    token = strtok(NULL, sep);
    sscanf(token, "%s", name);
    token = strtok(NULL, sep);
    sscanf(token, "%s", dir);

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
show(char* buffer){
    char* name;
    char* token;
    char sep[] = ",";
    struct command* c;
    FILE* f;
    long fsize;
    char* rdbuff;

    token = strtok(buffer, sep);
    token = strtok(NULL, sep);
    sscanf(token, "%s", name);

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

    rdbuff = calloc(1, fsize+1);
    if(!rdbuff){
        fclose(f);
        syslog(LOG_NOTICE,"flexctl: Memory alloc failed for read file buffer");
        return -1;
    }

    if(1 != fread(rdbuff, fsize, 1 ,f)){
        fclose(f);
        syslog(LOG_NOTICE,"flexctl: File read failed");
        free(rdbuff);
        return -1;
    }

    syslog(LOG_NOTICE,"%s", rdbuff);
    free(rdbuff);
    fclose(f);
    return 0;
}

int
set(char* buffer){

    return 0;
}


int
main(void){
    /*    pid_t pid, sid;

          pid = fork();
          if (pid < 0) {
          exit(EXIT_FAILURE);
          }
          if (pid > 0) {
          printf("PID: %d\n", pid);
          exit(EXIT_SUCCESS);
          }

          umask(0);


          sid = setsid();
          if (sid < 0) {
          exit(EXIT_FAILURE);
          }
          */

    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int length;
    long fsize;
    char* rdbuff;
    FILE* f;
    f = fopen("flexpath.ctl", "a+");
    int counter = 0;
    if(f < 0){
        syslog(LOG_NOTICE, "flexctl: couldn't open and create file");
        exit(EXIT_FAILURE);
    }
    fclose(f);


    int fd = inotify_init();
    if(fd < 0){
        syslog(LOG_NOTICE, "flexctl: failed to init inotify");
        exit(EXIT_FAILURE);
    }

    inotify_add_watch(fd, "flexpath.ctl", IN_MODIFY);

    syslog(LOG_NOTICE, "flexctl: entering while loop");
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
            f = fopen("flexpath.ctl", "rb");
            if(f < 0){
                syslog(LOG_NOTICE, "flexctl: couldn't open file");
                if(hash_commands != NULL){
                    HASH_CLEAR(hh, hash_commands);
                }
                exit(EXIT_FAILURE);
            }

            fseek(f, 0L, SEEK_END);
            fsize = ftell(f);
            rewind(f);

            syslog(LOG_NOTICE, "flexctl: fsize: %ld", fsize);

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
                            show(rdbuff);
                            break;
                        default:
                            syslog(LOG_NOTICE, "flexctl: unknown command %s", rdbuff);
                    }
                    break;
                default:
                    syslog(LOG_NOTICE, "flexctl: unknown command %s", rdbuff);
                    break;
            }

            fclose(f);
            free(rdbuff); 
            sleep(3);
        }

    }
    HASH_CLEAR(hh, hash_commands);
    exit(EXIT_SUCCESS);
}
