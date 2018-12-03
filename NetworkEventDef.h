#ifndef _NetworkEventDef_h_
#define  _NetworkEventDef_h_

#include <event2/event.h>
#include <event2/util.h>
#include <string>

enum CommandType
{
	COMMAND_TYPE_LISTEN				= 1,
	COMMAND_TYPE_ACCEPT				= 2,
	COMMAND_TYPE_CONNECT			= 3,
	COMMAND_TYPE_BIND_AND_CONNECT	= 4,
	COMMAND_TYPE_CLOSE				= 5,
	COMMAND_TYPE_SEND				= 6
};

class CommandBase
{
public:
	int32_t iMsgType;
	virtual void i_am_virtual(){}
};

class CommandListen : public CommandBase
{
public:
	int32_t iSocketId;
	std::string sIp;
	uint16_t iPort;
	CommandListen()
	{
		iMsgType = COMMAND_TYPE_LISTEN;
	}
};

class CommandConnect : public CommandBase
{
public:
	int32_t iSocketId;
	std::string sLocalIp;
	uint16_t iLocalPort;
	std::string sRemoteIp;
	uint16_t iRemotePort;
	CommandConnect()
	{
		iMsgType = COMMAND_TYPE_CONNECT;
	}
};

class CommandBindAndConnect : public CommandBase
{
public:
	int32_t iSocketId;
	std::string sLocalIp;
	uint16_t iLocalPort;
	std::string sRemoteIp;
	uint16_t iRemotePort;
	CommandBindAndConnect()
	{
		iMsgType = COMMAND_TYPE_BIND_AND_CONNECT;
	}
};

class CommandClose : public CommandBase
{
public:
	int32_t iSocketId;
	CommandClose()
	{
		iMsgType = COMMAND_TYPE_CLOSE;
	}
};

class CommandSend : public CommandBase
{
public:
	int32_t iSocketId;
	std::string msg;
	CommandSend()
	{
		iMsgType = COMMAND_TYPE_SEND;
	}
};

typedef evutil_socket_t socket_t;

enum ConnectState
{
	CS_CONNECTED		= 1,
	CS_NOT_CONNECTED	= 2,
	CS_LISTEN			= 3
};

struct SocketInfo
{
	int32_t iSocketId;
	evutil_socket_t sock;
	std::string sLocalIp;
	uint16_t iLocalPort;
	std::string sRemoteIp;
	uint16_t iRemotePort;
	struct bufferevent *bev;
	struct evconnlistener *evlstn;
	ConnectState cs;
	SocketInfo()
	{
		cs = CS_NOT_CONNECTED;
	}
};

#ifdef WIN32
#define LOCAL_SOCKETPAIR_AF AF_INET
#else
#define LOCAL_SOCKETPAIR_AF AF_UNIX
#endif

#endif
