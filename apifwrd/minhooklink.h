#pragma once

#include "MinHook.h"

typedef struct {
    LPCWSTR moduleName;
    LPCSTR  procName;
    LPVOID  pDetour;
    LPVOID*  pOriginal;
}HOOK_ITEM;

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
{
    return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

template <typename T>
inline MH_STATUS MH_CreateHookApiEx(
    LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, T** ppOriginal)
{
    return MH_CreateHookApi(
        pszModule, pszProcName, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}


BOOL start_hook();
void stop_hook();

