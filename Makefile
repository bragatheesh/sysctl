all: 
	gcc -g -pthread sys_ctl.c -o sys_ctl
clean:
	$(RM) sys_ctl
