#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdlib.h> 
#include<signal.h>
#include<ctype.h>
#include<netinet/in.h>
#include<assert.h>
#include<pthread.h>
#include<errno.h>

#define SERVER_STRING "Server: fortyhttpd/0.1.0\r\n"

char dir_name[64];
int server_fd = -1;
int client_fd = -1;

int read_line(int client, char* buf, int size);
void deal_error();
void deal_notfound(int client);
void send_header(int client, char* type);
void send_body(int client, FILE *fp);
void make_response(int client, char* file_path);
void *request_parse(void* arg);
void signal_handler_stop(int signal_num);

int read_line(int client, char* buf, int size)	//CRLF, /n
{
	int cnt = 0;
	char temp = '\0';
	int recv_size = 0;
	while ((temp != '\n') && (cnt < size-1)){
		recv_size = recv(client, &temp, 1, 0);
		if (recv_size > 0){
			if (temp == '\r'){
				recv_size = recv(client, &temp, 1, MSG_PEEK);	//保留
				if ((recv_size > 0) && (temp == '\n'))
     			recv(client, &temp, 1, 0);
    			else{
    				temp = '\n';
    			}
   			}
   			buf[cnt] = temp;
			cnt++;
		}
		else{
			temp = '\n';	
		}
	}
	buf[cnt] = '\0';
	return strlen(buf);
}
void deal_error()
{
	if((server_fd != -1)){
		printf("\033[41;37mclose server_fd\033[0m\n");
		close(server_fd);
	}
	if((client_fd != -1)){
		printf("\033[41;37mclose client_fd\033[0m\n");
		close(client_fd);
	}
	exit(1);
}
void deal_notfound(int client)
{
	char buf[1024];
	sprintf(buf, "Content-Type: text/html\r\nContent-Type: text/html\r\n\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><TITLE>404Not Found</TITLE>\r\n<BODY><P>404NOT FOUND\r\n</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
	
	return;
}
void send_header(int client, char* type)
{
	char buf[1024];
	
	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	if(send(client, buf, strlen(buf), 0) == -1){
			printf("\033[41;37msend error\033[0m\n");
			deal_error();
	}
	strcpy(buf, SERVER_STRING);
	if(send(client, buf, strlen(buf), 0) == -1){
			printf("\033[41;37msend error\033[0m\n");
			deal_error();
	}
	if(!strcmp(type, ".html") || !strcmp(type, "htm")){
		strcpy(buf, "Content-Type: text/html\r\n");
	}
	else if(!strcmp(type, ".css")){
		strcpy(buf, "Content-Type: text/css\r\n");
	}
	if(send(client, buf, strlen(buf), 0) == -1){
			printf("\033[41;37msend error\033[0m\n");
			deal_error();
	}
	strcpy(buf, "\r\n");
	if(send(client, buf, strlen(buf), 0) == -1){
			printf("\033[41;37msend error\033[0m\n");
			deal_error();
	}
	return;
}

void send_body(int client, FILE* fp)
{
	char buf[1024];
 	while (fgets(buf, sizeof(buf), fp) != NULL)
 	{
  		if(send(client, buf, strlen(buf), 0) == -1){
			printf("\033[41;37msend error\033[0m\n");
			deal_error();
		}
	}
	return;	
}
void make_response(int client, char* file_path)
{
	char type[64]; char* ptr = NULL;
	ptr = strstr(file_path, ".html");
	if(ptr){
		strcpy(type, ".html");
	}
	ptr = strstr(file_path, ".css");
	if(ptr){
		strcpy(type, ".css");
	}
	if(!strcmp(type, "nothing")){
		printf("\033[41;37mCANNOT support this type!!!!\033[0m\n");
		deal_error();
	}
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	int cnt = 0;
	char buf[1024];
	strcpy(buf, "nothing");
	cnt = read_line(client, buf, sizeof(buf));
	while ((cnt > 0) && strcmp("\n", buf)){
		cnt = read_line(client, buf, sizeof(buf));
	}
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/	
	FILE* fp = NULL;  
	fp = fopen(file_path, "r");
	if(fp == NULL){
		deal_notfound(client);
	}
	else{
		send_header(client, type);
		send_body(client, fp);
	}
	fclose(fp);
}
void *request_parse(void* arg)
{
	int client = *((int*)arg);printf("in request_parse: client:%d\n", client);
	char buf[1024]; char file_path[512]; char url[128]; char method[256];
	int line_len = 0;
	int ptr1 = 0, ptr2 = 0;
	//struct stat buffer;
	line_len = read_line(client, buf, sizeof(buf));
	printf("buf:%s\n", buf);
	while(!isspace(buf[ptr1]) && (ptr1<line_len-1)){
		method[ptr2++] = buf[ptr1++];
	}
	method[ptr2] = '\0';
	if(strcasecmp(method, "GET")){
		printf("\033[41;37mthis server only support \"GET\"!!\033[0m\n");
		deal_error();
	}
	if(isspace(buf[ptr1])){
		ptr1++;
	}
	ptr2 = 0;
	while(!isspace(buf[ptr1]) && (ptr1<line_len-1)){
		url[ptr2++] = buf[ptr1++];
	}
	url[ptr2] = '\0';
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	sprintf(file_path, "%s%s", dir_name,url);	
	//strcat(dir_name, url);
	//strcpy(file_path, dir_name);
	/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	if((file_path[strlen(file_path)-1] == '/')){
		//strcat(file_path, "index.html");
		sprintf(file_path, "%sindex.html", file_path);
	}
	make_response(client, file_path);
	close(client);
}
void sighandler(int);
void sighandler(int signum)
{
   
	if(signum == SIGINT){
		printf("\nthis is stop signal:%d\n", signum);
		if((server_fd != -1)){
			close(server_fd);
		}
		if((client_fd != -1)){
			close(client_fd);
		}
		exit(0);	
	}
}
void parse_path(char* path, char* new_path)
{
	int ptr = -1;
	char* temp;
	
	strcpy(new_path, path);
	
	temp = strstr(path, "../");
	while(temp){
		int slant_pos1 = -2; int slant_pos2 = -1;
		ptr = temp - path;
		for(int i = 0; i<ptr; i++){
			if(path[i] == '/'){
				if(slant_pos1 < slant_pos2)
					slant_pos1 = i;
				else
					slant_pos2 = i;	
			}
		}
		int slant_pos = slant_pos1<slant_pos2?slant_pos1:slant_pos2;
		if(slant_pos == -1){
			printf("\033[41;37mInvalid file path!!!!\033[0m\n");
			deal_error();
		}
		int diff = ptr - slant_pos + 2;
		int i;
		for(i = slant_pos+1; i<strlen(path)-diff; i++){
			if(path[i+diff] == '.' && path[i+diff+1] == '.' && path[i+diff+2] == '/'){
				diff += 3;
			}
			new_path[i] = path[i+diff];
		}
		if(!strcmp(new_path+strlen(new_path)-3, "../")){
			new_path[strlen(new_path)-3] = '\0';
		}
		strcpy(path, new_path);
		temp = strstr(path, "../");
		if(!temp){
			ptr = i;
		}
	}
	if(ptr >= 0){
		new_path[ptr] = '\0'; 
		path[ptr] = '\0';	
	}
	temp = strstr(path, "./");
	while(temp){
		ptr = temp - path;
		int i;
		int diff = 2;
		for(i = ptr; i<strlen(path)-2; i++){
			if(path[i+diff] == '.' && path[i+diff+1] == '/'){
				diff += 2;
			}
			new_path[i] = path[i+diff];		
		}
		strcpy(path, new_path);
		temp = strstr(path, "./");
		if(!temp){
			ptr = i;
		}
	}
	if(ptr>0){
		new_path[ptr] = '\0'; 
		path[ptr] = '\0';
	}
	return;
}
int main(int argc, char *argv[]) 
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;   
    int client_addr_len = sizeof(client_addr);
	u_short port = 0;
	pthread_t t1;
	/*--------读取命令行参数--------*/
	for (int i = 0; i < argc; i++) 
	{
		assert(argv[i]); // specification
	    printf("argv[%d] = %s\n", i, argv[i]);
	}	
	assert(!argv[argc]); // specification	
	if(!strcmp(argv[1], "-p") || !strcmp(argv[1], "--port")){
		char port_c[8];
		strcpy(port_c, argv[2]);
		port = (u_short)atoi(port_c);
		printf("\033[42;37mthe port is %d\033[0m\n", port);
		parse_path(argv[3], dir_name);
		//strcpy(dir_name, argv[3]);
		printf("\033[42;37mthe directory name is: %s\033[0m\n", dir_name);
	}
	else if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")){
		printf("\033[46;37mhelp:\033[0m\n");
		printf("\033[46;37mplease enter cmd like this:\033[0m\n");
		printf("\033[46;37m\"./httpd -p PORT ./site\" or \"./httpd --port PORT ./size\"\033[0m\n");
		printf("\033[46;37mthe PORT is a number which can be served as a port\033[0m\n");
	}
	else{
		printf("\033[41;37mINVALID cmd!!!!\033[0m\n");
		exit(1);
	}
	/*===============中断处理注册===============*/
	signal(SIGINT, sighandler);
	/*===============创建套接字===============*/
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd == -1){
		printf("\033[41;37mserver create socket failed!!!!\033[0m\n");
		exit(1);
	}
	memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
    	printf("\033[41;37mbind failed!!!!\033[0m\n");
    	exit(1);
    }
    int mw_optval = 1;
     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&mw_optval,sizeof(mw_optval));
    if (listen(server_fd, 4) < 0) {
        printf("\033[41;37mlisten failed!!!!\033[0m\n");
        exit(-1);
    }
    printf("\033[42;37mhttpd running on port %d\033[0m\n", port);
    while(1){
    	client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    	if(client_fd == -1){
    		printf("\033[41;37mclient create socket failed!!!!\033[0m\n");
    		exit(1);
    	}
    	/*============创建另一个线程处理报文信息============*/
    	printf("client_fd in main:%d\n", client_fd);
    	if(pthread_create(&t1, NULL, request_parse, (void *)&client_fd) != 0){
    		printf("\033[41;37mthread creating failed!!!!\033[0m\n");
    		exit(1);
    	}
    }
    close(server_fd);
	return 0;
}
//	#gcc -std=gnu99 -O1 -Wall -lpthread -o $(LAB) $(LAB).c
//	#gcc -g -o -Wall -lpthread $(LAB) $(LAB).c
