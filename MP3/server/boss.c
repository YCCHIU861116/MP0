#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "boss.h"
#include "common.h"
#define MINER_MAX 8
#define PASSWORD "Sherlock"

int load_config_file(struct server_config *config, char *path)
{
    fprintf(stderr,"loading config file...\n");
    int minernum = 0;
    char tmp[PATH_MAX],mtmp[PATH_MAX];
    char *mine = (char *)malloc(sizeof(char)*(PATH_MAX+1));
    strcpy(mine,"");
    struct pipe_pair *pipes = (struct pipe_pair*)malloc(sizeof(struct pipe_pair)*MINER_MAX);
	FILE *fp = fopen(path,"r");
	fscanf(fp,"%s%s",tmp,mtmp);
	while(fscanf(fp,"%s",tmp) != EOF){
		pipes[minernum].input_pipe = (char *)malloc(PATH_MAX+1);
		pipes[minernum].output_pipe = (char *)malloc(PATH_MAX+1);
		fscanf(fp,"%s%s",pipes[minernum].input_pipe,pipes[minernum].output_pipe);
		minernum++;
	}
	
	strcat(mine,mtmp);
    config->mine_file = mine; /* path to mine file */
    config->pipes =  pipes; /* array of pipe pairs */
    config->num_miners = minernum; /* number of miners */
	fclose(fp);
    return 0;
}

int link_start(struct fd_pair* miners, int minernum){
	struct todo Todo;
    for(int i = 0; i < minernum; i++){
    	Todo.Head.op = START;
    	strcpy(Todo.Head.password,"B05902029");
    	if(write(miners[i].input_fd,&Todo,sizeof(struct todo)) < 0){
    		fprintf(stderr,"start miner #%d error\n",i);
			continue;
		}
	} 
}

int assign_jobs(struct fd_pair* miners, int minernum,int charnum,char* mine_path,int zeronum) 
{
    /* TODO design your own (1) communication protocol (2) job assignment algorithm */
    fprintf(stderr,"assigning job to find %d-treasure in %d chars...\n",zeronum,charnum);
    struct todo Todo;
    for(int i = 0; i < minernum; i++){
    	Todo.Head.op = MINE;
    	strcpy(Todo.Head.password,"B05902029");
    	Todo.charnum = charnum;
    	Todo.minerid = i;
    	Todo.minernum = minernum;
    	strcpy(Todo.path,mine_path);
    	Todo.zeronum = zeronum;
    	if(write(miners[i].input_fd,&Todo,sizeof(struct todo)) < 0){
    		fprintf(stderr,"send miner #%d error\n",i);
			continue;
		}
	} 
}
  
int main(int argc, char **argv)
{
    /* sanity check on arguments */
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s CONFIG_FILE\n", argv[0]);
        exit(1);
    }

    /* load config file */
    struct server_config config;
    load_config_file(&config, argv[1]);

    /* open the named pipes */
    struct fd_pair client_fds[config.num_miners];
	
    for (int ind = 0; ind < config.num_miners; ind += 1)
    {
        struct fd_pair *fd_ptr = &client_fds[ind];
        struct pipe_pair *pipe_ptr = &config.pipes[ind];

        fd_ptr->input_fd = open(pipe_ptr->input_pipe, O_WRONLY);
        assert (fd_ptr->input_fd >= 0);

        fd_ptr->output_fd = open(pipe_ptr->output_pipe, O_RDONLY);
        assert (fd_ptr->output_fd >= 0);
    }
	int minefd = open(config.mine_file,O_WRONLY|O_APPEND);
	
    /* initialize data for select() */
    int maxfd = 0;
    fd_set readset;
    fd_set working_readset;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    FD_ZERO(&readset);
    FD_SET(STDIN_FILENO, &readset);
    // TODO add input pipes to readset, setup maxfd
    for(int i = 0; i < config.num_miners; i++){
    	FD_SET(client_fds[i].output_fd,&readset);
    	maxfd = (maxfd > client_fds[i].output_fd)? maxfd : client_fds[i].output_fd; 
	}
	maxfd++;
    /* assign jobs to clients */
	int treasurenum = 0, charnum = 1, appendlen = 0;
	int find = 0, notfindset = 0,mask = (1 << config.num_miners)-1;
	char append[100],best[33] = "";
	fprintf(stderr,"start linking miners...\n");
    link_start(client_fds,config.num_miners);
    assign_jobs(client_fds,config.num_miners,charnum,config.mine_file,treasurenum+1);

    while (1)
    {
        memcpy(&working_readset, &readset, sizeof(readset)); // why we need memcpy() here?
        select(maxfd, &working_readset, NULL, NULL, &timeout);

        if (FD_ISSET(STDIN_FILENO, &working_readset))
        {
            /* TODO handle user input here */
            char cmd[7];
			scanf("%s",cmd);
            if (strcmp(cmd, "status") == 0){
            	struct todo status;
            	status.Head.op = STATUS;
            	strcpy(status.Head.password,"B05902029");
            	for(int i = 0; i < config.num_miners; i++){
            		write(client_fds[i].input_fd,&status,sizeof(struct todo));
				}
    			printf("best %d-treasure %s in %d bytes\n",treasurenum,best,appendlen);
			}
    		else if (strcmp(cmd, "dump") == 0)
   			{
   				char path[PATH_MAX+1];
   				scanf("%s",path);
   				int dumpfd = open(path,O_WRONLY|O_NONBLOCK|O_TRUNC);
   				/*fd_set writeset,readset2;
   				FD_ZERO(&writeset);
   				FD_ZERO(&readset2);
   				FD_SET(dumpfd,&writeset);
   				FD_SET(minefd,&readset);
   				int max = (dumpfd > minefd)? dumpfd,minefd; 
   				select(max)*/
				write(dumpfd,best,32);
    		}
    		else if(strcmp(cmd, "quit") == 0)
    		{
        		struct todo quit;
        		quit.Head.op = QUIT;
        		strcpy(quit.Head.password,"B05902029");
        		for(int i = 0; i < config.num_miners; i++){
            		write(client_fds[i].input_fd,&quit,sizeof(struct todo));
				}
        		break;
        	/* TODO tell clients to cease their jobs and exit normally */
    		}
    		else{
    			fprintf(stderr,"wrong command\n");
			}
        }
        int fminer;
		struct report Report;
		for(int i = 0; i < config.num_miners && !find && notfindset < mask; i++){
			if(FD_ISSET(client_fds[i].output_fd,&working_readset)){
				//fprintf(stderr, "recv from pipe #%d...\n",i);
				if(read(client_fds[i] .output_fd,&Report,sizeof(struct report))<0){
					fprintf(stderr,"recv miner #%d error\n",i);
					continue;
				}
				if(strcmp(Report.Head.password,PASSWORD) != 0){
					fprintf(stderr,"password from pipe #%d is wrong\n");
					continue;
				}
				if(Report.find == treasurenum+1){
					find = 1;
					fminer = i;
				}
				else if(Report.find > 0 && Report.find < treasurenum+1) 
					fprintf(stderr, "%s are a little bit late\n",Report.name);
				else{
					fprintf(stderr,"%s didn't find %d-treasure\n",Report.name,treasurenum+1);
					notfindset += (1 << i);
				}
			}
		}
		if(find){
			treasurenum++;
			find = 0;
			notfindset = 0;
			charnum = 1;
			memcpy(append+appendlen,Report.append,Report.len);
			appendlen += Report.len;
			write(minefd,Report.append,Report.len);
			
			strcpy(best,Report.result);
			struct todo stop;
			stop.Head.op = STOP;
			strcpy(stop.Head.password , "B05902029");
			stop.zeronum = treasurenum;
			strcpy(stop.path,Report.name);
			strcpy(stop.message,Report.result);
			for(int i = 0; i < config.num_miners; i++){
				if(i == fminer)
					continue;
				write(client_fds[i].input_fd,&stop,sizeof(struct todo));
			}
			fprintf(stderr,"%s: ",Report.name);
			printf("A %d-treasure discovered! %s\n",treasurenum,Report.result);
			assign_jobs(client_fds,config.num_miners,charnum,config.mine_file,treasurenum+1);
		}
		else if(notfindset == mask){
			fprintf(stderr,"charnum = %d not found\n",charnum);
			notfindset = 0;
			charnum++;
			assign_jobs(client_fds,config.num_miners,charnum,config.mine_file,treasurenum+1);
		}
    }
    /* TODO close file descriptors */
	close(minefd);
	for(int i = 0; i < config.num_miners; i++){
		close(client_fds[i].input_fd);
		close(client_fds[i].output_fd);
	}

    return 0;
}
