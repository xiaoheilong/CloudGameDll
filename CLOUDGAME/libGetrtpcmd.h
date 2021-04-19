#pragma once
#include <string>  
#include <iostream> 
 class libGetrtpcmd
{
	public:
		static BYTE KeyCodeGet(int jskeycode);
		static std::string HttpGet(std::string url);
		static std::string HttpPostRaw(std::string url, std::string data);
		static bool KeyDown(HANDLE handle, int key);
		static bool KeyUp(HANDLE handle, int key);
		static bool KeyPing(HANDLE handle);
		static bool MouseMove(HANDLE handle, int x,int y,int code);
		static bool MouseUp(HANDLE handle, int x, int y, int code);
		static bool MouseDown(HANDLE handle, int x, int y, int code);
		static bool JoystickCtrl(HANDLE handle, int X, int Y, int Z, int rX, int rY, int rZ, int slider, int dial, int wheel, BYTE hat, BYTE buttons[16]);
		static bool KeyCodeInit();
		static BOOL DeviceOpen(HANDLE &handle, WORD wVID, WORD wPID);
		static void DeviceClose(HANDLE & handle);
		static bool wfile(std::string url, std::string filepath);
};

