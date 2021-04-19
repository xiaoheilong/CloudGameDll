// CloudGame.cpp : 定义 DLL 的导出函数。
//

#include "pch.h"
#include "framework.h"
#include <string>  
#include <iostream> 
#include "CloudGame.h"
#include "curl/curl.h"
#include "json/json.h"
#include <assert.h> 
#include "libGetrtpcmd.h"
#include "windows.h"
#include "shellapi.h" 
#include "shlwapi.h" 
#include "windows.h"
#include <fstream> 
#include <TlHelp32.h>
#include <thread>
#include <memory>
#include <atlconv.h>
#include <TCHAR.H>
#pragma comment(lib,"shlwapi.lib")
using namespace std;

// 这是导出变量的一个示例
CLOUDGAME_API int nCloudGame=0;
CLOUDGAME_API HANDLE handle_key;
CLOUDGAME_API HANDLE handle_mouse;
CLOUDGAME_API HANDLE handle_joystick;
/////////封装日志函数的接口
const char*   g_UI_ERROR = "[UI_ERROR]";
const char*   g_UI_INFO  = "[UI_INFO]";
const char*   g_UI_WARN  = "[UI_WARN]";
//////////////////////有关gst-launch-1.0的日志输出, 内部不支持多线程
std::shared_ptr<std::thread>  g_videoOutput = NULL;
std::shared_ptr<std::thread>  g_audioOutput = NULL;
bool  outputLogFlag = false;
std::string g_logPath;
///////////////////////
bool LogEx(const std::string logType, const std::string logMsg);
bool LogEx(const std::string logType, const string prefix, const std::string logMsg);
DWORD GetProcessidFromName(LPCTSTR name);

//////////
bool KillGstLaunchExe() {
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	std::string exe = "gst-launch-1.0.exe";
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);
		return false;
	}

	BOOL    bRet = FALSE;
	DWORD dwPid = -1;
	while (Process32Next(hProcessSnap, &pe32))
	{
		//将WCHAR转成const char*
		int iLn = WideCharToMultiByte(CP_UTF8, 0, const_cast<LPWSTR> (pe32.szExeFile), static_cast<int>(sizeof(pe32.szExeFile)), NULL, 0, NULL, NULL);
		std::string result(iLn, 0);
		WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, static_cast<int>(sizeof(pe32.szExeFile)), const_cast<LPSTR> (result.c_str()), iLn, NULL, NULL);
		if (0 == strcmp(exe.c_str(), result.c_str()))
		{
			dwPid = pe32.th32ProcessID;
			bRet = TRUE;
			break;
		}
	}
	CloseHandle(hProcessSnap);
	HANDLE hProcess = NULL;
	//打开目标进程
	hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwPid);
	if (hProcess == NULL) {
		return false;
	}
	//结束目标进程
	DWORD ret = TerminateProcess(hProcess, 0);
	if (ret == 0) {
		return false;
	}
	return true;

}



void CloseOutputLog() {
	/*outputLogFlag = false;
	if (g_videoOutput) {
		if (g_videoOutput->joinable()) {
			g_videoOutput->join();
		}
	}

	if (g_audioOutput) {
		if (g_audioOutput->joinable()) {
			g_audioOutput->join();
		}
	}*/
}

std::string StringReplace(const char *pszSrc, const char *pszOld, const char *pszNew)
{
	std::string strContent, strTemp;
	strContent.assign(pszSrc);
	std::string::size_type nPos = 0;
	while (true)
	{
		nPos = strContent.find(pszOld, nPos);
		strTemp = strContent.substr(nPos + strlen(pszOld), strContent.length());
		if (nPos == std::string::npos)
		{
			break;
		}
		strContent.replace(nPos, strContent.length(), pszNew);
		strContent.append(strTemp);
		nPos += strlen(pszNew) - strlen(pszOld) + 1; //防止重复替换 避免死循环
	}
	return strContent;
}

///////////////////
//////////////
static LogCallback  g_functionCallback = NULL;
bool LogEx(const std::string logType, const std::string logMsg) {
	if (g_functionCallback) {
		g_functionCallback(logType, logMsg);
		return true;
	}
	return false;
}

bool LogEx(const std::string logType,const string prefix ,  const std::string logMsg) {
	string str = "[";
	str += prefix;
	str += "]";
	str += logMsg;
	return LogEx(logType , str);
}

std::string TCHAR2STRING(TCHAR* str)
{

	std::string strstr = "";
	if (!str) {
		return strstr;
	}
	try
	{
		int iLen = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);

		char* chRtn = new char[iLen * sizeof(char)];

		WideCharToMultiByte(CP_ACP, 0, str, -1, chRtn, iLen, NULL, NULL);

		strstr = chRtn;
	}
	catch (std::exception e)
	{
	}

	return strstr;
}

TCHAR* String2TCHAR(std::string str) {
	if (str.empty()) {
		return NULL;
	}
	int iLen = strlen(str.c_str());
	TCHAR *chRtn = new TCHAR[iLen + 1];
	mbstowcs(chRtn, str.c_str(), iLen + 1);
	return chRtn;
}

void InvokeChildProcessConsole_CloudGame(std::string/*TCHAR **/ exe, bool show)
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
	si.wShowWindow = show ? SW_SHOW : SW_HIDE;
	si.cb = sizeof(si);
	//char* exePtr = (char*)(exe.c_str());
	TCHAR  *goalStr = NULL;
	goalStr = String2TCHAR(exe);
	if (!goalStr) {
		return;
	}
	if (CreateProcess(NULL, goalStr, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hThread);
		const int max = 500;
		char buf[max] = { 0 };
		DWORD dw;
		//do {
		//	if (ReadFile(hReadPipe, buf, max - 1, &dw, NULL))
		//	{
		//		//cout << buf << endl;
		//		LogEx(g_UI_INFO , buf);
		//		//			ZeroMemory(buf,max);
		//		
		//	}
		//	Sleep(100);
		//} while (outputLogFlag);

		CloseHandle(pi.hProcess);
	}
	delete[]  goalStr;
	goalStr = NULL;
	CloseHandle(hReadPipe);
	CloseHandle(hWritePipe);
}

///////////////
// 这是导出函数的一个示例。
CLOUDGAME_API int fnCloudGame(void)
{
	/*HANDLE handle = NULL;
	BOOL szbuffer = libGetrtpcmd::DeviceOpen(handle, 0xF00F,0x00000002);
	libGetrtpcmd::DeviceClose(handle);
	std::string tt = libGetrtpcmd::HttpGet("https://124.71.159.87:4443/rooms/io3ilhgr");*/
	//std::string tt = libGetrtpcmd::httpGet("https://baidu.com");
	libGetrtpcmd::KeyCodeInit();
    return 0;
}
LPCWSTR stringToLPCWSTR(std::string orig)
{
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t *wcstring = (wchar_t *)malloc(sizeof(wchar_t)*(orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
	return wcstring;
}
DWORD GetProcessidFromName(LPCTSTR name)
{
	PROCESSENTRY32 pe;
	DWORD id = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
		LogEx(g_UI_ERROR , "GetProcessidFromName Process32First is failure!");
		return 0;
	while (1)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == FALSE) {
			LogEx(g_UI_ERROR, "GetProcessidFromName  result is false!");
			break;
		}
		if (lstrcmp(pe.szExeFile, name) == 0)
		{
			id = pe.th32ProcessID;
			LogEx(g_UI_ERROR, "GetProcessidFromName  result is true!");
			break;
		}
	}
	CloseHandle(hSnapshot);
	return id;
}

CLOUDGAME_API bool push(char * port, char * url,char* hosturl, int framerate, int bitrate, int deadline, int cpuused, int x, int y, int mode, int capmode, int vol)
{
	string AUDIO_SSRC = "1111";
	string AUDIO_PT = "100";
	string VIDEO_SSRC = "2222";
	string VIDEO_PT = "107";
	string SERVER_URL(url);
	string ROOM_ID(port);
	string BROADCASTER_ID(port);
	string host_URL(hosturl);
	/*string SERVER_URL("https://124.71.159.87:4443/");
	string ROOM_ID("io3ilhgr");
	string BROADCASTER_ID("io3ilhgr");*/
	string logStr = "";
	logStr = string(" HttpGet: ");
	logStr += SERVER_URL;
	logStr += "rooms/";
	logStr += ROOM_ID;
	LogEx(g_UI_INFO, logStr);
	string re = libGetrtpcmd::HttpGet(SERVER_URL + "rooms/" + ROOM_ID);
	if (re.find("not found") != re.npos)
	{
		logStr += "is not exist!";
		LogEx(g_UI_WARN, logStr);
		logStr.clear();
		string re_creat = libGetrtpcmd::HttpGet(SERVER_URL + "rooms/creat/" + ROOM_ID);
		logStr = SERVER_URL + "rooms/creat/" + ROOM_ID;
		LogEx(g_UI_INFO , "HttpGet" , logStr);
	}
	logStr = SERVER_URL + "rooms/" + ROOM_ID + "/broadcasters/delete/" + BROADCASTER_ID;
	LogEx(g_UI_INFO, string(" HttpGet: ")  , logStr);
	string re_Broadcaster_del = libGetrtpcmd::HttpGet(SERVER_URL + "rooms/" + ROOM_ID + "/broadcasters/delete/" + BROADCASTER_ID);
	logStr += re_Broadcaster_del;
	LogEx(g_UI_INFO, "HttpGet recieve: " , logStr);
	string str_post = "{\r\n  \"id\": \""+ BROADCASTER_ID +"\",\r\n  \"displayName\": \"broadcaster\",\r\n  \"device\": {\r\n    \"name\": \"FFmpeg\"\r\n  },\r\n  \"rtpCapabilities\": {\r\n    \"codecs\": [\r\n      {\r\n        \"parameters\": {\r\n          \"useinbandfec\": 0,\r\n          \"packetization-mode\": 1,\r\n          \"profile-level-id\": \"42e01f\",\r\n          \"level-asymmetry-allowed\": 1\r\n        },\r\n        \"rtcpFeedback\": [\r\n          {\r\n            \"type\": \"nack\"\r\n          }\r\n        ],\r\n        \"mimeType\": \"video/H264\",\r\n        \"clockRate\": 90000,\r\n        \"kind\": \"video\",\r\n        \"preferredPayloadType\": 107,\r\n        \"channels\": 0\r\n      }\r\n    ]\r\n  }\r\n}";
	LogEx(g_UI_INFO , string("HttpPostRaw") , str_post);
	string re1 = libGetrtpcmd::HttpPostRaw(SERVER_URL + "rooms/" + ROOM_ID + "/broadcasters", str_post);
	LogEx(g_UI_INFO, string("HttpPostRaw recieve  re1:") , re1);
	string re_audio = "";
	string str_post_PlainTransport_js = "{\r\n  \"type\": \"plain\",\r\n  \"comedia\": true,\r\n  \"rtcpMux\": false,\r\n  \"enableSctp\": false\r\n}";
	Json::Reader reader;
	Json::Value root;
	string m_video_id = "";
	string m_audio_id = "";
	int m_video_port = 0;
	int m_audio_port = 0;
	int m_video_rtcpPort = 0;
	int m_audio_rtcpPort = 0;
	if (vol==0)
	{
		logStr = SERVER_URL + "rooms/" + ROOM_ID
			+ "/broadcasters/" + BROADCASTER_ID + "/transports" + str_post_PlainTransport_js;
		LogEx(g_UI_INFO, string("HttpPostRaw ") , logStr);
		re_audio = libGetrtpcmd::HttpPostRaw(SERVER_URL + "rooms/" + ROOM_ID
			+ "/broadcasters/" + BROADCASTER_ID + "/transports", str_post_PlainTransport_js);

		logStr =  re_audio;
		LogEx(g_UI_INFO, string("HttpPostRaw  recieve:") , logStr);
		if (reader.parse(re_audio, root))
		{
			m_audio_id = root["id"].asString();
			m_audio_port = root["port"].asInt();
			
			m_audio_rtcpPort = root["rtcpPort"].asInt();
			//////
			logStr = " m_audio_id: ";
			logStr += m_audio_id;
			logStr += " m_audio_port: ";
			logStr += to_string(m_audio_port);
			logStr += " m_audio_rtcpPort: ";
			logStr += to_string(m_audio_rtcpPort);
			LogEx(g_UI_INFO, logStr);
		}
		else {
			logStr = "get the re_audio params failure!";
			LogEx(g_UI_INFO, logStr);
			return false;
		}
	}

	logStr = SERVER_URL + "rooms/" + ROOM_ID
		+ "/broadcasters/" + BROADCASTER_ID + "/transports" + str_post_PlainTransport_js;
	LogEx(g_UI_INFO, string("HttpPostRaw")  , logStr.c_str());
	string re_video = libGetrtpcmd::HttpPostRaw(SERVER_URL + "rooms/" + ROOM_ID
		+ "/broadcasters/" + BROADCASTER_ID + "/transports", str_post_PlainTransport_js);
	logStr =  re_video;
	LogEx(g_UI_INFO, string("HttpPostRaw reciece :") , logStr.c_str());
	if (reader.parse(re_video, root))
	{
		m_video_id = root["id"].asString(); 
		m_video_port = root["port"].asInt();
		m_video_rtcpPort = root["rtcpPort"].asInt();

		//////
		logStr = " m_video_id: ";
		logStr += m_video_id;
		logStr += " m_video_port: ";
		logStr += std::to_string(m_video_port);
		logStr += " m_video_rtcpPort: ";
		logStr += std::to_string(m_video_rtcpPort);
		LogEx(g_UI_INFO, logStr);
	}
	else {

		logStr = "get the re_video params failure!";
		LogEx(g_UI_INFO, logStr.c_str());
		return false;
	}
	
	string str_Producer_video = "";
	if (mode == 0 || mode == 1)
	{
		str_Producer_video = "{\r\n  \"rtpParameters\": {\r\n    \"codecs\": [\r\n      {\r\n        \"parameters\": {\r\n          \"profile-level-id\": \"42e01f\",\r\n          \"spropstereo\": 0,\r\n          \"packetization-mode\": 1,\r\n          \"level-asymmetry-allowed\": 1,\r\n          \"x-google-start-bitrate\": 4096\r\n        },\r\n        \"mimeType\": \"video/h264\",\r\n        \"payloadType\": 107,\r\n        \"clockRate\": 90000,\r\n        \"channels\": 1\r\n      }\r\n    ],\r\n    \"encodings\": [\r\n      {\r\n        \"ssrc\": 2222\r\n      }\r\n    ]\r\n  },\r\n  \"kind\": \"video\"\r\n}";
	}
	if (mode == 2)
	{
		//str_Producer_video = "{\r\n  \"rtpParameters\": {\r\n    \"codecs\": [\r\n      {\r\n        \"parameters\": {\r\n          \"profile-level-id\": \"42e01f\",\r\n          \"spropstereo\": 0,\r\n          \"packetization-mode\": 1,\r\n          \"level-asymmetry-allowed\": 1,\r\n          \"x-google-start-bitrate\": 4096\r\n        },\r\n        \"mimeType\": \"video/h264\",\r\n        \"payloadType\": 107,\r\n        \"clockRate\": 90000,\r\n        \"channels\": 1\r\n      }\r\n    ],\r\n    \"encodings\": [\r\n      {\r\n        \"ssrc\": 2222\r\n      }\r\n    ]\r\n  },\r\n  \"kind\": \"video\"\r\n}";
		str_Producer_video = "{\r\n  \"rtpParameters\": {\r\n    \"codecs\": [\r\n      {\r\n        \"parameters\": {\r\n          \"profile-level-id\": \"42e01f\",\r\n          \"spropstereo\": 0,\r\n          \"packetization-mode\": 1,\r\n          \"level-asymmetry-allowed\": 1,\r\n          \"x-google-start-bitrate\": 4096\r\n        },\r\n        \"mimeType\": \"video/vp8\",\r\n        \"payloadType\": 107,\r\n        \"clockRate\": 90000,\r\n        \"channels\": 1\r\n      }\r\n    ],\r\n    \"encodings\": [\r\n      {\r\n        \"ssrc\": 2222\r\n      }\r\n    ]\r\n  },\r\n  \"kind\": \"video\"\r\n}";
	}
	logStr = SERVER_URL + "rooms/" + ROOM_ID
		+ "/broadcasters/" + BROADCASTER_ID + "/transports/" + m_video_id + "/producers", str_Producer_video;
	LogEx(g_UI_INFO , "HttpPostRaw" , logStr);
	string re_Producer_video = libGetrtpcmd::HttpPostRaw(SERVER_URL + "rooms/" + ROOM_ID
		+ "/broadcasters/" + BROADCASTER_ID + "/transports/" + m_video_id + "/producers", str_Producer_video);
	LogEx(g_UI_INFO, "HttpPostRaw recieve", re_Producer_video);
	string str_Producer_audio = "{\r\n  \"rtpParameters\": {\r\n    \"codecs\": [\r\n      {\r\n        \"parameters\": {\r\n          \"spropstereo\": 1\r\n        },\r\n        \"mimeType\": \"audio/opus\",\r\n        \"payloadType\": 100,\r\n        \"clockRate\": 48000,\r\n        \"channels\": 2\r\n      }\r\n    ],\r\n    \"encodings\": [\r\n      {\r\n        \"ssrc\": 1111\r\n      }\r\n    ]\r\n  },\r\n  \"kind\": \"audio\"\r\n}";
	if (vol == 0)
	{
		logStr = SERVER_URL + "rooms/" + ROOM_ID
			+ "/broadcasters/" + BROADCASTER_ID + "/transports/" + m_audio_id + "/producers", str_Producer_audio;
		LogEx(g_UI_INFO , "HttpPostRaw" , logStr);
		string re_Producer_audio = libGetrtpcmd::HttpPostRaw(SERVER_URL + "rooms/" + ROOM_ID
			+ "/broadcasters/" + BROADCASTER_ID + "/transports/" + m_audio_id + "/producers", str_Producer_audio);
		LogEx(g_UI_INFO, " HttpPostRaw reciece", logStr);
	}
	string str_videoscale = "";
	if (x != 0 && y != 0) 
	{
		str_videoscale = " videoscale !video/x-raw,width=" + to_string(x) + ",height=" + to_string(y) + ",framerate=" + to_string(framerate) + "/1!";
	}
	else
	{
		str_videoscale = " video/x-raw,framerate=" + to_string(framerate) + "/1!";
	}
	string str_cursor = "";
	/*if (chk_cur.Checked)
	{
		str_cursor = " cursor=true";
	}*/
	//gdiscreencapsrc dx9screencapsrc dxgiscreencapsrc
	string scr_type = "dxgiscreencapsrc";
	if (capmode==1)
	{
		scr_type = "dx9screencapsrc";
	}
	if (capmode==0)
	{
		scr_type = "dxgiscreencapsrc";
	}
	if (capmode==2)
	{
		scr_type = "gdiscreencapsrc";
	}
	string cmd_gst1 = "";
	if (mode==1)
	{
		cmd_gst1 += "gst-launch-1.0  -m rtpbin name=rtpbin " + scr_type + " " + str_cursor + " ! " +
			" queue ! decodebin ! videoconvert " +
			"! video/x-raw,format=I420,framerate=" + to_string(framerate) + "/1 ! x264enc tune=zerolatency speed-preset=1 dct8x8=true quantizer=23 pass=qual " +
			"! rtph264pay pt=" + VIDEO_PT + " ssrc=" + VIDEO_SSRC + " !rtprtxqueue max-size-time=2000 max-size-packets=0 " +
			"! rtpbin.send_rtp_sink_0 rtpbin.send_rtp_src_0 " +
			"! udpsink host=" + host_URL + " port=" + to_string(m_video_port) + " rtpbin.send_rtcp_src_0 " +
			"! udpsink host=" + host_URL + " port=" + to_string(m_video_rtcpPort) + " sync=false async=false ";
	}
	if (mode == 0)
	{
		cmd_gst1 += "gst-launch-1.0   -m rtpbin name=rtpbin " + scr_type + " " + str_cursor + " ! " +
			" queue ! decodebin ! videoconvert " +
			"! video/x-raw,format=I420,framerate=" + to_string(framerate) + "/1 ! nvh264enc preset=1 bitrate=" + to_string(bitrate)
			+ " rc-mode=2 gop-size=10  !h264parse config-interval=-1 !"
			+ " rtph264pay pt=" + VIDEO_PT + " ssrc=" + VIDEO_SSRC + " !rtprtxqueue max-size-time=2000 max-size-packets=0 " +
			"! rtpbin.send_rtp_sink_0 rtpbin.send_rtp_src_0 " +
			"! udpsink host=" + host_URL + " port=" + to_string(m_video_port) + " rtpbin.send_rtcp_src_0 " +
			"! udpsink host=" + host_URL + " port=" + to_string(m_video_rtcpPort) + " sync=false async=false ";
	}
	if (mode==2)
	{
		cmd_gst1 += "gst-launch-1.0  -m rtpbin name=rtpbin " + scr_type + " " + str_cursor + " ! " +
			" queue ! decodebin ! " + str_videoscale + " videoconvert " +
			"! vp8enc target-bitrate=" + to_string(bitrate) + "000 deadline=" + to_string(deadline) + " cpu-used=" + to_string(cpuused) + " " +
			"! rtpvp8pay pt=" + VIDEO_PT + " ssrc=" + VIDEO_SSRC + " picture-id-mode=2 " +
			"! rtpbin.send_rtp_sink_0 rtpbin.send_rtp_src_0 " +
			"! udpsink host=" + host_URL + " port=" + to_string(m_video_port) + " rtpbin.send_rtcp_src_0 " +
			"! udpsink host=" + host_URL + " port=" + to_string(m_video_rtcpPort) + " sync=false async=false";
	}

	
	string cmd_gst2 = "";
	if (vol==0)
	{
		cmd_gst2 = cmd_gst1 + " wasapisrc !  queue ! decodebin ! audioresample !audio/x-raw, rate=8000 ! audioconvert ! " +
			" opusenc ! rtpopuspay pt=" + AUDIO_PT + " ssrc=" + AUDIO_SSRC + " " +
			"! rtpbin.send_rtp_sink_1 rtpbin.send_rtp_src_1 ! udpsink host=" + host_URL + " port=" + to_string(m_audio_port) + " " +
			"rtpbin.send_rtcp_src_1 ! udpsink host=" + host_URL + " port=" + to_string(m_audio_rtcpPort) + " sync=false async=false";
		/*cmd_gst2 = cmd_gst1 + " wasapisrc !  queue ! decodebin ! audioresample !audio/x-raw, rate=8000 ! audioconvert ! " +
			" opusenc ! rtpopuspay pt=" + AUDIO_PT + " ssrc=" + AUDIO_SSRC + " " +
			"! rtpbin.send_rtp_sink_1 rtpbin.send_rtp_src_1 ! udpsink host=" + host_URL + " port=" + to_string(m_audio_port) + " " +
			"rtpbin.send_rtcp_src_1 ! udpsink host=" + host_URL + " port=" + to_string(m_audio_rtcpPort) + " sync=false async=false";*/
	}
	else {

	}

	TCHAR szBuf[MAX_PATH];
	ZeroMemory(szBuf, MAX_PATH);
	GetModuleFileName(NULL, szBuf, MAX_PATH);
	PathRemoveFileSpec(szBuf);

	TCHAR szBuf_path[MAX_PATH];
	ZeroMemory(szBuf_path, MAX_PATH);
	swprintf(szBuf_path, L"%s%s", szBuf, L"\\gstreamer\\1.0\\msvc_x86_64\\bin\\runvedio.bat");

	TCHAR szBuf_cmdpath[MAX_PATH];
	ZeroMemory(szBuf_cmdpath, MAX_PATH);
	swprintf(szBuf_cmdpath, L"%s%s", szBuf, L"\\gstreamer\\1.0\\msvc_x86_64\\bin\\runvedio.bat");
	PathRemoveFileSpec(szBuf_cmdpath);

	TCHAR szBuf_path_au[MAX_PATH];
	ZeroMemory(szBuf_path_au, MAX_PATH);
	swprintf(szBuf_path_au, L"%s%s", szBuf, L"\\gstreamer\\1.0\\msvc_x86_64\\bin\\runaudio.bat");

	ofstream ofs(szBuf_path); 
	if (!g_logPath.empty()) {
		cmd_gst1 += "  1>";
		cmd_gst1 += g_logPath;
	}
	ofs.write(cmd_gst1.c_str(), strlen(cmd_gst1.c_str()));
	ofs.close();

	//ShellExecute(NULL, L"open ", szBuf_path, L" ", szBuf_cmdpath, SW_SHOW);
	bool res_au = false;
	if (vol == 0) 
	{
		//system("taskkill  /F /im gst-launch-1.0.exe");
		/*if (!g_logPath.empty()) {
			cmd_gst2 += "  1>";
			cmd_gst2 += g_logPath;
		}*/
		ofstream ofs(szBuf_path_au);
		ofs.write(cmd_gst2.c_str(), strlen(cmd_gst2.c_str()));
		ofs.close();

		SHELLEXECUTEINFO ShExecInfo_au;
		ShExecInfo_au.cbSize = sizeof(SHELLEXECUTEINFO);
		ShExecInfo_au.fMask = SEE_MASK_NOCLOSEPROCESS;
		ShExecInfo_au.hwnd = NULL;
		ShExecInfo_au.lpVerb = NULL;
		ShExecInfo_au.lpFile = szBuf_path_au;
		ShExecInfo_au.lpParameters = NULL;
		ShExecInfo_au.lpDirectory = szBuf_cmdpath;
		ShExecInfo_au.nShow = SW_HIDE;
		ShExecInfo_au.hInstApp = NULL;
		logStr = string("szBuf_path_au:") + TCHAR2STRING(szBuf_path_au);
		logStr += "\n";
		logStr += "szBuf_cmdpath";
		logStr += TCHAR2STRING(szBuf_cmdpath);
		LogEx(g_UI_INFO, "ShellExecuteEx script:", logStr);
		//system("taskkill  /F /im gst-launch-1.0.exe");
		//KillGstLaunchExe();
		/////
		res_au = ShellExecuteEx(&ShExecInfo_au);
		//res_au = true;
		LogEx(g_UI_INFO, "ShellExecuteEx result:", res_au ? "success!" : "failure!");
	}
	else {
		SHELLEXECUTEINFO ShExecInfo;
		ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		ShExecInfo.hwnd = NULL;
		ShExecInfo.lpVerb = NULL;
		ShExecInfo.lpFile = szBuf_path;
		ShExecInfo.lpParameters = NULL;
		ShExecInfo.lpDirectory = szBuf_cmdpath;
		ShExecInfo.nShow = SW_HIDE;
		ShExecInfo.hInstApp = NULL;
		outputLogFlag = true;
		LogEx(g_UI_INFO, "KillGstLaunchExe result:", TCHAR2STRING(szBuf_path));
	    ShellExecuteEx(&ShExecInfo);
	}
	Sleep(3000);
	int rerun = GetProcessidFromName(L"gst-launch-1.0.exe");
	if(res_au)
	{
		logStr = "push success!";
		//logStr += to_string(rerun);
		LogEx(g_UI_INFO, "push stream status:", logStr);
		return true;
	}
	logStr = "push failure!";
	//logStr += to_string(rerun);
	LogEx(g_UI_INFO, "push stream status:", logStr);
	return false;
}			  

bool closepush(char * port, char * url)
{
	string SERVER_URL(url);
	string ROOM_ID(port);
	string BROADCASTER_ID(port);
	string logStr = "kill the gst-launch-1.0.exe";
	LogEx(g_UI_INFO , logStr);
	CloseOutputLog();
	//int rerun = GetProcessidFromName(L"gst-launch-1.0.exe");
	//if (rerun != 0)
	//{
		//system("taskkill  /F /im gst-launch-1.0.exe");
	/////
	//TCHAR szBuf[MAX_PATH];
	//ZeroMemory(szBuf, MAX_PATH);
	//GetModuleFileName(NULL, szBuf, MAX_PATH);
	//PathRemoveFileSpec(szBuf);

	//TCHAR szBuf_path[MAX_PATH];
	//ZeroMemory(szBuf_path, MAX_PATH);
	//swprintf(szBuf_path, L"%s%s", szBuf, L"\\gstreamer\\1.0\\msvc_x86_64\\bin\\killStreamer.bat");

	//TCHAR szBuf_cmdpath[MAX_PATH];
	//ZeroMemory(szBuf_cmdpath, MAX_PATH);
	//swprintf(szBuf_cmdpath, L"%s%s", szBuf, L"\\gstreamer\\1.0\\msvc_x86_64\\bin\\killStreamer.bat");
	//PathRemoveFileSpec(szBuf_cmdpath);
	//////////
	//SHELLEXECUTEINFO ShExecInfo;
	//ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	//ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	//ShExecInfo.hwnd = NULL;
	////ShExecInfo.lpVerb = NULL;
	//ShExecInfo.lpVerb = TEXT("runas");
	//ShExecInfo.lpFile = szBuf_path;
	//ShExecInfo.lpParameters = NULL;
	//ShExecInfo.lpDirectory = szBuf_cmdpath;
	//ShExecInfo.nShow = SW_HIDE;
	//ShExecInfo.hInstApp = NULL;
	bool res = KillGstLaunchExe();// ShellExecuteEx(&ShExecInfo);
	//ShellExecute(NULL, L"open ", szBuf_path, L" ", szBuf_cmdpath, SW_SHOW);
	//}
	BROADCASTER_ID = ROOM_ID;
	SERVER_URL = "https://" + SERVER_URL + ":4443/";
	//logStr = SERVER_URL + "rooms/" + ROOM_ID + "/broadcasters/delete/" + BROADCASTER_ID;
	//LogEx(g_UI_INFO , "HttpGet" , logStr);
	string re_Broadcaster_del = libGetrtpcmd::HttpGet(SERVER_URL + "rooms/" + ROOM_ID + "/broadcasters/delete/" + BROADCASTER_ID);
	//LogEx(g_UI_INFO, "HttpGet recieve", re_Broadcaster_del);
	return true;
}

bool KeyMouseInit(void)
{
	BOOL szbuffer_key =  false;
	BOOL szbuffer_mouse = false;
	BOOL szbuffer_joystick = false;
	string logStr = "";

	if (handle_key == NULL) 
	{
		szbuffer_key = libGetrtpcmd::DeviceOpen(handle_key, 0xF00F, 0x00000003);
	}
	if (handle_mouse == NULL)
	{
		szbuffer_mouse = libGetrtpcmd::DeviceOpen(handle_mouse, 0xF00F, 0x00000002);
	}
	if (handle_joystick == NULL)
	{
		szbuffer_joystick = libGetrtpcmd::DeviceOpen(handle_joystick, 0xF00F, 0x00000001);
	}

	if (szbuffer_key&&szbuffer_mouse&& szbuffer_joystick)
	{
		logStr = "KeyCodeInit sucess!";
		LogEx(g_UI_INFO , logStr);
		return libGetrtpcmd::KeyCodeInit();
	}
	
	return false;
}

bool KeyMouseClose(void)
{
	libGetrtpcmd::DeviceClose(handle_key);
	libGetrtpcmd::DeviceClose(handle_mouse);
	libGetrtpcmd::DeviceClose(handle_joystick);
	handle_key = NULL;
	handle_mouse = NULL;
	handle_joystick = NULL;
	return true;
}

bool KeyDown(int key)
{
	if (handle_key == INVALID_HANDLE_VALUE)
	{
		LogEx(g_UI_ERROR, "KeyDown", " handle_key == INVALID_HANDLE_VALUE");
		return false;
	}
	//Sleep(50);
	return libGetrtpcmd::KeyDown(handle_key, key);
}

bool KeyUp(int key)
{
	if (handle_key == INVALID_HANDLE_VALUE)
	{
		LogEx(g_UI_ERROR, "KeyUp", " handle_key == INVALID_HANDLE_VALUE");
		return false;
	}
	return libGetrtpcmd::KeyUp(handle_key, key);
}

bool KeyPing()
{
	if (handle_key == INVALID_HANDLE_VALUE)
	{
		LogEx(g_UI_ERROR, "KeyPing", " handle_key == INVALID_HANDLE_VALUE");
		return false;
	}
	return 	libGetrtpcmd::KeyPing(handle_key);;
}

bool MouseMove(int x, int y, int code)
{
	if (handle_mouse == INVALID_HANDLE_VALUE)
	{
		LogEx(g_UI_ERROR, "MouseMove", " handle_mouse == INVALID_HANDLE_VALUE");
		return false;
	}
	string logStr = string("x:") + to_string(x);
	logStr += string("y:");
	logStr += to_string(y);
	logStr += string("code:");
	logStr = to_string(code);
    //LogEx(g_UI_ERROR, "MouseUpInner", logStr);
	return libGetrtpcmd::MouseMove(handle_mouse, x, y, code);;
}

bool MouseUp(int x, int y, int code)
{
	if (handle_mouse == INVALID_HANDLE_VALUE)
	{
		LogEx(g_UI_ERROR, "MouseUp", " handle_mouse == INVALID_HANDLE_VALUE");
		return false;
	}
	string logStr = string("x:") + to_string(x);
	logStr += string("y:");
	logStr += to_string(y);
	logStr += string("code:");
	logStr = to_string(code);
    //LogEx(g_UI_ERROR, "MouseUpInner", logStr);
	//Sleep(50);
	return libGetrtpcmd::MouseUp(handle_mouse, x, y, code);;
}

bool MouseDown(int x, int y, int code)
{
	if (handle_mouse == INVALID_HANDLE_VALUE)
	{
		LogEx(g_UI_ERROR, "MouseDown", " handle_mouse == INVALID_HANDLE_VALUE");
		return false;
	}
	string logStr = string("x:") + to_string(x);
	logStr += string("y:");
	logStr += to_string(y);
	logStr += string("code:");
	logStr = to_string(code);
        //LogEx(g_UI_ERROR, "MouseDown", "");
	//Sleep(50);
	return libGetrtpcmd::MouseDown(handle_mouse, x, y, code);;
}

bool JoystickCtrl(int X, int Y, int Z, int rX, int rY, int rZ, int slider, int dial, int wheel, BYTE hat, BYTE buttons[16])
{
	if (handle_joystick == INVALID_HANDLE_VALUE)
	{
		LogEx(g_UI_ERROR, "JoystickCtrl", " handle_joystick == INVALID_HANDLE_VALUE");
		return false;
	}
	//Sleep(50);
	return libGetrtpcmd::JoystickCtrl(handle_joystick, X, Y, Z, rX, rY, rZ, slider, dial, wheel, hat, buttons);;
}

bool KeyMouseIsValid(void) {
	return (handle_key && handle_mouse) ? true : false;
}

bool StartGame(char * gamepath, char * command, int showMode)
{
	if (gamepath && 0 != strlen(gamepath)) {
		SHELLEXECUTEINFO ShExecInfo_au;
		ShExecInfo_au.cbSize = sizeof(SHELLEXECUTEINFO);
		ShExecInfo_au.fMask = SEE_MASK_NOCLOSEPROCESS;
		ShExecInfo_au.hwnd = NULL;
		ShExecInfo_au.lpVerb =  NULL;
		ShExecInfo_au.lpFile = stringToLPCWSTR(gamepath);
		if (command != NULL && 0 != strlen(command))
		{
			ShExecInfo_au.lpParameters = stringToLPCWSTR(command);
		}
		else {
			ShExecInfo_au.lpParameters = NULL;
		}
		ShExecInfo_au.lpDirectory = NULL;
		ShExecInfo_au.nShow = showMode;
		ShExecInfo_au.hInstApp = NULL;
		return ShellExecuteEx(&ShExecInfo_au);
	}
	return false;
}

bool wfile(char * url, char * filepath)
{
	string logStr = "url";
	logStr += url;
	logStr = "filepath";
	logStr += filepath;
        //LogEx(g_UI_ERROR, "StartGame", logStr);
	return libGetrtpcmd::wfile(url, filepath);
}
/// CloudGame运行日志输出接口
void SetRunLogCallback(LogCallback callback) {
	g_functionCallback = callback;
}

void SetGStreamerLogPath(char *  logFilePath) {
	if (logFilePath) {
		g_logPath = logFilePath;
	}
}

// 这是已导出类的构造函数。
CCloudGame::CCloudGame()
{
    return;
}
