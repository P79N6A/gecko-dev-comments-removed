





































#ifndef nsPrintOS2_h___
#define nsPrintOS2_h___

#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_SPLDOSPRINT
#define INCL_DEV
#define INCL_DEVDJP
#define INCL_GRE_DEVICE
#include <os2.h>
#include <pmddim.h>
#include "gfxCore.h"





NS_GFX_(BOOL) PrnInitialize(HMODULE hmodResources);
NS_GFX_(BOOL) PrnTerminate(void);


class NS_GFX PRTQUEUE;

#define MAX_PRINT_QUEUES  (128)

class NS_GFX PRINTDLG
{
public:
   PRINTDLG();
  ~PRINTDLG();
   void      RefreshPrintQueue();
   ULONG     GetNumPrinters();
   void      GetPrinter(ULONG printerNdx, char** printerName);
   PRTQUEUE* SetPrinterQueue(ULONG printerNdx);
   LONG      GetPrintDriverSize(ULONG printerNdx);
   PDRIVDATA GetPrintDriver(ULONG printerNdx);
   HDC       GetDCHandle(ULONG printerNdx);
   char*     GetDriverType(ULONG printerNdx);
   BOOL      ShowProperties(ULONG printerNdx);

private:
  ULONG      mQueueCount;
  PRTQUEUE*  mPQBuf[MAX_PRINT_QUEUES];
};



NS_GFX_(BOOL) PrnClosePrinter( PRTQUEUE *pPrintQueue);


NS_GFX_(HDC) PrnOpenDC( PRTQUEUE *pPrintQueue, PSZ pszApplicationName, int copies, int destination, char *file);


NS_GFX_(BOOL) PrnQueryHardcopyCaps( HDC hdc, PHCINFO pHCInfo);


NS_GFX_(BOOL) PrnAbortJob( HDC hdc);



#endif
