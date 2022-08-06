// GameMute.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include "volume_controller.h"
#include "systray.h"


int main()
{
    systray sys_tray;
	
	
    std::wcout << "Usage: add process names in lower case without extension to game_list.txt" << std::endl;
    std::wcout << "Minimize this window to hide to system tray as an icon" << std::endl;
    std::wcout << "To get the active process name in the format game mute expects press f1 while the game/process is active" << std::endl;
    std::wifstream infile("game_list.txt");
    std::wstring line;
    std::vector<std::wstring> games;
    while (std::getline(infile, line))
    {
        std::wistringstream iss(line);
        std::wcout << "Added " << line << " to list" << std::endl;
        games.push_back(line);
    }
	infile.close();
    volume_controller vm(games, 500);
    while (systray::running)
    {
        Sleep(10);
    }
    if (!systray::running)
        sys_tray.~systray();
}
