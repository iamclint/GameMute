// GameMute.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
//#include "gmProcess.h"
#include "VolumeController.h"
int main()
{
    std::wcout << "Usage: add process names in lower case without extension to game_list.txt" << std::endl;
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
    VolumeController vm(games, 500);
    std::getchar();
}
