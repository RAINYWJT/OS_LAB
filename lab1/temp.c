#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
int main(int argc, char *argv[]) {
 printf("hello world (pid: %d)\n", (int) getpid());
 int rc = fork();
 if (rc < 0) {
 printf("fork failed\n");
 exit(1);
 } else if (rc == 0) {
 printf("child process (pid: %d)\n", (int) getpid()); 
 } else {
 // the wait() system call will not return until the child has exited
 int rc_wait = wait(NULL);
 printf("parent of process %d (pid: %d)\n", rc, (int) getpid());
 }
 return 0;
}