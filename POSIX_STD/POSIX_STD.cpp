// POSIX_STD.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "json/json.h"
#include <list>
#include <mutex>
#include "process.h"
#include "RrConfig.h"
#include <memory>
#include "POSIX_MYSQL.h"
#include "spdlog/spdlog.h"
#include "STD.h"
#pragma comment(lib,"ws2_32.lib") 

//#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup")

#define MAX_MESSAGE_COUNT 1000   //最大接收消息的数量




typedef struct
{
	int length;
	uint8_t *message;
}MessageNode_t;

std::list<MessageNode_t>  m_MessageList; //消息队列
std::mutex m_Mutex;
std::mutex m_Mutex_log;

unsigned int __stdcall MessageRecvThread(LPVOID); //消息接收
void ProcessMessage();


unsigned int __stdcall MessageRecvThread(LPVOID)
{
	////////////////////////////////
	///////////读配置文件///////////
	////////////////////////////////
	rr::RrConfig config;
	config.ReadConfig("config.ini");
	int PORT = config.ReadInt("SOCKET", "Port", 0);
	////////////////////////////////
	
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return 0;
	}
	char recvbuf[1024];
	SOCKET  socket1;
	SOCKADDR_IN client;//分配一个地址结构体
	int len_client = sizeof(client);
	int	receive_bytes = 0;
	socket1 = socket(AF_INET, SOCK_DGRAM, 0);

	client.sin_family = AF_INET;
	client.sin_addr.s_addr = htonl(INADDR_ANY);
	client.sin_port = htons(PORT);

	if (::bind(socket1, (sockaddr*)&client, sizeof(client)) == SOCKET_ERROR)
	{
		return 0;
	}
	int timeout = 2000; //单位：毫秒
	if (setsockopt(socket1, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return 0;
	}

	//设置接收缓冲区大小, 设大一点可以减少丢包频率
	int nRecvBuf = 512 * 1024;
	if (setsockopt(socket1, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int)) == SOCKET_ERROR)
	{
		return 0;
	}


	std::shared_ptr<spdlog::logger>mylogger;
	mylogger = spdlog::get("log");

	std::shared_ptr<spdlog::logger>console;
	console = spdlog::get("console");

	int temp_size = 0;
	while (TRUE)
	{
		memset(recvbuf, 0, 1024);
		receive_bytes = recvfrom(socket1, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&client, &len_client);
		if (receive_bytes <= 0)
		{
			//std::cout << "等待从" << PORT << "端口接收到数据" << std::endl;
			console->info("等待从{}端口接收到数据", PORT);
			//mylogger->info("{}:{} 等待从{}端口接收到数据", __FILE__, __LINE__, PORT);
			Sleep(1);
			continue;
		}
		//std::cout << "接收到" << PORT << "端口" << receive_bytes << " 字节长的数据数据 " << std::endl;
		console->info("接收到{}端口{}字节长的数据数据", PORT, receive_bytes);
		mylogger->info("{}:{} 接收到{}端口{}字节长的数据数据", __FILE__, __LINE__ , PORT, receive_bytes);


		mylogger->info("{}:{} socket方式接收到指令的时间", __FILE__, __LINE__);

		if (m_MessageList.size() < MAX_MESSAGE_COUNT)   //超过1000个包时放弃数据
		{
			console->info("目前有{}条消息待处理", m_MessageList.size());
			mylogger->info("{}:{} 目前有{}条消息待处理", __FILE__, __LINE__, m_MessageList.size());
			//std::cout << "目前有" << m_MessageList.size() << "条消息待处理" << std::endl;
			MessageNode_t  temMessage;
			temMessage.length = receive_bytes + 1;
			temMessage.message = new uint8_t[receive_bytes + 1];
			memset(temMessage.message, 0, receive_bytes + 1);
			memcpy(temMessage.message, recvbuf, receive_bytes);

			m_Mutex.lock();
			m_MessageList.push_back(temMessage); 
			m_Mutex.unlock();

			mylogger->info("{}:{} 保存指令到消息队列", __FILE__, __LINE__);
		}
	}
	closesocket(socket1);
	return 0;
}


void ProcessMessage()
{
	std::shared_ptr<spdlog::logger>mylogger;
	mylogger = spdlog::get("log");

	std::shared_ptr<spdlog::logger>console;
	console = spdlog::get("console");


	Json::Reader reader;
	Json::Value root;
	std::list<MessageNode_t>::iterator itr;
	uint8_t *pstr;
	while (TRUE)
	{
		
		std::shared_ptr<STD>p(new STD);
		
		while (TRUE)
		{
			pstr = NULL;
			if (!m_MessageList.empty())
			{
				m_Mutex.lock();
				itr = m_MessageList.begin();
				if (itr != m_MessageList.end())
				{
					pstr = new uint8_t[itr->length];
					memset(pstr, 0, itr->length);
					memcpy(pstr, itr->message, itr->length);
				}
				delete[] itr->message; //释放消息内存
				m_MessageList.erase(itr);   //删除这条消息	
				m_Mutex.unlock();

				mylogger->info("{}:{} 从消息队列中读取到一条指令", __FILE__, __LINE__);
			}
			if (NULL != pstr)
			{
				std::shared_ptr<uint8_t>sp(pstr);

	

				mylogger->info("{}:{} 开始解析指令",__FILE__,__LINE__);

				std::cout << (char*)pstr << std::endl;
				if (reader.parse((char*)pstr, root))
				{
					p->test();

					int MODE = root["MODE"].asInt();
					std::cout << "MODE=" << MODE << std::endl;
					if (0 == MODE)
						break;   // MODE==0时销毁对象   

					switch (MODE)
					{
					case 1:      ////处理不同的情况
					{
									 std::cout << "mode:" << MODE << std::endl;
					}
						break;
					default:
						break;
					}

				}

			}
			Sleep(2);
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	auto console = spdlog::stdout_color_mt("console");
	console->set_level(spdlog::level::debug);


	auto mylogger = spdlog::basic_logger_mt("log", "logs/log.txt");
	mylogger->flush_on(spdlog::level::debug);



	HANDLE  m_hMessageRecvThread;
	m_hMessageRecvThread = (HANDLE)_beginthreadex(NULL, 0, MessageRecvThread, NULL, 0, NULL);
	ProcessMessage();


	/*{
		std::shared_ptr<POSIX_MYSQL>ps(new POSIX_MYSQL);
		ps->mysql_login(std::string("facerec"));
		sprintf_s(ps->sql_str, 1024, "insert into cs (IP,PORT,presetnum) values ('%s',%d,'%s')", \
			std::string("192.1.1.1").c_str(), 12345, std::string("123").c_str());
		ps->mysql_query_sqlstr();
	}*/


	

	system("pause");
	return 0;
}

