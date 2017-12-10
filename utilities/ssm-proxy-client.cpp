#include <stdio.h>
#include <stdlib.h>
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

int PConnector::readInt(char **p) {
	uint8_t v1 = **p; (*p)++;
	uint8_t v2 = **p; (*p)++;
	uint8_t v3 = **p; (*p)++;
	uint8_t v4 = **p; (*p)++;

	int v =(int)( v1 << 24 | v2 << 16 | v3 << 8 | v4);
	return v;
}

long PConnector::readLong(char **p) {
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

void PConnector::readRawData(char **p, char *d, int len) {
	for (int i = 0; i < len; ++i, (*p)++) {
		d[i] = **p;
	}
}

void PConnector::writeInt(char **p, int v) {
	**p = (v >> 24) & 0xff; (*p)++;
	**p = (v >> 16) & 0xff; (*p)++;
	**p = (v >> 8)  & 0xff; (*p)++;
	**p = (v >> 0)  & 0xff; (*p)++;
}

void PConnector::writeLong(char **p, long v) {
	**p = (v >> 56) & 0xff; (*p)++;
	**p = (v >> 48) & 0xff; (*p)++;
	**p = (v >> 40)  & 0xff; (*p)++;
	**p = (v >> 32)  & 0xff; (*p)++;
	this->writeInt(p, v);
}

void PConnector::writeRawData(char **p, char *d, int len) {
	for (int i = 0; i < len; ++i, (*p)++) **p = d[i];
}

bool PConnector::connectToServer(char* serverName, int port) {
	printf("connect to server\n");


	sock = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(serverName);
	if(connect(sock, (struct sockaddr *)&server, sizeof(server))) {
		fprintf(stderr, "connection error\n");
		return false;
	}
	printf("sock = %d\n", sock);
	printf("connect to server2	\n");
	return true;
}

void PConnector::serializeMessage(ssm_msg *msg, char *buf) {
	printf("serialize message\n");
	msg->msg_type = readLong(&buf);
	msg->res_type = readLong(&buf);
	msg->cmd_type = readInt(&buf);
	readRawData(&buf, msg->name, 32);
	msg->suid = readInt(&buf);
	msg->ssize = readLong(&buf);
	msg->hsize = readLong(&buf);
	msg->time = readLong(&buf);
}

bool PConnector::recvMsgFromServer(ssm_msg *msg, char *buf) {
	printf("ready to receive\n");
	int len = recv(sock, buf, sizeof(ssm_msg), 0);
	if (len > 0) {
		serializeMessage(msg, buf);
		return true;
	}
	return false;
}

bool PConnector::sendMsgToServer(int cmd_type, ssm_msg *msg) {
	ssm_msg msgbuf;
	char *buf, *p;
	if (msg == NULL) {
		msg = &msgbuf;
	}
	msg->msg_type = 1; // dummy
	msg->res_type = 8;
	msg->cmd_type = cmd_type;
	/* dummy start */
	for (int i = 0; i < 10; ++i) {
		msg->name[i] = 0x61 + i;
	}
	msg->suid = 16;
	msg->ssize = 17;
	msg->hsize = 18;
	msg->time = 4.2;
	/* dummy end */
	buf = (char*)malloc(sizeof(ssm_msg));
	p = buf;
	writeLong(&p, msg->msg_type);
	writeLong(&p, msg->res_type);
	writeInt(&p, msg->cmd_type);
	writeRawData(&p, msg->name, 32);
	writeInt(&p, msg->suid);
	writeLong(&p, msg->ssize);
	writeLong(&p, msg->hsize);
	writeLong(&p, msg->time);

	/*
	for (int i = 0; i < sizeof(ssm_msg); ++i) {
		if (i % 16 == 0) printf("\n");
		printf("%02x ", buf[i] & 0xff);
	}
	printf("\n");
	*/

	if (send(sock, buf, sizeof(ssm_msg), 0) == -1) {
		fprintf(stderr, "error happens\n");
	}
	free(buf);

	return true;
}
