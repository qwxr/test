#include "stdafx.h"
#include <winsock2.h>
#include "mysql.h"
#include <string>
#include <iostream>
#include "RrConfig.h"
#pragma comment(lib,"libmysql.lib") 

class POSIX_MYSQL{
public:
	POSIX_MYSQL();
	~POSIX_MYSQL();

public:
	BOOL mysql_login(std::string database_name);
	BOOL mysql_logout();
	BOOL mysql_query_sqlstr();

public:
	const int length = 1024;
	char *sql_str = nullptr;
	MYSQL m_sqlCon;
	std::string sz = "SET NAMES 'GB2312'";
	BOOL loginStatus = FALSE;
};