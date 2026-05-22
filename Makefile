# Iago Zagnoli Albergaria e Marcos Daniel Souza Netto

CC = g++
CFLAGS = -Wall -O2 -fsanitize=address -std=c++17

all: server_build client_build

server_build: server/main.cpp server/controller.cpp utils/communication.cpp utils/message.cpp
	$(CC) $(CFLAGS) server/main.cpp -o server.out

client_build: client/main.cpp client/controller.cpp utils/communication.cpp utils/message.cpp
	$(CC) $(CFLAGS) client/main.cpp -o client.out

run_serv: server_build
	./server.out $(arg1) $(arg2) $(arg3)

run_cli: client_build
	./client.out $(arg1) $(arg2)

clean:
	rm -f server.out client.out
