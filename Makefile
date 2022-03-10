server:
	gcc src/server.c -o server -g -O3
	./server

client:
	gcc src/client.c -o client -g -O3
	./client
