
#ifndef _SSM_PROXY_CLIENT_H_
#define _SSM_PROXY_CLIENT_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libssm.h"

class PConnector {
private:
	struct sockaddr_in server;
	int sock;
	void writeInt(char **p, int v);
	void writeLong(char **p, long v);
	void writeRawData(char **p, char *d, int len);



public:
	PConnector();
	~PConnector();

	bool connectToServer(char* serverName, int port);
	bool sendMsgToServer(int cmd_type, ssm_msg *msg);
};
#endif
