#include <Windows.h>
#include <shellapi.h>
class systray
{
public:
	static NOTIFYICONDATA icon;
	static HWND console;
	static HWND hwnd;
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void show();
	static void hide();
	static void close();
	static bool running;
	static void balloon_msg(std::wstring title, std::wstring msg);
	static HICON hIcon;
	systray();
private:
	void create_icon();
	void register_window();
	
	
};