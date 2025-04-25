#include <windows.h>
#include <detours.h>
#include <stdio.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

// 日志功能开关
#define ENABLE_LOGGING 1

// 原始函数指针
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

// 初始化日志文件
void InitLogFile() {
#if ENABLE_LOGGING
    if (!g_logFile) {
        g_logFile = fopen("KADENZF_CHS.log", "w");
    }
#endif
}

// 日志函数
void Log(const char* format, ...) {
#if ENABLE_LOGGING
    if (!g_logFile) {
        g_logFile = fopen("KADENZF_CHS.log", "a");
    }
    
    if (g_logFile) {
        va_list args;
        va_start(args, format);
        vfprintf(g_logFile, format, args);
        fprintf(g_logFile, "\n");
        fflush(g_logFile);
        va_end(args);
    }
#endif
}

// 关闭日志文件
void CloseLogFile() {
#if ENABLE_LOGGING
    if (g_logFile) {
        fclose(g_logFile);
        g_logFile = NULL;
    }
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
            Log("[Error] Path too long: %s", lpFileName);
            return OriginalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
                                  lpSecurityAttributes, dwCreationDisposition,
                                  dwFlagsAndAttributes, hTemplateFile);
        }

        // 查找MES目录位置
        const char* mesPos = strstr(lpFileName, "MES");
        if (!mesPos || (mesPos - lpFileName) >= MAX_PATH) {
            Log("[Error] Invalid MES path: %s", lpFileName);
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
            Log("[Error] CHS path too long: %s", fullChsPath);
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
            Log("[Hook] Redirected: %s -> %s", origFileName, fullChsPath);
        } else {
            Log("[Hook] CHS file not found (%s), using original: %s", fullChsPath, origFileName);
        }
    }
    
    return OriginalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
                              lpSecurityAttributes, dwCreationDisposition,
                              dwFlagsAndAttributes, hTemplateFile);
}

// 导出函数
extern "C" __declspec(dllexport) void InitializeHook() {
    Log("Initializing hooks...");
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)OriginalCreateFileA, HookedCreateFileA);
    DetourTransactionCommit();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            InitLogFile();
            Log("DLL loaded");
            InitializeHook();
            break;
        case DLL_PROCESS_DETACH:
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourDetach(&(PVOID&)OriginalCreateFileA, HookedCreateFileA);
            DetourTransactionCommit();
            Log("DLL unloaded");
            break;
    }
    return TRUE;
}
