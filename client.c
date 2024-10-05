#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"

int main()
{
	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, sigpipe_handler);

	pthread_t request, receive;
	int request_id, receive_id;
	int request_st, receive_st;

	client_init("127.0.0.1", PORT);

	request_id = pthread_create(&request, NULL, receive_thread, NULL);
	receive_id = pthread_create(&receive, NULL, request_thread, NULL);

	if(request_id < 0)
		error_handling("request thread create failure");
	if(receive_id < 0)
		error_handling("receive thread create failure");

	pthread_join(request, (void**)&request_st);
	pthread_join(receive, (void**)&receive_st);

	return 0;
}
