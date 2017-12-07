

#ifndef _SSM_PROXY_H_
#define _SSM_PROXY_H_

void test();

class ProxyServer {
public:
	ProxyServer();
	~ProxyServer();
	bool init();
	bool run();
};

#endif
