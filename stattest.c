#include<stdio.h>
#include<unistd.h>
#include<sys/stat.h>
int main()
{
	struct stat statbuf;
	//int fd = open("stattest.c",O_RDONLY);
	stat("stattest.c",&statbuf);
	printf("dev:\t%d\n",statbuf.st_dev);
	printf("ino:\t%d\n",statbuf.st_ino);
	printf("mode:\t%o\n",statbuf.st_mode);
	printf("nlink:\t%d\n",statbuf.st_nlink);
	printf("uid:\t%d\n",statbuf.st_uid);
	printf("gid:\t%d\n",statbuf.st_gid);
	printf("size:\t%d\n",statbuf.st_size);
	printf("blksize:\t%d\n",statbuf.st_blksize);
	printf("blocks:\t%d\n",statbuf.st_blocks);
	return 0;
}
