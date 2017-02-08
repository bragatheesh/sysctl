#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <linux/inotify.h>

int
main(void){
    /*pid_t pid, sid;

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


    /*int EVENT_SIZE = (sizeof(struct inotify_event));
    int EVENT_BUF_LEN = 1024 * (EVENT_SIZE + 16);
    char buffer[EVENT_BUF_LEN];*/
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
            exit(EXIT_FAILURE);
        }
        else{
            f = fopen("flexpath.ctl", "rb");
            if(f < 0){
                syslog(LOG_NOTICE, "flexctl: couldn't open file");
                exit(EXIT_FAILURE);
            }

            fseek(f, 0L, SEEK_END);
            fsize = ftell(f);
            rewind(f);
            
            syslog(LOG_NOTICE, "fsize: %d", fsize);
            
            rdbuff = calloc(1, fsize+1);            
            if(!rdbuff){
                fclose(f);
                syslog(LOG_NOTICE,"flexctl: Memory alloc failed for read file buffer");
                exit(EXIT_FAILURE);
            }

            if(1 != fread(rdbuff, fsize, 1 ,f)){
                fclose(f);
                syslog(LOG_NOTICE,"flexctl: File read failed: %s", rdbuff);
                free(rdbuff);
                exit(EXIT_FAILURE);
            }

            syslog(LOG_NOTICE, "flexctl: buffer: %s", rdbuff);
            fclose(f);
            free(rdbuff); 
            sleep(3);
        }

    }

    exit(EXIT_SUCCESS);
}
