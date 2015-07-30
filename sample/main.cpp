
#include "thriftlink_server.h"
#include "userdef_server.h"
#include <Windows.h>
#include "stdio.h"

#pragma comment(lib, "user32.lib")

bool inject_dll(HMODULE &hmodule, HHOOK &hhook, int target_threadid);
void eject_dll(HMODULE &hmodule, HHOOK &hhook);


int main(int argc, char **argv) {

    if (argc==3 && _stricmp(argv[1], "hook")==0){
        int threadid = atoi(argv[2]);
        HMODULE hmodule = NULL;
        HHOOK hhook = NULL;

        if (!inject_dll(hmodule, hhook, threadid)){
            printf("Injection failed.\n");
            return 1;
        }

        printf("Press enter key to stop.");

        // Message handler is required.
        MSG msg;
        bool runThread = true;
        while(runThread) {
            // Keep pumping...
            PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message==WM_KEYDOWN && msg.wParam==VK_RETURN){
                break;
            }
            Sleep(10);
        }
    
        getchar();

        eject_dll(hmodule, hhook);
        return 0;
    }
    else if ((argc==2||argc==3) && _stricmp(argv[1], "server")==0) {
        int port = 3900;
        if (argc==3) atoi(argv[2]);
        thrift_server_start(port);
        return 0;
    }

    printf("Usage:\n");
    printf(" %s hook thread_id\n", argv[0]);
    printf(" %s server [port_number]\n", argv[0]);
    return 1;
}

//
// This sample demonstrate how to inject dll by SetWindowsHookEx.
// You can inject apifwrd.dll to destination process simply by AppInit_DLLs registry key or
// implement your own method like CreateRemoteThread, ...
//

bool inject_dll(HMODULE &hmodule, HHOOK &hhook, int target_threadid)
{
    // Already hooked.
    if (hhook!=NULL) return true;

    // Load library
    if (hmodule==NULL) {
        hmodule = LoadLibraryW(L"apifwrd.dll");
        if (hmodule==NULL) return false;
    }

    // apifwrd.dll is ready to support SetWindowsHookEx() way.
    HOOKPROC addr = (HOOKPROC) GetProcAddress(hmodule, "HelperHookProcForSetWindowsHookEx");
    if (addr==NULL) {
        FreeLibrary(hmodule);
        return false;
    }

    // Install hook
    
    hhook = SetWindowsHookEx(WH_CBT, addr, hmodule, target_threadid);
    if (hhook==NULL) {
        FreeLibrary(hmodule);
        return false;
    }    
    
    return true;
}

void eject_dll(HMODULE &hmodule, HHOOK &hhook)
{
    if (hhook) {
        UnhookWindowsHookEx(hhook);
        hhook = NULL;
    }

    if (hmodule) {
        FreeLibrary(hmodule);
        hmodule = NULL;
    }

    return;
}