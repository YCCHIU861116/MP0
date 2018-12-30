#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include <fcntl.h>
#include <bsd/md5.h>
void md5(const char* str, size_t len, uint8_t digest[MD5_DIGEST_LENGTH]) {
  char result[33];
  MD5_CTX ctx;
  MD5Init(&ctx);
  MD5Update(&ctx, (const uint8_t*)str, len);
  //MD5Final(digest, &ctx);
  MD5End(&ctx,result);
  fprintf(stderr,"%s\n",result);
}
int Update(MD5_CTX ctx, char *buf, int len, unsigned char c){
	uint8_t digest[MD5_DIGEST_LENGTH];
	char result[33];
	buf[len] = c;
	len++;
	buf[len] = '\0';
	MD5Update(&ctx, (const unsigned char*)buf, len);
	MD5Final(digest, &ctx);
	MD5End(&ctx,result);
	fprintf(stderr,"%s\n",result);
	return 0;
}
int main()
{
	char path[50] = "input2.txt";
	int fd = open(path, O_RDONLY);
  	if (fd < 0) {
 	  return 0;
 	}
 	char buf[4096] = "RRRRRRRRR",buf2[4096] = "B05902029a";
	uint8_t digest[MD5_DIGEST_LENGTH];
 	md5(buf,9,digest);
 	md5(buf2,10,digest);
 	/*size_t len = 10;
 	MD5_CTX ctx;
 	MD5Init(&ctx);
	char result[33];
	//buf[9] = (char)122;
	MD5Update(&ctx, (const uint8_t*)buf, 9);
	MD5Final(digest, &ctx);
	MD5End(&ctx,result);
	fprintf(stderr,"%s\n",result);*/
 	/*for(int i = 0; i < 256; i++) {
 		//Update(ctx,buf,len,(unsigned char)i);	uint8_t digest[MD5_DIGEST_LENGTH];
		uint8_t digest[MD5_DIGEST_LENGTH];
		char result[33];
		buf[10] = (char)i;
		MD5Update(&ctx, (const unsigned char*)buf, len);
		MD5Final(digest, &ctx);
		MD5End(&ctx,result);
		fprintf(stderr,"%s\n",result);
	}*/
	close(fd);
	return 0;
}
