#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <semaphore.h> //included in 06.04 by jh

#include "info_sock.h"
#include "manager.h"
//#include "message.h"
#include "errhandle.h"

int CLIENT_MAX = 10;
int CLOSE_FLAG = 0;		//new variable in 06.01
int REFRESH_FLAG = 0;	//new variable in 06.04
sem_t semaphore;		//new variable in 06.04

int  server_init();
void server_close(struct pollfd* p_fds);
void pollfd_init(struct pollfd* p_fds, int serv_sock);
void accept_client(struct pollfd* p_fds, int serv_sock);	//updated in 06.04 by jh

int execute_message(int argc, char** argv, int fd); //updated in 06.07 by jh
void process_message(struct pollfd* p_fds, int serv_sock); //updated in 06.06 by jh

int select_everything(struct dessert** dsrt_p, struct drink** drnk_p, int* dsrt_c, int* drnk_c);//updated in 5.31 by jh
void monitor(struct dessert* dsrt_p, struct drink* drnk_p, int dsrt_c, int drnk_c, char** mesg);//updated in 5.31 by jh
void refresh_monitor(int fd);
void refresh_monitor_all(struct pollfd* p_fds);	//updated in 06.04 by jh

void* server_thread(void* data); //updated in 06.05 by jh
void* manager_thread(void* data);//updated in 06.04 by jh

void create_dessert_table();//created in 5.30 by tae hyeon
void create_drink_table();	//created in 5.30 by tae hyeon

void remove_dessert_table();//created in 5.30 by tae hyeon
void remove_drink_table();	//created in 5.30 by tae hyeon

void create_sales();//created in 5.31 by tae hyeon
void remove_sales();//created in 5.31 by tae hyeon
void print_sales();	//created in 5.31 by tae hyeon

int mesg_recv(int fd, char* mesg); //created in 06.06 by jh
void mesg_send(int fd, char* mesg); //created in 06.06 by jh

void switch_server_status(int status); //created in 6.06 by tae hyeon

void log_server_open(); //create in 06.07 by jh
void log_server_close();//create in 06.07 by jh

int server_init()
{
	int serv_sock;
	struct sockaddr_in serv_addr;
	int opt = 1;

    if((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    { error_handling("socket create failure"); }

	memset(&serv_addr, 0x00, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("socket bind failure");

	if(listen(serv_sock, 5) == -1)
		error_handling("backlog queue create failure");

	printf("server initialize success\n");
	log_server_open();

	return serv_sock;
}

void server_close(struct pollfd* p_fds)
{
	int clnt_sock, clnt_len;
	struct sockaddr_in clnt_addr;

	for(int i=POLL_START+1; i<CLIENT_MAX; i++)
	{
		if((p_fds+i)->fd != -1)
		{
			write((p_fds+i)->fd, "server closed", BUFSIZE);
			close((p_fds+i)->fd);
		}
	}

	log_server_close();
	close(p_fds->fd);
}

void pollfd_init(struct pollfd* p_fds, int serv_sock)
{
	p_fds->fd = serv_sock; //[0]
	p_fds->events = POLLIN;

	p_fds++;

	for(int i=POLL_START+1; i<CLIENT_MAX; i++) //[1]~[CLIENTMAX-1]
	{
		p_fds->fd = -1;
		p_fds++;
	}

	printf("pollfd initialize success\n");
}

//updated in 06.04 by jh
void accept_client(struct pollfd* p_fds, int serv_sock)
{
	int i, waiting = 0;
	int clnt_sock, clnt_len;
	struct sockaddr_in clnt_addr;

	//find available p_fds
	p_fds++;
	for(i=POLL_START+1; i<CLIENT_MAX; i++)
	{
		if(p_fds->fd < 0)
		{
			clnt_len = sizeof(clnt_addr);
			clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_len);

			if(clnt_sock == -1)
			{
				perror("client accept failure\n");
				return;
			}

			p_fds->fd = clnt_sock;
			p_fds->events = POLLIN;

			refresh_monitor(p_fds->fd);

			break;
		}
		else p_fds++;
	}
}

//updated in 06.07 by jh
/*************************************************/
int execute_message(int argc, char** argv, int fd)
{
	int dsrt_c = 0, dsrt_i = 1;
	int drnk_c = 0, drnk_i = 4;
	int name_i = 0, num_i = 0;
	int temp_i = 0, ret = 0, total = 0;
	int dsrt_check = 0, drnk_check = 0;
	int str_len = 0;
	int money = atoi(argv[argc-1]);
	int *update = NULL;

	char *empty = "0";

	void *dsrt_menu = NULL, *drnk_menu = NULL;

	for(int i=4; i<argc; i+=2)
	{
		if(is_drink(argv[i]))
		{
			drnk_i = i; // dsrt_i < dsrt < drnk_i
			dsrt_c = (int)((drnk_i-dsrt_i-1)/2);
			break;
		}
	}

	//drnk_i < drnk < argc-2
	drnk_c = (int)((argc-drnk_i-3)/3);

	if((strcmp(argv[dsrt_i+1], empty)) != 0 && ret >= 0 && dsrt_c > 0)
	{
		dsrt_check = 1;
		char** names = (char**)malloc(dsrt_c*sizeof(char*));
		update = (int*)malloc(2*dsrt_c*sizeof(int));
		struct dessert* dsrt_p = (struct dessert*)malloc(dsrt_c*sizeof(struct dessert));

		for(int i=0; i<dsrt_c; i++)
		{
			name_i = dsrt_i+2*i+1;
			temp_i = dsrt_i+2*i+2;
			str_len = strlen(argv[name_i]);
			names[i] = (char*)malloc(str_len*sizeof(char));

			strcpy(names[i], argv[name_i]);
			//update[2*i+1] = atoi(argv[temp_i]);
			//negative number, reject order
			if((update[2*i+1] = atoi(argv[temp_i])) <= 0)
			{ update[2*i+1] = 0; ret = -1; }
		}

		select_entity_nms(DESSERT, names, dsrt_c, &dsrt_menu);
		memcpy(dsrt_p, dsrt_menu, dsrt_c*sizeof(struct dessert));

		for(int i=0; i<dsrt_c; i++)
		{
			//dsrt find failure, reject order
			if(dsrt_p[i].id == 0)
			{ ret = -1; }
			//not enough number, reject order
			else if(dsrt_p[i].number < update[2*i+1])
			{ ret = -1; }
			else
			{ total += dsrt_p[i].price * update[2*i+1]; }

			update[2*i] = dsrt_p[i].id;
			update[2*i+1] *= -1;

			free(names[i]);
		}
		free(names);
		free(dsrt_p);
	}

	if(strcmp(argv[drnk_i+1], empty) != 0 && ret >= 0 && drnk_c > 0)
	{
		drnk_check = 1;
		char** names = (char**)malloc(drnk_c*sizeof(char*));
		int* types = (int*)malloc(drnk_c*sizeof(int));
		int* nums = (int*)malloc(drnk_c*sizeof(int));
		struct drink* drnk_p = (struct drink*)malloc(drnk_c*sizeof(struct drink));

		for(int i=0; i<drnk_c; i++)
		{
			name_i = drnk_i+3*i+1;
			temp_i = drnk_i+3*i+2;
			num_i = drnk_i+3*i+3;
			str_len = strlen(argv[name_i]);
			names[i] = (char*)malloc(str_len*sizeof(char));

			strcpy(names[i], argv[name_i]);

			if(!strcmp(argv[temp_i], "S")) { types[i] = 0; }
			else if(!strcmp(argv[temp_i], "M")) { types[i] = 1; }
			else if(!strcmp(argv[temp_i], "L")) { types[i] = 2; }
			else { ret = -1; } //invalid type, reject order

			//negative number, reject order
			if((nums[i] = atoi(argv[num_i])) <= 0)
			{ nums[i] = 0; ret = -1; }
		}

		select_entity_nms(DRINK, names, drnk_c, &drnk_menu);
		memcpy(drnk_p, drnk_menu, drnk_c*sizeof(struct drink));

		for(int i=0; i<drnk_c; i++)
		{
			//drink find failure, reject order
			if(drnk_p[i].id == 0)
			{ ret = -1; }

			total += drnk_p[i].price[types[i]] * nums[i];
			free(names[i]);
		}
		free(names);
		free(types);
		free(nums);
		free(drnk_p);
	}

	//order nothing
	if(dsrt_check == 0 && drnk_check == 0) { ret = -1; }
	//not enough money
	if(total > money) { ret = -2; }

	if(ret == -1)
	{
		mesg_send(fd, "reject order");
	}
	else if(ret == -2)
	{
		mesg_send(fd, "reject money");
	}
	else
	{
		if((strcmp(argv[dsrt_i+1], empty)) != 0)
		{
			//update failure
			if(update_entity(DESSERT, update, dsrt_c*2) < 0)
			{ mesg_send(fd, "reject order"); }
			//update success
			else
			{
				mesg_send(fd, "confirm order");
				update_sales_table(SALES, total);
				ret = 1; //updated in 06.04 by jh
			}
		}
		else
		{
			mesg_send(fd, "confirm order");
			update_sales_table(SALES, total);
			ret = 0; //updated in 06.07 by jh
		}
	}

	free(update);
	free(dsrt_menu);
	free(drnk_menu);
	return ret;
}
/*************************************************/

//updated in 06.06 by jh
void process_message(struct pollfd* p_fds, int serv_sock)
{
	int argc = 0, fd = 0;
	char **argv = NULL;
	char mesg[BUFSIZE] = {0};

	for(int i=POLL_START+1; i<CLIENT_MAX; i++)
	{
		fd = (p_fds+i)->fd;
		// fd is not set
		if(fd < 0)
		//if((p_fds+i)->fd < 0)
		{
			continue;
		}
		// fd is set and revent is POLLIN or POLLERR
		else if((p_fds+i)->revents & (POLLIN | POLLERR))
		{
			/*************updated***************/
			if(mesg_recv(fd, mesg) == -1) //read failure
			//if(read((p_fds+i)->fd, mesg, BUFSIZE) == -1)
				continue;
			//read success
			else
			{
				if(!strcmp(mesg, "disconnect"))
				{
					mesg_send(fd, "server disconnected");
					close(fd);
					(p_fds+i)->fd = -1;
					(p_fds+i)->revents = 0;
					continue;
				}

				//message analyze
				argc = analyze_message(mesg, &argv);

				mesg[0] = '\0';

				if(argc == -1) continue;

				//work with analyzed message
				if(!strcmp(argv[0], "request"))
				{
					if(execute_message(argc, argv, fd)==1)
					{
						sem_wait(&semaphore);
						REFRESH_FLAG = 1;
						sem_post(&semaphore);
					}
				}
				else
				{
					//invalid request
					/**********updated***************/
					mesg_send(fd, "reject order");
					//write((p_fds+i)->fd, "reject order\n", BUFSIZE);
					/********************************/
				}

			}
			/************************************/
		}
	}

	for(int i=0; i<argc; i++) free(argv[i]);
	free(argv);
}

//updated in 05.31 by jh
int select_everything(struct dessert** dsrt_p, struct drink** drnk_p, int* dsrt_c, int* drnk_c)
{
	void *dsrt_m = NULL, *drnk_m = NULL;

	*dsrt_c = select_entity_all(DESSERT, &dsrt_m);
	*drnk_c = select_entity_all(DRINK, &drnk_m);

	*dsrt_p = (struct dessert*)malloc(*(dsrt_c)*sizeof(struct dessert));
	*drnk_p = (struct drink*)malloc(*(drnk_c)*sizeof(struct drink));

	memcpy(*dsrt_p, dsrt_m, *(dsrt_c)*sizeof(struct dessert));
	memcpy(*drnk_p, drnk_m, *(drnk_c)*sizeof(struct drink));

	free(dsrt_m);
	free(drnk_m);

	return *(dsrt_c) + *(drnk_c);
}

void refresh_monitor(int fd)
{
	int all = 0;
	int dsrt_c = 0, drnk_c = 0;
	struct dessert	*dsrt_p = NULL;
	struct drink	*drnk_p = NULL;
	char* mesg = NULL;

	all = select_everything(&dsrt_p, &drnk_p, &dsrt_c, &drnk_c);

	monitor(dsrt_p, drnk_p, dsrt_c, drnk_c, &mesg);

	if(write(fd, mesg, BUFSIZE) == -1)
	{ perror("refresh failure\n"); }

	free(dsrt_p);
	free(drnk_p);
	free(mesg);
}

//updated in 06.04 by jh
void refresh_monitor_all(struct pollfd* p_fds)
{
	int all = 0;
	int dsrt_c = 0, drnk_c = 0;
	struct dessert	*dsrt_p = NULL;
	struct drink	*drnk_p = NULL;
	char* mesg = NULL;

	all = select_everything(&dsrt_p, &drnk_p, &dsrt_c, &drnk_c);

	monitor(dsrt_p, drnk_p, dsrt_c, drnk_c, &mesg);

	p_fds++;
	for(int i=POLL_START+1; i<CLIENT_MAX; i++)
	{
		/*updated**********/
		if(p_fds->fd != -1)
		{
			if(write(p_fds->fd, mesg, BUFSIZE) == -1)
			{ perror("refresh failure\n"); }
		}
		p_fds++;
		/******************/
	}

	free(dsrt_p);
	free(drnk_p);
	free(mesg);
}

//updated in 05.31 by jh
void monitor(struct dessert* dsrt_p, struct drink* drnk_p, int dsrt_c, int drnk_c, char** mesg)
{
	*mesg = (char*)malloc(BUFSIZE*sizeof(char));
	/***************updated****************/
	memset(*mesg, 0x00, BUFSIZE);

	char *refresh = "refresh ";
	char dsrt_mesg[BUFSIZE/2] = "";
	char drnk_mesg[BUFSIZE/2] = "";
	char dsrt_bar[WIDTH] = "";
	char drnk_bar[WIDTH] = "";
	char dsrt_att[WIDTH] = "";
	char drnk_att[WIDTH] = "";
	char menu[WIDTH] = "";
	/**************************************/
	//test
	strcpy(dsrt_bar, "--------------------dessert--------------------\n");
	strcpy(drnk_bar, "---------------------drink---------------------\n");

	sprintf(dsrt_att, "%s\t\t%s\t\t%s\n", "name", "price", "number");
	sprintf(drnk_att, "%s\t\t%s\t%s\t%s\n", "name", "S", "M", "L");

	strcat(dsrt_mesg, dsrt_bar);
	strcat(dsrt_mesg, dsrt_att);
	strcat(drnk_mesg, drnk_bar);
	strcat(drnk_mesg, drnk_att);

	for(int i=0; i<dsrt_c; i++)
	{
		if(dsrt_p[i].id == 0) continue;
		menu[0] = '\0';

		sprintf(menu, "%s\t\t%d\t\t%d\n",
				(dsrt_p+i)->name, (dsrt_p+i)->price, (dsrt_p+i)->number);

		strcat(dsrt_mesg, menu);
	}

	for(int i=0; i<drnk_c; i++)
	{
		if(drnk_p[i].id == 0) continue;
		menu[0] = '\0';

		sprintf(menu, "%s\t\t%d\t%d\t%d\n",
				(drnk_p+i)->name, (drnk_p+i)->price[0], (drnk_p+i)->price[1], (drnk_p+i)->price[2]);

		strcat(drnk_mesg, menu);
	}

	strcat(*mesg, refresh);
	strcat(*mesg, dsrt_mesg);
	strcat(*mesg, drnk_mesg);
}

//updated in 06.05 by jh
void* server_thread(void* data)
{
	int serv_sock;
	struct pollfd p_fds[CLIENT_MAX];
	void *ret = NULL;

	serv_sock = server_init();
	pollfd_init(p_fds, serv_sock);
	//queue_init(&queue);

	printf("server online\n");

	while(1)
	{
		if(CLOSE_FLAG == 1)
		{
			//server close
			printf("server offline\n");
			server_close(p_fds); //updated
			return ret;
		}
		if(poll(p_fds, CLIENT_MAX, 5000) < 0)
		{
			//printf("server - poll timeout\n");
			continue;
		}
		//accept client
		if(p_fds[POLL_START].revents & POLLIN)
		{
			accept_client(p_fds, serv_sock);
		}
		//process message
		process_message(p_fds, serv_sock);

		/*********updated*******/
		if(REFRESH_FLAG == 1)
		{
			refresh_monitor_all(p_fds);
			sem_wait(&semaphore);
			REFRESH_FLAG = 0;
			sem_post(&semaphore);
		}
		/***********************/
	}
}

//updated in 06.04 by jh
void* manager_thread(void* data)
{
	void* ret = NULL;
	int argc = 0, num = 0, len = 0;
	char mesg[BUFSIZE];
	char **argv;

	void* menu = NULL;

	sleep(1);

	printf("manager online\n");

	while(1)
	{
		printf("\n<press enter to continue>");
		clear_buffer();
		memset(mesg, 0x00, BUFSIZE);
		argc = 0; num = 0;
		argv = NULL; menu = NULL;

		printf("input command: ");
		fgets(mesg, BUFSIZE, stdin);
		len = strlen(mesg);

		if(len > 0 && mesg[len-1] == '\n')
		{
			mesg[len-1] = '\0';
		}

		if(!strcmp(mesg, "close server"))
		{
			CLOSE_FLAG = 1;
			free(menu);
			free(argv);
			return menu;
		}

		argc = analyze_message(mesg, &argv);

		if(argc < 2)
		{
			perror("wrong input format\n");
			for(int i=0; i<argc; i++)
				free(argv[i]);

			free(argv);
			continue;
		}

		num = manager(argc, argv, &menu);

		if(menu != NULL)
		{
			if(is_dessert(argv[1]))
			{
				struct dessert *dsrt_p = (struct dessert*)malloc(sizeof(struct dessert)*num);
				memcpy(dsrt_p, menu, sizeof(struct dessert)*num);

				for(int i=0; i<num; i++)
				{
					if(dsrt_p[i].id == 0) continue;
					printf("id: %d\tname: %s\tprice: %d\tnumber: %d\n",
							dsrt_p[i].id, dsrt_p[i].name, dsrt_p[i].price, dsrt_p[i].number);
				}
				free(dsrt_p);
			}
			else
			{
				struct drink *drnk_p = (struct drink*)malloc(sizeof(struct drink)*num);
				memcpy(drnk_p, menu, sizeof(struct drink)*num);

				for(int i=0; i<num; i++)
				{
					if(drnk_p[i].id == 0) continue;
					printf("id: %d\tname: %s\tprice: %d < %d < %d\n",
							drnk_p[i].id, drnk_p[i].name, drnk_p[i].price[0], drnk_p[i].price[1], drnk_p[i].price[2]);
				}
				free(drnk_p);
			}

		}


		/**updated***************/
		if(num == 0 && REFRESH_FLAG == 0)
		{
			sem_wait(&semaphore);
			REFRESH_FLAG = 1;
			sem_post(&semaphore);
		}
		/************************/
		for(int i=0; i<argc; i++)
		{
			free(argv[i]);
		}

		free(menu);
		free(argv);
		sleep(1);
	}
}

/**      ================================================  		**/
//created in 5.30 by tae hyeon

void create_dessert_table(){
	create_table(DESSERT);
}
void create_drink_table(){
	create_table(DRINK);
}

void remove_dessert_table(){
	remove_table(DESSERT);
}
void remove_drink_table(){
	remove_table(DRINK);
}

void create_sales(){	//created in 5.31 by tae hyeon
	create_sales_table(SALES);
}
void remove_sales(){	//created in 5.31 by tae hyeon
	remove_table(SALES);
}
void print_sales(){		//created in 5.31 by tae hyeon
	print_sales_table(SALES);
}

//created in 06.06 by jh
int mesg_recv(int fd, char* mesg)
{
	FILE* log_fp;
	time_t date_time;
	struct tm* dt;

	//file open failure
	if((log_fp = fopen("log.txt", "a")) == NULL)
	{
		return -1;
	}

	//read failure
	if(read(fd, mesg, BUFSIZE) < 0)
	{
		write(fd, "reject order", BUFSIZE);
		return -1;
	}
	else
	{
		if(strlen(mesg) > 1023)
		{
			write(fd, "reject order", BUFSIZE);
			return -1;
		}
		else
		{
			date_time = time(NULL);
			dt = localtime(&date_time);
			fprintf(log_fp, "[recv] [%d-%0*d-%0*d %0*d:%0*d:%0*d %s] [%s]\n",
					dt->tm_year+1900, 2, dt->tm_mon+1, 2, dt->tm_mday, 2, dt->tm_hour, 2, dt->tm_min, 2, dt->tm_sec, dt->tm_zone,
					mesg);
		}
	}

	fclose(log_fp);
	return 0;
}

//created in 06.06 by jh
void mesg_send(int fd, char* mesg)
{
	FILE* log_fp;
	time_t date_time;
	struct tm* dt;

	//file open failure
	if((log_fp = fopen("log.txt", "a")) == NULL)
	{
		return;
	}

	write(fd, mesg, BUFSIZE);
	date_time = time(NULL);
	dt = localtime(&date_time);
	fprintf(log_fp, "[send] [%d-%0*d-%0*d %0*d:%0*d:%0*d %s] [%s]\n",
			dt->tm_year+1900, 2, dt->tm_mon+1, 2, dt->tm_mday, 2, dt->tm_hour, 2, dt->tm_min, 2, dt->tm_sec, dt->tm_zone,
			mesg);

	fclose(log_fp);
}

void switch_server_status(int status){ //created in 6.06 by tae hyeon
	START_FLAG = status;
//	printf("[server]START_FLAG: %d\n", START_FLAG); test code

}

void log_server_open() //created in 06.07 by jh
{
	FILE* log_fp;
	time_t date_time;
	struct tm* dt;

	//file open failure
	if((log_fp = fopen("log.txt", "a")) == NULL)
	{
		return;
	}

	date_time = time(NULL);
	dt = localtime(&date_time);
	fprintf(log_fp, "[info] [%d-%0*d-%0*d %0*d:%0*d:%0*d %s] [%s]\n",
			dt->tm_year+1900, 2, dt->tm_mon+1, 2, dt->tm_mday, 2, dt->tm_hour, 2, dt->tm_min, 2, dt->tm_sec, dt->tm_zone,
			"server open");

	fclose(log_fp);
}

void log_server_close() //created in 06.07 by jh
{
	FILE* log_fp;
	time_t date_time;
	struct tm* dt;

	//file open failure
	if((log_fp = fopen("log.txt", "a")) == NULL)
	{
		return;
	}

	date_time = time(NULL);
	dt = localtime(&date_time);
	fprintf(log_fp, "[info] [%d-%0*d-%0*d %0*d:%0*d:%0*d %s] [%s]\n",
			dt->tm_year+1900, 2, dt->tm_mon+1, 2, dt->tm_mday, 2, dt->tm_hour, 2, dt->tm_min, 2, dt->tm_sec, dt->tm_zone,
			"server close");

	fclose(log_fp);
}
