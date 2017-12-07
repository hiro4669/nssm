#include <stdio.h>

#include "ssm-proxy.hpp"


ProxyServer::ProxyServer() {
	printf("Proxy Server created\n");
}

ProxyServer::~ProxyServer() {
	printf("proxy server deleted\n");
}

bool ProxyServer::init() {
	printf("init\n");
	return true;
}

bool ProxyServer::run() {
	printf("run\n");
	return true;
}




int main(void) {
	ProxyServer server;
	printf("Hello Proxy\n");
	test();
	
	return 0;
}
