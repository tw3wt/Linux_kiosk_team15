all: client server

client: client.c client.h cafe.h info_sock.h errhandle.h message.h
	gcc -o client client.c -lpthread

server: server.c server.h cafe.h info_sock.h errhandle.h message.h manager.h
	gcc -o server server.c -lpthread

