/**
 * This Thrift file can be included by other Thrift files that want to share
 * these definitions.
 */

namespace cpp userdefined


/**
 * Define here your functions to hook
 */
service SelfService {
  /* Spooler functions */
  map<string,i64> OpenPrinterA(1:string pPrinterName, 2:bool pDefaultExist, 3:string pDatatype, 4:binary pDevMode, 5:i32 DesiredAccess)
  map<string,i64> OpenPrinterW(1:binary pPrinterName, 2:bool pDefaultExist, 3:binary pDatatype, 4:binary pDevMode, 5:i32 DesiredAccess)
  bool StartPagePrinter(1:i64 hPrinter)
  i32 StartDocPrinterW(1:i64 hPrinter, 2:i32 Level, 3:binary pDocName, 4:binary pOutputFile, 5:binary pDatatype)
  map<string,i32> WritePrinter(1:i64 hPrinter, 2:binary pBuf, 3:i32 cbBuf)
  bool EndPagePrinter(1:i64 hPrinter)
  bool EndDocPrinter(1:i64 hPrinter)
  bool ClosePrinter(1:i64 hPrinter)

  bool CloseSpoolFileHandle(1:i64 hPrinter, 2:i64 hSpoolFile)
  i64 CommitSpoolData(1:i64 hPrinter, 2:i64 hSpoolFile, 3:i32 cbCommit)
  i32 DocumentEvent(1:i64 hPrinter, 2:i64 hdc, 3:i32 iEsc, 4:i32 cbIn, 5:binary pvIn, 6:i32 cbOut, 7:binary pvOut)
  map<string,binary> DocumentPropertiesW(1:i64 hWnd, 2:i64 hPrinter, 3:binary pDeviceName, 4:binary pDevModeInput, 5:i32 fMode) 
  map<string,binary> EnumFormsW(1:i64 hPrinter, 2:i32 Level, 3:i32 cbBuf)
  map<string,binary> EnumPrintersW(1:i32 Flags, 2:binary Name, 3:i32 Level, 4:i32 cbBuf) 
  bool FindClosePrinterChangeNotification(1:i64 hChange)
  i64 FindFirstPrinterChangeNotification(1:i64 hPrinter, 2:i32 fdwFilter, 3:i32 fdwOptions, 4:binary pPrinterNotifyOptions)
  map<string,binary> FindNextPrinterChangeNotification(1:i64 hChange, 2:binary pPrinterNotifyOptions)
  bool FreePrinterNotifyInfo(1:binary pPrinterNotifyInfo)
  map<string,i32> GetDefaultPrinterW(1:binary pszBuffer)
  map<string,binary> GetPrinterDataW(1:i64 hPrinter, 2:binary pValueName, 3:i32 nSize)
  map<string,binary> GetPrinterDataExW(1:i64 hPrinter, 2:binary pKeyName, 3:binary pValueName, 4:i32 nSize)
  map<string,binary> GetPrinterW(1:i64 hPrinter, 2:i32 Level, 3:i32 cbBuf)
  i64 GetSpoolFileHandle(1:i64 hPrinter)
  bool IsValidDevmodeW(1:binary pDevmode, 2:i32 DevmodeSize)
  map<string,i64> OpenPrinter2W(1:binary pPrinterName, 2:binary pDefault, 3:binary pOptions)


  /* Not spooler but special codes for samsung printers */
  i32 OpenUsbPort(1:i32 dwModel)
  i32 CloseUsbPort()
  i32 WriteUSB(1:binary pBuffer, 2:i32 nNumberOfBytesToWrite)
  i32 ReadUSB(1:binary pBuffer, 2:i32 nNumberOfByteToRead)
  i32 PrintBitmap(1:string pbmpDir, 2:binary data)
  i32 Print1DBarcode(1:i32 nCodeType, 2:i32 nWidth, 3:i32 nHeight, 4:i32 nHRI, 5:binary pBuffer)
  i32 PrintPDF417(1:i32 nColumns, 2:i32 nRows, 3:i32 nWidth, 4:i32 nHeight, 5:i32 nECLevel, 6:i32 nModule, 7:string pBuffer, 8:binary data)
  i32 PrintQRCode(1:i32 nModule, 2:i32 nSize, 3:i32 nECLevel, 4:binary pBuffer)
}
