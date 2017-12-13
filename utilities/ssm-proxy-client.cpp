#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#include "ssm.h"
#include "ssm-proxy-client.hpp"

PConnector::PConnector() {
	printf("PConnector constructor\n");
	sock = -1;
	mDataSize = 0;
	streamId = 0;
	mData = NULL;
	mProperty = NULL;
	mPropertySize = 0;
	streamName = "";

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

double PConnector::readDouble(char **p) {
	char buf[8];
	for (int i = 0; i < 8; ++i, (*p)++) {
		buf[7-i] = **p;
	}
	return *(double*)buf;
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

void PConnector::writeDouble(char **p, double v) {
	char *dp = (char*)&v;
	for (int i = 0; i < 8; ++i, (*p)++) {
		**p = dp[7 - i] & 0xff;
	}
}

void PConnector::writeRawData(char **p, char *d, int len) {
	for (int i = 0; i < len; ++i, (*p)++) **p = d[i];
}

bool PConnector::connectToServer(const char* serverName, int port) {
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

void PConnector::initRemote() {
	ssm_msg msg;
	char *msg_buf = (char*)malloc(sizeof(ssm_msg));
	connectToServer("127.0.0.1", 8080);
	if(!sendMsgToServer(MC_INITIALIZE, NULL)) {
		fprintf(stderr, "error in initRemote\n");
	}
	if (recvMsgFromServer(&msg, msg_buf)) {
		printf("msg = %d\n", (int)msg.cmd_type);
	} else {
		fprintf(stderr, "fail recvMsg\n");
	}
	free(msg_buf);

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
	/*
	for (int i = 0; i < 10; ++i) {
		msg->name[i] = 0x61 + i;
	}
	msg->suid = 16;
	msg->ssize = 17;
	msg->hsize = 18;
	msg->time = 4.2;
	*/
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
	writeDouble(&p, msg->time);
	//writeLong(&p, msg->time);

	/*
	for (int i = 0; i < sizeof(ssm_msg); ++i) {
		if (i % 16 == 0) printf("\n");
		printf("%02x ", buf[i] & 0xff);
	}
	printf("\n");
	*/

	if (send(sock, buf, sizeof(ssm_msg), 0) == -1) {
		fprintf(stderr, "error happens\n");
		free(buf);
		return false;
	}
	free(buf);
	return true;
}

bool PConnector::sendData(const char *data, size_t size) {
	if (send(sock, data, size, 0) == -1) {
		fprintf(stderr, "error in sendData\n");
		return false;
	}
	return true;
}

void PConnector::setBuffer(void *data, size_t dataSize, void *property, size_t propertySize) {
	mData = data;
	mDataSize = dataSize;
	mProperty = property;
	mPropertySize = propertySize;
	//printf("data size = %d, propertysize = %d\n", mDataSize, mPropertySize);
}

void PConnector::setStream(const char *streamName, int streamId = 0) {
	this->streamName = streamName;
	this->streamId = streamId;
}

void PConnector::createRemoteSSM( const char *name, int stream_id, size_t ssm_size, ssmTimeT life, ssmTimeT cycle ) {
	ssm_msg msg;
	int open_mode = SSM_READ | SSM_WRITE;
	size_t len;

	/* initialize check */
	if( !name ) {
		fprintf( stderr, "SSM ERROR : create : stream name is NOT defined, err.\n" );
		exit(1);
	}
	len = strlen( name );
	if( len == 0 || len >= SSM_SNAME_MAX ) {
		fprintf( stderr, "SSM ERROR : create : stream name length of '%s' err.\n", name );
		exit(1);
	}

	if( stream_id < 0 ) {
		fprintf( stderr, "SSM ERROR : create : stream id err.\n" );
		exit(1);
	}

	if( life <= 0.0 ) {
		fprintf( stderr, "SSM ERROR : create : stream life time err.\n" );
		exit(1);
	}

	if( cycle <= 0.0 ) {
		fprintf( stderr, "SSM ERROR : create : stream cycle err.\n" );
		exit(1);
	}

	if( life < cycle ) {
		fprintf( stderr, "SSM ERROR : create : stream saveTime MUST be larger than stream cycle.\n" );
		exit(1);
	}

	strncpy( msg.name, name, SSM_SNAME_MAX );
	msg.suid = stream_id;
	msg.ssize = ssm_size;
	msg.hsize = calcSSM_table( life, cycle ) ;	/* table size */
	msg.time = cycle;

	printf("msg.suid  = %d\n", msg.suid);
	printf("msg.ssize = %d\n", msg.ssize);
	printf("msg.hsize  = %d\n", msg.hsize);
	printf("msg.time  = %f\n", msg.time);

	char *msg_buf = (char*)malloc(sizeof(ssm_msg));
	if (!sendMsgToServer(MC_CREATE | open_mode, &msg)) {
		fprintf(stderr, "error in createRemoteSSM\n");
	}
	if (recvMsgFromServer(&msg, msg_buf)) {
		printf("msg %d\n", (int)msg.cmd_type);
	} else {
		fprintf(stderr, "fail recvMsg\n");
	}
	free(msg_buf);

}

bool PConnector::create(double saveTime, double cycle) {
	if( !mDataSize )	{
		std::cerr << "SSM::create() : data buffer of ''" << streamName << "', id = " << streamId << " is not allocked." << std::endl;
		return false;
	}

	this->createRemoteSSM(streamName, streamId, mDataSize, saveTime, cycle);
	return true;
}

bool PConnector::create(const char *streamName, int streamId, double saveTime, double cycle) {
	setStream(streamName, streamId);
	create(saveTime, cycle);
	return true;
}

bool PConnector::setProperty() {
	if (mPropertySize > 0) {
		setPropertyRemoteSSM(streamName, streamId, mProperty, mPropertySize);
		return true;
	} else {
		return false;
	}

}

bool PConnector::setPropertyRemoteSSM(const char *name, int sensor_uid, const void *adata, size_t size) {
	ssm_msg msg;
	char *ptr;
	const char *data = ( char * )adata;
	if( strlen( name ) > SSM_SNAME_MAX ) {
		fprintf(stderr, "name length error\n");
		return 0;
	}

	/* メッセージをセット */
	strncpy( msg.name, name, SSM_SNAME_MAX );
	msg.suid = sensor_uid;
	msg.ssize = size;
	msg.hsize = 0;
	msg.time = 0;

	char *msg_buf = (char*)malloc(sizeof(ssm_msg));
	if (!sendMsgToServer(MC_STREAM_PROPERTY_SET, &msg)) {
		fprintf(stderr, "error in setPropertyRemoteSSM\n");
	}
	if (recvMsgFromServer(&msg, msg_buf)) {
		printf("msg %d\n", (int)msg.cmd_type);

		/*
		for (int i = 0; i < 16; ++i) {
			printf("%02x ", data[i]);
		}
		printf("\n");
		*/
		sendData(data, size);
		if (recvMsgFromServer(&msg, msg_buf)) {
			printf("sendData ack msg %d\n", (int)msg.cmd_type);
		}

	} else {
		fprintf(stderr, "fail recvMsg\n");
	}
	free(msg_buf);

	return true;
}

void PConnector::setOffset(ssmTimeT offset) {

	printf("offset = %f\n", offset);
	ssm_msg msg;
	msg.hsize = 0;
	msg.ssize = 0;
	msg.time = offset;
	char *msg_buf = (char*)malloc(sizeof(ssm_msg));
	if (!sendMsgToServer(MC_OFFSET, &msg)) {
		fprintf(stderr, "error in setOffset\n");
	}
	if (recvMsgFromServer(&msg, msg_buf)) {
		printf("msg res offset %d\n", (int)msg.cmd_type);
	}
	free(msg_buf);
}
