#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h> // included by jh 06.01

#include "server.h"

int main(int argc, char *argv[])
{
	signal(SIGINT, SIG_IGN); //ignore ^C - jh 06.01

	if(argc < 2)
	{
		fprintf(stderr, "usage: %s max_client\n", argv[0]);
		return -1;
	}
	else
	{
		if((CLIENT_MAX = atoi(argv[1])) < 1)
		{
			error_handling("max client should be bigger than 0");
		}
		else
		{
			CLIENT_MAX++;
		}
	}

	pthread_t sthread, mthread;
	int sthread_id, mthread_id;
	int sstatus, mstatus;

	/*semaphore updated**********************/
	int sem_stat = 0;
	sem_stat = sem_init(&semaphore,0,1);
	/****************************************/

	/**       new code by tae hyeon 5.30                      **/
	int choice=99;

	create_sales(); // initial sales file make - tae hyeon 5.31

	while(choice != 0){
		printf("\n");
		printf("-----server start menu-----\n");
		printf("0.Start server\n");
		printf("1.Create table\n");
		printf("2.Remove table\n");
		printf("3.Print total sales\n");
		printf("Enter your choice: ");
		scanf("%d", &choice);
		clear_buffer();

		if(choice == 1){
			printf("\n");
			printf("What table do you create?\n");
			printf("4.Dessert table\n");
			printf("5.Drink table\n");
			printf("6.Sales table\n");
			printf("Enter your choice: ");
			scanf("%d", &choice);
			clear_buffer();

			printf("\n");
			if(choice == 4){ //create dessert table
				create_dessert_table();
			}
			else if(choice == 5){ //create drink table
				create_drink_table();
			}
			else if(choice == 6){
				create_sales();
			}
			else if(choice == 0){
				printf("Invalid choice. Please enter the correct number.\n");
				choice = 99;
			}
			else{
				printf("Invalid choice. Please enter the correct number.\n");
			}
		}
		else if(choice == 2){
			printf("\n");
			printf("What table do you remove?\n");
			printf("4.Dessert table\n");
			printf("5.Drink table\n");
			printf("6.Sales table\n");
			printf("Enter your choice: ");
			scanf("%d", &choice);
			clear_buffer();

			printf("\n");
			if(choice == 4){ //remove dessert table
				remove_dessert_table();
			}
			else if(choice == 5){ //remove drink table
				remove_drink_table();
			}
			else if(choice == 6){ //remove sales table
				remove_sales();
			}
			else if(choice == 0){
				printf("Invalid choice. Please enter the correct number.\n");
				choice = 99;
			}
			else{
				printf("Invalid choice. Please enter the correct number.\n");
			}
		}
		else if(choice == 3){
			printf("\n");
			print_sales();
		}
		else if(choice == 0){
			switch_server_status(SERVER_OPENED);  // new code by tae hyeon 6.06
			//printf("[test_server]START_FLAG: %d\n", START_FLAG); //test code
		}
		else{
			printf("Invalid choice. Please enter the correct number.\n");
		}
	}
	printf("Starting the server.....\n");
	printf("\n");

	/**  ===========================================================   **/

	sthread_id = pthread_create(&sthread, NULL, server_thread, NULL);
	mthread_id = pthread_create(&mthread, NULL, manager_thread, NULL);

	if(sthread_id < 0)
		error_handling("server thread create failure");
	if(mthread_id < 0)
		error_handling("manager thread create failure");

	pthread_join(sthread, (void**)&sstatus);
	pthread_join(mthread, (void**)&mstatus);

	/*semaphore updated**********************/
	sem_destroy(&semaphore);
	/****************************************/

	print_sales_table(SALES); //

	return 0;
}
