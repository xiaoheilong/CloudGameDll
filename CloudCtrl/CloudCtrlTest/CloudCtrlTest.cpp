// CloudCtrlTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#pragma once

#ifdef _WIN32
#pragma comment( lib, "ws2_32" )
#include <WinSock2.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <string>
#include <memory>

#include <iostream>
#include "CloudGame.h"
#include "windows.h"

#include "easywsclient.hpp"

#include "json/json.h"

#include "shellapi.h" 
#include "shlwapi.h" 
#include   <fstream> 
#include  <TlHelp32.h>
#pragma comment(lib,"shlwapi.lib")

using namespace std;

vector<string> split(const string& str, const string& delim) {
	vector<string> res;
	if ("" == str) return res;
	//先将要切割的字符串从string类型转换为char*类型  
	char * strs = new char[str.length() + 1]; //不要忘了  
	strcpy(strs, str.c_str());

	char * d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());

	char *p = strtok(strs, d);
	while (p) {
		string s = p; //分割得到的字符串转换为string类型  
		res.push_back(s); //存入结果数组  
		p = strtok(NULL, d);
	}

	return res;
}


DWORD GetProcessidFromName(LPCTSTR name)
{
	PROCESSENTRY32 pe;
	DWORD id = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
		//LogEx(g_UI_ERROR, "GetProcessidFromName Process32First is failure!");
	return 0;
	while (1)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == FALSE) {
			//LogEx(g_UI_ERROR, "GetProcessidFromName  result is false!");
			break;
		}
		if (lstrcmp(pe.szExeFile, name) == 0)
		{
			id = pe.th32ProcessID;
			//LogEx(g_UI_ERROR, "GetProcessidFromName  result is true!");
			break;
		}
	}
	CloseHandle(hSnapshot);
	return id;
}
//////////////////
LPCWSTR stringToLPCWSTR(std::string orig)
{
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t *wcstring = (wchar_t *)malloc(sizeof(wchar_t)*(orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);

	return wcstring;
}

///获取子进程的控制台输出
void InvokeChildProcessConsole(string exe)
{
	SECURITY_ATTRIBUTES saPipe;
	saPipe.nLength = sizeof(SECURITY_ATTRIBUTES);
	saPipe.lpSecurityDescriptor = NULL;
	saPipe.bInheritHandle = TRUE;

	HANDLE hReadPipe, hWritePipe;
	BOOL bSuccess = CreatePipe(&hReadPipe,
		&hWritePipe,
		&saPipe,
		0);
	if (!bSuccess)
		return;

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.hStdInput = hReadPipe;
	si.hStdOutput = hWritePipe;
	si.dwFlags = STARTF_USESTDHANDLES;
	si.cb = sizeof(si);
	char* exePtr = (char*)(exe.c_str());
	if (CreateProcess(NULL,exePtr , NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hThread);
		const int max = 500;
		char buf[max] = { 0 };
		DWORD dw;
		do {
			if (ReadFile(hReadPipe, buf, max - 1, &dw, NULL))
			{
				cout << buf << endl;
				//			ZeroMemory(buf,max);
			}
		} while (true);

		CloseHandle(pi.hProcess);
	}

	CloseHandle(hReadPipe);
	CloseHandle(hWritePipe);
}

bool str2byteEx(const std::string &str, BYTE &bRet)
{
	bRet = 0x00;       //结果
	size_t iPos = 1;   //位
	size_t power = 1;  //幂次

					   //没找的'x'返回
	if (std::string::npos == str.find("x"))
	{
		return false;
	}

	//从右往左依次取每个字符
	while (str.find("x") != (str.length() - iPos))
	{
		char cVal = str[str.length() - iPos];
		int iVal = int(cVal);

		//0~9
		if ((iVal >= 48) && (iVal <= 57))
		{
			bRet += ((iVal - 48) * power);
		}
		//A~F
		else if ((iVal >= 65) && (iVal <= 70))
		{
			bRet += ((iVal - 55) * power);
		}
		//a~f
		else if ((iVal >= 97) && (iVal <= 102))
		{
			bRet += ((iVal - 87) * power);
		}

		++iPos;
		power *= 16;
	}

	return true;
}

int main()
{
	////键盘鼠标初始化
	//KeyMouseInit();
	//Sleep(5000);
	//MouseDown(16383, 16383, 3);
	//MouseUp(16383, 16383, 3);
	//KeyDown(65);
	//KeyUp(65);
	//KeyMouseClose();
	////启动游戏
	//StartGame("C:\\Users\\ygh\\source\\repos\\ClgameDemoGamePC\\bin\\Debug\\ClgameDemoGamePC.exe", "111");
	////开始推流
	//push("io3ilhgr", "https://124.71.159.87:4443/","124.71.159.87", 30, 1000, 1, 4, 0, 0, 0, 0, 0);
	////关闭推流
	//closepush("io3ilhgr", "https://124.71.159.87:4443/");
	//bool re = wfile("https://www.baidu.com/", "1.txt");
	//int rerun = GetProcessidFromName(L"gst-launch-1.0.exe");
	//system("taskkill  /F /im gst-launch-1.0.exe");
	//InvokeChildProcessConsole("E://zyl//CloudGame//CloudCtrl//x64//Release//gstreamer//1.0//msvc_x86_64//bin//gst-launch-1.0.exe  -m rtpbin name=rtpbin dxgiscreencapsrc  !  queue ! decodebin ! videoconvert ! video/x-raw,format=I420,framerate=30/1 ! nvh264enc preset=1 bitrate=1000 rc-mode=2 gop-size=10  !h264parse config-interval=-1 ! rtph264pay pt=107 ssrc=2222 !rtprtxqueue max-size-time=2000 max-size-packets=0 ! rtpbin.send_rtp_sink_0 rtpbin.send_rtp_src_0 ! udpsink host=124.71.159.87 port=43880 rtpbin.send_rtcp_src_0 ! udpsink host=124.71.159.87 port=46246 sync=false async=false");
	/*InvokeChildProcessConsole("E:\\zyl\\CloudGame\\CloudCtrl\\x64\\Release\\gstreamer\\1.0\\msvc_x86_64\\bin\\gst-launch-1.0.exe   -m rtpbin name=rtpbin dxgiscreencapsrc  !  queue ! decodebin ! videoconvert ! video/x-raw,format=I420,framerate=30/1 ! nvh264enc preset=1 bitrate=1000 rc-mode=2 gop-size=10  !h264parse config-interval=-1 ! rtph264pay pt=107 ssrc=2222 !rtprtxqueue max-size-time=2000 max-size-packets=0 ! rtpbin.send_rtp_sink_0 rtpbin.send_rtp_src_0 ! udpsink host=124.71.159.87 port=45447 rtpbin.send_rtcp_src_0 ! udpsink host=124.71.159.87 port=48091 sync=false async=false");
	while (1) {
		Sleep(500);
	}*/
	BYTE *hatByte;
	std::string str1 = "10101010";
	hatByte =(BYTE*) str1.c_str();

	using easywsclient::WebSocket;
#ifdef _WIN32
	INT rc;
	WSADATA wsaData;

	rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (rc) {
		printf("WSAStartup Failed.\n");
		return 1;
	}
#endif
	KeyMouseInit();
	closepush("jack", "https://124.71.159.87:4443/");//io3ilhgr
	/////
	SetGStreamerLogPath("d://test.txt");
	push("jack", "https://124.71.159.87:4443/", "124.71.159.87", 30, 1000, 1, 4, 0, 0, 0, 0, 0);

	//KeyMouseInit();
	/////////////
	//////////////
	do{
		std::unique_ptr<WebSocket> ws(WebSocket::from_url("ws://124.71.159.87:4451/"));
		assert(ws);
		ws->send("{\r\n  \"type\": \"login\",\r\n  \"port\": \"200\",\r\n  \"key\": null,\r\n  \"ud\": 0\r\n}");
		while (ws->getReadyState() != WebSocket::CLOSED) {
			WebSocket::pointer wsp = &*ws; // <-- because a unique_ptr cannot be copied into a lambda
			ws->poll();
			ws->dispatch([wsp](const std::string & message) {
				printf(">>> %s\n", message.c_str());
				if (message != "")
				{
					Json::Reader reader;
					Json::Value root;
					if (reader.parse(message, root))
					{
						int ud = root["ud"].asInt();
						if (ud == 1)
						{
							int key = root["key"].asInt();
							KeyDown(key);
						}
						if (ud == 2)
						{
							int key = root["key"].asInt();
							KeyUp(key);
						}
						if (ud == 4)
						{
							string keycode = root["key"].asString();
							vector<string> AllStr = split(keycode, "|");
							int x = atoi(AllStr[0].c_str());
							int y = atoi(AllStr[1].c_str());
							int key = atoi(AllStr[2].c_str());
							MouseMove(x, y, key);
						}
						if (ud == 6)
						{
							string keycode = root["key"].asString();
							vector<string> AllStr = split(keycode, "|");
							int x = atoi(AllStr[0].c_str());
							int y = atoi(AllStr[1].c_str());
							int key = atoi(AllStr[2].c_str());
							MouseDown(x, y, key);
						}
						if (ud == 7)
						{
							string keycode = root["key"].asString();
							vector<string> AllStr = split(keycode, "|");
							int x = atoi(AllStr[0].c_str());
							int y = atoi(AllStr[1].c_str());
							int key = atoi(AllStr[2].c_str());
							MouseUp(x, y, key);
						}
					}
					//wsp->close(); 
				}
			});
		}
	} while (true);


#ifdef _WIN32
	WSACleanup();
#endif
	KeyMouseClose();
	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
