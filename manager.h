#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>

#include "cafe.h"
#include "message.h"

#define MAX_RETRY_COUNT 5

#define SERVER_OPENED 1 // define by tae hyeon 6.06
#define SERVER_CLOSED 0 // define by tae hyeon 6.06

int START_FLAG = 0; //new variable in 06.06 by tae hyeon

int manager(int argc, char* argv[], void** menu);

int create_table(char* table);
int remove_table(char* table);

int select_entity_all(char* table, void** menu);
int select_entity_ids(char* table, int* ids, int num, void** menu);
int select_entity_nms(char* table, char** names, int num, void** menu);
int insert_entity(char* table, int* ids, int num);
int update_entity(char* table, int* ids, int num);
int delete_entity(char* table, int* ids, int num);

int is_dessert(char* table);
int is_drink(char* table);
size_t file_size(char* table);
//void clear_buffer();

void settingLock(struct flock* filelock, short whence, short lockType, off_t start, off_t len);

int create_sales_table(char* table); // 5.31 tae hyeon
int update_sales_table(char* table, int sales); // 5.31 tae hyeon
int print_sales_table(char* table);// 5.31 tae hyeon

int manager(int argc, char* argv[], void** menu)
{
	int n = argc - 2;
	size_t file_s = 0;

	if(!(is_dessert(argv[1]) ^ is_drink(argv[1])))
	{
		printf("wrong table name\n");
		return -1;
	}

	if(n < 0)
	{
		printf("m argc: %d\n", argc);
		printf("wrong input format\n");
		return -1;
	}
	else if(n == 0)
	{
		//create
		if(!strcmp(argv[0], "create"))
		{
			if(START_FLAG == SERVER_CLOSED){
				create_table(argv[1]);
			}
			else{
				printf("you can't create table when server is already started.\n");
			}
		}
		//remove
		else if(!strcmp(argv[0], "remove"))
		{
			if(START_FLAG == SERVER_CLOSED){
				remove_table(argv[1]);
			}
			else{
				printf("you can't create table when server is already started.\n");
			}
		}
		else
		{
			printf("wrong command input\n");
			return -1;
		}
	}
	else
	{
		//select
		if(!strcmp(argv[0], "select"))
		{
			//select all - need to except where id == 0
			if(!strcmp(argv[2], "all"))
			{
				n = select_entity_all(argv[1], menu);

				return n;
			}
			//select with ids
			else if(!strcmp(argv[2], "id"))
			{
				n--;
				int* ids = (int*)malloc(n*sizeof(int));
				for(int i=0; i<n; i++) { ids[i] = atoi(argv[i+3]); }

				select_entity_ids(argv[1], ids, n, menu);

				free(ids);

				return n;
			}
			//select with names
			else if(!strcmp(argv[2], "name"))
			{
				n--;
				char** names = (char**)malloc(n*sizeof(char*));
				for(int i=0; i<n; i++)
				{
					names[i] = (char*)malloc(strlen(argv[i+3])*sizeof(char));
					strcpy(names[i], argv[i+3]);
				}

				select_entity_nms(argv[1], names, n, menu);

				for(int i=0; i<n; i++)
					free(names[i]);
				free(names);

				return n;
			}
			else
			{
				printf("wrong command input\n");
				return -1;
			}
		}
		//insert
		else if(!strcmp(argv[0], "insert"))
		{
			int* ids = (int*)malloc(n*sizeof(int));
			for(int i=0; i<n; i++) { ids[i] = atoi(argv[i+2]); }

			insert_entity(argv[1], ids, n);

			free(ids);
		}
		else if(!strcmp(argv[0], "update"))
		{
			int* ids = (int*)malloc(n*sizeof(int));
			for(int i=0; i<n; i++) { ids[i] = atoi(argv[i+2]); }

			update_entity(argv[1], ids, n);

			free(ids);
		}
		else if(!strcmp(argv[0], "delete"))
		{
			int* ids = (int*)malloc(n*sizeof(int));
			for(int i=0; i<n; i++) { ids[i] = atoi(argv[i+2]); }

			delete_entity(argv[1], ids, n);

			free(ids);
		}
		else
		{
			printf("wrong command input\n");
			return -1;
		}
	}

	return 0;
}

int create_table(char* table)
{
	FILE* fp = NULL;
	struct dessert	dsrt;
	struct drink	drnk;
	size_t dsrt_s = sizeof(struct dessert);
	size_t drnk_s = sizeof(struct drink);

	if((fp = fopen(table, "wb")) == NULL)
	{
		printf("file open failure\n");
		return -1;
	}

	if(is_dessert(table))
	{
		printf("id\tname\tprice\tnumber\n");

		while(1)
		{
			scanf("%d\t%s\t%d\t%d", &dsrt.id, dsrt.name, &dsrt.price, &dsrt.number);
			clear_buffer();

			if(dsrt.id < 1) break;

			fseek(fp, (dsrt.id - START_ID)*dsrt_s, SEEK_SET);

			fwrite(&dsrt, dsrt_s, 1, fp);
		}
	}
	else
	{
		printf("id\tname\tprice\n");
		while(1)
		{
			scanf("%d\t%s\t%d", &drnk.id, drnk.name, &drnk.price[1]);
			clear_buffer();

			if(drnk.id < 1) break;

			drnk.price[0] = (int)(drnk.price[1] * 0.8);
			drnk.price[2] = (int)(drnk.price[1] * 1.2);

			fseek(fp, (drnk.id - START_ID)*drnk_s, SEEK_SET);

			fwrite(&drnk, drnk_s, 1, fp);
		}
	}
	printf("%s created\n", table);

	fclose(fp);
	return 0;
}

int remove_table(char* table)
{
	if(remove(table) == 0)
	{ printf("%s deleted\n", table); }
	else
	{ printf("delete failure\n"); return -1; }

	return 0;
}

int select_entity_all(char* table, void** menu)
{
	FILE *fp = NULL;
	struct dessert	*dsrt_p = NULL;
	struct drink	*drnk_p = NULL;
	size_t dsrt_s = sizeof(struct dessert);
	size_t drnk_s = sizeof(struct drink);
	size_t file_s = file_size(table);

	struct flock fl;
	int num = 0;

	if((fp = fopen(table, "rb")) == NULL)
	{
		printf("file open failure\n");
		return -1;
	}

	if(is_dessert(table))
	{
		num = (int)(file_s/dsrt_s);
		*menu = (void*)malloc(file_s);
		dsrt_p = (struct dessert*)malloc(file_s);

		for(int i=0; i<num; i++)
		{
			fread(&dsrt_p[i], dsrt_s, 1, fp);
		}

		memcpy(*menu, dsrt_p, file_s);
	}
	else
	{
		num = (int)(file_s/drnk_s);
		*menu = (void*)malloc(file_s);
		drnk_p = (struct drink*)malloc(file_s);

		for(int i=0; i<(int)(file_s/drnk_s); i++)
		{
			fread(&drnk_p[i], drnk_s, 1, fp);
		}

		memcpy(*menu, drnk_p, file_s);
	}

	free(dsrt_p);
	free(drnk_p);
	fclose(fp);
	return num;
}

int select_entity_ids(char* table, int* ids, int num, void** menu)
{
	FILE *fp = NULL;
	struct dessert	*dsrt_p = NULL;
	struct drink	*drnk_p = NULL;
	size_t dsrt_s = sizeof(struct dessert);
	size_t drnk_s = sizeof(struct drink);
	size_t size = 0;

	if((fp = fopen(table, "rb")) == NULL)
	{
		printf("file open failure\n");
		return -1;
	}

	if(is_dessert(table))
	{
		size = num*dsrt_s;
		*menu = (void*)malloc(size);
		dsrt_p = (struct dessert*)malloc(size);

		for(int i=0; i<num; i++)
		{
			fseek(fp, (ids[i]-START_ID)*dsrt_s, SEEK_SET);
			fread(&dsrt_p[i], dsrt_s, 1, fp);
			/*
			printf("id: %d\tname: %s\tprice: %d\tnumber: %d\n",
					dsrt_p[i].id, dsrt_p[i].name, dsrt_p[i].price, dsrt_p[i].number);
			*/
		}

		memcpy(*menu, dsrt_p, size);
	}
	else
	{
		size = num*drnk_s;
		*menu = (void*)malloc(size);
		drnk_p = (struct drink*)malloc(size);

		for(int i=0; i<num; i++)
		{
			fseek(fp, (ids[i]-START_ID)*drnk_s, SEEK_SET);
			fread(&drnk_p[i], drnk_s, 1, fp);
			/*
			printf("id: %d\tname: %s\tprice: %d < %d < %d\n",
					drnk_p[i].id, drnk_p[i].name, drnk_p[i].price[0], drnk_p[i].price[1], drnk_p[i].price[2]);
			*/
		}

		memcpy(*menu, drnk_p, size);
	}

	free(dsrt_p);
	free(drnk_p);
	fclose(fp);
	return num;
}

int select_entity_nms(char* table, char** names, int num, void** menu)
{
	FILE* fp = NULL;
	struct dessert	*dsrt_p = NULL;
	struct drink	*drnk_p = NULL;
	size_t dsrt_s = sizeof(struct dessert);
	size_t drnk_s = sizeof(struct drink);
	size_t file_s = file_size(table);
	size_t size = 0;

	char name[NAMESIZE] = {0};
	int where = 0;
	int all = 0;
	int len = 0;

	if((fp = fopen(table, "rb")) == NULL)
	{
		printf("file open failure\n");
		return -1;
	}

	if(is_dessert(table))
	{
		all = (int)(file_s/dsrt_s);
		size = num*dsrt_s;
		*menu = (void*)malloc(size);
		dsrt_p = (struct dessert*)malloc(size);


		for(int i=0; i<num; i++)
		{
			where = 0; len = strlen(names[i]);
			while(1)
			{
				fseek(fp, where*dsrt_s, SEEK_SET);
				fread(name, len, 1, fp);

				if(!strncmp(name, names[i], len))
				{
					fseek(fp, -len, SEEK_CUR);
					fread(&dsrt_p[i], dsrt_s, 1, fp);
					break;
				}
				else where++;

				if(where >= all)
				{
					//printf("%s not exist\n", names[i]);
					memset(&dsrt_p[i], 0x00, dsrt_s);
					num = -1;
					break;
				}
			}

			if(where >= all) continue;
		}

		memcpy(*menu, dsrt_p, size);
	}
	else
	{
		all = (int)(file_s/drnk_s);
		size = num*drnk_s;
		*menu = (void*)malloc(size);
		drnk_p = (struct drink*)malloc(size);

		for(int i=0; i<num; i++)
		{
			where = 0; len = strlen(names[i]);
			while(1)
			{
				fseek(fp, where*drnk_s, SEEK_SET);
				fread(name, len, 1, fp);

				if(!strncmp(name, names[i], len))
				{
					fseek(fp, -len, SEEK_CUR);
					fread(&drnk_p[i], drnk_s, 1, fp);
					break;
				}
				else where++;

				if(where >= all)
				{
					//printf("%s not exist\n", names[i]);
					memset(&drnk_p[i], 0x00, drnk_s);
					num = -1;
					break;
				}
			}

			if(where >= all) continue;
		}

		memcpy(*menu, drnk_p, size);
	}

	free(dsrt_p);
	free(drnk_p);
	fclose(fp);
	return num;
}

int insert_entity(char* table, int* ids, int num)
{
	FILE *fp = NULL;
	struct dessert	dsrt;
	struct drink	drnk;
	size_t dsrt_s = sizeof(struct dessert);
	size_t drnk_s = sizeof(struct drink);

	struct flock fl;
	int fd=0, retry_count = 0, number_of_entity = 0;
	bool success = false;

	if((fp = fopen(table, "rb+")) == NULL)
	{
		perror("file open failure\n");
		return -1;
	}

	fd = fileno(fp);

	if(is_dessert(table))
	{
		printf("id\tname\tprice\tnumber\n");
		for(int i=0; i<num; i++)
		{
			dsrt.id = ids[i];
			printf("%d\t", ids[i]);
			scanf("%s\t%d\t%d", dsrt.name, &dsrt.price, &dsrt.number);
			clear_buffer();
			fseek(fp, (dsrt.id - START_ID)*dsrt_s, SEEK_SET);

			settingLock(&fl, SEEK_SET, F_WRLCK, ftell(fp), dsrt_s);

			while(retry_count<MAX_RETRY_COUNT){  	//try lock until max retry count
				if(fcntl(fd, F_SETLK, &fl) != -1){ 	//if lock succeed, break out
					success = true;
					break;
				}  /// end of if
				sleep(2);
				retry_count++;
			} // end of while

			fwrite(&dsrt, dsrt_s, 1, fp);

			fl.l_type = F_UNLCK;    //unlock sequence

			if(fcntl(fd, F_SETLK, &fl) == -1){
				printf("file unlock failed\n");
				return -2;
			}// end of if
		}
	}
	else
	{
		printf("id\tname\tprice\n");
		for(int i=0; i<num; i++)
		{
			drnk.id = ids[i];
			printf("%d\t", ids[i]);
			scanf("%s\t%d", drnk.name, &drnk.price[1]);
			clear_buffer();
			drnk.price[0] = (int)(drnk.price[1] * 0.8);
			drnk.price[2] = (int)(drnk.price[1] * 1.2);
			fseek(fp, (drnk.id - START_ID)*drnk_s, SEEK_SET);

			settingLock(&fl, SEEK_SET, F_WRLCK, ftell(fp), drnk_s);

			while(retry_count<MAX_RETRY_COUNT){  	//try lock until max retry count
				if(fcntl(fd, F_SETLK, &fl) != -1){ 	//if lock succeed, break out
					success = true;
					break;
				}  // end of if
			}// end of while

			fwrite(&drnk, drnk_s, 1, fp);

			fl.l_type = F_UNLCK;    //unlock sequence
			if(fcntl(fd, F_SETLK, &fl) == -1){
				printf("file unlock failed\n");
				return -2;
			}// end of if
		}
	}

	fclose(fp);
	return 0;
}

int update_entity(char* table, int* ids, int num)
{
	FILE *fp = NULL;
	struct dessert	dsrt;
	size_t dsrt_s = sizeof(struct dessert);

	int fd = 0, retry_count=0;
	bool success = false;
	struct flock fl;

	if(num%2 == 1)
	{
		perror("wrong number of arguments\n");
		return -1;
	}

	if((fp = fopen(table, "rb+")) == NULL)
	{
		perror("file open failure\n");
		return -1;
	}

	fd = fileno(fp);

	if(is_dessert(table))
	{
		for(int i=0; i<num/2; i++)
		{
			fseek(fp, (ids[i*2] - START_ID)*dsrt_s, SEEK_SET);

			settingLock(&fl, SEEK_SET, F_RDLCK, ftell(fp), dsrt_s);

			while(retry_count<MAX_RETRY_COUNT){		//try lock until max retry count
				if(fcntl(fd, F_SETLK, &fl) != -1){ 	//if lock succeed, break out
					success = true;
					break;
				}// end of if
				sleep(2);
				retry_count++;
			}// end of while

			if(fread(&dsrt, dsrt_s, 1, fp) > 0 && (dsrt.id > 0))
			{
				dsrt.number += ids[i*2+ 1];
				fseek(fp, -dsrt_s, SEEK_CUR);
				fwrite(&dsrt, dsrt_s, 1, fp);
			}// end of if
			else { /*printf("dessert_id %d is not found\n", ids[i])*/ }

			fl.l_type = F_UNLCK;
			if(fcntl(fd, F_SETLK, &fl) == -1){
				printf("file unlock failed\n");
				return -2;
			}// end of if
		}
	}
	else
	{
		//change after
		perror("can't update number of drink\n");
		return -1;
	}

	fclose(fp);
	return 0;
}

int delete_entity(char* table, int* ids, int num)
{
	FILE* fp = NULL;
	struct dessert	dsrt;
	struct drink	drnk;
	size_t dsrt_s = sizeof(struct dessert);
	size_t drnk_s = sizeof(struct drink);

	int fd = 0, retry_count=0;
	bool success = false;
	struct flock fl;

	memset(&dsrt, 0x00, dsrt_s);
	memset(&drnk, 0x00, drnk_s);

	char c;

	if((fp = fopen(table, "rb+")) == NULL)
	{
		printf("file open failure\n");
		return -1;
	}

	fd = fileno(fp);

	if(is_dessert(table))
	{
		for(int i=0; i<num; i++)
		{
			fseek(fp, (ids[i] - START_ID)*dsrt_s, SEEK_SET);

			settingLock(&fl, SEEK_SET, F_WRLCK, ftell(fp), dsrt_s);

			while(retry_count<MAX_RETRY_COUNT){  	//try lock until max retry count
				if(fcntl(fd, F_SETLK, &fl) != -1){ 	//if lock succeed, break out
					success = true;
					break;
				}// end of if ===============
				sleep(2);
				retry_count++;
			}  //end of while

			fwrite(&dsrt, sizeof(dsrt), 1, fp);

			fl.l_type = F_UNLCK;    //unlock sequence
			if(fcntl(fd, F_SETLK, &fl) == -1){
				printf("file unlock failed\n");
				return -2;
			}// end of if
		}// end of for
	}// end of if
	else
	{
		for(int i=0; i<num; i++)
		{
			fseek(fp, (ids[i] - START_ID)*drnk_s, SEEK_SET);

			settingLock(&fl, SEEK_SET, F_WRLCK, ftell(fp), drnk_s);

			while(retry_count<MAX_RETRY_COUNT){  	//try lock until max retry count
				if(fcntl(fd, F_SETLK, &fl) != -1){  //if lock succeed, break out
					success = true;
					break;
				}// end of if
				sleep(2);
				retry_count++;
			}  // end of while

			fwrite(&drnk, sizeof(drnk), 1, fp);
			printf("id: %d deleted\n", ids[i]);

			fl.l_type = F_UNLCK;    //unlock sequence
			if(fcntl(fd, F_SETLK, &fl) == -1){
				printf("file unlock failed\n");
				return -2;
			}// end of if
		}// end of for
	}

	fclose(fp);
	close(fd);   			//===============================

	return 0;
}

int is_dessert(char* table)
{
	if(!strcmp(table, DESSERT)) return 1;
	else return 0;
}

int is_drink(char* table)
{
	if(!strcmp(table, DRINK)) return 1;
	else return 0;
}

size_t file_size(char* table)
{
	struct stat buf;
	if(stat(table, &buf) < 0)
	{
		printf("stat check failure\n");
		return -1;
	}
	return buf.st_size;
}

/*
void clear_buffer()
{
	while(getchar()!= '\n');
}
*/

void settingLock(struct flock* filelock, short whence, short lockType, off_t start, off_t len) {
    filelock->l_type = lockType;
    filelock->l_whence = whence;
    filelock->l_start = start;
    filelock->l_len = len;
}

int create_sales_table(char* table) //5.31 tae hyeon
{
	FILE* fp = NULL;    //create empty sales file
	struct sales sale;
	/*
	if ((fp = fopen(table, "ab")) != NULL) {   // if file already exist, not initialize total_sales
		fread(&sale, sizeof(struct sales), 1, fp);
		printf("The sales file is already exist.\n");

		fclose(fp);
	} else {
		sale.total_sales = 0;
	}
	*/
	//remove content if file written already
	if((fp = fopen(table, "wb")) == NULL)
	{
		printf("file open failure\n");
		return -1;
	}

	//fwrite(&sale, sizeof(struct sales), 1, fp);

	fclose(fp);
	return 0;
}

int update_sales_table(char* table, int sale_money){ //5.31 tae hyeon
		FILE *fp = NULL;
		struct sales sale;
		size_t sale_s = sizeof(struct sales);
		size_t file_s = file_size(table);

		if((fp = fopen(table, "ab")) == NULL)//
		{
			perror("file open failure\n");
			return -1;
		}

		sale.index = (int)(file_s/sale_s) + 1;
		sale.total_sales = sale_money;
		sale.sale_time = time(NULL);

		fwrite(&sale, sale_s, 1, fp); //

		//fread(&sale, sizeof(struct sales), 1, fp);

		//sale.total_sales += sales;

		//fseek(fp, 0, SEEK_SET);
		//fwrite(&sale, sizeof(struct sales), 1, fp);

		fclose(fp);
		return 0;
}

int print_sales_table(char* table){ //5.31 tae hyeon
		FILE *fp = NULL;
		struct sales sale;
		size_t sale_s = sizeof(struct sales);
		size_t file_s = file_size(table);
		int num = (int)(file_s/sale_s);

		if((fp = fopen(table, "rb")) == NULL)
		{
			perror("file open failure\n");
			return -1;
		}

		printf("\nindex\tsales\tsale_time\n");

		for(int i=0; i<num; i++)
		{
			fseek(fp, i*sale_s, SEEK_SET);
			fread(&sale, sale_s, 1, fp);
			printf("%d\t%d\t%s", sale.index, sale.total_sales, ctime(&sale.sale_time));
		}

		//fread(&sale, sizeof(struct sales), 1, fp);

		//printf("Total sales : %d ", sale.total_sales);

		fclose(fp);
		return 0;
}
