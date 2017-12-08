#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>

#include "libssm.h"

#include "printlog.hpp"

#include "ssm-proxy.hpp"

extern pid_t my_pid; // for debug

ProxyServer::ProxyServer() {
	printf("Proxy Server created\n");
}

ProxyServer::~ProxyServer() {
	printf("proxy server deleted\n");
}

bool ProxyServer::init() {
	printf("init\n");
	memset(&this->server, 0, sizeof(this->server));
	this->server.wait_socket = -1;
	this->server.server_addr.sin_family      = AF_INET;
	this->server.server_addr.sin_addr.s_addr = htonl(SERVER_IP);
	this->server.server_addr.sin_port        = htons(SERVER_PORT);

	return this->open();
}

bool ProxyServer::open() {
	this->server.wait_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->server.wait_socket == -1) {
		perror("open socket error");
		return false;
	}
	if (bind(this->server.wait_socket, (struct sockaddr*)&this->server.server_addr,
			sizeof(this->server.server_addr)) == -1) {
		perror("server bind");
		return false;
	}
	if (listen(this->server.wait_socket, 5) == -1) {
		perror("server open");
		return false;
	}

	return true;
}

bool ProxyServer::wait() {
	memset(&this->client, 0, sizeof(this->client));
	this->client.data_socket = -1;
	for (;;) {
		socklen_t client_addr_len = sizeof(this->client.client_addr);
		this->client.data_socket = accept(this->server.wait_socket,
				(struct sockaddr*)&this->client.client_addr,
				&client_addr_len);

		if (this->client.data_socket != -1) break;
		if (errno == EINTR) continue;
		perror("server open accept");
		return false;
	}
	return true;
}

bool ProxyServer::server_close() {
	if (this->server.wait_socket != -1) {
		close(this->server.wait_socket);
		this->server.wait_socket = -1;
	}
	return true;
}

bool ProxyServer::client_close() {
	if (this->client.data_socket != -1) {
		close(this->client.data_socket);
		this->client.data_socket = -1;
	}
	return true;
}

void ProxyServer::handleCommand() {
	printf("handlecommand\n");
	char *buf = (char*)malloc(sizeof(ssm_msg));
	while(true) {
		printf("wait recv\n");
		int len = recv(this->client.data_socket, buf, sizeof(ssm_msg), 0);
		printf("len = %d\n", len);
		if (len == 0) break;
	}



	free(buf);
	/*
	if ( !initSSM ()) {
		//logError << "ssm init error." << std::endl;
		std::cerr << "Error" << std::endl;
		exit(1);
	}

	printf("mypid = %d\n", my_pid);
	*/
}




bool ProxyServer::run() {
	printf("run\n");
	while(wait()) {
		pid_t child_pid = fork();
		if (child_pid == -1) { // fork failed
			break;
		} else if (child_pid == 0) { // child
			this->server_close();
			this->handleCommand();
			this->client_close();
			printf("end of process");
			exit(1);
		} else { // parent
			this->client_close();
		}
	}
	server_close();
	return true;
}




int main(void) {
	ProxyServer server;
	printf("size_t size = %d\n", (int)sizeof(size_t));
	server.init();
	server.run();
	test();
	
	return 0;
}
