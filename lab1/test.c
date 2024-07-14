#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void)
{
if (fork() == 0)
 {
printf("a");
 } else {
wait(NULL);
printf("b");
 }
printf("c");
return 0;
}
