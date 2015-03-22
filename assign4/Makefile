run_client: client.out
	./client.out

run_server: server.out
	./server.out

client.out: client.c
	gcc -g -Wall client.c -o client.out

server.out: server.c
	gcc -g -Wall server.c -o server.out

clean:
	sudo ipcrm -Q 1234 && sudo ipcrm -Q 1235