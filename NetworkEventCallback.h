#ifndef _NetworkEventCallback_h_
#define _NetworkEventCallback_h_

class NetworkEventCallback
{
public:
	NetworkEventCallback();
	~NetworkEventCallback();
	static void readcb4cmd(struct bufferevent *bev, void *ptr);
	static void eventcb4cmd(struct bufferevent *bev, short what, void *ctx);
	static void readcb(struct bufferevent *bev, void *ptr);
	static void eventcb(struct bufferevent *bev, short what, void *ptr);
	static void listenercb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *ptr);
private:
	static void tosockaddr(struct sockaddr_in &sin, const std::string &sIp, uint16_t iPort);
	static void getsockname(socket_t sock, std::string &sIp, uint16_t &iPort);
	static void getpeername(socket_t sock, std::string &sIp, uint16_t &iPort);
	static int bind(socket_t sock, std::string sIp, uint16_t iPort);
	static socket_t socket();
	static void doCloseSocket(socket_t sock, void *ptr);
};

#endif

