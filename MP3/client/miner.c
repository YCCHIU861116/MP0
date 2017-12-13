#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bsd/md5.h>
#include <limits.h> 

#include "common.h"
#define MAX_BUFSIZE 1000000000 
#define	PASSWORD "B05902029" 

int recursiveUpdate(char *buf,int len,MD5_CTX ctx,int target,int zeronum,char *result,int fd){
	if(len == target){
		MD5Update(&ctx, (const unsigned char*)buf, len);
		MD5End(&ctx,result);
		//buf[len] = '\0';
		//fprintf(stderr,"len = %d,%ld,%s,%s\n",len ,strlen(buf),buf,result);
		int i;
		for(i = 0; i < 32 && result[i] == '0'; i++){} 
		return ((i == zeronum)? 1 : 0); 
	}
	//MD5_CTX tmp = ctx;
	if(len == 4096){
		MD5Update(&ctx, (const unsigned char*)buf, len);
		buf += 4096;
	}
	for(int i = 0; i <= 255; i++){
		struct todo tmp;
		if(read(fd,&tmp,sizeof(struct todo)) > 0){
			if(strcmp(tmp.Head.password,PASSWORD) != 0)
				fprintf(stderr,"wrong password\n");
			else{
				switch(tmp.Head.op){
					case MINE:
						fprintf(stderr,"Mine op\n");
						break;
					case STOP:
						printf("%s wins a %d-treasure! %s\n",tmp.path,tmp.zeronum,tmp.message);
						return -1;
					case STATUS:
						printf("I'm working on %s\n",result);
						break;
					case QUIT:
						printf("BOSS is at rest.\n");
						return -2;
					default:
						fprintf(stderr,"Unknown OP\n");
				}
			}
		}
		if(len >= 4096)
			buf[len-4096] = (unsigned char)i;
		else
			buf[len] = (unsigned char)i;
		int ret = recursiveUpdate(buf,len+1,ctx,target,zeronum,result,fd);
		if(ret == 1 || ret == -1)
			return ret; 
	}
	return 0;
}

int main(int argc, char **argv)
{
    /* parse arguments */
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s CONFIG_FILE\n", argv[0]);
        exit(1);
    }

    char *name = argv[1];
    char *input_pipe = argv[2];
    char *output_pipe = argv[3];

    /* create named pipes */
    int ret;
    ret = mkfifo(input_pipe, 0644);
    assert (!ret);

    ret = mkfifo(output_pipe, 0644);
    assert (!ret);

    /* open pipes */
    int input_fd = open(input_pipe, O_RDONLY);
    assert (input_fd >= 0);

    int output_fd = open(output_pipe, O_WRONLY);
    assert (output_fd >= 0);

    /* TODO write your own (1) communication protocol (2) computation algorithm */
    /* To prevent from blocked read on input_fd, try select() or non-blocking I/O */
	
	struct todo Todo;
	MD5_CTX ctx;
	char minebuf[5100],result[33];
	MD5Init(&ctx);
	
	while(1){
		if(read(input_fd,&Todo,sizeof(struct todo))){
			if(strcmp(Todo.Head.password,PASSWORD) != 0){
				fprintf(stderr,"wrong password\n");
				continue;
			}
			switch(Todo.Head.op){
				case START:
					printf("BOSS is mindful.\n");
					break;
				case MINE:{
					fprintf(stderr,"assigned job charnum = %d, target treasure = %d\n",Todo.charnum,Todo.zeronum);
					int minefd;
					if((minefd = open(Todo.path,O_RDONLY)) < 0){
						fprintf(stderr,"open mine error: %s\n",Todo.path);
						continue;
					}
					int find = 0;
					size_t len;
					while ((len = read(minefd, minebuf, 4096)) == 4096){
						struct todo tmp;
 						MD5Update(&ctx, (const unsigned char*)minebuf, len);
						if(read(input_fd,&tmp,sizeof(struct todo))>0){
							if(strcmp(tmp.Head.password,PASSWORD) != 0)
								fprintf(stderr,"wrong password\n");
							else if(tmp.Head.op == QUIT){
								printf("BOSS is at rest.\n");
								continue;
							}
							else if(tmp.Head.op == STOP){
								fprintf(stderr,"main: ");
								printf("%s wins a %d-treasure! %s\n",Todo.path,Todo.zeronum,Todo.message);
								continue;
							}
						}
					}
 					close(minefd);
					for(int i = Todo.minerid; i < 256 && !find; i += Todo.minernum){
						minebuf[len] = (unsigned char)i;
						find = recursiveUpdate(minebuf,len+1,ctx,len+Todo.charnum,Todo.zeronum,result,output_fd);
					}
			
					struct report Report;
					strcpy(Report.Head.password,"Sherlock");
					strcpy(Report.name,name);
					Report.find = (find > 0)? Todo.zeronum : find;
					if(find > 0){
						memcpy(Report.append,minebuf+len,Todo.charnum);
						Report.len = Todo.charnum;
						strcpy(Report.result,result);
						printf("I win a %d-treasure! %s\n",Todo.zeronum,result);
						if(write(output_fd,&Report,sizeof(struct report)) < 0){
							fprintf(stderr,"send Report error\n");
							continue;
						}
					}
					else if(find == 0){
						fprintf(stderr,"Charnum = %d not found\n",Todo.charnum);
						if(write(output_fd,&Report,sizeof(struct report)) < 0){
							fprintf(stderr,"send Report error\n");
							continue;
						}
					}
					else
						MD5Init(&ctx);
					break;
				}
				case STOP:
					fprintf(stderr,"main: ");
					printf("%s wins a %d-treasure! %s\n",Todo.path,Todo.zeronum,Todo.message);
					MD5Init(&ctx);
					break;
				case STATUS:
					fprintf(stderr,"No assigned jobs\n");
					break;
				case QUIT:
					printf("BOSS is at rest.\n");
					MD5Init(&ctx);
					break;
				default:
					fprintf(stderr,"Unknown op\n");
					continue;
			}
		}
	}
    return 0;//R402GIGANT
}
