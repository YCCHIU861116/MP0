#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<math.h>
#include"list_file.h"
#include "md5.h"
#define NAME_MAXLENGTH 257 
#define BUF_SIZE 65536
#define MAX_FILE_NUM 1000
#define MAX_FILE_SIZE 100000000 
const char LR[NAME_MAXLENGTH] ="/.loser_record";
const char LC[NAME_MAXLENGTH] ="/.loser_config";
char buf[BUF_SIZE],msg[MAX_FILE_SIZE+1];
struct files{
	char name[NAME_MAXLENGTH];
	char MD5[33];
};

int compare(const void* data1, const void* data2){
	return strcmp(*(char**)data1,*(char**)data2);
}
void MD52(const char *file,const char *dir,char *md5){
	FILE* fp = fopen(".tmp1234tmp","w");
	fclose(fp);
	char command[10000];
	strcpy(command,"md5sum ");
	strcat(command,dir);
	strcat(command,"/");
	strcat(command,file);
	strcat(command," >> ");
	strcat(command,".tmp1234tmp");
	system(command);
	fp = fopen(".tmp1234tmp","r");
	fscanf(fp,"%s",md5);
	system ("rm -f .tmp1234tmp");
	return;
}
void MD5(const char *file,const char *dir,char *M){
	int j,k;
	char whole[NAME_MAXLENGTH],tmp[4][4][33];
 //   const char *msg = "The quick brown fox jumps over the lazy dog.";
 	strcpy(whole,dir);
	strcat(whole,"/");
 	strcat(whole,file);
//	printf("%s\n",whole);
 	FILE *fd = fopen(whole,"r");
 	int length = fread(msg,1,MAX_FILE_SIZE,fd);
	msg[length] = '\0';
	if(length > MAX_FILE_SIZE/2){
		fclose(fd);
		MD52(file,dir,M);
		return;
	}
    unsigned *d = md5(msg, strlen(msg));
    WBunion u;
 
    //printf("= 0x");
   	strcpy(M,"");
    for (j=0;j<4; j++){
        u.w = d[j];
        for (k=0;k<4;k++){
			sprintf(tmp[j][k],"%02x",u.b[k]);
			strcat(M,tmp[j][k]);
		} 
    }
//	printf("%s %s\n",msg,M);
	fclose(fd);
}
void Findchr(FILE* fp,char c){
	char *pound = NULL;
	//printf("%ld\n",ftell(fp));
	while(ftell(fp) > BUF_SIZE-1 && pound == NULL){
		fseek(fp,(1-BUF_SIZE),SEEK_CUR);
		fread(buf,sizeof(char),BUF_SIZE-1,fp);
		buf[BUF_SIZE-1] = '\0';
		pound = strrchr(buf,c);
		fseek(fp,(1-BUF_SIZE),SEEK_CUR);
	} 
	if(pound == NULL){//#1234567890
		int rest = (int)ftell(fp);//12
		fseek(fp,0,SEEK_SET);
		fread(buf,sizeof(char),rest-1,fp);
		buf[rest-1] = '\0';
		pound = strrchr(buf,c);
		fseek(fp,0,SEEK_SET);
	}
	fseek(fp,pound-buf,SEEK_CUR);
	//printf("%ld\n",ftell(fp));
	return;
} 
int main(int argc, char* argv[])
{
	FILE *fp;
	char locallr[NAME_MAXLENGTH],locallc[NAME_MAXLENGTH];
	
	if(argc == 4){// loser log num dir
		strcpy(locallr,argv[3]);
		strcat(locallr,LR);
		fp = fopen(locallr,"r");
		if(fp != NULL){
			fseek(fp,0,SEEK_END);
			int prepos = (int)ftell(fp);
			for(int i = 0; i < atoi(argv[2]) && prepos > 1; i++){
				Findchr(fp,'#'); 
				int beginpos = (int)ftell(fp);
				while((int)ftell(fp) < prepos){
					char c = fgetc(fp);
					if(!((c=='\n') && (((int)ftell(fp))==prepos) && ((beginpos == 1) ||(i == atoi(argv[2])-1))))
						putchar(c);
				}
				if((i == 0) && (beginpos != 1))
					putchar('\n');
				prepos = beginpos;
				fseek(fp,prepos-1,SEEK_SET);
			} 
			fclose(fp);
		}
	}
	else if(argc == 3){
		int oldfilesum = 0,findalias = 0;
		char M[33],alias[NAME_MAXLENGTH],equal[2],command[7]="nothin";
		struct files oldfiles[MAX_FILE_NUM];
		struct FileNames file_names = list_file(argv[2]);
		qsort(file_names.names,file_names.length,sizeof(char*),compare);
		
		strcpy(locallc,argv[2]);
		strcat(locallc,LC);
		fp = fopen(locallc,"r");
		if(fp != NULL){
			while((fscanf(fp,"%s%s%s",alias,equal,command)==3) && !findalias){
				if(strcmp(alias,argv[1]) == 0)
					findalias = (strcmp(command,"status")==0)? 1 : 2;
			} 
			fclose(fp);
		}
		
		strcpy(locallr,argv[2]);
		strcat(locallr,LR);
		fp = fopen(locallr,"r");
		
		if((strcmp(argv[1],"status")==0) || (findalias == 1)){
			printf("[new_file]\n");
			if(fp == NULL){
				for (int i = 0; i < file_names.length; i++)
    				printf("%s\n", file_names.names[i]);
    			printf("[modified]\n[copied]\n");
			}	
			else{
				fseek(fp,0,SEEK_END);
				Findchr(fp,')');
				fseek(fp,1,SEEK_CUR);
				while(!feof(fp)){
					fscanf(fp,"%s%s",oldfiles[oldfilesum].name,oldfiles[oldfilesum].MD5);
				//	printf("%s %s\n",oldfiles[oldfilesum].name,oldfiles[oldfilesum].MD5);
					oldfilesum ++;
				}
				int newnum[file_names.length],newsum = 0;
				int modifiednum[file_names.length],mdfsum = 0;
				int copiednum[file_names.length],cpsum = 0,copiedsource[file_names.length];
				for (int i = 0; i < file_names.length; i++){
					MD5(file_names.names[i],argv[2],M);
				//	printf("md5 %d %s\n",i,md5);
					if(!(strcmp(file_names.names[i],oldfiles[i-newsum-cpsum].name))){//filenames are same 
						if(strcmp(M,oldfiles[i-newsum-cpsum].MD5)){//modified 
							modifiednum[mdfsum] = i;
							mdfsum++;
						}
			//			printf("filenames are same\n");
					}else{//filenames are different
						int findcp = 0;
						for(int j = 0; j < file_names.length && !findcp; j++){
							if(!(strcmp(M,oldfiles[j].MD5))){// copied
								findcp = 1;
								copiednum[cpsum] = i;
								copiedsource[cpsum] = j;
								cpsum++;
							}
						}
						if(!findcp){//new
							newnum[newsum] = i;
							newsum++;
						}
					}
				}
				for(int i = 0; i < newsum; i++)
					printf("%s\n",file_names.names[newnum[i]]);
				printf("[modified]\n");
				for(int i = 0; i < mdfsum; i++)
					printf("%s\n",file_names.names[modifiednum[i]]);
				printf("[copied]\n");
				for(int i = 0; i < cpsum; i++)
					printf("%s => %s\n",oldfiles[copiedsource[i]].name,file_names.names[copiednum[i]]);
				fclose(fp);
			}
		}
		else if((strcmp(argv[1],"commit") == 0) || (findalias == 2)){
			if(fp == NULL){
				if(file_names.length > 0){
					fp = fopen(locallr,"w");
					fprintf(fp,"# commit 1\n[new_file]\n");
					for (int i = 0; i < file_names.length; i++)
    					fprintf(fp,"%s\n", file_names.names[i]);
    				fprintf(fp,"[modified]\n[copied]\n(MD5)\n");
					for (int i = 0; i < file_names.length; i++){
						MD5(file_names.names[i],argv[2],M);
						fprintf(fp,"%s %s\n",file_names.names[i],M);
					}
				}
			}else{
				int commitnum;
				fseek(fp,0,SEEK_END);
				Findchr(fp,'#');
				fseek(fp,9,SEEK_CUR);
				fscanf(fp,"%d",&commitnum);
				
				fseek(fp,0,SEEK_END);
				Findchr(fp,')');
				fseek(fp,1,SEEK_CUR);
				while(!feof(fp)){
					fscanf(fp,"%s%s",oldfiles[oldfilesum].name,oldfiles[oldfilesum].MD5);
					oldfilesum ++;
				}
				int newnum[file_names.length],newsum = 0;
				int modifiednum[file_names.length],mdfsum = 0;
				int copiednum[file_names.length],cpsum = 0,copiedsource[file_names.length];
				for (int i = 0; i < file_names.length; i++){
					MD5(file_names.names[i],argv[2],M);
					if(!(strcmp(file_names.names[i],oldfiles[i-newsum-cpsum].name))){//filenames are same 
						if(strcmp(M,oldfiles[i-newsum-cpsum].MD5)){//modified 
							modifiednum[mdfsum] = i;
							mdfsum++;
						}
					}else{//filenames are different
						int findcp = 0;
						for(int j = 0; j < file_names.length && !findcp; j++){
							if(!(strcmp(M,oldfiles[j].MD5))){// copied
								findcp = 1;
								copiednum[cpsum] = i;
								copiedsource[cpsum] = j;
								cpsum++;
							}
						}
						if(!findcp){
							//printf("%s\n", file_names.names[i]);
							newnum[newsum] = i;
							newsum++;
						}
					}
				}
				fclose(fp);
				if(newsum +mdfsum + cpsum){
					fp = fopen(locallr,"a");
					fprintf(fp,"\n# commit %d\n[new_file]\n",commitnum+1);
					for(int i = 0; i < newsum; i++)
						fprintf(fp,"%s\n",file_names.names[newnum[i]]);
					fprintf(fp,"[modified]\n");
					for(int i = 0; i < mdfsum; i++)
						fprintf(fp,"%s\n",file_names.names[modifiednum[i]]);
					fprintf(fp,"[copied]\n");
					for(int i = 0; i < cpsum; i++)
						fprintf(fp,"%s => %s\n",oldfiles[copiedsource[i]].name,file_names.names[copiednum[i]]);
					fprintf(fp,"(MD5)\n");
					for(int i = 0; i < file_names.length; i++){
						MD5(file_names.names[i],argv[2],M);
						fprintf(fp,"%s %s\n",file_names.names[i],M);
					}
					fclose(fp);
				}
			}
		}
		free_file_names(file_names);
	}
	else
		fprintf(stderr,"error\n");
	/*struct FileNames file_names = list_file(dir);
  	for (int i = 0; i < file_names.length; i++) {
    	printf("%s\n", file_names.names[i]);
  	}
	free_file_names(file_names);*/
	return 0;
} 
