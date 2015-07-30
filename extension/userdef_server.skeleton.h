#ifndef __USER_DEF_SERVER_H
#define __USER_DEF_SERVER_H


#include "Shellapi.h"
#pragma comment(lib, "shell32.lib")


class ApiForwardHandler : virtual public ApiForwardIf {
public:
    ApiForwardHandler() {}

public:
    //
    // Add your hooking function here
    //

    int32_t ShellAboutW(const std::string& szApp) {
        printf("ShellAboutW()\n");
        ::ShellAboutW(NULL, (LPCWSTR)szApp.c_str(), NULL, NULL);
        return TRUE;
    }

};


#endif //__USER_DEF_SERVER_H