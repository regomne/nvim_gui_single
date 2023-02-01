#include <stdint.h>
#include <memory>
#include <string>
#include <sstream>
#include <windows.h>
#include <psapi.h>

#include "defer.h"
#include "log.h"

using namespace std;

constexpr const wchar_t* kNvimGuiTempFileName = L"\\nvim_gui_server_pid_file.tmp";
constexpr const wchar_t* kNvimListenAddr = L"127.0.0.1:19821";

wstring try_quote_parameter(const wchar_t* para)
{
    if (wcschr(para, L' ') != nullptr || wcschr(para, L'"') != nullptr)
    {
        // C++比较复杂，下面这段换成python：
        // para2 = para.replace('"', '\\"')
        // if para2.endswith('\\'): para2 += '\\'
        // s = '"' + para2 + '"'
        auto s = wstring(L"\"");
        auto slen = wcslen(para);
        s.reserve(slen * 2 + 2);
        for (size_t i = 0; i < slen; i++)
        {
            if (para[i] == L'"')
            {
                s += L"\\\"";
            }
            else
            {
                s += para[i];
            }
        }
        if (s[s.length() - 1] == L'\\')
        {
            s += L'\\';
        }
        s += L"\"";
        return s;
    }
    return para;
}

wstring combine_parameters(wchar_t** argv, int argc)
{
    if (argc == 0)
    {
        return L"";
    }
    wstringstream args;
    for (int i = 0; i < argc; i++)
    {
        args << L" ";
        args << try_quote_parameter(argv[i]);
    }
    return args.str();
}

int run_nvim(const wchar_t* file_path)
{
    auto cmd_buffer_size = 100 + wcslen(file_path);
    auto cmd_buffer = unique_ptr<wchar_t[]>(new wchar_t[cmd_buffer_size]);
    swprintf_s(cmd_buffer.get(), cmd_buffer_size, LR"("nvim" --server %s --remote-tab "%s")", kNvimListenAddr, file_path);
    
    STARTUPINFO start_info = {};
    start_info.cb = sizeof(start_info);
    start_info.dwFlags = STARTF_USESHOWWINDOW;
    start_info.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION proc_info;
    if (!CreateProcess(nullptr, cmd_buffer.get(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &start_info, &proc_info))
    {
        return GetLastError();
    }
    CloseHandle(proc_info.hThread);
    CloseHandle(proc_info.hProcess);
    return 0;
}

int get_nvim_gui_pid_from_temp_path(int& pid)
{
    wchar_t temp_path[MAX_PATH];
    if (GetTempPath(ARRAYSIZE(temp_path), temp_path) == 0)
    {
        LOG_ERROR("Get temp path err:%d", GetLastError());
        return 1;
    }
    auto pid_fname = wstring(temp_path) + kNvimGuiTempFileName;
    auto hf = CreateFile(pid_fname.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hf == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("open temp file %ws err:%d", pid_fname.c_str(), GetLastError());
        return 2;
    }
    defer{ CloseHandle(hf); };

    DWORD bytes_read;
    if (!ReadFile(hf, &pid, sizeof(pid), & bytes_read, nullptr) || bytes_read != sizeof(pid))
    {
        LOG_ERROR("read temp file %ws err:%d", pid_fname.c_str(), GetLastError());
        return 3;
    }
    return 0;
}

int write_nvim_gui_pid_to_temp_path(int pid)
{
    wchar_t temp_path[MAX_PATH];
    if (GetTempPath(ARRAYSIZE(temp_path), temp_path) == 0)
    {
        LOG_ERROR("Get temp path err:%d", GetLastError());
        return 1;
    }
    auto pid_fname = wstring(temp_path) + kNvimGuiTempFileName;
    auto hf = CreateFile(pid_fname.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hf == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("open temp file %ws err:%d", pid_fname.c_str(), GetLastError());
        return 2;
    }
    defer{ CloseHandle(hf); };

    DWORD bytes_wrote;
    if (!WriteFile(hf, &pid, sizeof(pid), &bytes_wrote, nullptr) || bytes_wrote != sizeof(pid))
    {
        LOG_ERROR("write temp file %ws err:%d", pid_fname.c_str(), GetLastError());
        return 3;
    }
    return 0;
}

bool is_pid_a_nvim_gui(const wchar_t* proc_name, int pid)
{
    auto proc_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!proc_handle)
    {
        LOG_ERROR("open process %d err:%d", pid, GetLastError());
        return false;
    }
    defer{ CloseHandle(proc_handle); };
    wchar_t name[MAX_PATH];
    if (GetModuleFileNameEx(proc_handle, 0, name, MAX_PATH))
    {
        LOG_INFO("process %d file name: %ws", pid, name);
        return wcsstr(name, proc_name) != nullptr;
    }
    else
    {
        LOG_ERROR("get module file name err:%d", GetLastError());
        return false;
    }
}

int create_nvim_gui(int& pid, const wchar_t* gui_proc_name, const wchar_t* file_path, const wchar_t* extra_args)
{
    auto cmd_buffer_size = 100 + wcslen(file_path);
    auto cmd_buffer = unique_ptr<wchar_t[]>(new wchar_t[cmd_buffer_size]);
    swprintf_s(cmd_buffer.get(), cmd_buffer_size, LR"("%s" "%s" %s -- --listen %s)", gui_proc_name, file_path, extra_args, kNvimListenAddr);

    STARTUPINFO start_info = {};
    start_info.cb = sizeof(start_info);
    PROCESS_INFORMATION proc_info;
    if (!CreateProcess(nullptr, cmd_buffer.get(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &start_info, &proc_info))
    {
        LOG_ERROR("create proc err:%d", GetLastError());
        return GetLastError();
    }

    CloseHandle(proc_info.hThread);
    CloseHandle(proc_info.hProcess);
    pid = proc_info.dwProcessId;
    LOG_INFO("%s created, pid:%d", gui_proc_name, pid);
    return 0;
}

bool is_main_window(HWND handle)
{
    return GetWindow(handle, GW_OWNER) == nullptr && IsWindowVisible(handle);
}

wchar_t WndTitle[1024];
bool find_main_window_by_pid(int pid, HWND& hwnd)
{
    struct MyData
    {
        int pid;
        HWND main_hwnd;
    };
    MyData data = { pid, nullptr };
    EnumWindows([](HWND handle, LPARAM lparam) {
        auto data = (MyData*)lparam;
        DWORD wnd_pid = 0;
        GetWindowThreadProcessId(handle, &wnd_pid);
        GetWindowText(handle, WndTitle, 1024);
        LOG_DEBUG("window:%08X pid:%6d title:%ws", handle, wnd_pid, WndTitle);
        if (wnd_pid == data->pid && is_main_window(handle))
        {
            data->main_hwnd = handle;
            return FALSE;
        }
        return TRUE;
        }, (LPARAM)&data);
    if (data.main_hwnd == nullptr)
    {
        LOG_ERROR("Can't find main window of pid:%d", pid);
        return false;
    }
    LOG_INFO("window found");
    hwnd = data.main_hwnd;
    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd)
{
    Logger::init_log(Logger::DBGVIEW, Logger::LOG_LINFO);

    int argc;
    auto argv = CommandLineToArgvW(GetCommandLine(), &argc);
    defer{ LocalFree(argv); };
    if (argc < 3)
    {
        MessageBox(nullptr, L"Usage error", nullptr, MB_ICONERROR);
        return 0;
    }

    int nvim_gui_pid = 0;
    bool nvim_gui_open = false;
    auto ret = get_nvim_gui_pid_from_temp_path(nvim_gui_pid);
    if (ret == 0)
    {
        nvim_gui_open = is_pid_a_nvim_gui(argv[1], nvim_gui_pid);
    }

    if (!nvim_gui_open)
    {
        auto launcher_extra_args = combine_parameters(&argv[3], argc - 3);
        ret = create_nvim_gui(nvim_gui_pid, argv[1], argv[2], launcher_extra_args.c_str());
        if (ret != 0)
        {
            wchar_t err[100];
            swprintf_s(err, L"Create nvim_gui error: %d", ret);
            MessageBox(nullptr, err, nullptr, MB_ICONERROR);
            return 0;
        }
        write_nvim_gui_pid_to_temp_path(nvim_gui_pid);
    }
    else
    {
        auto run_ret = run_nvim(argv[2]);
        if (run_ret != 0)
        {
            wchar_t err[100];
            swprintf_s(err, L"Create process error: %d", run_ret);
            MessageBox(nullptr, err, nullptr, MB_ICONERROR);
            return 0;
        }

        HWND main_hwnd;
        if (find_main_window_by_pid(nvim_gui_pid, main_hwnd))
        {
            SetForegroundWindow(main_hwnd);
        }
    }

    return 0;
}
