#include "volume_controller.h"
#include "systray.h"
#include <iostream>
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

void volume_controller::remove_extension(std::wstring& data)
{
    auto pos = data.find_last_of('.');
    if (pos != std::wstring::npos)
        data.erase(pos);
}
void volume_controller::remove_path(std::wstring& data)
{
    auto pos = data.find_last_of('\\');
    if (pos != std::wstring::npos)
        data.erase(0, pos + 1);
}
void volume_controller::string_to_lower(std::wstring& data)
{
    std::transform(data.begin(), data.end(), data.begin(),
        [](unsigned char c) { return std::tolower(c); });

}

void volume_controller::fix_name(std::wstring& data)
{
    remove_path(data);
    remove_extension(data);
    string_to_lower(data);
    
}

std::wstring volume_controller::get_process_name(DWORD pid)
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (GetProcessImageFileName(hProcess, szProcessName, sizeof(szProcessName) / sizeof(TCHAR)))
    {
		std::wstring name = szProcessName;
		fix_name(name);
		return name;
	}
	return std::wstring();
		
}

std::unordered_map<std::wstring, ISimpleAudioVolume*> volume_controller::EnumSessions(IAudioSessionManager2* pSessionManager)
{

    if (!pSessionManager)
    {
        return std::unordered_map<std::wstring, ISimpleAudioVolume*>{};
    }
    std::unordered_map<std::wstring, ISimpleAudioVolume*> rval;
    HRESULT hr = S_OK;

    int cbSessionCount = 0;
    LPWSTR pswSession = NULL;

    IAudioSessionEnumerator* pSessionList = NULL;
    IAudioSessionControl* pSessionControl = NULL;
    IAudioSessionControl2* pSessionControl2 = NULL;
    ISimpleAudioVolume* audioVolume;
    hr = pSessionManager->GetSessionEnumerator(&pSessionList);

    // Get the session count.
    hr = pSessionList->GetCount(&cbSessionCount);
    for (int index = 0; index < cbSessionCount; index++)
    {
        // CoTaskMemFree(pswSession);
        SAFE_RELEASE(pSessionControl);

        // Get the <n>th session.
        hr = pSessionList->GetSession(index, &pSessionControl);

        hr = pSessionControl->QueryInterface(
            __uuidof(IAudioSessionControl2), (void**)&pSessionControl2);

        hr = pSessionControl2->IsSystemSoundsSession();
        if (S_OK == hr)
            continue;
        //pSessionControl2->GetDisplayName()
        hr = pSessionControl2->GetDisplayName(&pswSession);
        DWORD id = 0;
        pSessionControl2->GetProcessId(&id);
        std::wstring current_session_proc = get_process_name(id);
		for (auto& pm : process_names)
            if (current_session_proc == pm)
            {
                pSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&audioVolume);
                rval[pm] = audioVolume;
            }
    }

done:
    CoTaskMemFree(pswSession);
    SAFE_RELEASE(pSessionControl);
    SAFE_RELEASE(pSessionList);
    SAFE_RELEASE(pSessionControl2);
    return rval;

}

HRESULT volume_controller::CreateSessionManager(IAudioSessionManager2** ppSessionManager)
{

    HRESULT hr = S_OK;

    IMMDevice* pDevice = NULL;
    IMMDeviceEnumerator* pEnumerator = NULL;
    IAudioSessionManager2* pSessionManager = NULL;

    CoInitialize(0);

    // Create the device enumerator.
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&pEnumerator);

    // Get the default audio device.
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);

    // Get the session manager.
    hr = pDevice->Activate(
        __uuidof(IAudioSessionManager2), CLSCTX_ALL,
        NULL, (void**)&pSessionManager);

    // Return the pointer to the caller.
    *(ppSessionManager) = pSessionManager;
    (*ppSessionManager)->AddRef();


done:

    // Clean up.
    SAFE_RELEASE(pSessionManager);
    SAFE_RELEASE(pEnumerator);
    SAFE_RELEASE(pDevice);

    return hr;
}
void volume_controller::update()
{
    static HWND previous_foreground_window = GetForegroundWindow();
	HWND foreground_window = GetForegroundWindow();
    if (foreground_window != previous_foreground_window)
    {
		DWORD pid = 0;
        DWORD ppid = 0;
        GetWindowThreadProcessId(foreground_window, &pid);
        GetWindowThreadProcessId(previous_foreground_window, &ppid);
        std::wstring proc_name = get_process_name(pid);
        std::wstring previous_proc_name = get_process_name(ppid);
        std::wcout << "Activated proc_name: " << proc_name << " Previous proc_name: " << previous_proc_name << std::endl;
        if (std::find(process_names.begin(), process_names.end(), proc_name) != process_names.end()) //Process has gained focus
		{
			
            CreateSessionManager(&manager);
            std::unordered_map<std::wstring, ISimpleAudioVolume*> sessions = EnumSessions(manager);
			if (sessions.find(proc_name) != sessions.end())
			{
				ISimpleAudioVolume* volume = sessions[proc_name];
                volume->SetMute(false, 0);
                std::wcout << "UnMuted: " << proc_name << std::endl;
			}
            SAFE_RELEASE(manager);
		}
		
        if (std::find(process_names.begin(), process_names.end(), previous_proc_name) != process_names.end()) //Process has lost focus
        {
            CreateSessionManager(&manager);
            std::unordered_map<std::wstring, ISimpleAudioVolume*> sessions = EnumSessions(manager);
            if (sessions.find(previous_proc_name) != sessions.end())
            {
                ISimpleAudioVolume* volume = sessions[previous_proc_name];
                volume->SetMute(true, 0);
                std::wcout << "Muted: " << previous_proc_name << std::endl;
            }
            SAFE_RELEASE(manager);
        }
    }
	
    previous_foreground_window = foreground_window;
}
void volume_controller::run()
{
    is_running = true;
    update_thread = std::thread([this]()
        {
            while (is_running)
            {
                update();
             
                std::this_thread::sleep_for(std::chrono::milliseconds(update_time));
            }
        });
    update_thread.detach();
    bind_thread = std::thread([this]()
        {
            while (is_running)
            {
                if (GetAsyncKeyState(VK_F1) & 0x01)
                {
                    HWND foreground_window = GetForegroundWindow();
                    DWORD pid = 0;
                    GetWindowThreadProcessId(foreground_window, &pid);
                    std::wstring proc_name = get_process_name(pid);
                    std::wcout << "Current process name: " << proc_name << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    bind_thread.detach();
}
volume_controller::volume_controller() : process_names{}, update_thread{}, update_time{500}
{
    run();
}
volume_controller::volume_controller(std::vector<std::wstring>& proc_names, int update_interval) : process_names{ proc_names }, update_thread{}, update_time{ update_interval } {
    run();
}
volume_controller::~volume_controller()
{
    is_running = false;
}