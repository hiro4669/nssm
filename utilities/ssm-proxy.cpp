#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>

#include "libssm.h"
#include "ssm-time.h"
#include "ssm.h"

#include "printlog.hpp"

#include "ssm-proxy.hpp"

extern pid_t my_pid; // for debug

DataCommunicator::DataCommunicator(uint16_t nport, char* mData, size_t d_size, size_t t_size, SSMApiBase *pstream) {
	printf("DataCommunicatir new\n");
	this->mData = mData;
	this->mDataSize = d_size;
	this->ssmTimeSize = t_size;
	this->mFullDataSize = d_size + t_size;
	this->pstream = pstream;

	this->server.wait_socket = -1;
	this->server.server_addr.sin_family      = AF_INET;
	this->server.server_addr.sin_addr.s_addr = htonl(SERVER_IP);
	this->server.server_addr.sin_port        = htons(nport);

	if(!this->sopen()) {
		perror("errororor\n");
	}
}

DataCommunicator::~DataCommunicator() {
	this->sclose();
}

bool DataCommunicator::receiveData() {
	int len = recv(this->client.data_socket, mData, mFullDataSize, 0);
	if (len != mFullDataSize) {
		return false;
	}
	return true;
}
void DataCommunicator::handleData() {
	char *p;
	ssmTimeT time;
	while(true) {
		if (!receiveData()) {
			fprintf(stderr, "receiveData Error happends\n");
			break;
		}
		p = &mData[8];
		time = *(reinterpret_cast<ssmTimeT*>(mData));
		printf("time = %f\n", time);
		pstream->write(time);
		for (int i = 0; i < 8; ++i) {
			printf("%02x ", p[i] & 0xff);
		}
		printf("\n");
	}
	pstream->showRawData();
}
void* DataCommunicator::run(void* args) {
	printf("runrunrun\n");

	if(rwait()) {
		handleData();
		printf("end of thread\n");
	}
	return nullptr;
}

bool DataCommunicator::sopen() {
	this->server.wait_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->server.wait_socket == -1) {
		perror("open socket error");
		return false;
	}
	if (bind(this->server.wait_socket, (struct sockaddr*)&this->server.server_addr,
			sizeof(this->server.server_addr)) == -1) {
		perror("data com bind");
		return false;
	}
	if (listen(this->server.wait_socket, 5) == -1) {
		perror("data com open");
		return false;
	}
	printf("Data Communicagtor open!!\n");
	return true;
}

bool DataCommunicator::sclose() {
	if (this->client.data_socket != -1) {
		close(this->client.data_socket);
		this->client.data_socket = -1;
	}
	if (this->server.wait_socket != -1) {
		close(this->server.wait_socket);
		this->server.wait_socket = -1;
	}

	return true;
}

bool DataCommunicator::rwait() {
	memset(&this->client, 0, sizeof(this->client));
	this->client.data_socket = -1;
	for (;;) {
		socklen_t client_addr_len = sizeof(this->client.client_addr);
		printf("wait!!!\n");
		this->client.data_socket = accept(this->server.wait_socket,
				(struct sockaddr*)&this->client.client_addr,
				&client_addr_len);
		printf("pppppp\n");
		if (this->client.data_socket != -1) break;
		if (errno == EINTR) continue;
		perror("server open accept");
		return false;
	}
	printf("wait2!!!\n");
	return true;
}




ProxyServer::ProxyServer() {
	printf("Proxy Server created\n");
	nport = SERVER_PORT;
	mData = NULL;
	mDataSize = 0;
	ssmTimeSize = sizeof(ssmTimeT);
	mFullDataSize = 0;
	mProperty = NULL;
	mPropertySize = 0;
	com = nullptr;

}

ProxyServer::~ProxyServer() {
	printf("proxy server deleted\n");
	this->server_close();
	free(mData);
	mData = NULL;
	delete com;
	com = nullptr;
}

bool ProxyServer::init() {
	printf("init\n");
	setupSigHandler();
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

double ProxyServer::readDouble(char **p) {
	char buf[8];
	for (int i = 0; i < 8; ++i, (*p)++) {
		buf[7-i] = **p;
	}
	return *(double*)buf;
}

void ProxyServer::readRawData(char **p, char *d, int len) {
	for (int i = 0; i < len; ++i, (*p)++) {
		d[i] = **p;
	}
}

void ProxyServer::writeInt(char **p, int v) {
	**p = (v >> 24) & 0xff; (*p)++;
	**p = (v >> 16) & 0xff; (*p)++;
	**p = (v >> 8)  & 0xff; (*p)++;
	**p = (v >> 0)  & 0xff; (*p)++;
}

void ProxyServer::writeLong(char **p, long v) {
	**p = (v >> 56) & 0xff; (*p)++;
	**p = (v >> 48) & 0xff; (*p)++;
	**p = (v >> 40)  & 0xff; (*p)++;
	**p = (v >> 32)  & 0xff; (*p)++;
	this->writeInt(p, v);
}

void ProxyServer::writeDouble(char **p, double v) {
	char *dp = (char*)&v;
	for (int i = 0; i < 8; ++i, (*p)++) {
		**p = dp[7 - i] & 0xff;
	}
}

void ProxyServer::writeRawData(char **p, char *d, int len) {
	for (int i = 0; i < len; ++i, (*p)++) **p = d[i];
}


void ProxyServer::serializeMessage(ssm_msg *msg, char *buf) {
	msg->msg_type = readLong(&buf);
	msg->res_type = readLong(&buf);
	msg->cmd_type = readInt(&buf);
	readRawData(&buf, msg->name, 32);
	msg->suid = readInt(&buf);
	msg->ssize = readLong(&buf);
	msg->hsize = readLong(&buf);
	//msg->time = readLong(&buf);
	msg->time = readDouble(&buf);
	msg->saveTime = readDouble(&buf);


	/*
	printf("msg_type = %d\n", msg->msg_type);
	printf("res_type = %d\n", msg->res_type);
	printf("cmd_type = %d\n", msg->cmd_type);


	for (int i = 0; i < 10; ++i) {
		printf("%02x ", msg->name[i] & 0xff);
	}
	printf("\n");
	printf("suid = %d\n", msg->suid);
	printf("ssize = %d\n", msg->ssize);
	printf("hsize = %d\n", msg->hsize);
	printf("time = %f\n", msg->time);
	*/
}

bool ProxyServer::receiveData() {
	//char *p;
	int len = recv(this->client.data_socket, mData, mFullDataSize, 0);
	if (len != mFullDataSize) {
		return false;
	}
	return true;
}
void ProxyServer::handleData() {
	char *p;
	ssmTimeT time;
	while(true) {
		printf("wait bulk data\n");
		if (!receiveData()) {
			fprintf(stderr, "receiveData error happens!\n");
			break;
		}
		p = &mData[8];
		time = *(reinterpret_cast<ssmTimeT*>(mData));
		printf("time = %f\n", time);
		stream.write(time);
		for (int i = 0; i < 8; ++i) {
			printf("%02x ", p[i] & 0xff);
		}
		printf("\n");
	}
	printf("--- raw data ---\n");
	stream.showRawData();
}

int ProxyServer::receiveMsg(ssm_msg *msg, char *buf) {
	int len = recv(this->client.data_socket, buf, sizeof(ssm_msg), 0);
	if (len > 0) {
		serializeMessage(msg, buf);
	}
	return len;
}

int ProxyServer::sendMsg(int cmd_type, ssm_msg *msg) {
	ssm_msg msgbuf;
	size_t len;
	char *buf, *p;
	if (msg == NULL) {
		msg = &msgbuf;
	}

	msg->cmd_type = cmd_type;
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
	writeDouble(&p, msg->saveTime);

	if ((len = send(this->client.data_socket, buf, sizeof(ssm_msg), 0)) == -1) {
		fprintf(stderr, "error happens\n");
	}

	free(buf);
	return len;
}

void ProxyServer::handleCommand() {
	printf("handlecommand\n");
	fprintf(stderr, "nport = %d\n", nport);
	ssm_msg msg;
	char *buf = (char*)malloc(sizeof(ssm_msg));
	while(true) {
		printf("wait recv\n");
		int len = receiveMsg(&msg, buf);
		printf("len in process = %d\n", len);
		if (len == 0) break;
		switch (msg.cmd_type & 0x1f) {
		case MC_NULL: {
			break;
		}
		case MC_INITIALIZE: {
			printf("MC_INITIALIZE\n");

			if (!initSSM()) {
				fprintf(stderr, "init ssm error in ssm-proxy\n");
				sendMsg(MC_FAIL, &msg);
				break;
			} else {
				sendMsg(MC_RES, &msg);
			}
			break;
		}
		case MC_CREATE: {
			printf("MC_CREATE\n");
			/*
			printf("strean name = %s\n", msg.name);
			printf("stream_id = %d\n", msg.suid);
			printf("ssm_size = %d\n", msg.ssize);
			printf("hsize =  %d\n", msg.hsize);
			printf("msg.time(cycle) = %f\n", msg.time);
			printf("msg.saveTime = %f\n", msg.saveTime);
			*/
			mDataSize = msg.ssize;
			mFullDataSize = mDataSize + sizeof(ssmTimeT);
			if (mData) {
				free(mData);
			}
			mData = (char*)malloc(mFullDataSize);
			if (mData == NULL) {
				fprintf(stderr, "fail to create mData\n");
				sendMsg(MC_FAIL, &msg);
			} else {
				stream.setDataBuffer(&mData[sizeof(ssmTimeT)], mDataSize);
				if (!stream.create(msg.name, msg.suid, msg.saveTime, msg.time)) {
					sendMsg(MC_FAIL, &msg);
					break;
				}
				printf("srream is created\n");
				sendMsg(MC_RES, &msg);
			}
			break;
		}
		case MC_STREAM_PROPERTY_SET: {
			printf("MC_STREAM_PROPERTY_SET\n");
			mPropertySize = msg.ssize;
			if (mProperty) {
				free(mProperty);
			}
			mProperty = (char*)malloc(mPropertySize);
			if (mProperty == NULL) {
				sendMsg(MC_FAIL, &msg);
				break;
			}
			stream.setPropertyBuffer(mProperty, mPropertySize);

			sendMsg(MC_RES, &msg);
			int len = recv(this->client.data_socket, mProperty, msg.ssize, 0);
			/*
			for (int i = 0; i < 8; ++i) {
				printf("%02x ", mProperty[i]);
			}
			printf("\n");
			ssmTimeT t = gettimeOffset();
			printf("time = %f\n", t);
			*/


			if (len > 0) {
				printf("receive property\n");
				if (mPropertySize && !stream.setProperty()) {
					sendMsg(MC_FAIL, &msg);
					break;
				}
				sendMsg(MC_RES, &msg);
			} else {
				sendMsg(MC_FAIL, &msg);
			}
			break;
		}
		case MC_OFFSET: {
			printf("MC_OFFSET\n");

			ssmTimeT offset = msg.time;
			printf("time  = %f\n", offset);
			settimeOffset(offset);
			printf("time2 = %f\n", gettimeOffset());

			sendMsg(MC_RES, &msg);

			//handleData();
			//com->wait();
			break;
		}
		case MC_CONNECTION: {
			printf("MC_CONNECTION\n");
			msg.suid = nport;
			com = new DataCommunicator(nport, mData, mDataSize, ssmTimeSize, &stream);
			com->start(nullptr);
			sendMsg(MC_RES, &msg);
			break;
		}
		case MC_TERMINATE: {
			printf("MC_TERMINATE\n");
			sendMsg(MC_RES, &msg);
			goto END_PROC;
			break;
		}
		default: {
			fprintf(stderr, "NOTICE : unknown msg %d", msg.cmd_type);
			break;
		}
		}


	}

END_PROC:
	free(buf);
	if (com) {
		com->wait();
	}
	free(com);

	// SSMの終了
	// 時間の初期化
	inittimeSSM(  );
	endSSM(  );
}




bool ProxyServer::run() {
	printf("run\n");
	while(wait()) {
		++nport;
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

void ProxyServer::setupSigHandler() {
	struct sigaction act;
	memset(&act, 0, sizeof(act));   /* sigaction構造体をとりあえずクリア */
	act.sa_handler = &ProxyServer::catchSignal; /* SIGCHLD発生時にcatch_SIGCHLD()を実行 */
	sigemptyset(&act.sa_mask);  /* catch_SIGCHLD()中の追加シグナルマスクなし */
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &act, NULL);
}

void ProxyServer::catchSignal(int signo) {
	//printf("catch signal!!!!");
	pid_t child_pid = 0;
    /* すべての終了している子プロセスに対してwaitpid()を呼ぶ */
    do {
        int child_ret;
        child_pid = waitpid(-1, &child_ret, WNOHANG);
        /* すべての終了している子プロセスへwaitpid()を呼ぶと
           WNOHANGオプションによりwaitpid()は0を返す */
    } while(child_pid>0);
}



int main(void) {
	ProxyServer server;
	printf("size_t size = %d\n", (int)sizeof(size_t));
	server.init();
	server.run();
	test();
	
	return 0;
}
