// POSIX_STD.cpp : �������̨Ӧ�ó������ڵ㡣
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

#define MAX_MESSAGE_COUNT 1000   //��������Ϣ������




typedef struct
{
	int length;
	uint8_t *message;
}MessageNode_t;

std::list<MessageNode_t>  m_MessageList; //��Ϣ����
std::mutex m_Mutex;
std::mutex m_Mutex_log;

unsigned int __stdcall MessageRecvThread(LPVOID); //��Ϣ����
void ProcessMessage();


unsigned int __stdcall MessageRecvThread(LPVOID)
{
	////////////////////////////////
	///////////�������ļ�///////////
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
	SOCKADDR_IN client;//����һ����ַ�ṹ��
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
	int timeout = 2000; //��λ������
	if (setsockopt(socket1, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
	{
		return 0;
	}

	//���ý��ջ�������С, ���һ����Լ��ٶ���Ƶ��
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
			//std::cout << "�ȴ���" << PORT << "�˿ڽ��յ�����" << std::endl;
			console->info("�ȴ���{}�˿ڽ��յ�����", PORT);
			//mylogger->info("{}:{} �ȴ���{}�˿ڽ��յ�����", __FILE__, __LINE__, PORT);
			Sleep(1);
			continue;
		}
		//std::cout << "���յ�" << PORT << "�˿�" << receive_bytes << " �ֽڳ����������� " << std::endl;
		console->info("���յ�{}�˿�{}�ֽڳ�����������", PORT, receive_bytes);
		mylogger->info("{}:{} ���յ�{}�˿�{}�ֽڳ�����������", __FILE__, __LINE__ , PORT, receive_bytes);


		mylogger->info("{}:{} socket��ʽ���յ�ָ���ʱ��", __FILE__, __LINE__);

		if (m_MessageList.size() < MAX_MESSAGE_COUNT)   //����1000����ʱ��������
		{
			console->info("Ŀǰ��{}����Ϣ������", m_MessageList.size());
			mylogger->info("{}:{} Ŀǰ��{}����Ϣ������", __FILE__, __LINE__, m_MessageList.size());
			//std::cout << "Ŀǰ��" << m_MessageList.size() << "����Ϣ������" << std::endl;
			MessageNode_t  temMessage;
			temMessage.length = receive_bytes + 1;
			temMessage.message = new uint8_t[receive_bytes + 1];
			memset(temMessage.message, 0, receive_bytes + 1);
			memcpy(temMessage.message, recvbuf, receive_bytes);

			m_Mutex.lock();
			m_MessageList.push_back(temMessage); 
			m_Mutex.unlock();

			mylogger->info("{}:{} ����ָ���Ϣ����", __FILE__, __LINE__);
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
				delete[] itr->message; //�ͷ���Ϣ�ڴ�
				m_MessageList.erase(itr);   //ɾ��������Ϣ	
				m_Mutex.unlock();

				mylogger->info("{}:{} ����Ϣ�����ж�ȡ��һ��ָ��", __FILE__, __LINE__);
			}
			if (NULL != pstr)
			{
				std::shared_ptr<uint8_t>sp(pstr);

	

				mylogger->info("{}:{} ��ʼ����ָ��",__FILE__,__LINE__);

				std::cout << (char*)pstr << std::endl;
				if (reader.parse((char*)pstr, root))
				{
					p->test();

					int MODE = root["MODE"].asInt();
					std::cout << "MODE=" << MODE << std::endl;
					if (0 == MODE)
						break;   // MODE==0ʱ���ٶ���   

					switch (MODE)
					{
					case 1:      ////����ͬ�����
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

