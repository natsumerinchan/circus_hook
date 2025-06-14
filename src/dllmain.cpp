#include <windows.h>
#include <detours.h>
#include <stdio.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

// 日志功能开关
#define ENABLE_LOGGING 1

// 原始函数指针
static HFONT (WINAPI *OriginalCreateFontA)(
    int nHeight,
    int nWidth,
    int nEscapement,
    int nOrientation, 
    int fnWeight,
    DWORD fdwItalic,
    DWORD fdwUnderline,
    DWORD fdwStrikeOut,
    DWORD fdwCharSet,
    DWORD fdwOutputPrecision,
    DWORD fdwClipPrecision,
    DWORD fdwQuality,
    DWORD fdwPitchAndFamily,
    LPCSTR lpszFace) = CreateFontA;

static HFONT (WINAPI *OriginalCreateFontW)(
    int nHeight,
    int nWidth,
    int nEscapement,
    int nOrientation,
    int fnWeight,
    DWORD fdwItalic, 
    DWORD fdwUnderline,
    DWORD fdwStrikeOut,
    DWORD fdwCharSet,
    DWORD fdwOutputPrecision,
    DWORD fdwClipPrecision,
    DWORD fdwQuality,
    DWORD fdwPitchAndFamily,
    LPCWSTR lpszFace) = CreateFontW;

static HANDLE (WINAPI *OriginalCreateFileA)(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile) = CreateFileA;

// 全局日志文件指针
static FILE* g_logFile = NULL;
static CRITICAL_SECTION g_csLog;

// 初始化日志文件
void InitLogFile() {
#if ENABLE_LOGGING
    InitializeCriticalSection(&g_csLog);
    // 使用UTF-8模式打开日志文件
    g_logFile = _wfopen(L"CIRCUS_HOOK.log", L"w, ccs=UTF-8");
#endif
}

// 日志函数
void Log(const wchar_t* format, ...) {
#if ENABLE_LOGGING
    if (!g_logFile) return;
    
    EnterCriticalSection(&g_csLog);
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    // 格式化日志前缀
    wchar_t prefix[256];
    swprintf(prefix, L"[%04d-%02d-%02d %02d:%02d:%02d] ", 
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond);
    
    // 格式化日志主体
    va_list args;
    va_start(args, format);
    wchar_t buffer[1024];
    vswprintf(buffer, format, args);
    va_end(args);
    
    // 组合并写入日志(UTF-8)
    fwprintf(g_logFile, L"%s%s\r\n", prefix, buffer);
    fflush(g_logFile);
    
    LeaveCriticalSection(&g_csLog);
#endif
}

// 声明WndProc函数指针
typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
static WNDPROC OriginalWndProc = (WNDPROC)0x408B80;

// Hooked WndProc函数
LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_SETTEXT || uMsg == WM_NCPAINT) {
        SetWindowTextW(hWnd, L"ホームメイド スイーツ claude-3-7-sonnet翻译补丁 || 作者：natsumerin@御爱同萌==雨宮ゆうこ@moyu || 允许转载但严禁倒卖和冒充人工汉化发布");
    }
    return OriginalWndProc(hWnd, uMsg, wParam, lParam);
}

// Hooked CreateFontA函数 - 重定向至CreateFontW
HFONT WINAPI HookedCreateFontA(
    int nHeight,
    int nWidth,
    int nEscapement,
    int nOrientation,
    int fnWeight,
    DWORD fdwItalic,
    DWORD fdwUnderline,
    DWORD fdwStrikeOut,
    DWORD fdwCharSet,
    DWORD fdwOutputPrecision,
    DWORD fdwClipPrecision,
    DWORD fdwQuality,
    DWORD fdwPitchAndFamily,
    LPCSTR lpszFace) {
    
    // 转换为Unicode字符串(CP936编码)
    WCHAR wszFaceName[MAX_PATH] = {0};
    if (lpszFace && *lpszFace) {
        MultiByteToWideChar(932, 0, lpszFace, -1, wszFaceName, MAX_PATH);
    }
    
    // 修改为使用GBK字符集(0x86)和黑体
    LPCWSTR newFace = L"黑体";
    
    // 使用Unicode日志
    if (lpszFace && *lpszFace) {
    Log(L"[Hook] CreateFontA redirected to CreateFontW: %s -> 黑体 (Charset: 0x86)", 
        wszFaceName);
    } else {
        Log(L"[Hook] CreateFontA redirected to CreateFontW: NULL -> 黑体 (Charset: 0x86)");
    }
    
    return OriginalCreateFontW(nHeight, nWidth, nEscapement, nOrientation,
                              fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut,
                              0x86, fdwOutputPrecision, fdwClipPrecision,
                              fdwQuality, fdwPitchAndFamily, newFace);
}

// 关闭日志文件
void CloseLogFile() {
#if ENABLE_LOGGING
    if (g_logFile) {
        fclose(g_logFile);
        g_logFile = NULL;
    }
    DeleteCriticalSection(&g_csLog);
#endif
}

// 自定义CreateFileA函数
HANDLE WINAPI HookedCreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile) {
        
    const char* origFileName = lpFileName;
    char newFileName[MAX_PATH] = {0};
    
    // 只处理游戏目录下的MES文件
    if (lpFileName && strstr(lpFileName, "AdvData\\MES")) {
        // 确保路径缓冲区安全
        if (strlen(lpFileName) >= MAX_PATH) {
            Log(L"[Error] Path too long: %S", lpFileName);
            return OriginalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
                                  lpSecurityAttributes, dwCreationDisposition,
                                  dwFlagsAndAttributes, hTemplateFile);
        }

        // 查找MES目录位置
        const char* mesPos = strstr(lpFileName, "MES");
        if (!mesPos || (mesPos - lpFileName) >= MAX_PATH) {
            Log(L"[Error] Invalid MES path: %S", lpFileName);
            return OriginalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
                                  lpSecurityAttributes, dwCreationDisposition,
                                  dwFlagsAndAttributes, hTemplateFile);
        }

        // 初始化并构建CHS路径
        char fullChsPath[MAX_PATH] = {0};
        strncpy(fullChsPath, lpFileName, mesPos - lpFileName);
        fullChsPath[mesPos - lpFileName] = '\0';
        strncat(fullChsPath, "MES_CHS\\", MAX_PATH - strlen(fullChsPath) - 1);
        strncat(fullChsPath, mesPos + 3, MAX_PATH - strlen(fullChsPath) - 1);
        
        // 确保路径安全
        if (strlen(fullChsPath) >= MAX_PATH) {
            Log(L"[Error] CHS path too long: %S", fullChsPath);
            return OriginalCreateFileA(origFileName, dwDesiredAccess, dwShareMode,
                                  lpSecurityAttributes, dwCreationDisposition,
                                  dwFlagsAndAttributes, hTemplateFile);
        }
        
        // 检查CHS路径是否存在，改为不区分大小写检查
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(fullChsPath, &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            FindClose(hFind);
            lpFileName = fullChsPath;
            Log(L"[Hook] Redirected: %S -> %S", origFileName, fullChsPath);
        } else {
            Log(L"[Hook] CHS file not found (%S), using original: %S", fullChsPath, origFileName);
        }
    }
    
    return OriginalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
                              lpSecurityAttributes, dwCreationDisposition,
                              dwFlagsAndAttributes, hTemplateFile);
}

// 导出函数
extern "C" __declspec(dllexport) void InitializeHook() {
    Log(L"Initializing hooks...");
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)OriginalCreateFontA, HookedCreateFontA);
    DetourAttach(&(PVOID&)OriginalCreateFileA, HookedCreateFileA);
    DetourAttach(&(PVOID&)OriginalWndProc, HookedWndProc);
    DetourTransactionCommit();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            InitLogFile();
    Log(L"DLL loaded");
            InitializeHook();
            break;
        case DLL_PROCESS_DETACH:
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourDetach(&(PVOID&)OriginalCreateFontA, HookedCreateFontA);
            DetourDetach(&(PVOID&)OriginalCreateFileA, HookedCreateFileA);
            DetourTransactionCommit();
    Log(L"DLL unloaded");
            break;
    }
    return TRUE;
}
