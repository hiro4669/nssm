#include <stdio.h>
#include <unistd.h>
#include "ssm-proxy-client.hpp"

PConnector::PConnector() {
	printf("PConnector constructor\n");
	sock = -1;
}

PConnector::~PConnector() {
	printf("PConnector destructor\n");	
	if (sock != -1) {
		close(sock);
	}
}

bool PConnector::connectToServer(char* serverName, int port) {
	printf("connect to server\n");


	sock = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(serverName);
	if(!connect(sock, (struct sockaddr *)&server, sizeof(server))) {
		fprintf(stderr, "connection error");
		return false;
	}
	printf("sock = %d\n", sock);
	printf("connect to server2	\n");
	return true;
}
