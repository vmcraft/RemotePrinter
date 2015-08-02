#include "thriftlink_client.h"
#include "minhooklink.h"
#include <iostream> 
#include <fstream>


std::string _server_ip = "127.0.0.1";
int _server_port = 3900;

// We will not hook the following processes forcely.
char *_with_hook[] = {
                    "POS_Main.exe",
                    "iexplore.exe",
#ifdef _DEBUG
                    "notepad.exe",
#endif
                    NULL
};
char *_host_process[] = {
                    "sample.exe",
                    NULL
};
char *_preloaded_dlls[] = {
                    "WINSPOOL.DRV",
                    NULL
};



bool _dont_hook_if_connection_failed = false;

bool read_from_file(std::string& file_path, std::string& binary_data) ;
void print_hex(const char* buffer, size_t len);

using namespace userdefined;

#define HOOK_ITEM_FUNC(MODULE_NAME, FUNCTION_NAME) {L#MODULE_NAME, #FUNCTION_NAME, (LPVOID)&Detour##FUNCTION_NAME, (LPVOID*)&fp##FUNCTION_NAME}
#define SAFE_HANDLE(VAL) (sizeof(VAL)==sizeof(int64_t) ? (int64_t)VAL  : ((int64_t)VAL)&0x00000000FFFFFFFF)
#define THRIFT_A_TO_STRING(VAL) (std::string((LPSTR)(VAL)))                                                                 // string to binary
#define THRIFT_W_TO_STRING(VAL) (std::string((LPCSTR)(VAL), ((VAL)?wcslen((LPCWSTR)VAL)*sizeof(WCHAR)+sizeof(WCHAR):0)))    // wstring to binary
#define THRIFT_B_TO_STRING(POI, LEN) (std::string((LPSTR)(POI), (((LPSTR)POI)!=NULL ? LEN : 0)))                            // buffer to binary
#define THRIFT_SAFE_GET(MAP, NAME, DEFAULT) ( MAP.find(NAME)!=MAP.end() ? MAP.at(NAME) : DEFAULT )
#define RP_SET_OFFSET(BUFFER,OFFSET) ((DWORD)OFFSET==(DWORD)-1 ? NULL : (PVOID)((PUCHAR)BUFFER+(DWORD)OFFSET))


// Define original function pointer
// Spooler functions
BOOL (WINAPI *fpOpenPrinterA)(LPSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSA pDefault) = NULL;
BOOL (WINAPI *fpOpenPrinterW)(LPWSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSW pDefault) = NULL;
BOOL (WINAPI *fpStartPagePrinter)(HANDLE hPrinter) = NULL;
DWORD (WINAPI *fpStartDocPrinterW)(HANDLE hPrinter, DWORD  Level, LPBYTE pDocInfo) = NULL;
BOOL (WINAPI *fpWritePrinter)(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf, LPDWORD pcWritten) = NULL;
BOOL (WINAPI *fpEndPagePrinter)(HANDLE hPrinter) = NULL;
BOOL (WINAPI *fpEndDocPrinter)(HANDLE hPrinter) = NULL;
BOOL (WINAPI *fpClosePrinter)(HANDLE hPrinter) = NULL;

BOOL (WINAPI *fpCloseSpoolFileHandle)(HANDLE hPrinter, HANDLE hSpoolFile) = NULL;
HANDLE (WINAPI *fpCommitSpoolData)(HANDLE hPrinter, HANDLE hSpoolFile, DWORD cbCommit) = NULL;
HRESULT (WINAPI *fpDocumentEvent)(HANDLE hPrinter, HDC hdc, INT iEsc, ULONG cbIn, PVOID pvIn, ULONG cbOut, PVOID pvOut) = NULL;
LONG (WINAPI *fpDocumentPropertiesW)(HWND hWnd, HANDLE hPrinter, LPWSTR pDeviceName, PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput, DWORD fMode) = NULL;
BOOL (WINAPI *fpEnumFormsW)(HANDLE hPrinter, DWORD Level, LPBYTE  pForm, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned) = NULL;
BOOL (WINAPI *fpEnumPrintersW)(DWORD Flags, LPWSTR Name,DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned) = NULL;
BOOL (WINAPI *fpFindClosePrinterChangeNotification)(HANDLE hChange) = NULL;
HANDLE (WINAPI *fpFindFirstPrinterChangeNotification)(HANDLE hPrinter, DWORD fdwFilter, DWORD fdwOptions, LPVOID pPrinterNotifyOptions) = NULL;
BOOL (WINAPI *fpFindNextPrinterChangeNotification)(HANDLE hChange,PDWORD pdwChange,LPVOID pPrinterNotifyOptions,LPVOID *ppPrinterNotifyInfo) = NULL;
BOOL (WINAPI *fpFreePrinterNotifyInfo)(PPRINTER_NOTIFY_INFO pPrinterNotifyInfo) = NULL;
BOOL (WINAPI *fpGetDefaultPrinterW)(LPWSTR pszBuffer, LPDWORD pcchBuffer) = NULL;
DWORD (WINAPI *fpGetPrinterDataW)(HANDLE hPrinter, LPWSTR  pValueName, LPDWORD pType, LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded) = NULL;
DWORD (WINAPI *fpGetPrinterDataExW)(HANDLE hPrinter,LPCWSTR pKeyName, LPCWSTR pValueName,LPDWORD pType, LPBYTE  pData, DWORD nSize, LPDWORD pcbNeeded) = NULL;
BOOL (WINAPI *fpGetPrinterW)(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD cbBuf, LPDWORD pcbNeeded) = NULL;
HANDLE (WINAPI *fpGetSpoolFileHandle)(HANDLE hPrinter) = NULL;
BOOL (WINAPI *fpIsValidDevmodeW)(PDEVMODEW pDevmode, size_t DevmodeSize) = NULL;
BOOL (WINAPI *fpOpenPrinter2W)(LPCWSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSW pDefault, /*PPRINTER_OPTIONS*/ PVOID pOptions) = NULL;
BOOL (WINAPI *fpOpenPrinter2A)(LPCSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSA pDefault, /*PPRINTER_OPTIONS*/ PVOID pOptions) = NULL;



// Not a spooler, special codes for samsung printers
int (WINAPI *fpOpenUsbPort)(int) = NULL;
int (WINAPI *fpCloseUsbPort)() = NULL;
int (WINAPI *fpWriteUSB)(const char*, int) = NULL;
int (WINAPI *fpReadUSB)(const char*, int) = NULL;
int (WINAPI *fpPrintBitmap)(const char*) = NULL;
int (WINAPI *fpPrint1DBarcode)(int,int,int,int,char*) = NULL;
int (WINAPI *fpPrintPDF417)(int,int,int,int,int,int,char*) = NULL;
int (WINAPI *fpPrintQRCode)(int,int,int,char*) = NULL;


typedef struct _RP_PRINTER_OPTIONS {
  UINT  cbSize;
  DWORD dwFlags;
} RP_PRINTER_OPTIONS, *PRP_PRINTER_OPTIONS;



// Define detour function


BOOL WINAPI DetourOpenPrinterA(LPSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSA pDefault){
    printf("DetourOpenPrinterA()\n");
    if (!ensure_connection()) {
        printf("DetourOpenPrinterA() bypass.\n");
        return fpOpenPrinterA(pPrinterName, phPrinter, pDefault);
    }

    std::string printerName(NULL, 0);
    std::map<string, int64_t> ret;

    if (pPrinterName!=NULL) printerName = std::string(pPrinterName);

    if (pDefault) {
        _api->OpenPrinterA(ret,
            printerName,
            true,
            THRIFT_A_TO_STRING(pDefault->pDatatype),
            THRIFT_B_TO_STRING(pDefault->pDevMode, pDefault->pDevMode->dmSize),
            pDefault->DesiredAccess
            );
    }
    else {
        _api->OpenPrinterA(ret,
            printerName,
            false,
            std::string(NULL, 0),
            std::string(NULL, 0),
            0
            );
    }

    BOOL result = THRIFT_SAFE_GET(ret, "return", FALSE);
    if ( result) {
        if (phPrinter){
            *phPrinter = (HANDLE)THRIFT_SAFE_GET(ret, "phPrinter", NULL);
            printf("PrinterHandle=%x\n", *phPrinter);
        }
        return result;
    }

    return result;
}

BOOL WINAPI DetourOpenPrinterW(LPWSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSW pDefault){
    printf("DetourOpenPrinterW()\n");

    if (!ensure_connection()) {
        printf("DetourOpenPrinterW() bypass.\n");
        return fpOpenPrinterW(pPrinterName, phPrinter, pDefault);
    }

    std::string printerName(NULL, 0);
    std::map<string, int64_t> ret;

    if (pPrinterName!=NULL) printerName = std::string((char*)pPrinterName, wcslen(pPrinterName)*sizeof(WCHAR)+sizeof(WCHAR));

    if (pDefault) {
        _api->OpenPrinterW(ret, printerName, true,
            THRIFT_W_TO_STRING(pDefault->pDatatype),
            THRIFT_B_TO_STRING(pDefault->pDevMode, pDefault->pDevMode->dmSize),
            pDefault->DesiredAccess);
    }
    else {
        _api->OpenPrinterW(ret, printerName, false,
            std::string(NULL, 0),
            std::string(NULL, 0),
            0);
    }

    BOOL result = THRIFT_SAFE_GET(ret, "return", FALSE);
    if ( result) {
        if (phPrinter) {
            *phPrinter = (HANDLE)THRIFT_SAFE_GET(ret, "phPrinter", NULL);
            printf("PrinterHandle=%x\n", *phPrinter);
        }
        return result;
    }

    return result;
}

BOOL WINAPI DetourStartPagePrinter(HANDLE hPrinter) {
    printf("DetourStartPagePrinter()\n");
    if (!ensure_connection()) {
        printf("DetourStartPagePrinter() bypass.\n");
        return fpStartPagePrinter(hPrinter);
    }

    return _api->StartPagePrinter(SAFE_HANDLE(hPrinter));
}

DWORD WINAPI DetourStartDocPrinterW(HANDLE hPrinter, DWORD  Level, LPBYTE pDocInfo) {
    printf("DetourStartDocPrinterW()\n");
    if (!ensure_connection()) {
        printf("DetourStartDocPrinterW() bypass.\n");
        return fpStartDocPrinterW(hPrinter, Level, pDocInfo);
    }

    std::string pDocName(NULL, 0);
    std::string pOutputFile(NULL, 0);
    std::string pDatatype(NULL, 0);

    LPDOC_INFO_1W localDocInfo = (LPDOC_INFO_1W) pDocInfo;

    if (localDocInfo) {
        if (localDocInfo->pDocName)
            pDocName = std::string((char*)localDocInfo->pDocName, wcslen(localDocInfo->pDocName)*sizeof(WCHAR)+sizeof(WCHAR));
        if (localDocInfo->pOutputFile)
            pOutputFile = std::string((char*)localDocInfo->pOutputFile, wcslen(localDocInfo->pOutputFile)*sizeof(WCHAR)+sizeof(WCHAR));
        if (localDocInfo->pDatatype)
            pDatatype = std::string((char*)localDocInfo->pDatatype, wcslen(localDocInfo->pDatatype)*sizeof(WCHAR)+sizeof(WCHAR));
    }

    return _api->StartDocPrinterW(SAFE_HANDLE(hPrinter), Level, pDocName, pOutputFile, pDatatype);
}

BOOL WINAPI DetourWritePrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf, LPDWORD pcWritten) {
    printf("DetourWritePrinter()\n");
    if (!ensure_connection()) {
        printf("DetourWritePrinter() bypass.\n");
        return fpWritePrinter(hPrinter, pBuf, cbBuf, pcWritten);
    }

    std::map<string, int32_t> ret;

    _api->WritePrinter(ret, SAFE_HANDLE(hPrinter), std::string((char*)pBuf, cbBuf), cbBuf);

    if (pcWritten) *pcWritten = THRIFT_SAFE_GET(ret, "pcWritten", 0);
    return (BOOL)THRIFT_SAFE_GET(ret, "return", FALSE);
}

BOOL WINAPI DetourEndPagePrinter(HANDLE hPrinter) {
    printf("DetourEndPagePrinter()\n");
    if (!ensure_connection()) {
        printf("DetourEndPagePrinter() bypass.\n");
        return fpEndPagePrinter(hPrinter);
    }
    return _api->EndPagePrinter(SAFE_HANDLE(hPrinter));
}

BOOL WINAPI DetourEndDocPrinter(HANDLE hPrinter) {
    printf("DetourEndDocPrinter()\n");
    if (!ensure_connection()) {
        printf("DetourEndDocPrinter() bypass.\n");
        return fpEndDocPrinter(hPrinter);
    }
    return _api->EndDocPrinter(SAFE_HANDLE(hPrinter));
}

BOOL WINAPI DetourClosePrinter(HANDLE hPrinter) {
    printf("DetourClosePrinter()\n");
    if (!ensure_connection()) {
        printf("DetourClosePrinter() bypass.\n");
        return fpClosePrinter(hPrinter);
    }

    return _api->ClosePrinter(SAFE_HANDLE(hPrinter));
}


BOOL WINAPI DetourCloseSpoolFileHandle(
  _In_ HANDLE hPrinter,
  _In_ HANDLE hSpoolFile
){
    printf("DetourCloseSpoolFileHandle()\n");
    if (!ensure_connection()) {
        printf("DetourCloseSpoolFileHandle() bypass.\n");
        return fpCloseSpoolFileHandle(hPrinter, hSpoolFile);
    }

    return _api->CloseSpoolFileHandle(SAFE_HANDLE(hPrinter), SAFE_HANDLE(hSpoolFile));
}

HANDLE WINAPI DetourCommitSpoolData(
  _In_ HANDLE hPrinter,
  _In_ HANDLE hSpoolFile,
       DWORD  cbCommit
) {
    printf("DetourCommitSpoolData()\n");
    if (!ensure_connection()) {
        printf("DetourCommitSpoolData() bypass.\n");
        return fpCommitSpoolData(hPrinter, hSpoolFile, cbCommit);
    }
    return (HANDLE)_api->CommitSpoolData(SAFE_HANDLE(hPrinter), SAFE_HANDLE(hSpoolFile), cbCommit);
}

HRESULT WINAPI DetourDocumentEvent(
  _In_  HANDLE hPrinter,
  _In_  HDC    hdc,
        INT    iEsc,
        ULONG  cbIn,
  _In_  PVOID  pvIn,
        ULONG  cbOut,
  _Out_ PVOID  pvOut
) {
    printf("DetourDocumentEvent()\n");
    return S_FALSE;
}

LONG WINAPI DetourDocumentPropertiesW(
  _In_  HWND     hWnd,
  _In_  HANDLE   hPrinter,
  _In_  LPTSTR   pDeviceName,
  _Out_ PDEVMODE pDevModeOutput,
  _In_  PDEVMODE pDevModeInput,
  _In_  DWORD    fMode
) {
    printf("DetourDocumentPropertiesW()\n");
    if (!ensure_connection()) {
        printf("DetourDocumentPropertiesW() bypass.\n");
        return fpDocumentPropertiesW(hWnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput, fMode);
    }

    if (fpDocumentPropertiesW==NULL) return FALSE;

    ArgDocumentPropertiesW arg;
    ArgDocumentPropertiesW result;

    arg.hHwnd = SAFE_HANDLE(hWnd);
    arg.hPrinter = SAFE_HANDLE(hPrinter);
    arg.pDeviceName = THRIFT_W_TO_STRING(pDeviceName);
    arg.pDevModeInput = THRIFT_B_TO_STRING(pDevModeInput, (pDevModeInput != NULL ? pDevModeInput->dmSize : 0));
    arg.pDevModeOutput = THRIFT_B_TO_STRING(pDevModeOutput, (pDevModeOutput != NULL ? pDevModeOutput->dmSize : 0));
    arg.fMode = fMode;
   
    _api->DocumentPropertiesW(result, arg);

    if ((LONG)result.ret>=0 && pDevModeOutput) {
        memcpy(pDevModeOutput, result.pDevModeOutput.c_str(), result.pDevModeOutput.size());
    }

    SetLastError(result.lasterror);
    return (LONG)result.ret;
}

BOOL WINAPI DetourEnumFormsW(
  _In_  HANDLE  hPrinter,
  _In_  DWORD   Level,
  _Out_ LPBYTE  pForm,
  _In_  DWORD   cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned
) {
    printf("DetourEnumFormsW()\n");
    if (!ensure_connection()) {
        printf("DetourEnumFormsW() bypass.\n");
        return fpEnumFormsW(hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);
    }

    return FALSE;
}

//
// FIXME: This function has not been implemented yet.
//

BOOL WINAPI DetourEnumPrintersW(
  _In_  DWORD   Flags,
  _In_  LPWSTR  Name,
  _In_  DWORD   Level,
  _Out_ LPBYTE  pPrinterEnum,
  _In_  DWORD   cbBuf,
  _Out_ LPDWORD pcbNeeded,
  _Out_ LPDWORD pcReturned
) {
    printf("DetourEnumPrintersW()\n");
    if (!ensure_connection()) {
        printf("DetourEnumPrintersW() bypass.\n");
        return fpEnumPrintersW(Flags, Name, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
    }

    if (fpEnumPrintersW==NULL) return FALSE;
    
    ArgEnumPrintersW arg;
    ArgEnumPrintersW result;

    arg.Flags = Flags;
    arg.Name = THRIFT_W_TO_STRING(Name);
    arg.Level = Level;
    arg.cbBuf = cbBuf;
   
    _api->EnumPrintersW(result, arg);

    if (pcbNeeded) *pcbNeeded = result.pcbNeeded;
    if (pcReturned) *pcReturned = result.pcReturned;

    if (result.ret && pPrinterEnum) {
        memcpy(pPrinterEnum, result.pPrinterEnum.c_str(), cbBuf);

        switch(arg.Level){
        case 1:
            {
            LPPRINTER_INFO_1 info = (LPPRINTER_INFO_1) pPrinterEnum;
            info->pDescription = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pDescription"]);
            info->pName = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pName"]);
            info->pComment = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pComment"]);
            break;
            }
        case 2:
            {
            LPPRINTER_INFO_2 info = (LPPRINTER_INFO_2) pPrinterEnum;
            info->pServerName = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pServerName"]);
            info->pPrinterName = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pPrinterName"]);
            info->pShareName = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pShareName"]);
            info->pPortName = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pPortName"]);
            info->pDriverName = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pDriverName"]);
            info->pComment = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pComment"]);
            info->pLocation = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pLocation"]);
            info->pDevMode = (LPDEVMODE)RP_SET_OFFSET(info, result.int32PrinterEnum["pDevMode"]);
            info->pSepFile = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pSepFile"]);
            info->pPrintProcessor = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pPrintProcessor"]);
            info->pParameters = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pParameters"]);
            break;
            }
        case 4:
            {
            LPPRINTER_INFO_4 info = (LPPRINTER_INFO_4) pPrinterEnum;
            info->pPrinterName = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pPrinterName"]);
            info->pServerName = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pServerName"]);
            break;
            }
        case 5:
            {
            LPPRINTER_INFO_5 info = (LPPRINTER_INFO_5) pPrinterEnum;
            info->pPrinterName = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pPrinterName"]);
            info->pPortName = (LPWSTR)RP_SET_OFFSET(info, result.int32PrinterEnum["pPortName"]);
            break;
            }
        default:
            printf("DetourEnumPrintersW() Unknown Leven %d\n", arg.Level);
            result.ret = false;
        }
    }

    return (BOOL)result.ret;
}

BOOL WINAPI DetourFindClosePrinterChangeNotification(
  _In_ HANDLE hChange
) {
    printf("DetourFindClosePrinterChangeNotification()\n");
    if (!ensure_connection()) {
        printf("DetourFindClosePrinterChangeNotification() bypass.\n");
        return fpFindClosePrinterChangeNotification(hChange);
    }

    return FALSE;
}

HANDLE WINAPI DetourFindFirstPrinterChangeNotification(
  _In_     HANDLE hPrinter,
           DWORD  fdwFilter,
           DWORD  fdwOptions,
  _In_opt_ LPVOID pPrinterNotifyOptions
) {
    printf("DetourFindFirstPrinterChangeNotification()\n");
    if (!ensure_connection()) {
        printf("DetourFindFirstPrinterChangeNotification() bypass.\n");
        return fpFindFirstPrinterChangeNotification(hPrinter, fdwFilter, fdwOptions, pPrinterNotifyOptions);
    }

    return INVALID_HANDLE_VALUE;
}

BOOL WINAPI DetourFindNextPrinterChangeNotification(
  _In_      HANDLE hChange,
  _Out_opt_ PDWORD pdwChange,
  _In_opt_  LPVOID pPrinterNotifyOptions,
  _Out_opt_ LPVOID *ppPrinterNotifyInfo
) {
    printf("DetourFindNextPrinterChangeNotification()\n");
    if (!ensure_connection()) {
        printf("DetourFindNextPrinterChangeNotification() bypass.\n");
        return fpFindNextPrinterChangeNotification(hChange, pdwChange, pPrinterNotifyOptions, ppPrinterNotifyInfo);
    }

    return FALSE;
}

BOOL WINAPI DetourFreePrinterNotifyInfo(
  _In_ PPRINTER_NOTIFY_INFO pPrinterNotifyInfo
) {
    printf("DetourFreePrinterNotifyInfo()\n");
    if (!ensure_connection()) {
        printf("DetourFreePrinterNotifyInfo() bypass.\n");
        return fpFreePrinterNotifyInfo(pPrinterNotifyInfo);
    }

    return FALSE;
}

BOOL WINAPI DetourGetDefaultPrinterW(
  _In_    LPTSTR  pszBuffer,
  _Inout_ LPDWORD pcchBuffer
) {
    printf("DetourGetDefaultPrinterW()\n");
    if (!ensure_connection()) {
        printf("DetourGetDefaultPrinterW() bypass.\n");
        return fpGetDefaultPrinterW(pszBuffer, pcchBuffer);
    }

    if (pcchBuffer==NULL) return FALSE;

    ArgGetDefaultPrinterW args;
    ArgGetDefaultPrinterW result;

    args.pcchBuffer = (pcchBuffer?*pcchBuffer:0);

    _api->GetDefaultPrinterW(result,args);

    if (pcchBuffer) *pcchBuffer = result.pcchBuffer;
    if (result.ret) {
       memcpy(pszBuffer, result.pszBuffer.c_str(), result.pcchBuffer);
    }

    return (BOOL)result.ret;
}

DWORD WINAPI DetourGetPrinterDataW(
  _In_  HANDLE  hPrinter,
  _In_  LPTSTR  pValueName,
  _Out_ LPDWORD pType,
  _Out_ LPBYTE  pData,
  _In_  DWORD   nSize,
  _Out_ LPDWORD pcbNeeded
) {
    printf("DetourGetPrinterDataW()\n");
    if (!ensure_connection()) {
        printf("DetourGetPrinterDataW() bypass.\n");
        return fpGetPrinterDataW(hPrinter, pValueName, pType, pData, nSize, pcbNeeded);
    }

    return 0;
}

DWORD WINAPI DetourGetPrinterDataExW(
  _In_  HANDLE  hPrinter,
  _In_  LPCTSTR pKeyName,
  _In_  LPCTSTR pValueName,
  _Out_ LPDWORD pType,
  _Out_ LPBYTE  pData,
  _In_  DWORD   nSize,
  _Out_ LPDWORD pcbNeeded
) {
    printf("DetourGetPrinterDataExW()\n");
    if (!ensure_connection()) {
        printf("DetourGetPrinterDataExW() bypass.\n");
        return fpGetPrinterDataExW(hPrinter, pKeyName, pValueName, pType, pData, nSize, pcbNeeded);
    }

    return 0;
}

//
// FIXME: You have to implement another levels.
//

BOOL WINAPI DetourGetPrinterW(
  _In_  HANDLE  hPrinter,
  _In_  DWORD   Level,
  _Out_ LPBYTE  pPrinter,
  _In_  DWORD   cbBuf,
  _Out_ LPDWORD pcbNeeded
) {
    printf("DetourGetPrinterW()\n");
    if (!ensure_connection()) {
        printf("DetourGetPrinterW() bypass.\n");
        return fpGetPrinterW(hPrinter, Level, pPrinter, cbBuf, pcbNeeded);
    }

    if (fpGetPrinterW==NULL) return FALSE;
    
    ArgGetPrinterW arg;
    ArgGetPrinterW result;

    arg.hPrinter = SAFE_HANDLE(hPrinter);
    arg.Level = Level;
    arg.cbBuf = cbBuf;
   
    _api->GetPrinterW(result, arg);
    SetLastError(result.lasterror);

    if (pcbNeeded) *pcbNeeded = result.pcbNeeded;

    if (result.ret) {
        memcpy(pPrinter, result.pPrinter.c_str(), result.pcbNeeded);

        switch(arg.Level){
        
        //
        // Only level2 implemented.
        //

        case 2:
            {
            PPRINTER_INFO_2 info = (PPRINTER_INFO_2) pPrinter;

            info->pServerName = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pServerName"]);
            info->pPrinterName = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pPrinterName"]);
            info->pShareName = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pShareName"]);
            info->pPortName = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pPortName"]);
            info->pDriverName = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pDriverName"]);
            info->pComment = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pComment"]);
            info->pLocation = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pLocation"]);
            info->pDevMode = (PDEVMODE)RP_SET_OFFSET(info, result.int32Args["pDevMode"]);
            info->pSepFile = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pSepFile"]);
            info->pPrintProcessor = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pPrintProcessor"]);
            info->pDatatype = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pDatatype"]);
            info->pParameters = (LPWSTR)RP_SET_OFFSET(info, result.int32Args["pParameters"]);
            info->pSecurityDescriptor = NULL;
            break;
            }
        default:
            printf("DetourGetPrinterW() Unknown Leven %d\n", arg.Level);
            result.ret = false;
            break;
        }
    }

    return (BOOL)result.ret;

}

HANDLE WINAPI DetourGetSpoolFileHandle(
  _In_ HANDLE hPrinter
) {
    printf("DetourGetSpoolFileHandle()\n");
    if (!ensure_connection()) {
        printf("DetourGetSpoolFileHandle() bypass.\n");
        return fpGetSpoolFileHandle(hPrinter);
    }

    return NULL;
}

BOOL WINAPI DetourIsValidDevmodeW(
  _In_ PDEVMODE pDevmode,
       size_t   DevmodeSize
) {
    printf("DetourIsValidDevmodeW()\n");
    if (!ensure_connection()) {
        printf("DetourIsValidDevmodeW() bypass.\n");
        return fpIsValidDevmodeW(pDevmode, DevmodeSize);
    }

    return FALSE;
}

BOOL WINAPI DetourOpenPrinter2W(
  _In_  LPCWSTR            pPrinterName,
  _Out_ LPHANDLE           phPrinter,
  _In_  LPPRINTER_DEFAULTSW pDefault,
  _In_  /*PPRINTER_OPTIONS*/ PVOID   pOptions
) {
    printf("DetourOpenPrinter2W()\n");
    if (!ensure_connection()) {
        printf("DetourOpenPrinter2W() bypass.\n");
        return fpOpenPrinter2W(pPrinterName, phPrinter, pDefault, pOptions);
    }

    std::string printerName = THRIFT_W_TO_STRING(pPrinterName);
    std::map<string, int64_t> ret;

    if (pDefault) {
        _api->OpenPrinter2W(
            ret, printerName, true,
            THRIFT_W_TO_STRING(pDefault->pDatatype),
            THRIFT_B_TO_STRING(pDefault->pDevMode, pDefault->pDevMode->dmSize),
            pDefault->DesiredAccess,
            THRIFT_B_TO_STRING(pOptions, ((PRP_PRINTER_OPTIONS)pOptions)->cbSize));
    }
    else {
        _api->OpenPrinter2W(
            ret, printerName, false,
            std::string(NULL, 0),
            std::string(NULL, 0),
            0,
            THRIFT_B_TO_STRING(pOptions, ((PRP_PRINTER_OPTIONS)pOptions)->cbSize));
    }

    BOOL result = THRIFT_SAFE_GET(ret, "return", false);
    if ( result) {
        if (phPrinter) {
            *phPrinter = (HANDLE) THRIFT_SAFE_GET(ret, "phPrinter", NULL);
            printf("PrinterHandle=%x\n", *phPrinter);
        }
        return result;
    }

    return result;
}


BOOL WINAPI DetourOpenPrinter2A(
  _In_  LPCSTR            pPrinterName,
  _Out_ LPHANDLE           phPrinter,
  _In_  LPPRINTER_DEFAULTSA pDefault,
  _In_  /*PPRINTER_OPTIONS*/ PVOID   pOptions
) {
    printf("DetourOpenPrinter2A()\n");
    if (!ensure_connection()) {
        printf("DetourOpenPrinter2A() bypass.\n");
        return fpOpenPrinter2A(pPrinterName, phPrinter, pDefault, pOptions);
    }

    std::string printerName(pPrinterName);
    std::map<string, int64_t> ret;

    if (pDefault) {
        _api->OpenPrinter2A(
            ret, printerName, true,
            THRIFT_A_TO_STRING(pDefault->pDatatype),
            THRIFT_B_TO_STRING(pDefault->pDevMode, pDefault->pDevMode->dmSize),
            pDefault->DesiredAccess,
            THRIFT_B_TO_STRING(pOptions, ((PRP_PRINTER_OPTIONS)pOptions)->cbSize));
    }
    else {
        _api->OpenPrinter2A(
            ret, printerName, false,
            std::string(NULL, 0),
            std::string(NULL, 0),
            0,
            THRIFT_B_TO_STRING(pOptions, ((PRP_PRINTER_OPTIONS)pOptions)->cbSize));
    }

    BOOL result = THRIFT_SAFE_GET(ret, "return", false);
    if ( result) {
        if (phPrinter) {
            *phPrinter = (HANDLE) THRIFT_SAFE_GET(ret, "phPrinter", NULL);
            printf("PrinterHandle=%x\n", *phPrinter);
        }
        return result;
    }

    return result;
}



//
// Not a spooler, special codes for samsung printers
//

int WINAPI DetourOpenUsbPort(int dwModel)
{
    printf("DetourOpenUsbPort()\n");
    if (!ensure_connection()) return 0;
    return _api->OpenUsbPort(dwModel);
}

int WINAPI DetourCloseUsbPort()
{
    printf("DetourCloseUsbPort()\n");
    if (!ensure_connection()) return 0;
    return _api->CloseUsbPort();
}

int WINAPI DetourWriteUSB(char* pBuffer, int nNumberOfBytesToWrite)
{
    printf("DetourWriteUSB()\n");
    if (!ensure_connection()) return 0;
    return _api->WriteUSB(std::string(pBuffer, nNumberOfBytesToWrite), nNumberOfBytesToWrite);
}

int WINAPI DetourReadUSB(char* pBuffer, int nNumberOfByteToRead)
{
    printf("DetourReadUSB()\n");
    if (!ensure_connection()) return 0;
    std::string arg0(pBuffer, nNumberOfByteToRead);
    int result = _api->ReadUSB(arg0, nNumberOfByteToRead);
    memcpy(pBuffer, arg0.c_str(), min(arg0.size(), nNumberOfByteToRead));
    return result;
}

int WINAPI DetourPrintBitmap(char* pbmpDir) {
    printf("DetourPrintBitmap()\n");
    if (!ensure_connection()) return 0;

    if (pbmpDir==NULL) return 0;

    std::string bmp_data;
    if (!read_from_file(std::string(pbmpDir), bmp_data)){
        printf("No such file %s\n", pbmpDir);
        return 0;
    }

    return _api->PrintBitmap(pbmpDir, bmp_data);
}

int WINAPI DetourPrint1DBarcode(int nCodeType, int nWidth, int nHeight, int nHRI, char* pBuffer) {
    printf("DetourPrint1DBarcode()\n");
    if (!ensure_connection()) return 0;
    return _api->Print1DBarcode(nCodeType, nWidth, nHeight, nHRI, std::string(pBuffer));
}

int WINAPI DetourPrintPDF417(int nColumns, int nRows, int nWidth, int nHeight, int nECLevel,
                    int nModule, char* pBuffer) {
    printf("DetourPrintPDF417()\n");

    printf("DetourPrintPDF417()\n");
    if (!ensure_connection()) return 0;

    if (pBuffer==NULL) return 0;

    std::string bmp_data;
    if (!read_from_file(std::string(pBuffer), bmp_data)){
        printf("No such file %s\n", pBuffer);
        return 0;
    }

    return _api->PrintPDF417(nColumns, nRows, nWidth, nHeight, nECLevel, nModule, pBuffer, bmp_data);
}

int WINAPI DetourPrintQRCode(int nModule, int nSize, int nECLevel, char* pBuffer) {
    printf("DetourPrintQRCode()\n");
    if (!ensure_connection()) return 0;
    return _api->PrintQRCode(nModule, nSize, nECLevel, std::string(pBuffer));
}


bool read_from_file(std::string& file_path, std::string& binary_data) {
    try{
        std::ifstream t(file_path.c_str());
        if (!t.is_open()) return false;

        t.seekg(0, std::ios::end);   
        binary_data.reserve(t.tellg());
        t.seekg(0, std::ios::beg);

        binary_data.assign((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    }
    catch(std::ifstream::failure e) {
        return false;
    }

    return true;
}

void print_hex(const char* buffer, size_t len) 
{
    for (int i=0  ; i< len ; ++i) {
        printf("%hhX", buffer[i]);
    }

    printf("\n");
}




// Define hook items
HOOK_ITEM _hook_items[] = {
    // Spooler functions
    
    HOOK_ITEM_FUNC(WINSPOOL.DRV, OpenPrinterA),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, OpenPrinterW),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, StartPagePrinter),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, StartDocPrinterW),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, WritePrinter),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, EndPagePrinter),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, ClosePrinter),

    /*
    HOOK_ITEM_FUNC(WINSPOOL.DRV, CloseSpoolFileHandle),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, CommitSpoolData),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, DocumentEvent),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, DocumentPropertiesW),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, EnumFormsW),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, EnumPrintersW),    
    HOOK_ITEM_FUNC(WINSPOOL.DRV, FindClosePrinterChangeNotification),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, FindFirstPrinterChangeNotification),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, FindNextPrinterChangeNotification),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, FreePrinterNotifyInfo),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, GetDefaultPrinterW),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, GetPrinterDataW),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, GetPrinterDataExW),    
    HOOK_ITEM_FUNC(WINSPOOL.DRV, GetPrinterW),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, GetSpoolFileHandle),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, IsValidDevmodeW),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, OpenPrinter2W),
    HOOK_ITEM_FUNC(WINSPOOL.DRV, OpenPrinter2A),
    */

    // Not a spooler, special codes for samsung printers
    HOOK_ITEM_FUNC(BXLPDIR.DLL, OpenUsbPort),
    HOOK_ITEM_FUNC(BXLPDIR.DLL, CloseUsbPort),
    HOOK_ITEM_FUNC(BXLPDIR.DLL, WriteUSB),
    HOOK_ITEM_FUNC(BXLPDIR.DLL, ReadUSB),
    HOOK_ITEM_FUNC(BXLPDIR.DLL, PrintBitmap),
    HOOK_ITEM_FUNC(BXLPDIR.DLL, Print1DBarcode),
    HOOK_ITEM_FUNC(BXLPDIR.DLL, PrintPDF417),
    HOOK_ITEM_FUNC(BXLPDIR.DLL, PrintQRCode),

    {NULL, NULL, NULL, NULL}
};

