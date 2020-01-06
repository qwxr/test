#include "stdafx.h"
#include "POSIX_MYSQL.h"

POSIX_MYSQL::POSIX_MYSQL()
{
	sql_str = new char[length];
	memset(sql_str, 0, length);
}

POSIX_MYSQL::~POSIX_MYSQL()
{
	
	delete [] sql_str;
	std::cout << "delete sql_str." << std::endl;
	mysql_logout();
}

BOOL POSIX_MYSQL::mysql_login(std::string database_name)  ///
{
	rr::RrConfig config;
	config.ReadConfig("config.ini");
	std::string HostName = config.ReadString("MYSQL", "HostName", "127.0.0.1");
	std::string UserName = config.ReadString("MYSQL", "UserName", "root");
	std::string PassWord = config.ReadString("MYSQL", "PassWord", "clear123");
	int Port = config.ReadInt("MYSQL", "Port", 3306);

	std::cout << HostName << "," << UserName << "," << PassWord <<","<< Port << std::endl;
	mysql_init(&m_sqlCon);
	mysql_options(&m_sqlCon, MYSQL_SET_CHARSET_NAME, "gbk");
	if (!(mysql_real_connect(&m_sqlCon, HostName.c_str(), UserName.c_str(), PassWord.c_str(), database_name.c_str(), Port, NULL, 0)))
	{
		printf("fail connect!\n");
		return FALSE;
	}
	else
	{
		loginStatus = TRUE;
		printf("connect ok ...\n");
		mysql_query(&m_sqlCon, sz.c_str());
		mysql_real_query(&m_sqlCon, sz.c_str(), (UINT)sz.length());
		return TRUE;
	}
}

BOOL POSIX_MYSQL::mysql_logout()
{
	if (loginStatus)
	{
		mysql_close(&m_sqlCon);
		std::cout << "logout mysql" << std::endl;
	}
	return TRUE;
}

BOOL POSIX_MYSQL::mysql_query_sqlstr()
{
	mysql_real_query(&m_sqlCon, sql_str, (UINT)strlen(sql_str));
	return TRUE;
}