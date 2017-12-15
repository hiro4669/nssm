

#ifndef _SSM_PROXY_H_
#define _SSM_PROXY_H_

void test();

#define SERVER_PORT     8080            /* サーバ用PORT */
#define SERVER_IP       0x00000000UL    /* サーバ用待ち受けIP */
#define BUFFER_SIZE     1024            /* バッファバイト数 */

/* クライアントからの接続を待つサーバの情報を表現する構造体 */
typedef struct {
    int                 wait_socket;    /* サーバ待ち受け用ソケット */
    struct sockaddr_in  server_addr;    /* サーバ待ち受け用アドレス */
} TCPSERVER_INFO;

/* クライアントとの接続に関する情報を保存する構造体 */
typedef struct {
    int                 data_socket;    /* クライアントとの通信用ソケット */
    struct sockaddr_in  client_addr;    /* クライアントのアドレス */
} TCPCLIENT_INFO;




class ProxyServer {
private:
	TCPSERVER_INFO server;
	TCPCLIENT_INFO client;

	char* mData;
	size_t mDataSize;
	size_t ssmTimeSize;

	bool open();
	bool wait();

	void serializeMessage(ssm_msg *msg, char *buf);

	int  readInt(char **p);
	long readLong(char **p);
	double readDouble(char **p);
	void readRawData(char **p, char *d, int len);

	void writeInt(char **p, int v);
	void writeLong(char **p, long v);
	void writeDouble(char **p, double v);
	void writeRawData(char **p, char *d, int len);


	int receiveMsg(ssm_msg *msg, char *buf);
	int sendMsg(int cmd_type, ssm_msg *msg);

	void receiveData();

public:
	ProxyServer();
	~ProxyServer();
	bool init();
	bool run();
	bool server_close();
	bool client_close();
	void handleCommand();




};

#endif
