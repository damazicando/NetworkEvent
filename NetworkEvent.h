#pragma once

#include "Singleton.h"
#include <stdio.h>
#include <vector>
#include <future>
#include "ThreadQueue.h"
#include "NetworkEventDef.h"
#include <map>
#include <event2/event.h>
#include <event2/util.h>
#include <event2/buffer.h>
#include "NetworkEventCallback.h"

enum NetworkMessageType
{
	NMT_RECV		= 2,
	NMT_ACCEPT		= 3,
	NMT_CONNECTED	= 4,
	NMT_CLOSED		= 5
};

class NetworkMessage
{
public:
	NetworkMessageType iMsgType;
	socket_t socketid;
	std::string sLocalIp;
	uint16_t iLocalPort;
	std::string sRemoteIp;
	uint16_t iRemotePort;
	std::string sMsg;
};

template
<
	int bytes_of_field_size = 4,
	int max_msg_len	= 1000,
	bool field_size_self_include = true
>
class VarLenDelimit
{
public:
	static int getmsg(struct evbuffer *bff, std::string &msg)
	{
		size_t bfflen = evbuffer_get_length(bff);
		if (bfflen < bytes_of_field_size)
		{
			return 0;
		}
		
		std::string s;
		s.resize(bytes_of_field_size +1);
		evbuffer_copyout(bff, (void *)s.c_str(), bytes_of_field_size);
		
		uint64_t msglen = 0;
		for (int i = 0; i < bytes_of_field_size; i++)
		{
			msglen <<= 8;
			msglen += (uint32_t)s[i];
		}

		if (!field_size_self_include)
		{
			msglen += bytes_of_field_size;
		}

		if (msglen > max_msg_len)
		{
			std::cout << "VarLenDelimit error " << msglen << std::endl;
			return -1;
		}

		if (msglen < bfflen)
		{
			return 0;
		}

		msg.resize(msglen);
		evbuffer_remove(bff, (void *)msg.c_str(), msglen);

		return 1;
	}
};

class NetworkEvent 
{
public:
	typedef std::function<void(NetworkMessage)> recvfunc_t;
	NetworkEvent(recvfunc_t func);
	~NetworkEvent();
	friend class NetworkEventCallback;
public:
	int listen(const std::string &sIp, uint16_t iPort);
	template<class DelimitPolicy = VarLenDelimit<4>>
	int connect(const std::string &sLocalIp, uint16_t iLocalPort, const std::string &sRemoteIp, uint16_t iRemotePort)
	{
		int32_t iSocketId = iBaseSocketId_.fetch_add(1);
		auto cnnt = std::make_shared<CommandBindAndConnect>();
		cnnt->iSocketId = iSocketId;
		cnnt->sLocalIp = sLocalIp;
		cnnt->iLocalPort = iLocalPort;
		cnnt->sRemoteIp = sRemoteIp;
		cnnt->iRemotePort = iRemotePort;
		commandQueue_.push_back(cnnt);
		std::cout << "user connect " << std::endl;
		remind();
		return iSocketId;
	}
	template<class DelimitPolicy = VarLenDelimit<4>>
	int connect(const std::string &sRemoteIp, uint16_t iRemotePort)
	{
		int32_t iSocketId = iBaseSocketId_.fetch_add(1);
		auto cnnt = std::make_shared<CommandConnect>();
		cnnt->iSocketId = iSocketId;
		cnnt->sRemoteIp = sRemoteIp;
		cnnt->iRemotePort = iRemotePort;
		commandQueue_.push_back(cnnt);
		std::cout << "user connect " << std::endl;
		remind();
		return iSocketId;
	}
	void send(int32_t iSocketId, const std::string &msg);
	void close(int32_t iSocketId);
private:
	void eventDispatchLoop();
	void recvMessageDispatch();
	void remind();
private:
	std::future<void> ftr4eventloop_;
	std::future<void> ftr4rcvmsgq_;
	std::atomic_bool terminate_;
private:
	ThreadQueue<std::shared_ptr<CommandBase>> commandQueue_;
	ThreadQueue<std::shared_ptr<CommandBase>> messageQueueForSend_;
	ThreadQueue<NetworkMessage> messageQueueForRecv_;
	event_base *base_;
	evutil_socket_t fds_[2];
private:
	recvfunc_t func_;
private:
	std::map<socket_t, SocketInfo> mpinfo;
	std::map<int32_t, SocketInfo> mpinfo4id;
private:
	bool makeNetworkMessage(int32_t iSocketId, NetworkMessageType t, const std::string &raw, NetworkMessage &msg);
	bool makeNetworkMessage(socket_t sock, NetworkMessageType t, const std::string &raw, NetworkMessage &msg);
private:
	std::atomic_uint32_t iBaseSocketId_;
};

