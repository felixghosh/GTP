server:
	gcc src/server.c -o out/server -g -O3
	./out/server

client:
	gcc src/client.c -o out/client -g -O3
	./out/client
