#include "pch.h"
#include "libGetrtpcmd.h"
#include <string>  
#include <iostream> 
#include "curl/curl.h"
#include <assert.h> 
#include <stdio.h>
#include <stdlib.h>

#include <io.h>

#include <list>
#include <map>

#include <setupapi.h>
#include <hidsdi.h>
#include <fstream> 
using namespace std;

#pragma comment(lib, "hid.lib")
#pragma comment(lib,"setupapi") 

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

list<std::string>  intlist;
int* keycodelist;
int Modkeycodelist;
map<int, BYTE> KeyCode;
map<int, BYTE> ModifiersKeyCodes;
#define  SKIP_PEER_VERIFICATION 1  
//#define  SKIP_HOSTNAME_VERFICATION 1  
#pragma pack(push)
#pragma pack(1)
typedef struct _HIDMINI_CONTROL_INFO {
	UCHAR ReportId;
	UCHAR CommandCode;
	UINT32 timeout; //you must ping the driver every (timeout / 5) seconds or the driver will reset itself and release all keypresses
	BYTE modifier = 0;
	BYTE padding = 0;
	BYTE key0 = 0;
	BYTE key1 = 0;
	BYTE key2 = 0;
	BYTE key3 = 0;
	BYTE key4 = 0;
	BYTE key5 = 0;
} HIDMINI_CONTROL_INFO, *PHIDMINI_CONTROL_INFO;

#pragma pack(1)
typedef struct _HIDMINI_CONTROL_INFO_Mouse {
	BYTE ReportId;
	BYTE CommandCode;
	BYTE buttons;
	UINT16 X;
	UINT16 Y;
} HIDMINI_CONTROL_INFO_Mouse, *PHIDMINI_CONTROL_INFO_Mouse;

#pragma pack(1)
typedef struct _HIDMINI_CONTROL_INFO_Joystick {
	UCHAR ReportId;
	UCHAR ControlCode;
	unsigned short X;
	unsigned short Y;
	unsigned short Z;
	unsigned short rX;
	unsigned short rY;
	unsigned short rZ;
	unsigned short slider;
	unsigned short dial;
	unsigned short wheel;
	BYTE hat; //array of 8 bits, one per hat position
	BYTE buttons[16]; //array of 128 bits, one per button
} HIDMINI_CONTROL_INFO_Joystick, *PHIDMINI_CONTROL_INFO_Joystick;
#pragma pack(pop)

struct FtpFile {
	const char *filename;
	FILE *stream;
};

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct FtpFile *out = (struct FtpFile *)stream;
	if (out && !out->stream) {
		out->stream = fopen(out->filename, "wb");//���ļ�����д��
		if (!out->stream)
			return -1;
	}
	return fwrite(buffer, size, nmemb, out->stream);
}

/*
ptr��ָ��洢���ݵ�ָ�룬
size��ÿ����Ĵ�С��
nmemb��ָ�����Ŀ��
stream���û�������
���Ը���������Щ��������Ϣ����֪����ptr�е����ݵ��ܳ�����size*nmemb
*/
size_t call_wirte_func(const char *ptr, size_t size, size_t nmemb, std::string *stream)
{
	assert(stream != NULL);
	size_t len = size * nmemb;
	stream->append(ptr, len);
	return len;
}
int write_func(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	string* buffer = (string*)userdata;
	size_t len = size*nmemb;
	// 	int offset = strlen(buffer);
	// 	memcpy(buffer + offset, ptr, len);
	// 	buffer[len + offset] = 0;
	buffer->append(ptr, len);
	return len;
}
// ����http header�ص�����    
size_t header_callback(const char  *ptr, size_t size, size_t nmemb, std::string *stream)
{
	assert(stream != NULL);
	size_t len = size * nmemb;
	stream->append(ptr, len);
	return len;
}

BYTE libGetrtpcmd::KeyCodeGet(int jskeycode)
{
	auto it = KeyCode.find(jskeycode);
	if (it != KeyCode.end()) {
		return KeyCode[jskeycode];
	}
	
	auto itModifiers = ModifiersKeyCodes.find(jskeycode);
	if (itModifiers != ModifiersKeyCodes.end()) {
		return ModifiersKeyCodes[jskeycode];
	}
	return 0;
}

std::string libGetrtpcmd::HttpGet(std::string url)
{
	CURL *curl;
	CURLcode code;
	std::string szbuffer;
	std::string szheader_buffer;
	char errorBuffer[CURL_ERROR_SIZE];
	//std::string url = "http://www.douban.com";
	//std::string url = "https://vip.icbc.com.cn/icbc/perbank/index.jsp";  
	std::string useragent = "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0.1";
	/*
	CURL_GLOBAL_ALL                //��ʼ�����еĿ��ܵĵ��á�
	CURL_GLOBAL_SSL               //��ʼ��֧�� ��ȫ�׽��ֲ㡣
	CURL_GLOBAL_WIN32            //��ʼ��win32�׽��ֿ⡣
	CURL_GLOBAL_NOTHING         //û�ж���ĳ�ʼ����
	*/
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl) {
		// Զ��URL��֧�� http, https, ftp  
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent.c_str());
		// �ٷ����ص�DLL����֧��GZIP��Accept-Encoding:deflate, gzip  
		//curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip, deflate");
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);//������Ϣ��  
		//https ����ר�ã�start  
#ifdef SKIP_PEER_VERIFICATION  
		//����������SSL��֤����ʹ��CA֤��  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, FALSE);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		//���������SSL��֤�����ָ��һ��CA֤��Ŀ¼  
		//curl_easy_setopt(curl, CURLOPT_CAPATH, "this is ca ceat");  
		//curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT);
		//curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
#endif  

#ifdef SKIP_HOSTNAME_VERFICATION  
		//��֤�������˷��͵�֤�飬Ĭ���� 2(��)��1���У���0�����ã�  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif  
		//https ����ר�ã�end  

		//����cookieֵ��������  
		//curl_easy_setopt(curl, CURLOPT_COOKIE, "name1=var1; name2=var2;");   
		/* �������ͨ�Ž���cookie��Ĭ�����ڴ��У������ǲ����ڴ����е��ļ������� */
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "./cookie.txt");
		/* ����CURL�����������cookie�������ͷ��ڴ��д������ļ� */
		curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "./cookie.txt");

		/* POST ���� */
		// curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");  
		//�����ض����������  
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);
		//����301��302��ת����location  
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		//ץȡ���ݺ󣬻ص�����  
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &szbuffer);
		//ץȡͷ��Ϣ���ص�����  
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&szheader_buffer);

		/*curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);*/
		/*
		CURLE_OK    �������һ�ж���
		CURLE_UNSUPPORTED_PROTOCOL  ��֧�ֵ�Э�飬��URL��ͷ��ָ��
		CURLE_COULDNT_CONNECT   �������ӵ�remote �������ߴ���
		CURLE_REMOTE_ACCESS_DENIED  ���ʱ��ܾ�
		CURLE_HTTP_RETURNED_ERROR   Http���ش���
		CURLE_READ_ERROR    �������ļ�����
		CURLE_SSL_CACERT    ����HTTPSʱ��ҪCA֤��·��
		*/
		code = curl_easy_perform(curl);
		if (CURLE_OK == code) {
			double val;

			/* check for bytes downloaded */
			code = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &val);
			if ((CURLE_OK == code) && (val > 0))
				printf("Data downloaded: %0.0f bytes.\n", val);

			/* check for total download time */
			code = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &val);
			if ((CURLE_OK == code) && (val > 0))
				printf("Total download time: %0.3f sec.\n", val);

			/* check for average download speed */
			code = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &val);
			if ((CURLE_OK == code) && (val > 0))
				printf("Average download speed: %0.3f kbyte/sec.\n", val / 1024);
			
			printf("%s\n", szbuffer.c_str());
		}
		else {
			//fprintf(stderr, "Failed to get '%s' [%s]\n", url, errorBuffer);
			// exit(EXIT_FAILURE);  
		}

		/* �ͷ��ڴ� */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();

	//getchar();
	return szbuffer;
}

std::string libGetrtpcmd::HttpPostRaw(std::string url, std::string data)
{
	CURL *curl;
	CURLcode code;
	std::string szbuffer;
	std::string szheader_buffer;
	char errorBuffer[CURL_ERROR_SIZE];
	//std::string url = "http://www.douban.com";
	//std::string url = "https://vip.icbc.com.cn/icbc/perbank/index.jsp";  
	std::string useragent = "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0.1";
	/*
	CURL_GLOBAL_ALL                //��ʼ�����еĿ��ܵĵ��á�
	CURL_GLOBAL_SSL               //��ʼ��֧�� ��ȫ�׽��ֲ㡣
	CURL_GLOBAL_WIN32            //��ʼ��win32�׽��ֿ⡣
	CURL_GLOBAL_NOTHING         //û�ж���ĳ�ʼ����
	*/
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl) {
		// Զ��URL��֧�� http, https, ftp  
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent.c_str());
		// �ٷ����ص�DLL����֧��GZIP��Accept-Encoding:deflate, gzip  
		
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);//������Ϣ��  
		//https ����ר�ã�start  
#ifdef SKIP_PEER_VERIFICATION  
		//����������SSL��֤����ʹ��CA֤��  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, FALSE);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		//���������SSL��֤�����ָ��һ��CA֤��Ŀ¼  
		//curl_easy_setopt(curl, CURLOPT_CAPATH, "this is ca ceat");  
		//curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT);
		//curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
#endif  

#ifdef SKIP_HOSTNAME_VERFICATION  
		//��֤�������˷��͵�֤�飬Ĭ���� 2(��)��1���У���0�����ã�  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif  
		//https ����ר�ã�end  

		//����cookieֵ��������  
		//curl_easy_setopt(curl, CURLOPT_COOKIE, "name1=var1; name2=var2;");   
		/* �������ͨ�Ž���cookie��Ĭ�����ڴ��У������ǲ����ڴ����е��ļ������� */
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "./cookie.txt");
		/* ����CURL�����������cookie�������ͷ��ڴ��д������ļ� */
		curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "./cookie.txt");

		/* POST ���� */
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS,data.c_str());  
		//�����ض����������  
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);
		//����301��302��ת����location  
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		//ץȡ���ݺ󣬻ص�����  
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &szbuffer);
		//ץȡͷ��Ϣ���ص�����  
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&szheader_buffer);

		/*curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);*/
		/*
		CURLE_OK    �������һ�ж���
		CURLE_UNSUPPORTED_PROTOCOL  ��֧�ֵ�Э�飬��URL��ͷ��ָ��
		CURLE_COULDNT_CONNECT   �������ӵ�remote �������ߴ���
		CURLE_REMOTE_ACCESS_DENIED  ���ʱ��ܾ�
		CURLE_HTTP_RETURNED_ERROR   Http���ش���
		CURLE_READ_ERROR    �������ļ�����
		CURLE_SSL_CACERT    ����HTTPSʱ��ҪCA֤��·��
		*/
		code = curl_easy_perform(curl);
		if (CURLE_OK == code) {
			double val;

			/* check for bytes downloaded */
			code = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &val);
			if ((CURLE_OK == code) && (val > 0))
				printf("Data downloaded: %0.0f bytes.\n", val);

			/* check for total download time */
			code = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &val);
			if ((CURLE_OK == code) && (val > 0))
				printf("Total download time: %0.3f sec.\n", val);

			/* check for average download speed */
			code = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &val);
			if ((CURLE_OK == code) && (val > 0))
				printf("Average download speed: %0.3f kbyte/sec.\n", val / 1024);

			printf("%s\n", szbuffer.c_str());
		}
		else {
			//fprintf(stderr, "Failed to get '%s' [%s]\n", url, errorBuffer);
			// exit(EXIT_FAILURE);  
		}

		/* �ͷ��ڴ� */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return szbuffer;
}

bool libGetrtpcmd::KeyDown(HANDLE handle,int jskeycode)
{
	HIDMINI_CONTROL_INFO keyinfo;
	keyinfo.CommandCode = 2;
	keyinfo.ReportId = 1;
	keyinfo.timeout = 1000;
	keyinfo.padding = 0;
	
	auto it = KeyCode.find(jskeycode);
	if (it != KeyCode.end()) {
		bool blnhs = false;
		for (int i = 0; i < 5; i++)
		{
			if (keycodelist[i] == jskeycode)
			{
				blnhs = true;
			}
		}
		if (!blnhs) 
		{
			for (int i = 0; i < 5; i++)
			{
				if (keycodelist[i] == 0)
				{
					keycodelist[i] = jskeycode;
					break;
				}
			}
		}
		//keyinfo.key0 = KeyCode[jskeycode];
		keyinfo.key0 = KeyCode[keycodelist[0]];
		keyinfo.key1 = KeyCode[keycodelist[1]];
		keyinfo.key2 = KeyCode[keycodelist[2]];
		keyinfo.key3 = KeyCode[keycodelist[3]];
		keyinfo.key4 = KeyCode[keycodelist[4]];
	}
	auto itModifiers = ModifiersKeyCodes.find(jskeycode);
	if (itModifiers != ModifiersKeyCodes.end()) {
		Modkeycodelist = jskeycode;
	}
	keyinfo.modifier = ModifiersKeyCodes[Modkeycodelist];
	return HidD_SetFeature(handle, &keyinfo, sizeof(keyinfo)+1);
}
bool libGetrtpcmd::KeyUp(HANDLE handle, int jskeycode)
{
	HIDMINI_CONTROL_INFO keyinfo;
	keyinfo.CommandCode = 2;
	keyinfo.ReportId = 1;
	keyinfo.timeout = 1000;
	keyinfo.padding = 0;
	for (int i = 0; i < 5; i++) 
	{
		if (keycodelist[i] == jskeycode) 
		{
			keycodelist[i] = 0;
		}
	}
	if (Modkeycodelist == jskeycode) 
	{
		Modkeycodelist = 0;
	}
	keyinfo.modifier = ModifiersKeyCodes[Modkeycodelist];
	keyinfo.key0 = KeyCode[keycodelist[0]];
	keyinfo.key1 = KeyCode[keycodelist[1]];
	keyinfo.key2 = KeyCode[keycodelist[2]];
	keyinfo.key3 = KeyCode[keycodelist[3]];
	keyinfo.key4 = KeyCode[keycodelist[4]];
	return HidD_SetFeature(handle, &keyinfo, sizeof(keyinfo) + 1);
}
bool libGetrtpcmd::KeyPing(HANDLE handle)
{
	HIDMINI_CONTROL_INFO keyinfo;
	keyinfo.CommandCode = 3;
	keyinfo.ReportId = 1;
	keyinfo.timeout = 1000;
	keyinfo.padding = 0;
	return HidD_SetFeature(handle, &keyinfo, sizeof(keyinfo) + 1);
}
bool libGetrtpcmd::MouseMove(HANDLE handle, int x, int y, int code)
{
	HIDMINI_CONTROL_INFO_Mouse mouseinfo;
	mouseinfo.buttons = 0x00000000;
	if (code == 1)
	{
		mouseinfo.buttons = 0x00000001;
	}
	if (code == 2)
	{
		mouseinfo.buttons = 4;
	}
	if (code == 3)
	{
		mouseinfo.buttons = 2;
	}
	mouseinfo.X = x;
	mouseinfo.Y = y;
	mouseinfo.ReportId = 1;
	mouseinfo.CommandCode = 2;
	return HidD_SetFeature(handle, &mouseinfo, sizeof(mouseinfo) + 1);
}
bool libGetrtpcmd::MouseUp(HANDLE handle, int x, int y, int code)
{
	HIDMINI_CONTROL_INFO_Mouse mouseinfo;
	mouseinfo.buttons = 0x00000000;
	if (code == 1)
	{
		mouseinfo.buttons = 0x00000001;
	}
	if (code == 2)
	{
		mouseinfo.buttons = 4;
	}
	if (code == 3)
	{
		mouseinfo.buttons = 2;
	}
	mouseinfo.X = x;
	mouseinfo.Y = y;
	mouseinfo.ReportId = 1;
	mouseinfo.CommandCode = 2;
	return HidD_SetFeature(handle, &mouseinfo, sizeof(mouseinfo) + 1);
}
bool libGetrtpcmd::MouseDown(HANDLE handle, int x, int y, int code)
{
	HIDMINI_CONTROL_INFO_Mouse mouseinfo;
	mouseinfo.buttons = 0x00000000;
	if (code == 1)
	{
		mouseinfo.buttons = 0x00000001;
	}
	if (code == 2)
	{
		mouseinfo.buttons = 4;
	}
	if (code == 3)
	{
		mouseinfo.buttons = 2;
	}
	mouseinfo.X = x;
	mouseinfo.Y = y;
	mouseinfo.ReportId = 1;
	mouseinfo.CommandCode = 2;
	return HidD_SetFeature(handle, &mouseinfo, sizeof(mouseinfo)+1);
}
bool libGetrtpcmd::JoystickCtrl(HANDLE handle, int X, int Y, int Z, int rX, int rY, int rZ, int slider, int dial, int wheel, BYTE hat, BYTE buttons[16])
{
	char logContent[MAX_PATH];
	memset(logContent , 0 , sizeof(logContent));
	//sprintf(logContent, "x: %d y :%d x:%d rx:%d  ry:%d  rz:%d slider:%d dial:%d  wheel : %d  hat:%s buttons:%s", X, Y, Z, rX, rY, rZ, slider, dial, wheel, (char*)hat, (char*)buttons);
	/*std::string logStr = "enter the JoystickCtrl!";
	std::ofstream  ofs("d://testPrint.txt" , ios::out | ios::ate);
	ofs.write(logContent, strlen(logContent));*/
	
	HIDMINI_CONTROL_INFO_Joystick joystickinfo;
	joystickinfo.X =X;
	joystickinfo.Y = Y;
	joystickinfo.Z = Z;
	joystickinfo.rX =rX;
	joystickinfo.rY =rY;
	joystickinfo.rZ =rZ;
	joystickinfo.slider = slider;
	joystickinfo.dial = dial;
	joystickinfo.wheel = wheel;
	joystickinfo.hat = hat;
	for (int i = 0; i < 16; i++) 
	{
		if (buttons[i] != NULL) 
		{
			joystickinfo.buttons[i] = buttons[i];
		}
		else {
			joystickinfo.buttons[i] = 0x00000000;
		}
		
	}
	joystickinfo.ControlCode = 2;
	joystickinfo.ReportId = 1;
	return HidD_SetFeature(handle, &joystickinfo, sizeof(joystickinfo));
	/*logStr = "stepover the JoystickCtrl!";
	ofs.write(logStr.c_str(), strlen(logStr.c_str()));
	ofs.close();*/
}
bool libGetrtpcmd::KeyCodeInit()
{
	keycodelist = new int[5];
	for (int i = 0; i < 5; i++)
	{
		keycodelist[i] = 0;
	}
	KeyCode.insert(pair<int, BYTE>(0, 0));
	KeyCode.insert(pair<int, BYTE>(48, 39));
	KeyCode.insert(pair<int, BYTE>(49, 30));
	KeyCode.insert(pair<int, BYTE>(50, 31));
	KeyCode.insert(pair<int, BYTE>(51, 32));
	KeyCode.insert(pair<int, BYTE>(52, 33));
	KeyCode.insert(pair<int, BYTE>(53, 34));
	KeyCode.insert(pair<int, BYTE>(54, 35));
	KeyCode.insert(pair<int, BYTE>(55, 36));
	KeyCode.insert(pair<int, BYTE>(56, 37));
	KeyCode.insert(pair<int, BYTE>(57, 38));
	KeyCode.insert(pair<int, BYTE>(65, 4));
	KeyCode.insert(pair<int, BYTE>(66, 5));
	KeyCode.insert(pair<int, BYTE>(67, 6));
	KeyCode.insert(pair<int, BYTE>(68, 7));
	KeyCode.insert(pair<int, BYTE>(69, 8));
	KeyCode.insert(pair<int, BYTE>(70, 9));
	KeyCode.insert(pair<int, BYTE>(71, 10));
	KeyCode.insert(pair<int, BYTE>(72, 11));
	KeyCode.insert(pair<int, BYTE>(73, 12));
	KeyCode.insert(pair<int, BYTE>(74, 13));
	KeyCode.insert(pair<int, BYTE>(75, 14));
	KeyCode.insert(pair<int, BYTE>(76, 15));
	KeyCode.insert(pair<int, BYTE>(77, 16));
	KeyCode.insert(pair<int, BYTE>(78, 17));
	KeyCode.insert(pair<int, BYTE>(79, 18));
	KeyCode.insert(pair<int, BYTE>(80, 19));
	KeyCode.insert(pair<int, BYTE>(81, 20));
	KeyCode.insert(pair<int, BYTE>(82, 21));
	KeyCode.insert(pair<int, BYTE>(83, 22));
	KeyCode.insert(pair<int, BYTE>(84, 23));
	KeyCode.insert(pair<int, BYTE>(85, 24));
	KeyCode.insert(pair<int, BYTE>(86, 25));
	KeyCode.insert(pair<int, BYTE>(87, 26));
	KeyCode.insert(pair<int, BYTE>(88, 27));
	KeyCode.insert(pair<int, BYTE>(89, 28));
	KeyCode.insert(pair<int, BYTE>(90, 29));
	KeyCode.insert(pair<int, BYTE>(96, 98));
	KeyCode.insert(pair<int, BYTE>(97, 89));
	KeyCode.insert(pair<int, BYTE>(98, 90));
	KeyCode.insert(pair<int, BYTE>(99, 91));
	KeyCode.insert(pair<int, BYTE>(100, 92));
	KeyCode.insert(pair<int, BYTE>(101, 93));
	KeyCode.insert(pair<int, BYTE>(102, 94));
	KeyCode.insert(pair<int, BYTE>(103, 95));
	KeyCode.insert(pair<int, BYTE>(104, 96));
	KeyCode.insert(pair<int, BYTE>(105, 97));
	KeyCode.insert(pair<int, BYTE>(106, 85));
	KeyCode.insert(pair<int, BYTE>(107, 87));
	KeyCode.insert(pair<int, BYTE>(108, 88));
	KeyCode.insert(pair<int, BYTE>(109, 86));
	KeyCode.insert(pair<int, BYTE>(110, 99));
	KeyCode.insert(pair<int, BYTE>(111, 84));
	KeyCode.insert(pair<int, BYTE>(112, 58));
	KeyCode.insert(pair<int, BYTE>(113, 59));
	KeyCode.insert(pair<int, BYTE>(114, 60));
	KeyCode.insert(pair<int, BYTE>(115, 61));
	KeyCode.insert(pair<int, BYTE>(116, 62));
	KeyCode.insert(pair<int, BYTE>(117, 63));
	KeyCode.insert(pair<int, BYTE>(118, 64));
	KeyCode.insert(pair<int, BYTE>(119, 65));
	KeyCode.insert(pair<int, BYTE>(120, 66));
	KeyCode.insert(pair<int, BYTE>(121, 67));
	KeyCode.insert(pair<int, BYTE>(122, 68));
	KeyCode.insert(pair<int, BYTE>(123, 69));
	KeyCode.insert(pair<int, BYTE>(8, 42));
	KeyCode.insert(pair<int, BYTE>(9, 43));
	KeyCode.insert(pair<int, BYTE>(13, 40));
	KeyCode.insert(pair<int, BYTE>(20, 57));
	KeyCode.insert(pair<int, BYTE>(27, 41));
	KeyCode.insert(pair<int, BYTE>(32, 44));
	KeyCode.insert(pair<int, BYTE>(33, 75));
	KeyCode.insert(pair<int, BYTE>(34, 78));
	KeyCode.insert(pair<int, BYTE>(35, 77));
	KeyCode.insert(pair<int, BYTE>(36, 74));
	KeyCode.insert(pair<int, BYTE>(37, 80));
	KeyCode.insert(pair<int, BYTE>(38, 82));
	KeyCode.insert(pair<int, BYTE>(39, 79));
	KeyCode.insert(pair<int, BYTE>(40, 81));
	KeyCode.insert(pair<int, BYTE>(45, 73));
	KeyCode.insert(pair<int, BYTE>(46, 76));
	KeyCode.insert(pair<int, BYTE>(144, 83));
	KeyCode.insert(pair<int, BYTE>(186, 51));
	KeyCode.insert(pair<int, BYTE>(187, 46));
	KeyCode.insert(pair<int, BYTE>(188, 54));
	KeyCode.insert(pair<int, BYTE>(189, 45));
	KeyCode.insert(pair<int, BYTE>(190, 55));
	KeyCode.insert(pair<int, BYTE>(191, 56));
	KeyCode.insert(pair<int, BYTE>(192, 53));
	KeyCode.insert(pair<int, BYTE>(219, 47));
	KeyCode.insert(pair<int, BYTE>(220, 49));
	KeyCode.insert(pair<int, BYTE>(221, 48));
	KeyCode.insert(pair<int, BYTE>(222, 50));
	ModifiersKeyCodes.insert(pair<int, BYTE>(0, 0));
	ModifiersKeyCodes.insert(pair<int, BYTE>(17, 1));
	ModifiersKeyCodes.insert(pair<int, BYTE>(16, 2));
	ModifiersKeyCodes.insert(pair<int, BYTE>(18, 4));
	ModifiersKeyCodes.insert(pair<int, BYTE>(91, 8));
	ModifiersKeyCodes.insert(pair<int, BYTE>(317, 16));
	ModifiersKeyCodes.insert(pair<int, BYTE>(316, 32));
	ModifiersKeyCodes.insert(pair<int, BYTE>(318, 64));
	ModifiersKeyCodes.insert(pair<int, BYTE>(391, 128));
	return true;
}
BOOL libGetrtpcmd::DeviceOpen(HANDLE &handle, WORD wVID, WORD wPID)
{
	BOOL bRet = FALSE;
	GUID hidGuid;
	HDEVINFO hardwareDeviceInfo;
	SP_INTERFACE_DEVICE_DATA deviceInfoData;
	PSP_INTERFACE_DEVICE_DETAIL_DATA functionClassDeviceData = NULL;
	ULONG predictedLength = 0;
	ULONG requiredLength = 0;
	CloseHandle(handle);
	handle = INVALID_HANDLE_VALUE;
	deviceInfoData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
	HidD_GetHidGuid(&hidGuid);
	hardwareDeviceInfo = SetupDiGetClassDevs(&hidGuid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
	for (int i = 0; i < 1024; i++)
	{
		if (!SetupDiEnumDeviceInterfaces(hardwareDeviceInfo, 0, &hidGuid, i, &deviceInfoData)) continue;
		SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo, &deviceInfoData, NULL, 0, &requiredLength, NULL);
		predictedLength = requiredLength;
		functionClassDeviceData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(predictedLength);
		if (!functionClassDeviceData) continue;
		functionClassDeviceData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
		if (!SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo, &deviceInfoData, functionClassDeviceData, predictedLength, &requiredLength, NULL))break;
		handle = CreateFile(functionClassDeviceData->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, 0, NULL);
		if (handle != INVALID_HANDLE_VALUE)
		{
			HIDD_ATTRIBUTES attri;
			HidD_GetAttributes(handle, &attri);
			if ((attri.VendorID == wVID) &&
				(attri.ProductID == wPID))
			{
				bRet = TRUE;
				break;
			}
			CloseHandle(handle);
			handle = INVALID_HANDLE_VALUE;
		}
	}
	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
	return bRet;
}
void  libGetrtpcmd::DeviceClose(HANDLE &handle)

{
	CloseHandle(handle);
	handle = INVALID_HANDLE_VALUE;
}
bool libGetrtpcmd::wfile(std::string url, std::string filepath)
{
	CURL *curl;
	CURLcode res;
	struct FtpFile ftpfile = {
		filepath.c_str(), //�������ص����ص��ļ�λ�ú�·��
		NULL
	};
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();  //��ʼ��һ��curlָ��
	if (curl) { //curl������ڵ������ִ�еĲ���
		//����Զ�˵�ַ
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
#ifdef SKIP_PEER_VERIFICATION  
		//����������SSL��֤����ʹ��CA֤��  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, FALSE);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		//���������SSL��֤�����ָ��һ��CA֤��Ŀ¼  
		//curl_easy_setopt(curl, CURLOPT_CAPATH, "this is ca ceat");  
		//curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT);
		//curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
#endif  
		//ִ��д���ļ�������
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);//�������ݱ�д�룬�ص����������ã�
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile); //���ýṹ���ָ�봫�ݸ��ص�����
		//����ʱ��㱨���е���Ϣ�������STDERR��ָ����CURLOPT_STDERR��
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_USERPWD, "SUREN:SUREN");
		//д���ļ�
		res = curl_easy_perform(curl);
		//�ͷ�curl����
		curl_easy_cleanup(curl);
		if (res != CURLE_OK)
		{
			return false;
		}
	}
	if (ftpfile.stream)
	{
		//�ر��ļ���
		fclose(ftpfile.stream);
	}
	//�ͷ�ȫ��curl����
	curl_global_cleanup();
	return true;
}