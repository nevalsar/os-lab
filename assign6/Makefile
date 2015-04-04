run_server: server commence
	./server

run_client: client
	./client

client: client.c
	gcc -g -Wall client.c -o client

commence: commence.c 
	gcc -g -Wall commence.c -o commence

server: server.c
	gcc -g -Wall server.c -o server

clean:
	sh ipc_rm.sh