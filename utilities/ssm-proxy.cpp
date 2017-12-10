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

int ProxyServer::readInt(char **p) {
	uint8_t v1 = **p; (*p)++;
	uint8_t v2 = **p; (*p)++;
	uint8_t v3 = **p; (*p)++;
	uint8_t v4 = **p; (*p)++;

	int v =(int)( v1 << 24 | v2 << 16 | v3 << 8 | v4);
	return v;
}

long ProxyServer::readLong(char **p) {
	uint8_t v1 = **p; (*p)++;
	uint8_t v2 = **p; (*p)++;
	uint8_t v3 = **p; (*p)++;
	uint8_t v4 = **p; (*p)++;
	uint8_t v5 = **p; (*p)++;
	uint8_t v6 = **p; (*p)++;
	uint8_t v7 = **p; (*p)++;
	uint8_t v8 = **p; (*p)++;

	long lv = (long)((long)v1 << 56 | (long)v2 << 48 | (long)v3 << 40 | (long)v4 << 32
			| (long)v5 << 24 | (long)v6 << 16 | (long)v7 << 8 | (long)v8);
	return lv;
}

void ProxyServer::readRawData(char **p, char *d, int len) {
	for (int i = 0; i < len; ++i, (*p)++) {
		d[i] = **p;
	}
}

void ProxyServer::serializeMessage(ssm_msg *msg, char *buf) {
	msg->msg_type = readLong(&buf);
	msg->res_type = readLong(&buf);
	msg->cmd_type = readInt(&buf);
	readRawData(&buf, msg->name, 32);
	msg->suid = readInt(&buf);
	msg->ssize = readLong(&buf);
	msg->hsize = readLong(&buf);
	msg->time = readLong(&buf);

	/*
	printf("msg_type = %d\n", msg->msg_type);
	printf("res_type = %d\n", msg->res_type);
	printf("cmd_type = %d\n", msg->cmd_type);


	for (int i = 0; i < 10; ++i) {
		printf("%02x ", msg->name[i]);
	}
	printf("\n");
	printf("suid = %d\n", msg->suid);
	printf("ssize = %d\n", msg->ssize);
	printf("hsize = %d\n", msg->hsize);
	*/
}

void ProxyServer::handleCommand() {
	printf("handlecommand\n");
	ssm_msg msg;
	char *buf = (char*)malloc(sizeof(ssm_msg));
	while(true) {
		printf("wait recv\n");
		int len = recv(this->client.data_socket, buf, sizeof(ssm_msg), 0);
		printf("len = %d\n", len);
		if (len == 0) break;
		serializeMessage(&msg, buf);
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
