#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#define BUF_SIZE 6
int main(int argc, char* argv[])
{
	/*FILE *fp = fopen(argv[1],"r");
	if(fp == NULL)
		fprintf(stderr,"error\n");
	char buf[BUF_SIZE];
	fseek(fp,atoi(argv[2]),SEEK_SET);
	printf("%ld %ld\n",ftell(fp),fread(buf,sizeof(char),1,fp));
	buf[1] = '\0';
	printf("%s\n",buf);
	return 0;*/
	printf("%f",sin(1));
}
