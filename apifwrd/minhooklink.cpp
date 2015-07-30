#include <Windows.h>
#include <stdio.h>
#include <list>
#include "minhooklink.h"


#if defined _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

extern HOOK_ITEM _hook_items[];
extern char* _preloaded_dlls[];


typedef struct {
    std::string dll_path;
    HMODULE hmod;
}PRELOADED_DLLS_INFO;

std::list<std::shared_ptr<PRELOADED_DLLS_INFO>> _preloaded_dll_list;



BOOL start_hook()
{
    // Load dlls 
    for (int i=0 ; _preloaded_dlls[i] != NULL ; ++i) {
        HMODULE hMod = LoadLibraryA(_preloaded_dlls[i]);
        if (hMod) {
            std::shared_ptr<PRELOADED_DLLS_INFO> info(new PRELOADED_DLLS_INFO());
            info->dll_path = _preloaded_dlls[i];
            info->hmod = hMod;
            _preloaded_dll_list.push_back(info);
        }
    }

    // Initialize MinHook.
    if (MH_Initialize() != MH_OK)
    {
        printf("MH_Initialize() failed.\n");
        return false;
    }

    bool result = true;

    for (int i=0 ; _hook_items[i].procName!=NULL ; ++i) {
        if (MH_CreateHookApi(
            _hook_items[i].moduleName, _hook_items[i].procName, _hook_items[i].pDetour, _hook_items[i].pOriginal) != MH_OK)
        {
            printf("MH_CreateHookApiEx('%s') failed.\n", _hook_items[i].procName);
            result = false;
            continue;
        }

        HMODULE hModule;
        LPVOID  pTarget;

        hModule = GetModuleHandleW(_hook_items[i].moduleName);
        if (hModule == NULL) {
            result = false;
            continue;
        }

        pTarget = GetProcAddress(hModule, _hook_items[i].procName);
        if (pTarget == NULL) {
            result = false;
            continue;
        }

        if (MH_EnableHook(pTarget) != MH_OK) {
            result = false;
            continue;
        }

        printf("MH_CreateHookApiEx('%s') successful.\n", _hook_items[i].procName);
    }

    return result;
}

void stop_hook()
{
    for (int i=0 ; _hook_items[i].procName!=NULL ; ++i) {
        HMODULE hModule;
        LPVOID  pTarget;

        hModule = GetModuleHandleW(_hook_items[i].moduleName);
        if (hModule == NULL) continue;

        pTarget = GetProcAddress(hModule, _hook_items[i].procName);
        if (pTarget == NULL) continue;

        if (MH_DisableHook(pTarget) != MH_OK) {
            printf("MH_DisableHook(%s) failed\n", _hook_items[i].procName);
            continue;
        }
    }

    // Free dlls
    for (std::list<std::shared_ptr<PRELOADED_DLLS_INFO>>::iterator it = _preloaded_dll_list.begin();
                it != _preloaded_dll_list.end(); it++) {
        if ((*it)->hmod) {
            FreeLibrary((*it)->hmod);
            (*it)->hmod = NULL;
        }
    }

    // Uninitialize MinHook.
    MH_Uninitialize();
}
