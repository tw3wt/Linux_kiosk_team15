#define BUFSIZE 2048
#define WIDTH 51		//defined in 05.30 by jh
#define DELIMITER " "	//defined in 05.31 by jh

int analyze_message(char* mesg, char*** argv); //updated in 05.31 by jh
void clear_buffer();

//updated in 05.31 by jh
int analyze_message(char* mesg, char*** argv)
{
	//char *delimiter = " "; - removed.. use defined value DELIMITER
	char *temp = NULL, *str = NULL;
	int counter = 0;

	//updated if mesg has no space return -1
	if(strstr(mesg, DELIMITER) == NULL) return -1; // changed

	temp = strtok_r(mesg, DELIMITER, &str); // changed

	*argv = (char**)malloc(sizeof(char*)*(counter+1));
	**argv = (char*)malloc(sizeof(char)*strlen(temp));
	strcpy(**argv, temp); counter++;

	while((temp = strtok_r(NULL, DELIMITER, &str)) != NULL) // changed
	{
		*argv = (char**)realloc(*argv, sizeof(char*)*(counter+1));
		*(*argv+counter) = (char*)malloc(sizeof(char)*strlen(temp));

		strcpy(*(*argv+counter), temp); counter++;
	}

	return counter;
}

void clear_buffer()
{
	while(getchar()!= '\n');
}
