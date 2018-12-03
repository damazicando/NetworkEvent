#include "pch.h"
#include "NetworkEvent.h"
#include <future>
#include <thread>
#include<event.h>  
#include<listener.h>  
#include<bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>
#include <event2/listener.h>
#include "NetworkEventCallback.h"
#include "NetworkEventDef.h"
#include <iostream>
#include "StringFormat.h"

int32_t NetworkEvent::listen(const std::string &sIp, uint16_t iPort)
{
	int32_t iSocketId = iBaseSocketId_.fetch_add(1);
	std::shared_ptr<CommandListen> lstn = std::make_shared<CommandListen>();
	lstn->iSocketId = iSocketId;
	lstn->sIp = sIp;
	lstn->iPort = iPort;
	commandQueue_.push_back(lstn);
	std::cout << "user listen " << sIp << ":" << iPort << std::endl;
	remind();
	return iSocketId;
}

void NetworkEvent::send(int32_t iSocketId, const std::string &msg)
{
	auto snd = std::make_shared<CommandSend>();
	snd->iSocketId = iBaseSocketId_.fetch_add(1);
	snd->msg = msg;
	messageQueueForSend_.push_back(snd);
	std::cout << "user send msg" << std::endl;
	remind();
}

NetworkEvent::NetworkEvent(recvfunc_t func)
{
	iBaseSocketId_ = 200;

	auto vrson = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(vrson, &wsaData);

	int ret = 0;
	base_ = event_base_new();
	ret = evutil_socketpair(LOCAL_SOCKETPAIR_AF, SOCK_STREAM, 0, fds_);
	assert(ret >= 0);

	struct bufferevent *bev;
	bev = bufferevent_socket_new(base_, fds_[1], BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, NetworkEventCallback::readcb4cmd, NULL, NetworkEventCallback::eventcb4cmd, this);
	bufferevent_enable(bev, EV_READ | EV_PERSIST);
	
	terminate_ = false;
	ftr4eventloop_ = std::async(std::launch::async, &NetworkEvent::eventDispatchLoop, this);
	ftr4rcvmsgq_ = std::async(std::launch::async, &NetworkEvent::recvMessageDispatch, this);
	func_ = func;
}

NetworkEvent::~NetworkEvent()
{
}

void NetworkEvent::remind()
{
	int ret = ::send(fds_[0], "c", 1, 0);
	//std::cout << "remind the dispatch loop" << ret << std::endl;
}

/*
template<class DelimitPolicy>
void NetworkEvent::connect(const std::string &sLocalIp, uint16_t iLocalPort, const std::string &sRemoteIp, uint16_t iRemotePort)
{
	auto cnnt = std::make_shared<CommandBindAndConnect>();
	cnnt->sLocalIp = sLocalIp;
	cnnt->iLocalPort = iLocalPort;
	cnnt->sRemoteIp = sRemoteIp;
	cnnt->iRemotePort = iRemotePort;
	commandQueue_.push_back(cnnt);
	std::cout << "user connect " << std::endl;
	remind();
}
*/

/*
template<class DelimitPolicy>
void NetworkEvent::connect(const std::string &sRemoteIp, uint16_t iRemotePort)
{
	auto cnnt = std::make_shared<CommandConnect>();
	cnnt->sRemoteIp = sRemoteIp;
	cnnt->iRemotePort = iRemotePort;
	commandQueue_.push_back(cnnt);
	std::cout << "user connect " << std::endl;
	remind();
}
*/

void NetworkEvent::close(int32_t iSocketId)
{
	auto cls = std::make_shared<CommandClose>();
	cls->iSocketId = iSocketId;
	commandQueue_.push_back(cls);
	std::cout << "user close " << std::endl;
	remind();
}

void NetworkEvent::eventDispatchLoop()
{
	while (!terminate_)
	{
		event_base_dispatch(base_);
	}
}

void NetworkEvent::recvMessageDispatch()
{
	while (!terminate_)
	{
		NetworkMessage msg;
		if (!messageQueueForRecv_.pop_front(msg, 500))
		{
			continue;
		}

		std::cout << "recvMessageDispatch" << std::endl;

		try
		{
			func_(msg);
		}
		catch (std::exception &ex)
		{
			std::cout << ex.what() << std::endl;
		}
		catch (...)
		{
		}
	}
}

bool NetworkEvent::makeNetworkMessage(int32_t iSocketId, NetworkMessageType t, const std::string &raw, NetworkMessage &msg)
{
	auto itr = mpinfo4id.find(iSocketId);
	if (itr == mpinfo4id.end())
	{
		return false;
	}

	SocketInfo &tInfo = itr->second;
	msg.iLocalPort = tInfo.iLocalPort;
	msg.iRemotePort = tInfo.iRemotePort;
	msg.iMsgType = t;
	msg.sLocalIp = tInfo.sLocalIp;
	msg.socketid = iSocketId;
	msg.sRemoteIp = tInfo.sRemoteIp;
	msg.sMsg = raw;

	return true;
}

bool NetworkEvent::makeNetworkMessage(socket_t sock, NetworkMessageType t, const std::string &raw, NetworkMessage &msg)
{
	auto itr = mpinfo.find(sock);
	if (itr == mpinfo.end())
	{
		return false;
	}

	SocketInfo &tInfo = itr->second;
	msg.iLocalPort = tInfo.iLocalPort;
	msg.iRemotePort = tInfo.iRemotePort;
	msg.iMsgType = t;
	msg.sLocalIp = tInfo.sLocalIp;
	msg.socketid = itr->second.iSocketId;
	msg.sRemoteIp = tInfo.sRemoteIp;
	msg.sMsg = raw;

	return true;
}

NetworkEventCallback::NetworkEventCallback()
{

}

NetworkEventCallback::~NetworkEventCallback()
{

}

void NetworkEventCallback::tosockaddr(struct sockaddr_in &sin, const std::string &sIp, uint16_t iPort)
{
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = LOCAL_SOCKETPAIR_AF;
	sin.sin_port = htons(iPort);
	auto r = evutil_inet_pton(LOCAL_SOCKETPAIR_AF, sIp.c_str(), &sin.sin_addr);
	assert(r);
}

void NetworkEventCallback::getsockname(socket_t sock, std::string &sIp, uint16_t &iPort)
{
	struct sockaddr_in sin;
	int len = sizeof(sin);
	::getsockname(sock, (struct sockaddr *)&sin, &len);
	iPort = ntohs(sin.sin_port);
	char dst[100];
	evutil_inet_ntop(LOCAL_SOCKETPAIR_AF, &sin.sin_addr, dst, 100);
	sIp = dst;
}

void NetworkEventCallback::getpeername(socket_t sock, std::string &sIp, uint16_t &iPort)
{
	struct sockaddr_in sin;
	int len = sizeof(sin);
	::getpeername(sock, (struct sockaddr *)&sin, &len);
	iPort = ntohs(sin.sin_port);
	char dst[100];
	evutil_inet_ntop(LOCAL_SOCKETPAIR_AF, &sin.sin_addr, dst, 100);
	sIp = dst;
}

int32_t NetworkEventCallback::bind(socket_t sock, std::string sIp, uint16_t iPort)
{
	struct sockaddr_in sin;
	tosockaddr(sin, sIp, iPort);
	return ::bind(sock, (struct sockaddr *)&sin, sizeof(sin));
}

socket_t NetworkEventCallback::socket()
{
	return ::socket(LOCAL_SOCKETPAIR_AF, SOCK_STREAM, IPPROTO_TCP);
}

#if 0
#define BEV_EVENT_READING	0x01	/**< error encountered while reading */
#define BEV_EVENT_WRITING	0x02	/**< error encountered while writing */
#define BEV_EVENT_EOF		0x10	/**< eof file reached */
#define BEV_EVENT_ERROR		0x20	/**< unrecoverable error encountered */
#define BEV_EVENT_TIMEOUT	0x40	/**< user-specified timeout reached */
#define BEV_EVENT_CONNECTED	0x80	/**< connect operation finished. */
#endif
void NetworkEventCallback::eventcb4cmd(struct bufferevent *bev, short what, void *ctx)
{
	std::cout << "eventcb4cmd" << std::endl;

	NetworkEvent *ne = (NetworkEvent *)ctx;

	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
	{
		assert(false);
	}
}

void NetworkEventCallback::readcb4cmd(struct bufferevent *bev, void *ptr)
{
	std::cout << "readcb4cmd" << std::endl;

	char buf[1024];
	struct evbuffer *input = bufferevent_get_input(bev);
	evbuffer_remove(input, buf, sizeof(buf));
	NetworkEvent *ne = (NetworkEvent *)ptr;

	std::shared_ptr<CommandBase> cmd;
	while (ne->commandQueue_.pop_front(cmd, 0))
	{
		if (cmd->iMsgType == COMMAND_TYPE_LISTEN)
		{
			std::shared_ptr<CommandListen> lstn = std::dynamic_pointer_cast<CommandListen>(cmd);
			struct sockaddr_in sin;
			tosockaddr(sin, lstn->sIp, lstn->iPort);
			struct evconnlistener *evlstn = evconnlistener_new_bind(ne->base_, NetworkEventCallback::listenercb, ptr, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
				(struct sockaddr *)&sin, sizeof(sin));

			SocketInfo tInfo;
			tInfo.bev = NULL;
			tInfo.cs = CS_LISTEN;
			tInfo.iLocalPort = lstn->iPort;
			tInfo.iRemotePort = -1;
			tInfo.iSocketId = lstn->iSocketId;
			tInfo.sLocalIp = lstn->sIp;
			tInfo.evlstn = evlstn;
			tInfo.sock = evconnlistener_get_fd(evlstn);
			ne->mpinfo[tInfo.sock] = tInfo;
			ne->mpinfo4id[tInfo.iSocketId] = tInfo;
		}

		if (cmd->iMsgType == COMMAND_TYPE_CONNECT || cmd->iMsgType == COMMAND_TYPE_BIND_AND_CONNECT)
		{
			std::shared_ptr<CommandConnect> cnnct = std::dynamic_pointer_cast<CommandConnect>(cmd);
			auto sock = socket();
			if (cmd->iMsgType == COMMAND_TYPE_BIND_AND_CONNECT)
			{
				bind(sock, cnnct->sLocalIp, cnnct->iLocalPort);
			}
			struct bufferevent *bev = bufferevent_socket_new(ne->base_, sock, BEV_OPT_CLOSE_ON_FREE);
			struct sockaddr_in sin;
			tosockaddr(sin, cnnct->sRemoteIp, cnnct->iRemotePort);
			bufferevent_setcb(bev, &NetworkEventCallback::readcb, NULL, NetworkEventCallback::eventcb, ne);
			bufferevent_enable(bev, EV_READ | EV_PERSIST);
			bufferevent_socket_connect(bev, (const sockaddr*)&sin, sizeof(sin));

			std::string sIp;
			uint16_t iPort;
			getsockname(sock, sIp, iPort);
			SocketInfo tInfo;
			tInfo.bev = bev;
			tInfo.cs = CS_NOT_CONNECTED;
			tInfo.sRemoteIp = cnnct->sRemoteIp;
			tInfo.iRemotePort = cnnct->iRemotePort;
			tInfo.iLocalPort = iPort;
			tInfo.sLocalIp = sIp;
			tInfo.evlstn = NULL;
			tInfo.sock = sock;
			tInfo.iSocketId = cnnct->iSocketId;
			ne->mpinfo[sock] = tInfo;
			ne->mpinfo4id[tInfo.iSocketId] = tInfo;
		}
		
		if (cmd->iMsgType == COMMAND_TYPE_CLOSE)
		{
			std::shared_ptr<CommandClose> cls = std::dynamic_pointer_cast<CommandClose>(cmd);
			NetworkMessage nmsg;
			if (ne->makeNetworkMessage(cls->iSocketId, NMT_CLOSED, "", nmsg))
			{
				ne->messageQueueForRecv_.push_back(nmsg);
			}
			doCloseSocket(cls->iSocketId, ptr);
		}
	}

	while (ne->messageQueueForSend_.pop_front(cmd, 0))
	{
		if (cmd->iMsgType == COMMAND_TYPE_SEND)
		{
			std::shared_ptr<CommandSend> snd = std::dynamic_pointer_cast<CommandSend>(cmd);
			auto itr = ne->mpinfo4id.find(snd->iSocketId);
			if (itr == ne->mpinfo4id.end())
			{
				continue;
			}
			auto tInfo = itr->second;
			bufferevent_write(tInfo.bev, snd->msg.c_str(), snd->msg.size());
		}
	}
}

void NetworkEventCallback::readcb(struct bufferevent *bev, void *ptr)
{
	std::cout << "readcb" << std::endl;

	NetworkEvent *ne = (NetworkEvent *)ptr;
	struct evbuffer *bff = bufferevent_get_input(bev);
	assert(bff != NULL);	
	auto sock = bufferevent_getfd(bev);

	while (1)
	{
		std::string msg;
		int ret = VarLenDelimit<>::getmsg(bff, msg);
		if (ret == -1)
		{
			NetworkMessage nmsg;
			if (ne->makeNetworkMessage(sock, NMT_CLOSED, "", nmsg))
			{
				ne->messageQueueForRecv_.push_back(nmsg);
			}
			doCloseSocket(sock, ptr);
			break;
		}

		if (ret == 1)
		{
			NetworkMessage nmsg;
			if (ne->makeNetworkMessage(sock, NMT_RECV, msg, nmsg))
			{
				ne->messageQueueForRecv_.push_back(nmsg);
			}

			continue;
		}

		break;
	}
}

void NetworkEventCallback::eventcb(struct bufferevent *bev, short what, void *ptr)
{
	std::cout << "eventcb" << std::endl;

	// An event occurred during a read operation on the bufferevent.
	if (what & BEV_EVENT_READING) {
		std::cout << "BEV_EVENT_READING" << std::endl;
	}

	// An event occurred during a write operation on the bufferevent.
	if (what & BEV_EVENT_WRITING) {
		std::cout << "BEV_EVENT_WRITING" << std::endl;
	}

	// An error occurred during a bufferevent operation.
	if (what & BEV_EVENT_ERROR) {
		std::cout << "BEV_EVENT_ERROR" << std::endl;
	}

	// A timeout expired on the bufferevent.
	if (what & BEV_EVENT_TIMEOUT) {
		std::cout << "BEV_EVENT_TIMEOUT" << std::endl;
	}

	// We got an end-of-line indication on the bufferevent.
	if (what & BEV_EVENT_EOF) {
		std::cout << "BEV_EVENT_EOF" << std::endl;
	}

	// We finished a requested connection on the bufferevent.
	if (what & BEV_EVENT_CONNECTED) {
		std::cout << "BEV_EVENT_CONNECTED" << std::endl;
	}

	NetworkEvent *ne = (NetworkEvent *)ptr;
	auto sock = bufferevent_getfd(bev);

	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
	{
		if (what & BEV_EVENT_EOF) {
			std::cout << "event come, connection break" << std::endl;
		}
		else if (what & BEV_EVENT_ERROR) {
			std::cout << "event come, connection error" << std::endl;
		}

		auto itr = ne->mpinfo.find(sock);
		if (itr != ne->mpinfo.end())
		{
			NetworkMessage nmsg;
			if (ne->makeNetworkMessage(itr->second.iSocketId, NMT_CLOSED, "", nmsg))
			{
				ne->messageQueueForRecv_.push_back(nmsg);
			}
			doCloseSocket(sock, ptr);
		}

		return;
	}
	
	if (what & BEV_EVENT_CONNECTED)
	{
		auto itr = ne->mpinfo.find(sock);
		if (itr != ne->mpinfo.end())
		{
			SocketInfo &tInfo = itr->second;
			tInfo.cs = CS_CONNECTED;
			ne->mpinfo4id[tInfo.iSocketId] = tInfo;

			NetworkMessage tMsg;
			if (ne->makeNetworkMessage(tInfo.iSocketId, NMT_CONNECTED, "", tMsg))
			{
				ne->messageQueueForRecv_.push_back(tMsg);
			}
		}
	}
}

void NetworkEventCallback::listenercb(
	struct evconnlistener *listener, evutil_socket_t sock, struct sockaddr *sa, int socklen, void *ptr)
{
	std::cout << "listenercb" << std::endl;

	NetworkEvent *ne = (NetworkEvent *)ptr;
	struct bufferevent *bev;

	bev = bufferevent_socket_new(ne->base_, sock, BEV_OPT_CLOSE_ON_FREE);
	if (!bev)
	{
		std::cout << "fail to bufferevent_socket_new" << std::endl;
	}
	bufferevent_setcb(bev, readcb, NULL, eventcb, ptr);
	bufferevent_enable(bev, EV_READ|EV_PERSIST);

	SocketInfo tInfo;
	tInfo.bev = bev;
	tInfo.cs = CS_CONNECTED;
	getsockname(sock, tInfo.sLocalIp, tInfo.iLocalPort);
	getpeername(sock, tInfo.sRemoteIp, tInfo.iRemotePort);
	tInfo.sock = sock;
	tInfo.iSocketId = ne->iBaseSocketId_.fetch_add(1);
	ne->mpinfo[sock] = tInfo;
	ne->mpinfo4id[tInfo.iSocketId] = tInfo;

	NetworkMessage tMsg;
	if (ne->makeNetworkMessage(tInfo.iSocketId, NMT_ACCEPT, "", tMsg))
	{
		ne->messageQueueForRecv_.push_back(tMsg);
	}
}

void NetworkEventCallback::doCloseSocket(socket_t sock, void *ptr)
{
	NetworkEvent *ne = (NetworkEvent *)ptr;
	auto itr = ne->mpinfo.find(sock);
	if (itr != ne->mpinfo.end())
	{
		bufferevent_free(itr->second.bev);
		ne->mpinfo4id.erase(itr->second.iSocketId);
		ne->mpinfo.erase(itr);
	}
}


