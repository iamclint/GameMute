#include <stdint.h>
#include <iostream>
#include "systray.h"
#include "gm.embed"
#include <thread>
#include "resource.h"
#include <windowsx.h>
#define APPWM_ICONNOTIFY (WM_APP + 1)
NOTIFYICONDATA systray::icon{};
HWND systray::console{};
bool systray::running{};
HWND systray::hwnd{};
HICON systray::hIcon{};
class __declspec(uuid("2b01df78-99b2-4ee2-bc1a-1a7c692f5296")) MuteIcon;
void ShowContextMenu(HWND hwnd, POINT pt)
{
	HMENU hMenu = LoadMenu(nullptr, MAKEINTRESOURCE(IDC_CONTEXTMENU));
	if (hMenu)
	{
		HMENU hSubMenu = GetSubMenu(hMenu, 0);
		if (hSubMenu)
		{
			// our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
			SetForegroundWindow(hwnd);

			// respect menu drop alignment
			UINT uFlags = TPM_RIGHTBUTTON;
			if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
			{
				uFlags |= TPM_RIGHTALIGN;
			}
			else
			{
				uFlags |= TPM_LEFTALIGN;
			}

			TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
		}
		DestroyMenu(hMenu);
	}
}

void systray::balloon_msg(std::wstring title, std::wstring msg)
{
	// Display a balloon message for a print job with a custom icon
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.uFlags = NIF_INFO | NIF_GUID;
	nid.guidItem = __uuidof(MuteIcon);
	//nid.dwInfoFlags = NIIF_USER;
	nid.uFlags = NIF_INFO | NIF_GUID;
	nid.hWnd = hwnd;
	//nid.hBalloonIcon = hIcon;
	lstrcpy(nid.szInfoTitle, title.c_str());
	lstrcpy(nid.szInfo, msg.c_str());
	
	//LoadIconMetric(g_hInst, MAKEINTRESOURCE(baloon), LIM_LARGE, &nid.hBalloonIcon);
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void systray::show()
{
	ShowWindow(console, SW_RESTORE);
	ShowWindow(console, SW_SHOW);
	BringWindowToTop(console);
}
void systray::hide()
{
	ShowWindow(console, SW_HIDE);
}
void systray::close()
{
	Shell_NotifyIcon(NIM_DELETE, &systray::icon);
	//CloseWindow(console);
	CloseWindow(hwnd);
	DestroyWindow(hwnd);
	//DestroyWindow(console);
	PostQuitMessage(0);
	running = false;
}

LRESULT CALLBACK systray::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_COMMAND:
	{
		int const wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
			case ID_CONSOLE:
			{
				show();
				break;
			}

			case ID_EXIT:
			{
				close();
				break;
			}
		}
	}
	break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_DESTROY:
	{
		close();
		return 0;
	}
	case APPWM_ICONNOTIFY:
	{
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
		{
			show();
			return 0;
		}
		case WM_LBUTTONUP:
		{
			return 0;
		}
		case WM_RBUTTONUP:
		{
			POINT pt;
			if (GetCursorPos(&pt))
				ShowContextMenu(hwnd, pt);
			return 0;
		}

		return 0;
		}
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void systray::create_icon()
{
	int icon_width = GetSystemMetrics(SM_CXICON);
	int icon_height = GetSystemMetrics(SM_CYICON);
	int offset = LookupIconIdFromDirectoryEx((PBYTE)&embedded::gm, TRUE, icon_width, icon_height, LR_DEFAULTCOLOR);
	hIcon = CreateIconFromResourceEx((PBYTE)&embedded::gm + offset, sizeof(embedded::gm), TRUE, 0x30000, icon_width, icon_height, LR_DEFAULTCOLOR);
	if (hIcon == NULL) {
		printf("Failed to create icon\n%ix%i %i\n%d\n", icon_width, icon_height, offset, GetLastError());
	}
}

systray::systray()
{
	running = true;
	console = GetConsoleWindow();
	create_icon();
	SendMessage(console, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(console, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	register_window();
	std::thread console_monitor([this]()
		{
			while (running)
			{
				if (IsIconic(console))
					hide();
				Sleep(10);
			}
		});
	console_monitor.detach();
}

void systray::register_window()
{
	std::thread messagethread([this]()
		{
			const wchar_t CLASS_NAME[] = L"GM Hidden Window";
			WNDCLASS wc = { };
			wc.hCursor = LoadCursor(NULL, IDC_ARROW);
			wc.lpfnWndProc = WndProc;
			wc.hInstance = nullptr;// GetModuleHandle(NULL);
			wc.lpszClassName = CLASS_NAME;
			RegisterClass(&wc);
			hwnd = CreateWindowEx(
				0,                              // Optional window styles.
				CLASS_NAME,                     // Window class
				L"Game mute hidden window for messages",    // Window text
				WS_OVERLAPPEDWINDOW,            // Window style

				// Size and position
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

				NULL,       // Parent window    
				NULL,       // Menu
				NULL,  // Instance handle
				NULL        // Additional application data
			);
			if (hwnd == NULL)
			{
				printf("Failed to create window");
				return;
			}
			
			icon.cbSize = sizeof(NOTIFYICONDATA);
			icon.hWnd = hwnd;
			icon.uID = 1;
			icon.guidItem = __uuidof(MuteIcon);
			icon.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
			icon.hBalloonIcon = hIcon;
			icon.hIcon = hIcon;
			icon.uCallbackMessage = APPWM_ICONNOTIFY;
			lstrcpy(icon.szTip, L"Game mute");
			lstrcpy(icon.szInfoTitle, L"Game mute");
			Shell_NotifyIcon(NIM_ADD, &icon);
			//Shell_NotifyIcon(NIM_SETVERSION, &icon);
			MSG msg;
			while (GetMessage(&msg, hwnd, 0, 0) && running)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

		}
	);
	messagethread.detach();
}

