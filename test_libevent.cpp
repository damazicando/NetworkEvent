// test_libevent.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <thread>
#include "NetworkEvent.h"
#include "StringFormat.h"
#include "StringUtil.h"

void helloWorld()
{
	struct event_base *p_event_base = event_base_new();
}

int main()
{
    std::cout << "Hello World!\n"; 
	std::this_thread::sleep_for(std::chrono::seconds(2));

	auto cb = [](const NetworkMessage &msg)
	{
		std::string s;
		s += FormatI("Get one network message :\n");
		s += FormatI("msgtype:{0}, local:{1}:{2}, remote:{3}:{4}, socketid:{5}",
			msg.iMsgType, msg.sLocalIp, msg.iLocalPort, msg.sRemoteIp, msg.iRemotePort, msg.socketid);
		std::cout << s << std::endl;
		if (msg.iMsgType == 2)
		{
			std::cout << "msg: " << hexStr(msg.sMsg) << std::endl;
		}
	};

	NetworkEvent tDrive(cb);
	int iSocketId = tDrive.listen("127.0.0.1", 10012);
	uint16_t iRemotePort = 10013;
	tDrive.connect("127.0.0.1", iRemotePort);

	int x;
	std::cin >> x;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
