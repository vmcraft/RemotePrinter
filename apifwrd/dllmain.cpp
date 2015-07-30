#include "thriftlink_client.h"
#include "minhooklink.h"
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "apifwrd.h"

bool IsHookProcess(DWORD pid);
bool IsHostProcess(DWORD pid);
extern bool _dont_hook_if_connection_failed;
extern std::string _server_ip;
extern int _server_port;

FILE* concoleAllocated = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
        printf("apifwrd.dll loaded.\n");

        if (IsHookProcess(GetCurrentProcessId())) {
#ifdef _DEBUG
            if (AllocConsole()){
                concoleAllocated = freopen("CONOUT$", "w", stdout);
            }
#endif
            if (!thrift_connect(_server_ip.c_str(), _server_port)) {
                printf("thrift_connect() failed.\n");
                if (_dont_hook_if_connection_failed) {
                    return TRUE;
                }
            }
            else {
                printf("thrift_connect() successful.\n");
            }

            if (!start_hook()) {
            }

            printf("start_hook() called.\n");

        }
        else if (IsHostProcess(GetCurrentProcessId())) {
            break;
        }

        break;

	case DLL_THREAD_ATTACH:
        break;

	case DLL_THREAD_DETACH:
        break;

	case DLL_PROCESS_DETACH:
        if (IsHookProcess(GetCurrentProcessId())) {
            stop_hook();
            printf("stop_hook()\n");
            thrift_close();
            printf("thrift_close()\n");
        }
        else if (IsHostProcess(GetCurrentProcessId())){
        }
        printf("apifwrd.dll unloaded.\n");
#ifdef _DEBUG
        // Don't free to check messages to analyze after unloaded.
        if (concoleAllocated) {
            fclose(concoleAllocated);
		    FreeConsole();
        }
#endif
        break;
	}
	return TRUE;
}



