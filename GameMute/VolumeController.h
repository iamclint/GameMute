#pragma once
#include <Windows.h>
#include <cctype>
#include <unordered_map>
#include <tchar.h>
#include <psapi.h>
#include <thread>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <unordered_map>
class VolumeController
{
public:
	std::vector<std::wstring> process_names;
public:
	VolumeController();
	~VolumeController();
	VolumeController(std::vector<std::wstring>& proc_names, int update_interval=500);

private:
	bool is_running;
	int update_time;
	std::thread update_thread;
	std::thread bind_thread;
	IAudioSessionManager2* manager = nullptr;
	void update();
	void run();
	void fix_name(std::wstring& data);
	void remove_extension(std::wstring& data);
	void remove_path(std::wstring& data);
	void string_to_lower(std::wstring& data);
	std::wstring get_process_name(DWORD pid);
	std::unordered_map<std::wstring, ISimpleAudioVolume*> EnumSessions(IAudioSessionManager2* pSessionManager);
	HRESULT CreateSessionManager(IAudioSessionManager2** ppSessionManager);
};

