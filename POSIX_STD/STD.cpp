#include "stdafx.h"
#include"STD.h"


void STD::test()
{
	std::shared_ptr<spdlog::logger>mylogger;
	mylogger = spdlog::get("log");
	mylogger->info("{}:{} STD::test", __FILE__, __LINE__);
}