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

// 3. open thread
	clock_gettime(CLOCK_REALTIME,&ts);
	int start=ts.tv_sec;
	printf("open Thread : 0 sec \n");	
	pthread_t p_thread[10];
	int t;
	for(t=0;t<file_num;t++){
		socket_data* sd=(socket_data*)malloc(sizeof(socket_data));
		sd->port=serv_addr.sin_port;
		sd->clnt_sock=clnt_sock;
		sd->order=t;
		sd->file_name=file_list[t];
		pthread_create(&p_thread[t],NULL,send_file,(void*)sd);
	}
	for(t=0;t<file_num;t++){
		pthread_join(p_thread[t],NULL);	
	}	
// 4. send file by multi-thread
	
	
	clock_gettime(CLOCK_REALTIME,&ts);
	printf("finish : %ld sec\n",ts.tv_sec-start);
	close(clnt_sock);
	close(serv_sock);
	return 0;
}

void* send_file(void* socketdata){

//1. connect server-client
	socket_data* sd=(socket_data*)socketdata;

	char buf[30];
	int ct_sock;
	int sv_sock;
	struct sockaddr_in sv_addr;
	struct sockaddr_in ct_addr;
	socklen_t ct_addr_size;	
	
	unsigned short port=sd->port+sd->order*10-1;
	while(1){
		sv_sock=socket(PF_INET, SOCK_STREAM,0);
		if(sv_sock==-1)error_handling("socket() error");	
		memset(&sv_addr,0,sizeof(sv_addr));
		sv_addr.sin_family=AF_INET;
		sv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
		sv_addr.sin_port=++port;
		if(bind(sv_sock,(struct sockaddr*) &sv_addr,sizeof(sv_addr))==-1){
			continue;		
		}
		break;
	}
	
	

	if(listen(sv_sock,5)==-1){
		error_handling("listen() error");
	}	
	write(sd->clnt_sock,&port,sizeof(unsigned short));
	
	ct_addr_size =sizeof(ct_addr);
	ct_sock=accept(sv_sock,(struct sockaddr*)&ct_addr,&ct_addr_size);
	if(ct_sock==-1) error_handling("accept() error");

//2. start send file
	struct stat file;
	char* filename=sd->file_name;

	stat(filename, &file);
	int fd = open(filename, O_RDONLY);
	int size = file.st_size;
	printf("fd:%d / size:%d /filename:%s\n",fd,size,filename);
	if (fd == -1) return NULL;


	send(ct_sock, &size, sizeof(int), 0);
	sleep(1);
	send(ct_sock, filename, sizeof(char)*30,0);

	if (size) {	
		printf("sending %s file\n",filename);
		printf("fd : %d , size = %d \n",fd,size);
		sendfile(ct_sock, fd, NULL, size);
		printf("Finish\n");		
	}
	close(ct_sock);
	close(sv_sock);
}
void error_handling(char* message){
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}

