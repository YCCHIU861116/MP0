#include<stdio.h>
#include<unistd.h>
#include<sys/stat.h>
int main()
{
	//mkdir("notexist",);
	printf("%d\n",access("notexist",F_OK));
}
