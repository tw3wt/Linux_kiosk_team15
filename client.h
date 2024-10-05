#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

#include "info_sock.h"
#include "message.h"
#include "errhandle.h"
#include "cafe.h"

int RECEIVE_FLAG = 0;

int clnt_sock;
int client_init(const char* ip, unsigned int port);

void* request_thread(void* data);
void* receive_thread(void* data); //updated 06.05

void sigint_handler(int signo); //updated 06.05
void sigpipe_handler(int signo); //create 06.06

char* select_category();
char* select_dsrt();
char* select_drnk();
char* select_dsrt_default();
char* select_drnk_default();

char* add_mesg_dsrt_1(char* mesg, char* arr, char* num);
char* add_mesg_dsrt_2(char* arr,char* num);
char* add_mesg_drnk_1(char* mesg, char* arr,char* size, char* num);
char* add_mesg_drnk_2(char* arr,char* size, char* num);

int ii=0;
int jj=0;
int kk_ds=0;
int kk_dr=0;
int flag_ds=0;
int flag_dr=0;

int client_init(const char* ip, unsigned int port)
{
	struct sockaddr_in serv_addr;

	//create socket
	if((clnt_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{ error_handling("socket create failure\n"); }

	//set address and port of destination
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip); //localhost
	serv_addr.sin_port = htons(port);

	if(connect(clnt_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{ error_handling("connect failure"); }

	return clnt_sock;
}

//updated in 06.09
void* request_thread(void* data)
{
	char mesg[BUFSIZE];
	void* ret = NULL;

	sleep(1);
	while(1)
	{
		mesg[0] = '\0';
		if(RECEIVE_FLAG == 1)
		{
			strcpy(mesg, select_category());
			//printf("%s\n", mesg);
			RECEIVE_FLAG = 0;

			if(write(clnt_sock, mesg, BUFSIZE) == -1)
			{
				perror("request failure\n");
				continue;
			}
			sleep(1);
		}
		else
		{
			sleep(1);
		}
	}
	return ret;
}

//updated in 06.09
void* receive_thread(void* data)
{
	char mesg[BUFSIZE] = {0};
	char *str = NULL;
	void* ret = NULL;

	while(1)
	{
		mesg[0] = '\0';

		if(read(clnt_sock, mesg, BUFSIZE) <= 0)
			sleep(1);
		else
		{
			strtok_r(mesg, DELIMITER, &str);
			// receive refresh -> refresh monitor
			if(!strcmp(mesg, "refresh"))
			{
				printf("%s\n", str);
			}
			// receive confirm -> disconnect
			else if(!strcmp(mesg, "confirm"))
			{
				/***********updated************/
				//confirmed and notify disconnection to server
				printf("confirmed\n");
				write(clnt_sock, "disconnect", BUFSIZE);
				/******************************/
			}
			// receive reject -> modify basket
			else if(!strcmp(mesg, "reject"))
			{
				printf("rejected\n");
				while(getchar()=='\n')break;
				ii=0;
				jj=0;
				kk_ds=0;
				kk_dr=0;
				flag_ds=0;
				flag_dr=0;
				//strtok_r(NULL, DELIMITER, &str); --> str = "money" or "order"
			}
			else if(!strcmp(mesg, "server"))
			{
				/***********updated************/
				printf("connection closed\n");
				close(clnt_sock);
				exit(0);
				/******************************/
			}
			else
			{
				//receive invalid message from server
			}
			RECEIVE_FLAG = 1;
		}

	}

	return ret;
}

//updated 06.05
void sigint_handler(int signo)
{
	char mesg[BUFSIZE] = {0};
	write(clnt_sock, "disconnect", BUFSIZE);
	read(clnt_sock, mesg, BUFSIZE);
	close(clnt_sock);
	exit(0);
}

//create 06.06
void sigpipe_handler(int signo)
{
	fprintf(stderr, "server closed\n");
	close(clnt_sock);
	exit(0);
}

//updated 06.09
char* select_category() {
	char *mesg=(char*)malloc(sizeof(char)*1024);
	char* msg_ds=(char*)malloc(sizeof(char)*1024);
	char* msg_dr=(char*)malloc(sizeof(char)*1024);
	strcpy(mesg,"");
	strcpy(msg_ds,"");
	strcpy(msg_dr,"");
	char req[4096]="request";
	while (1) {
		printf("a) dessert\t b) drink ( c) confirm d) quit )\n");

		char tmp;
		scanf("%c", &tmp);
		if (tmp == 'a') {
			flag_ds=1;
			char* atmp;
			atmp=select_dsrt();
			//mesg = select_dsrt();
			if(strcmp(atmp,"")==0) {
				while(getchar()=='\n')break;
				continue;
			}
			if(kk_ds==0) {
				strcat(msg_ds,atmp);
				kk_ds=1;
			}else{
				strcat(msg_ds," ");
				strcat(msg_ds,atmp);
			}
			while(getchar()=='\n')break;
			free(atmp);
		}
		else if (tmp == 'b') {
			flag_dr=1;
			char* atmp;
			atmp=select_drnk();
			if(strcmp(atmp,"")==0) {
				while(getchar()=='\n')break;
				continue;
			}
			if(kk_dr==0) {
				strcat(msg_dr,atmp);
				kk_dr=1;
			}else{
				strcat(msg_dr," ");
				strcat(msg_dr,atmp);
			}
			while(getchar()=='\n')break;
			free(atmp);
		}
		else if(tmp=='c') {
			printf("Enter \"confirm\" to confirm the menu (Enter \'1\' not to confirm the menu)\n");
			char con[8];
			scanf("%s", con);
		if (strcmp(con,"confirm")==0) {
			while(getchar()=='\n')break;
			if(flag_ds==0&&flag_dr==0) {
				char* tmp_ds;
				tmp_ds=select_dsrt_default();
				strcpy(msg_ds,tmp_ds);
				free(tmp_ds);
				char* tmp_dr;
				tmp_dr=	select_drnk_default();
				strcpy(msg_dr,tmp_dr);
				free(tmp_dr);
			}else{
				if(flag_ds==0) {
				char* tmp_ds;
				tmp_ds=select_dsrt_default();
				strcpy(msg_ds,tmp_ds);
				free(tmp_ds);
			}
			if(flag_dr==0) {
				char* tmp_dr;
				tmp_dr=	select_drnk_default();
				strcpy(msg_dr,tmp_dr);
				free(tmp_dr);
			}
		}

		strcat(msg_ds," ");
		strcat(msg_ds,msg_dr);
		strcpy(mesg,msg_ds);
		strcat(req," ");
		strcat(req,mesg);
		mesg=req;
		char fee[50]={0};
		if(flag_ds==0&&flag_dr==0) {
			strcpy(fee,"0");
		}else {
			printf("Enter the money to pay: ");
			scanf("%s",fee);
		}
			strcat(mesg," fee");
			strcat(mesg," ");
			strcat(mesg,fee);
			break;
		}
		else if (con[0] == '1') {
			while(getchar()=='\n')break;
			continue;
		}
	}
		else if(tmp=='d') {
			printf("connection closed\n");
			char mesg[BUFSIZE] = {0};
			write(clnt_sock, "disconnect", BUFSIZE);
			read(clnt_sock, mesg, BUFSIZE);
			close(clnt_sock);
			exit(0);
		}else{
			printf("non - existent menu, try again\n");
		}
	}
	free(msg_ds);
	free(msg_dr);
	return mesg;
}

//updated 06.09
char* select_dsrt() {
	char msg_ds[BUFSIZE] = "dessert";
	char* msg;
	int cntl=0;
	int cnt=0;
	while (1) {
		char temp[1024]={0};

		char i[2]={0};

		printf("select dessert(Enter \"1 1\" to go to prev screen): \n");
			scanf("%s", temp);
		scanf("%s", i);

		if(temp[0]=='1'&&i[0]=='1') {
			if(cnt==0)
				return "";
			else
				break;
		}
		if(ii==0) {
			msg = add_mesg_dsrt_1(msg_ds, temp, i);
			ii=1;
			cntl=1;
		}else{
			if(cntl==0) {
				msg = add_mesg_dsrt_2(temp, i);
				cntl=1;
			}else {
				strcat(msg," ");
				strcat(msg,add_mesg_dsrt_2(temp, i));
			}
		}
		cnt=1;
	}
	return msg;
}

//updated 06.09
char* select_drnk() {
	char msg_dr[BUFSIZE] = "drink";
	char* msg;
	int cntl=0;
	int cnt=0;
	while (1) {
		char temp[1024]={0};

		char sz[10]={0};
		char i[10]={0};

		printf("select drink(Enter \"1 1 1\" to go to prev screen): \n");
		scanf("%s", temp);
		scanf("%s", sz);
		scanf("%s", i);

		if(temp[0]=='1'&&sz[0]=='1'&&i[0]=='1'){
			if(cnt==0)
				return "";
			else
				break;
		}
		if(jj==0) {
			msg = add_mesg_drnk_1(msg_dr, temp,sz, i);
			cntl=1;
			jj=1;
		}else{
			if(cntl==0) {
				msg = add_mesg_drnk_2(temp,sz, i);
				cntl=1;
			}else {
				strcat(msg," ");
				strcat(msg,add_mesg_drnk_2(temp,sz, i));
			}

		}
		cnt=1;
	}
	return msg;
}

//updated 06.10
char* select_dsrt_default() {
	char msg_ds[BUFSIZE] = "dessert";

	strcat(msg_ds," ");
	strcat(msg_ds,"0");
	strcat(msg_ds," ");
	strcat(msg_ds,"0");
	char* temp=(char*)malloc(sizeof(char)*BUFSIZE);
	strcpy(temp,msg_ds);
	return temp;
}

//updated 06.10
char* select_drnk_default() {
	char msg_dr[BUFSIZE] ="drink";
	char* temp=(char*)malloc(sizeof(char)*BUFSIZE);
	strcat(msg_dr," ");
	strcat(msg_dr,"0");
	strcat(msg_dr," ");
	strcat(msg_dr,"0");
	strcat(msg_dr," ");
	strcat(msg_dr,"0");
	strcpy(temp,msg_dr);
	return temp;
}

//updated 06.10
char* add_mesg_dsrt_1(char* mesg, char* arr, char* num) {
	//mesg=(char*)malloc(sizeof(char)*1024);
	char ch[2]=" ";
	strcat(mesg, ch);
	strcat(mesg, arr);
	strcat(mesg, ch);
	strcat(mesg,num);
	char* temp=(char*)malloc(sizeof(char)*1024);
	strcpy(temp,mesg);
	return temp;
}

//updated 06.10
char* add_mesg_dsrt_2( char* arr, char* num) {
	//mesg=(char*)malloc(sizeof(char)*1024);
	char ch[2]=" ";
	strcat(arr, ch);
	strcat(arr, num);
	char* temp=(char*)malloc(sizeof(char)*1024);
	strcpy(temp,arr);
	return temp;
}

//updated 06.10
char* add_mesg_drnk_1(char* mesg, char* arr,char* size, char* num) {
	//mesg=(char*)malloc(sizeof(char)*1024);
	//strcpy(mesg,"");
	char ch[2]=" ";
	strcat(mesg, ch);
	strcat(mesg, arr);
	strcat(mesg, ch);
	strcat(mesg, size);
	strcat(mesg, ch);
	strcat(mesg, num);
	char* temp=(char*)malloc(sizeof(char)*4096);
	strcpy(temp,mesg);
	return temp;
}

//updated 06.10
char* add_mesg_drnk_2(char* arr,char* size, char* num) {
	//mesg=(char*)malloc(sizeof(char)*1024);
	//strcpy(mesg,"");
	char ch[2]=" ";
	strcat(arr, ch);
	strcat(arr, size);
	strcat(arr, ch);
	strcat(arr, num);
	char* temp=(char*)malloc(sizeof(char)*4096);
	strcpy(temp,arr);
	return temp;
}
