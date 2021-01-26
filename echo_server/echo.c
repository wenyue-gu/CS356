#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

int server(uint16_t port);
int client(const char * addr, uint16_t port);

#define MAX_MSG_LENGTH (1300)
#define MAX_BACK_LOG (5)

int main(int argc, char ** argv)
{
	if (argc < 3 || (argv[1][0] == 'c' && argc < 4)) {
		printf("Usage: myprog c <port> <address> or myprog s <port>\n");
		return 0;
	}

	uint32_t number = atoi(argv[2]);
	if (number < 1024 || number > 65535) {
		fprintf(stderr, "port number should be larger than 1023 and less than 65536\n");
		return 0;
	}

	uint16_t port = atoi(argv[2]);
	
	if (argv[1][0] == 'c') {
		return client(argv[3], port);
	} else if (argv[1][0] == 's') {
		return server(port);
	} else {
		fprintf(stderr, "unkonwn commend type %s\n", argv[1]);
		return 0;
	}
	return 0;
}

int client(const char * addr, uint16_t port)
{
	int sock;
	struct sockaddr_in server_addr;
	char msg[MAX_MSG_LENGTH], reply[MAX_MSG_LENGTH];

	if ((sock = socket(AF_INET, SOCK_STREAM/* use tcp */, 0)) < 0) {
		perror("Create socket error:");
		return 1;
	}

	printf("Socket created\n");
	server_addr.sin_addr.s_addr = inet_addr(addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connect error:");
		return 1;
	}

	printf("Connected to server %s:%d\n", addr, port);
	printf("Enter message: \n");
	while (fgets(msg, MAX_MSG_LENGTH, stdin)) {
		if (send(sock, msg, strnlen(msg, MAX_MSG_LENGTH), 0) < 0) {
			perror("Send error:");
			return 1;
		}
		int recv_len = 0;
		if ((recv_len = recv(sock, reply, MAX_MSG_LENGTH, 0)) < 0) {
			perror("Recv error:");
			return 1;
		}
		if (recv_len == 0){
			printf("Server disconnected, client quit.\n");
			break;
		}
		reply[recv_len] = '\0';
		printf("Server reply:\n%s", reply);
		printf("Enter message: \n");
	}
	if (send(sock, "", 0, 0) < 0) {
		perror("Send error:");
		return 1;
	}
	return 0;
}

int server(uint16_t port)
{
	/*
		Add your code here
	*/
	return 0;
}
