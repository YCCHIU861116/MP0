#include<limits.h>
#define START 0x00
#define MINE 0x01
#define STOP 0x02
#define STATUS 0x03
#define QUIT 0x04

struct head{
	char password[10];
	unsigned char op;
};
struct todo{
	struct head Head;
	int zeronum;
	int charnum;
	unsigned char minerid;  // 0 ~ minernum-1
	unsigned char minernum;
	char path[PATH_MAX+1];
	char message[1000];
};
struct report{
	struct head Head;
	char name[1025];
	unsigned char find; 
	char append[32];
	unsigned char len;
	char result[33];
};
