
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
	void *mFullData;
	size_t mFullDataSize;


	void writeInt(char **p, int v);
	void writeLong(char **p, long v);
	void writeDouble(char **p, double v);
	void writeRawData(char **p, char *d, int len);

	int  readInt(char **p);
	long readLong(char **p);
	double readDouble(char **p);
	void readRawData(char **p, char *d, int len);

	void serializeMessage(ssm_msg *msg, char *buf);
	bool createRemoteSSM( const char *name, int stream_id, size_t ssm_size, ssmTimeT life, ssmTimeT cycle );
	bool setPropertyRemoteSSM(const char *name, int sensor_uid, const void *adata, size_t size);

	bool sendData(const char *data, size_t size);

public:
	PConnector();
	~PConnector();

	bool connectToServer(const char* serverName, int port);
	bool sendMsgToServer(int cmd_type, ssm_msg *msg);
	bool recvMsgFromServer(ssm_msg *msg, char *buf);

	bool initRemote();
	void setStream(const char *streamName, int streamId);
	void setBuffer(void *data, size_t dataSize, void *property, size_t propertySize, void *fulldata);
	bool create(const char *streamName, int streamId, double saveTime, double cycle);
	bool create(double saveTime, double cycle);
	bool setProperty();
	void setOffset(ssmTimeT offset);

	bool write( ssmTimeT time); // write bulkdata with time




};
#endif
