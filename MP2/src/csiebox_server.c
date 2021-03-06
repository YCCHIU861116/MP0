#include "csiebox_server.h"

#include "csiebox_common.h"
#include "connect.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 
#include <errno.h>

#define DATABUF_SIZE 4000
#define PATHLEN_SIZE 301

static int parse_arg(csiebox_server* server, int argc, char** argv);
static void handle_request(csiebox_server* server, int conn_fd);
static int get_account_info(
  csiebox_server* server,  const char* user, csiebox_account_info* info);
static void* login(
  csiebox_server* server, int conn_fd, csiebox_protocol_login* login);
static void logout(csiebox_server* server, int conn_fd);
static char* get_user_homedir(
  csiebox_server* server, csiebox_client_info* info);

#define DIR_S_FLAG (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)//permission you can use to create new file
#define REG_S_FLAG (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)//permission you can use to create new directory

//read config file, and start to listen
void csiebox_server_init(
  csiebox_server** server, int argc, char** argv) {
  csiebox_server* tmp = (csiebox_server*)malloc(sizeof(csiebox_server));
  if (!tmp) {
    fprintf(stderr, "server malloc fail\n");
    return;
  }
  memset(tmp, 0, sizeof(csiebox_server));
  if (!parse_arg(tmp, argc, argv)) {
    fprintf(stderr, "Usage: %s [config file]\n", argv[0]);
    free(tmp);
    return;
  }
  int fd = server_start();
  if (fd < 0) {
    fprintf(stderr, "server fail\n");
    free(tmp);
    return;
  }
  tmp->client = (csiebox_client_info**)
      malloc(sizeof(csiebox_client_info*) * getdtablesize());
  if (!tmp->client) {
    fprintf(stderr, "client list malloc fail\n");
    close(fd);
    free(tmp);
    return;
  }
  memset(tmp->client, 0, sizeof(csiebox_client_info*) * getdtablesize());
  tmp->listen_fd = fd;
  *server = tmp;
}

//wait client to connect and handle requests from connected socket fd
int csiebox_server_run(csiebox_server* server) {
  int conn_fd, conn_len;
  struct sockaddr_in addr;
  while (1) {
    memset(&addr, 0, sizeof(addr));
    conn_len = 0;
    // waiting client connect
    conn_fd = accept(
      server->listen_fd, (struct sockaddr*)&addr, (socklen_t*)&conn_len);
    if (conn_fd < 0) {
      if (errno == ENFILE) {
          fprintf(stderr, "out of file descriptor table\n");
          continue;
        } else if (errno == EAGAIN || errno == EINTR) {
          continue;
        } else {
          fprintf(stderr, "accept err\n");
          fprintf(stderr, "code: %s\n", strerror(errno));
          break;
        }
    }
    // handle request from connected socket fd
    handle_request(server, conn_fd);
  }
  return 1;
}

void csiebox_server_destroy(csiebox_server** server) {
  csiebox_server* tmp = *server;
  *server = 0;
  if (!tmp) {
    return;
  }
  close(tmp->listen_fd);
  int i = getdtablesize() - 1;
  for (; i >= 0; --i) {
    if (tmp->client[i]) {
      free(tmp->client[i]);
    }
  }
  free(tmp->client);
  free(tmp);
}

//read config file
static int parse_arg(csiebox_server* server, int argc, char** argv) {
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
  int accept_config_total = 2;
  int accept_config[2] = {0, 0};
  while ((keylen = getdelim(&key, &keysize, '=', file) - 1) > 0) {
    key[keylen] = '\0';
    vallen = getline(&val, &valsize, file) - 1;
    val[vallen] = '\0';
    fprintf(stderr, "config (%d, %s)=(%d, %s)\n", keylen, key, vallen, val);
    if (strcmp("path", key) == 0) {
      if (vallen <= sizeof(server->arg.path)) {
        strncpy(server->arg.path, val, vallen);
        accept_config[0] = 1;
      }
    } else if (strcmp("account_path", key) == 0) {
      if (vallen <= sizeof(server->arg.account_path)) {
        strncpy(server->arg.account_path, val, vallen);
        accept_config[1] = 1;
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

//It is a sample function
//you may remove it after understanding
int sampleFunction(int conn_fd, csiebox_protocol_meta* meta) {
  
  printf("In sampleFunction:\n");
  uint8_t hash[MD5_DIGEST_LENGTH];
  memset(&hash, 0, sizeof(hash));
  md5_file(".gitignore", hash);
  printf("pathlen: %d\n", meta->message.body.pathlen);
  if (memcmp(hash, meta->message.body.hash, sizeof(hash)) == 0) {
    printf("hashes are equal!\n");
  }

  //use the pathlen from client to recv path 
  char buf[400];
  memset(buf, 0, sizeof(buf));
  recv_message(conn_fd, buf, meta->message.body.pathlen);
  printf("This is the path from client:%s\n", buf);

  //send OK to client
  csiebox_protocol_header header;
  memset(&header, 0, sizeof(header));
  header.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
  header.res.op = CSIEBOX_PROTOCOL_OP_SYNC_META;
  header.res.datalen = 0;
  header.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
  if(!send_message(conn_fd, &header, sizeof(header))){
    fprintf(stderr, "send fail\n");
    return 0;
  }

  return 1;
}

int modify_time(char *filename, struct stat *stat){
	struct timeval new_times[2];
	new_times[0].tv_sec = (long)stat->st_atime;
	new_times[0].tv_usec = 0;
	new_times[1].tv_sec = (long)stat->st_mtime;
	new_times[1].tv_usec = 0;
	return lutimes(filename,new_times);
}

int send_response(int conn_fd, uint8_t op, uint8_t status){
	csiebox_protocol_header header;
	memset(&header, 0, sizeof(header));
	header.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
	header.res.op = op;
	header.res.datalen = 0;
	header.res.status = status;
	if(!send_message(conn_fd, &header, sizeof(header))){
    	fprintf(stderr, "send fail\n");
    	return 0;
	}
	return 1;
}

static char* sync_meta(int conn_fd, csiebox_protocol_meta *meta, char* homedir){
	char cpathname[PATHLEN_SIZE];
	char *spathname = (char*)malloc(sizeof(char) * PATHLEN_SIZE);
	recv_message(conn_fd,cpathname,meta->message.body.pathlen);
	sprintf(spathname,"%s%s",homedir,cpathname);
	fprintf(stderr,"processing meta of %s\n",spathname);
	struct stat parentstat;
	uint8_t hash[MD5_DIGEST_LENGTH];
	memset(&hash, 0, sizeof(hash));
	
	if(S_ISDIR(meta->message.body.stat.st_mode)){
		if(access(spathname,F_OK) == -1){
			spathname[strrchr(spathname,'/')-spathname] = '\0';
			lstat(spathname,&parentstat);
			spathname[strlen(spathname)] = '/';
			mkdir(spathname,meta->message.body.stat.st_mode);
			spathname[strrchr(spathname,'/')-spathname] = '\0';
			modify_time(spathname,&parentstat);
			spathname[strlen(spathname)] = '/';
		}
		if(chmod(spathname,meta->message.body.stat.st_mode) == -1)
			fprintf(stderr,"chmod error\n");
		if(modify_time(spathname,&meta->message.body.stat) == -1)
			fprintf(stderr,"time change error\n");
		send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_META,CSIEBOX_PROTOCOL_STATUS_OK);
	}
	else if(S_ISLNK(meta->message.body.stat.st_mode)){
		ssize_t linklen;
		char buf[PATHLEN_SIZE];
		if((linklen = readlink(spathname,buf,PATHLEN_SIZE)) == -1){
			spathname[strrchr(spathname,'/')-spathname] = '\0';
			lstat(spathname,&parentstat);
			spathname[strlen(spathname)] = '/';
			fprintf(stderr,"reading symlink of %s \n", spathname);
			if(errno != ENOENT){ 
				fprintf(stderr,"readlink %s fail\n", spathname);
				return spathname;
			} 
			if(symlink("anything",spathname) == -1)
				fprintf(stderr,"make symlink fail\n");
			if(modify_time(spathname,&meta->message.body.stat) == -1)
				fprintf(stderr,"time change error\n");
			spathname[strrchr(spathname,'/')-spathname] = '\0';
			modify_time(spathname,&parentstat);
			spathname[strlen(spathname)] = '/';
			send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_META,CSIEBOX_PROTOCOL_STATUS_MORE);
		}
		else{
			md5(buf,linklen,hash);
			if(modify_time(spathname,&meta->message.body.stat) == -1)
				fprintf(stderr,"time change error\n");
			if(memcmp(hash, meta->message.body.hash, sizeof(hash)))
				send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_META,CSIEBOX_PROTOCOL_STATUS_MORE);
			else
				send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_META,CSIEBOX_PROTOCOL_STATUS_OK);
		}
	}
	else if(access(spathname,F_OK) == -1){
		spathname[strrchr(spathname,'/')-spathname] = '\0';
		lstat(spathname,&parentstat);
		spathname[strlen(spathname)] = '/';
		int fd;
		if((fd = open(spathname,O_WRONLY|O_CREAT)) == -1){//change mtime of parent dir
			fprintf(stderr,"fail while creating file\n");
			send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_META,CSIEBOX_PROTOCOL_STATUS_FAIL);
			return spathname;
		}
		close(fd);
		spathname[strrchr(spathname,'/')-spathname] = '\0';
		modify_time(spathname,&parentstat);
		spathname[strlen(spathname)] = '/';
		if(chmod(spathname,meta->message.body.stat.st_mode) == -1)
			fprintf(stderr,"chmod error\n");
		if(modify_time(spathname,&meta->message.body.stat) == -1)
			fprintf(stderr,"time change error\n");
		send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_META,CSIEBOX_PROTOCOL_STATUS_MORE);
	}	
	else{
		md5_file(spathname, hash);
		if(chmod(spathname,meta->message.body.stat.st_mode) == -1)
			fprintf(stderr,"chmod error\n");
		if(modify_time(spathname,&meta->message.body.stat) == -1)
			fprintf(stderr,"time change error\n");
		if (memcmp(hash, meta->message.body.hash, sizeof(hash)))
			send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_META,CSIEBOX_PROTOCOL_STATUS_MORE);
		else
			send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_META,CSIEBOX_PROTOCOL_STATUS_OK);
	}
	return spathname;
}

static void sync_file(int conn_fd, csiebox_protocol_file *file,char *pathname){
	char databuf[DATABUF_SIZE];
	struct stat statbuf,parentstat;
	lstat(pathname,&statbuf);
	recv_message(conn_fd,databuf,file->message.body.datalen);
	fprintf(stderr,"processing data of %s\n",pathname);
	if(S_ISLNK(statbuf.st_mode)){
		pathname[strrchr(pathname,'/')-pathname] = '\0';
		lstat(pathname,&parentstat);
		pathname[strlen(pathname)] = '/';
		unlink(pathname);
		databuf[file->message.body.datalen] = '\0';
		if(symlink(databuf,pathname) == -1){
			fprintf(stderr,"symlink %s -> %s error\n",pathname,databuf);
			send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_FILE,CSIEBOX_PROTOCOL_STATUS_FAIL);
			return;
		}
		pathname[strrchr(pathname,'/')-pathname] = '\0';
		modify_time(pathname,&parentstat);
		pathname[strlen(pathname)] = '/';
	}
	else{
		int fd;
		if((fd = open(pathname,O_WRONLY|O_TRUNC)) < 0){
			fprintf(stderr,"%s open fail\n",pathname);
			send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_FILE,CSIEBOX_PROTOCOL_STATUS_FAIL);
			return;
		}
		if(write(fd,databuf,file->message.body.datalen) < file->message.body.datalen){
			fprintf(stderr,"%s write fail\n",pathname);
			send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_FILE,CSIEBOX_PROTOCOL_STATUS_FAIL);
			return;
		}
	}
	if(modify_time(pathname,&statbuf) == -1)
		fprintf(stderr,"time change error\n");
	send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_FILE,CSIEBOX_PROTOCOL_STATUS_OK);
}

static void sync_hardlink(int conn_fd, csiebox_protocol_hardlink *hardlink, char* homedir){
	char combinedname[PATHLEN_SIZE*2];
	char source[PATHLEN_SIZE],target[PATHLEN_SIZE];
	struct stat parentstat;
	recv_message(conn_fd,combinedname,hardlink->message.body.srclen+hardlink->message.body.targetlen+1);
	sprintf(target,"%s%s",homedir,combinedname+hardlink->message.body.srclen);
	combinedname[hardlink->message.body.srclen] = '\0';
	sprintf(source,"%s%s",homedir,combinedname);
	fprintf(stderr,"processing hardlink %s -> %s\n",source,target);
	
	source[strrchr(source,'/')-source] = '\0';
	lstat(source,&parentstat);
	source[strlen(source)] = '/';
	if(link(target,source) == -1){
		fprintf(stderr,"link fail\n");
		send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_HARDLINK,CSIEBOX_PROTOCOL_STATUS_FAIL);
		return;
	}
	source[strrchr(source,'/')-source] = '\0';
	modify_time(source,&parentstat);
	source[strlen(source)] = '/';
	send_response(conn_fd,CSIEBOX_PROTOCOL_OP_SYNC_HARDLINK,CSIEBOX_PROTOCOL_STATUS_OK);
}

//this is where the server handle requests, you should write your code here
static void handle_request(csiebox_server* server, int conn_fd) {
	char *pathname = NULL,homedir[PATH_MAX];
  csiebox_protocol_header header;
  memset(&header, 0, sizeof(header));
  while (recv_message(conn_fd, &header, sizeof(header))) {
    if (header.req.magic != CSIEBOX_PROTOCOL_MAGIC_REQ) {
      continue;
    }
    switch (header.req.op) {
      case CSIEBOX_PROTOCOL_OP_LOGIN:
        fprintf(stderr, "login\n");
        csiebox_protocol_login req;
        if (complete_message_with_header(conn_fd, &header, &req)) {
			login(server, conn_fd, &req);
			sprintf(homedir,"%s/%s",server->arg.path,server->client[conn_fd]->account.user);
        }
        break;
      case CSIEBOX_PROTOCOL_OP_SYNC_META:
        fprintf(stderr, "sync meta\n");
        csiebox_protocol_meta meta;
        if(pathname != NULL)
        	free(pathname);
        if (complete_message_with_header(conn_fd, &header, &meta)) {
        	pathname = sync_meta(conn_fd, &meta, homedir);
        }
        break;
      case CSIEBOX_PROTOCOL_OP_SYNC_FILE:
        fprintf(stderr, "sync file\n");
        csiebox_protocol_file file;
        if (complete_message_with_header(conn_fd, &header, &file)) {
        	sync_file(conn_fd, &file, pathname);
        }
        break;
      case CSIEBOX_PROTOCOL_OP_SYNC_HARDLINK:
        fprintf(stderr, "sync hardlink\n");
        csiebox_protocol_hardlink hardlink;
        if (complete_message_with_header(conn_fd, &header, &hardlink)) {
			sync_hardlink(conn_fd,&hardlink,homedir);
        }
        break;
      case CSIEBOX_PROTOCOL_OP_SYNC_END:
        fprintf(stderr, "sync end\n");
        csiebox_protocol_header end;
          //====================
          //        TODO
          //====================
        break;
      case CSIEBOX_PROTOCOL_OP_RM:
        fprintf(stderr, "rm\n");
        csiebox_protocol_rm rm;
        if (complete_message_with_header(conn_fd, &header, &rm)) {
          //====================
          //        TODO
          //====================
        }
        break;
      default:
        fprintf(stderr, "unknown op %x\n", header.req.op);
        break;
    }
  }
  fprintf(stderr, "end of connection\n");
  logout(server, conn_fd);
}

//open account file to get account information
static int get_account_info(
  csiebox_server* server,  const char* user, csiebox_account_info* info) {
  FILE* file = fopen(server->arg.account_path, "r");
  if (!file) {
    return 0;
  }
  size_t buflen = 100;
  char* buf = (char*)malloc(sizeof(char) * buflen);
  memset(buf, 0, buflen);
  ssize_t len;
  int ret = 0;
  int line = 0;
  while ((len = getline(&buf, &buflen, file) - 1) > 0) {
    ++line;
    buf[len] = '\0';
    char* u = strtok(buf, ",");
    if (!u) {
      fprintf(stderr, "illegal form in account file, line %d\n", line);
      continue;
    }
    if (strcmp(user, u) == 0) {
      memcpy(info->user, user, strlen(user));
      char* passwd = strtok(NULL, ",");
      if (!passwd) {
        fprintf(stderr, "illegal form in account file, line %d\n", line);
        continue;
      }
      md5(passwd, strlen(passwd), info->passwd_hash);
      ret = 1;
      break;
    }
  }
  free(buf);
  fclose(file);
  return ret;
}

//handle the login request from client
static void *login(
  csiebox_server* server, int conn_fd, csiebox_protocol_login* login) {
  int succ = 1;
  csiebox_client_info* info =
    (csiebox_client_info*)malloc(sizeof(csiebox_client_info));
  memset(info, 0, sizeof(csiebox_client_info));
  if (!get_account_info(server, login->message.body.user, &(info->account))) {
    fprintf(stderr, "cannot find account\n");
    succ = 0;
  }
  if (succ &&
      memcmp(login->message.body.passwd_hash,
             info->account.passwd_hash,
             MD5_DIGEST_LENGTH) != 0) {
    fprintf(stderr, "passwd miss match\n");
    succ = 0;
  }

  csiebox_protocol_header header;
  memset(&header, 0, sizeof(header));
  header.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
  header.res.op = CSIEBOX_PROTOCOL_OP_LOGIN;
  header.res.datalen = 0;
  if (succ) {
    if (server->client[conn_fd]) {
      free(server->client[conn_fd]);
    }
    info->conn_fd = conn_fd;
    server->client[conn_fd] = info;
    header.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
    header.res.client_id = info->conn_fd;
    char* homedir = get_user_homedir(server, info);
    mkdir(homedir, DIR_S_FLAG);
    free(homedir);
  } else {
    header.res.status = CSIEBOX_PROTOCOL_STATUS_FAIL;
    free(info);
  }
  send_message(conn_fd, &header, sizeof(header));
}

static void logout(csiebox_server* server, int conn_fd) {
  free(server->client[conn_fd]);
  server->client[conn_fd] = 0;
  close(conn_fd);
}

static char* get_user_homedir(
  csiebox_server* server, csiebox_client_info* info) {
  char* ret = (char*)malloc(sizeof(char) * PATH_MAX);
  memset(ret, 0, PATH_MAX);
  sprintf(ret, "%s/%s", server->arg.path, info->account.user);
  return ret;
}

