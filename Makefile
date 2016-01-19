build: client.c
	gcc client.c -o client

clean: client
	rm client
