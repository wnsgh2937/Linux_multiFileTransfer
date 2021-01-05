#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
void error_handling(char* message);
void* send_file(void* file_name);



typedef struct{
	unsigned short port;
	int order;
	int clnt_sock;
	char* file_name;
}socket_data;


int main(int argc, char *argv[]){
	struct timespec ts;	
	int clnt_sock;
	int serv_sock;
	char buf[30];

	struct stat obj;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;	

	if(argc!=2){
		printf("Usage : %s <port>\n",argv[0]);
		exit(1);
	}

	serv_sock=socket(PF_INET, SOCK_STREAM,0);
	if(serv_sock==-1){
		error_handling("socket() error");	
	}


	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi(argv[1]));

	if(bind(serv_sock,(struct sockaddr*) &serv_addr,sizeof(serv_addr))==-1){
		error_handling("bind() error");
	}
	
	printf("waiting...\n");	
	if(listen(serv_sock,5)==-1){
		error_handling("listen() error");
	}

	
	clnt_addr_size =sizeof(clnt_addr);
	clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
	if(clnt_sock==-1) error_handling("accept() error");

// 0.get "Hello_Server"
	char message[20];
	recv(clnt_sock, message, 20, 0);
	printf("%s\n",message);

// 1. send ls information

	//1-1. struct send info

	struct stat send_info;
	system("ls >ls.txt");
	stat("ls.txt",&send_info);
	int send_info_size=send_info.st_size;

	//1-2. send info

	send(clnt_sock,&send_info_size,sizeof(int),0);

	//1-3. send txt file
	
	int ls_FD=open("ls.txt",O_RDONLY);	
	sendfile(clnt_sock,ls_FD,NULL,send_info_size);

// 2. get send file information

	char* file_list[10];
	int file_num=0;
	while(1){
		char* fileName=(char*)malloc(sizeof(char)*30);
		recv(clnt_sock,fileName,sizeof(char)*30,0);
		if(fileName[0]=='x') break;
		file_list[file_num++]=fileName;
	}
	for(int i=0;i<file_num;i++){
		printf("%d : %s\n",i,file_list[i]);
	}


	clock_gettime(CLOCK_REALTIME,&ts);
	int start=ts.tv_sec;
	printf("start : 0 sec\n");
	for(int i=0;i<file_num;i++){
		struct stat file;
		char* filename=file_list[i];
		
		stat(filename, &file);
		int fd = open(filename, O_RDONLY);
		int size = file.st_size;
		printf("fd:%d / size:%d /filename:%s\n",fd,size,filename);
		if (fd == -1) return 0;

		send(clnt_sock, &size, sizeof(int), 0);
		sleep(1);
		send(clnt_sock, filename, sizeof(char)*30,0);

		if (size) {	
			printf("sending %s file\n",filename);
			printf("fd : %d , size = %d \n",fd,size);
			clock_gettime(CLOCK_REALTIME,&ts);
			sendfile(clnt_sock, fd, NULL, size);
			clock_gettime(CLOCK_REALTIME,&ts);
			printf("Finish %d file: %ld sec\n",i,ts.tv_sec-start);
			clock_gettime(CLOCK_REALTIME,&ts);	
		}
	}
	clock_gettime(CLOCK_REALTIME,&ts);
	printf("All Finish : %ld sec\n",ts.tv_sec-start);
	close(clnt_sock);
	close(serv_sock);
	return 0;
}



void error_handling(char* message){
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}

