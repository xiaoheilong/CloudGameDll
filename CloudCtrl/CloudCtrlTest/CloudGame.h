// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 CLOUDGAME_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// CLOUDGAME_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef CLOUDGAME_EXPORTS
#define CLOUDGAME_API __declspec(dllexport)
#else
#define CLOUDGAME_API __declspec(dllimport)
#endif
typedef void(*LogCallback)(const char* logType, const char *  logMsg);
// 此类是从 dll 导出的
class CLOUDGAME_API CCloudGame {
public:
	CCloudGame(void);
	// TODO: 在此处添加方法。
};

//推流
//port	唯一标识	Y	String	游戏机惟一标识
//url	推送地址	Y	string	媒体服务器的推流地址 例：https://124.71.159.87:4443/
//hosturl	推送域名	Y	string	媒体服务器的推流地址域名或ip，最好是IP  例：124.71.159.87
//framerate	帧数	N	int	默认为30，设置过大，容易导致推流失败
//bitrate	码率	N	Int	默认为1000。码率越高，清晰度越高，设置过大，容易导致推流失败
//deadline	缓冲数	N	int	默认为1，缓冲越大，编码越清晰，但是增加延迟
//cpuused	线程数	N	int	默认为4，编码使用的进程数，请根据机器性能设置
//x	分辩率x	N	int	默认为0指屏幕宽，自定义分辩率，会增加编码耗时
//y	分辩率y	N	int	默认为0指屏幕高
//mode	编码方式	N	int	默认为0
//							0:h264 硬件编码（需要硬件支持）正常高端nv显卡都支持
//							1 : h264 软件编码
//							2 : vp8 软件编码
//capmode	采集方式	N	int	默认为0
//							0 : dx方式 性能最好
//							1 : dx9方式 部分老游戏需要
//							2 : gdi方式 部分老游戏需要
//vol	是否需要声音	N	int	0需要 1不需要
CLOUDGAME_API  bool push(char * port, char * url, char* hosturl, int framerate, int bitrate, int deadline, int cpuused, int x, int y, int mode, int capmode, int vol);

//关闭推流
//port	唯一标识	Y	String	游戏机惟一标识
//url	推送地址	Y	string	媒体服务器的推流地址 例：https://124.71.159.87:4443/
CLOUDGAME_API  bool closepush(char * port, char * url);

//键盘鼠标初始化
CLOUDGAME_API bool KeyMouseInit(void);

//键盘鼠标关闭
CLOUDGAME_API bool KeyMouseClose(void);

//键盘按下 key内容为js里键盘按键的keycode   注：按键AB同时按下。命令顺序 按下A> 按下B>抬起A>抬起B
CLOUDGAME_API bool KeyDown(int key);
//键盘抬起 key内容为js里键盘按键的keycode
CLOUDGAME_API bool KeyUp(int key);
//键盘Ping 必须在每1秒内对键盘执行ping操作，不然键盘将复位
CLOUDGAME_API bool KeyPing();

//注key格式为  x | y | keycode
//X，y为希望鼠标移动到的坐标值。。。屏幕从左上角到右下角的x, y值为1到32767，与屏幕自身分辩率无关。例：移动到屏幕中间就相当于x:16383 y:16383 code:0
//Keycode指移动鼠标时，鼠标左中右键按下情况。 1 左键 3 右键 2中键 0没有按键按下
//鼠标移动
CLOUDGAME_API bool MouseMove(int x, int y, int code);
//鼠标抬起
CLOUDGAME_API bool MouseUp(int x, int y, int code);
//鼠标按下
CLOUDGAME_API bool MouseDown(int x, int y, int code);

//启动游戏
//gamepath 游戏路径 command 命令参数
CLOUDGAME_API void StartGame(char * gamepath, char * command);

//下载文件
//url 下载路径 filepath 文件路径
CLOUDGAME_API bool wfile(char * url, char * filepath);

/// CloudGame运行日志输出接口  callback不能被阻塞 ,取出值后日志就返回, 
CLOUDGAME_API void SetRunLogCallback(LogCallback  callback);

/////////设置gStreamer输出日志的路径
CLOUDGAME_API void SetGStreamerLogPath(char *  logFilePath);