#include<stdio.h>
#include<string.h>
#include<ctype.h>
#define BUFSIZE 200
int main(int argc, char* argv[])
{
	char buf[BUFSIZE];
	FILE *fp;
	
	fp = (argv[2] == NULL)? stdin:fopen(argv[2],"r");
	if(fp == NULL){
		fprintf(stderr,"error\n");
		return 0;
	}	
	while(fgets(buf,BUFSIZE,fp) != NULL && (buf[0]!='\n') ){
		int sum = 0, gap_length;
		char *head = buf;
		while( (gap_length = strcspn(head,argv[1]) )!= strlen(head)){
			sum ++;
			head += (gap_length+1);
		}	
		printf("%d\n",sum);
	}
	if(fp != stdin)
		fclose(fp);
	return 0;
}
