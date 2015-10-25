all: client server

client: client.c
	$(CC) client.c -o client -m32

server: server.c
	$(CC) server.c -o server -m32

clean:
	rm client server
