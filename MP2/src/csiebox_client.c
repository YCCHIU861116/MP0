#include "csiebox_client.h"

#include "csiebox_common.h"
#include "connect.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DATABUF_SIZE 4000
#define PATHLEN_SIZE 301
#define FILENUM_MAX 300

struct nameino{
	ino_t inodenum;
	char filename[PATHLEN_SIZE];
};

static int parse_arg(csiebox_client* client, int argc, char** argv);
static int login(csiebox_client* client);

//read config file, and connect to server
void csiebox_client_init(
  csiebox_client** client, int argc, char** argv) {
  csiebox_client* tmp = (csiebox_client*)malloc(sizeof(csiebox_client));
  if (!tmp) {
    fprintf(stderr, "client malloc fail\n");
    return;
  }
  memset(tmp, 0, sizeof(csiebox_client));
  if (!parse_arg(tmp, argc, argv)) {
    fprintf(stderr, "Usage: %s [config file]\n", argv[0]);
    free(tmp);
    return;
  }
  int fd = client_start(tmp->arg.name, tmp->arg.server);
  if (fd < 0) {
    fprintf(stderr, "connect fail\n");
    free(tmp);
    return;
  }
  tmp->conn_fd = fd;
  *client = tmp;
}

//show how to use csiebox_protocol_meta header
//other headers is similar usage
//please check out include/common.h
//using .gitignore for example only for convenience  
int sampleFunction(csiebox_client* client){
  csiebox_protocol_meta req;
  memset(&req, 0, sizeof(req));
  req.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
  req.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_META;
  req.message.header.req.datalen = sizeof(req) - sizeof(req.message.header);
  char path[] = "just/a/testing/path";
  req.message.body.pathlen = strlen(path);

  //just show how to use these function
  //Since there is no file at "just/a/testing/path"
  //I use ".gitignore" to replace with
  //In fact, it should be 
  //lstat(path, &req.message.body.stat);
  //md5_file(path, req.message.body.hash);
  lstat(".gitignore", &req.message.body.stat);
  md5_file(".gitignore", req.message.body.hash);


  //send pathlen to server so that server can know how many charachers it should receive
  //Please go to check the samplefunction in server
  if (!send_message(client->conn_fd, &req, sizeof(req))) {
    fprintf(stderr, "send fail\n");
    return 0;
  }

  //send path
  send_message(client->conn_fd, path, strlen(path));

  //receive csiebox_protocol_header from server
  csiebox_protocol_header header;
  memset(&header, 0, sizeof(header));
  if (recv_message(client->conn_fd, &header, sizeof(header))) {
    if (header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
        header.res.op == CSIEBOX_PROTOCOL_OP_SYNC_META &&
        header.res.status == CSIEBOX_PROTOCOL_STATUS_OK) {
      printf("Receive OK from server\n");
      return 1;
    } else {
      return 0;
    }
  }
  return 0;
}

int modify_time(char *filename, struct stat *stat){
	struct timeval new_times[2];
	new_times[0].tv_sec = (long)stat->st_atime;
	new_times[0].tv_usec = 0;
	new_times[1].tv_sec = (long)stat->st_mtime;
	new_times[1].tv_usec = 0;
	return lutimes(filename,new_times);
}

int recv_response(int conn_fd,uint8_t flag){
	csiebox_protocol_header header;
	memset(&header, 0, sizeof(header));
	if(recv_message(conn_fd, &header, sizeof(header))) {
		if(header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES && header.res.op == flag ) {
			if(header.res.status == CSIEBOX_PROTOCOL_STATUS_OK){
    			fprintf(stderr,"Receive OK from server\n");
    			return 1;
			}
			else if(header.res.status == CSIEBOX_PROTOCOL_STATUS_MORE){
    			fprintf(stderr,"Receive MORE from server\n");
    			return 2;
			}
			else{
    			fprintf(stderr,"Receive FAIL from server\n");
    			return 0;
			}
    	} else {
    		return 0;
    	}
	}
	return 0;
}

int sync_meta(char *pathname, int conn_fd, struct stat *stat,int cdirlen){
	csiebox_protocol_meta req;
	memset(&req, 0, sizeof(req));
	req.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
	req.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_META;
	req.message.header.req.datalen = sizeof(req) - sizeof(req.message.header);
	req.message.body.pathlen = strlen(pathname)+1-cdirlen;
	req.message.body.stat = *stat;
	if(S_ISDIR(stat->st_mode) == 0){
		if(S_ISLNK(stat->st_mode)){
			char buf[PATHLEN_SIZE];
			ssize_t len = readlink(pathname,buf,DATABUF_SIZE);
			md5(buf,len,req.message.body.hash);
		}
		else
			md5_file(pathname,req.message.body.hash);
	}
	fprintf(stderr,"sending meta of %s\n",pathname);
	if(send_message(conn_fd,&req,sizeof(req)) == 0){
		fprintf(stderr,"send error\n");
		return 0;
	}
	send_message(conn_fd, pathname+cdirlen, req.message.body.pathlen);
	return recv_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_META);
}

int sync_file(char *pathname,int conn_fd,struct stat *stat){
	csiebox_protocol_file req;
	memset(&req, 0, sizeof(req));
	req.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
	req.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
	req.message.header.req.datalen = sizeof(req) - sizeof(req.message.header);
	int fd;
	char buf[DATABUF_SIZE];
	if(S_ISLNK(stat->st_mode)){
		ssize_t tmp;
		if((tmp = readlink(pathname,buf,DATABUF_SIZE)) == -1){
			fprintf(stderr,"readlink %s fail\n",pathname);
			return 0;
		}
		buf[tmp] = '\0';
		fprintf(stderr,"softlink point to %s\n",buf);
		req.message.body.datalen = (size_t)tmp;
	}
	else{
		if((fd = open(pathname,O_RDONLY)) == -1){
			fprintf(stderr,"open %s fail\n",pathname);
			return 0;
		}
		req.message.body.datalen = (size_t)read(fd,buf,DATABUF_SIZE);
	}
	if(send_message(conn_fd,&req,sizeof(req)) == 0){
		fprintf(stderr,"send fail\n");
		return 0;
	}
	fprintf(stderr,"sending data of %s\n",pathname);
	send_message(conn_fd,buf,req.message.body.datalen);
	return recv_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_FILE);
}

char *searchhardlink(ino_t inodenum,struct nameino *inodelist, int *inodesum){
	for(int i = 0; i < (*inodesum); i++){
		if(inodenum == inodelist[i].inodenum)
			return inodelist[i].filename;
	}
	return NULL;
}

int sync_hardlink(char *target, char *source, int conn_fd,int cdirlen){
	csiebox_protocol_hardlink req;
	memset(&req, 0, sizeof(req));
	req.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
	req.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_HARDLINK;
	req.message.header.req.datalen = sizeof(req) - sizeof(req.message.header);
	req.message.body.srclen = strlen(source+cdirlen);
	req.message.body.targetlen = strlen(target+cdirlen);
	if(send_message(conn_fd,&req,sizeof(req)) == 0){
		fprintf(stderr,"send fail\n");
		return 0;
	}
	fprintf(stderr,"sending hardlink %s -> %s\n",source,target);
	strcat(source,target+cdirlen);
	send_message(conn_fd,source+cdirlen,req.message.body.srclen+req.message.body.targetlen+1);
	source[req.message.body.srclen] = '\0';
	return recv_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_HARDLINK);
} 

int cdir_traverse(char *path,int cdirlen,int conn_fd,int *maxdepth, 
		char* longestpath,struct nameino *inodelist,int *inodesum){
	int len = strlen(path);
	struct stat *statbuf = (struct stat*)malloc(sizeof(struct stat));
	if(lstat(path,statbuf) <0){
		fprintf(stderr,"%s lstat error\n",path);
		return 0;
	}
	char *target;
	if((S_ISDIR(statbuf->st_mode) == 0)&&((target = searchhardlink(statbuf->st_ino,inodelist,inodesum)) != NULL))
		sync_hardlink(target,path,conn_fd,cdirlen);
	else{
		if(S_ISDIR(statbuf->st_mode) == 0){
			inodelist[*inodesum].inodenum = statbuf->st_ino;
			strcpy(inodelist[*inodesum].filename,path);
			(*inodesum)++;
		}
		if(sync_meta(path,conn_fd,statbuf,cdirlen) == 2)
			sync_file(path,conn_fd,statbuf);
	}
	
	//dealing with deepestpath	
	int depthsum = 0;
	char *ptr = path;
	while((ptr = strchr(ptr,'/')) != NULL){
		ptr++;
		depthsum++;
	} 
	if(depthsum > *maxdepth){
		fprintf(stderr,"deeper path (%d, %s) ,opening longestPath.txt...\n",depthsum,path);
		int fd = open(longestpath,O_WRONLY|O_CREAT|O_TRUNC);
		if(fd < 0){
			fprintf(stderr,"open longestPath.txt fail\n");
			return 0; 
		} 
		fchmod(fd,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
		write(fd,path,strlen(path));
		close(fd); 
		*maxdepth = depthsum;
	}
		
	if(S_ISDIR(statbuf->st_mode)){
		struct dirent *dp;
		DIR *dir = opendir(path);
		if(dir == NULL){
			fprintf(stderr,"opendir error\n");
			return 0;
		}
		strcpy(path+len,"/");
		while ((dp = readdir(dir)) != NULL) {
			if(!(strcmp(dp->d_name,".")) || !(strcmp(dp->d_name,"..")))
				continue;
			strcpy(path+len+1,dp->d_name);
			if(!(strcmp(path,longestpath)))
				continue;
			if(cdir_traverse(path,cdirlen,conn_fd,maxdepth,longestpath,inodelist,inodesum) == 0)
				return 0;
		} 
		closedir(dir);
	}
	path[len] = '\0';
	modify_time(path,statbuf);
	//sync_meta(path,conn_fd,statbuf);
	free(statbuf);
	return 1;
}

//this is where client sends request, you sould write your code here
int csiebox_client_run(csiebox_client* client) {
  if (!login(client)) {
    fprintf(stderr, "login fail\n");
    return 0;
  }
  fprintf(stderr, "login success\n");
  

  //This is a sample function showing how to send data using defined header in common.h
  //You can remove it after you understand
  //sampleFunction(client);
  int maxdepth = 0,inodesum = 0;
  char pathname[PATHLEN_SIZE],longestpath[51];
  strcpy(pathname,client->arg.path);
  strcpy(longestpath,pathname);
  strcat(longestpath,"/longestPath.txt");
  struct nameino inodelist[FILENUM_MAX];
  
  cdir_traverse(pathname,strlen(client->arg.path),client->conn_fd,&maxdepth,longestpath,inodelist,&inodesum);

  struct stat statbuf;
  lstat(longestpath,&statbuf);
  if(sync_meta(longestpath,client->conn_fd,&statbuf,strlen(client->arg.path))==2);
	sync_file(longestpath,client->conn_fd,&statbuf);
  modify_time(longestpath,&statbuf);
  return 1;
}

void csiebox_client_destroy(csiebox_client** client) {
  csiebox_client* tmp = *client;
  *client = 0;
  if (!tmp) {
    return;
  }
  close(tmp->conn_fd);
  free(tmp);
}

//read config file
static int parse_arg(csiebox_client* client, int argc, char** argv) {
  if (argc != 2) {
    return 0;
  }
  FILE* file = fopen(argv[1], "r");
  if (!file) {
    return 0;
  }
  fprintf(stderr, "reading config...\n");
  size_t keysize = 20, valsize = 20;
  char* key = (char*)malloc(sizeof(char) * keysize);
  char* val = (char*)malloc(sizeof(char) * valsize);
  ssize_t keylen, vallen;
  int accept_config_total = 5;
  int accept_config[5] = {0, 0, 0, 0, 0};
  while ((keylen = getdelim(&key, &keysize, '=', file) - 1) > 0) {
    key[keylen] = '\0';
    vallen = getline(&val, &valsize, file) - 1;
    val[vallen] = '\0';
    fprintf(stderr, "config (%d, %s)=(%d, %s)\n", keylen, key, vallen, val);
    if (strcmp("name", key) == 0) {
      if (vallen <= sizeof(client->arg.name)) {
        strncpy(client->arg.name, val, vallen);
        accept_config[0] = 1;
      }
    } else if (strcmp("server", key) == 0) {
      if (vallen <= sizeof(client->arg.server)) {
        strncpy(client->arg.server, val, vallen);
        accept_config[1] = 1;
      }
    } else if (strcmp("user", key) == 0) {
      if (vallen <= sizeof(client->arg.user)) {
        strncpy(client->arg.user, val, vallen);
        accept_config[2] = 1;
      }
    } else if (strcmp("passwd", key) == 0) {
      if (vallen <= sizeof(client->arg.passwd)) {
        strncpy(client->arg.passwd, val, vallen);
        accept_config[3] = 1;
      }
    } else if (strcmp("path", key) == 0) {
      if (vallen <= sizeof(client->arg.path)) {
        strncpy(client->arg.path, val, vallen);
        accept_config[4] = 1;
      }
    }
  }
  free(key);
  free(val);
  fclose(file);
  int i, test = 1;
  for (i = 0; i < accept_config_total; ++i) {
    test = test & accept_config[i];
  }
  if (!test) {
    fprintf(stderr, "config error\n");
    return 0;
  }
  return 1;
}

static int login(csiebox_client* client) {
  csiebox_protocol_login req;
  memset(&req, 0, sizeof(req));
  req.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
  req.message.header.req.op = CSIEBOX_PROTOCOL_OP_LOGIN;
  req.message.header.req.datalen = sizeof(req) - sizeof(req.message.header);
  memcpy(req.message.body.user, client->arg.user, strlen(client->arg.user));
  md5(client->arg.passwd,
      strlen(client->arg.passwd),
      req.message.body.passwd_hash);
  if (!send_message(client->conn_fd, &req, sizeof(req))) {
    fprintf(stderr, "send fail\n");
    return 0;
  }
  csiebox_protocol_header header;
  memset(&header, 0, sizeof(header));
  if (recv_message(client->conn_fd, &header, sizeof(header))) {
    if (header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
        header.res.op == CSIEBOX_PROTOCOL_OP_LOGIN &&
        header.res.status == CSIEBOX_PROTOCOL_STATUS_OK) {
      client->client_id = header.res.client_id;
      return 1;
    } else {
      return 0;
    }
  }
  return 0;
}
