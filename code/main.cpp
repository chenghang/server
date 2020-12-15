#include "HttpServer.h"
#include "/mnt/hgfs/myshare/server/code/log/Logging.h"
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>

int main(int argc, char* argv[])   //argc表示运行文件时命令行参数的个数argv为数组里面存储参数
{
	LOG << "Welcome to the server!";
	int port = 8888;    //默认端口
	if(argc >= 2) {
		port = atoi(argv[1]);
	}
	int numThread = 4;
	if(argc >= 3) {     
		numThread = atoi(argv[2]);  //int atoi(const char *str)，把参数 str 所指向的字符串转换为一个整数（类型为 int 型）。
	}

	myserver::HttpServer server(port, numThread);   //创建一个服务器对象
	
	server.run();

	return 0;
}
