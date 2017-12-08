
#ifndef _SSM_PROXY_CLIENT_H_
#define _SSM_PROXY_CLIENT_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class PConnector {
private:
	struct sockaddr_in server;
	int sock;


public:
	PConnector();
	~PConnector();

	bool connectToServer(char* serverName, int port);
};
#endif
