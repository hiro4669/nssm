
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

	const char *streamName;
	int streamId;
	void *mData;								///< データのポインタ
	size_t mDataSize;						///< データ構造体のサイズ
	void *mProperty;							///< プロパティのポインタ
	size_t mPropertySize;					///< プロパティサイズ


	void writeInt(char **p, int v);
	void writeLong(char **p, long v);
	void writeDouble(char **p, double v);
	void writeRawData(char **p, char *d, int len);

	int  readInt(char **p);
	long readLong(char **p);
	double readDouble(char **p);
	void readRawData(char **p, char *d, int len);

	void serializeMessage(ssm_msg *msg, char *buf);

public:
	PConnector();
	~PConnector();

	bool connectToServer(const char* serverName, int port);
	bool sendMsgToServer(int cmd_type, ssm_msg *msg);
	bool recvMsgFromServer(ssm_msg *msg, char *buf);

	void initRemote();
	void setStream(const char *streamName, int streamId);
	void setBuffer(void *data, size_t dataSize, void *property, size_t propertySize);
	bool create(const char *streamName, int streamId, double saveTime, double cycle);
	bool create(double saveTime, double cycle);
	void createRemoteSSM( const char *name, int stream_id, size_t ssm_size, ssmTimeT life, ssmTimeT cycle );


};
#endif
