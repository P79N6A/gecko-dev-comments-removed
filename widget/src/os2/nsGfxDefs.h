



































#ifndef _nsgfxdefs_h
#define _nsgfxdefs_h



#include "nscore.h"

#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DEV
#include <os2.h>
#include "prlog.h"
#include "nsHashtable.h"

#include <uconv.h> 

#define COLOR_CUBE_SIZE 216

void PMERROR(const char *str);








#ifdef DEBUG
  extern void DEBUG_LogErr(long ReturnCode, const char* ErrorExpression,
                           const char* FileName, const char* FunctionName,
                           long LineNum);

  inline long CheckSuccess(long ReturnCode, long SuccessCode,
                           const char* ErrorExpression, const char* FileName,
                           const char* FunctionName, long LineNum)
  {
    if (ReturnCode != SuccessCode) {
      DEBUG_LogErr(ReturnCode, ErrorExpression, FileName, FunctionName, LineNum);
    }
    return ReturnCode;
  }

  #define CHK_SUCCESS(ReturnCode, SuccessCode)                          \
          CheckSuccess(ReturnCode, SuccessCode, #ReturnCode, __FILE__,  \
                       __FUNCTION__, __LINE__)

  inline long CheckFailure(long ReturnCode, long ErrorCode,
                           const char* ErrorExpression, const char* FileName,
                           const char* FunctionName, long LineNum)
  {
    if (ReturnCode == ErrorCode) {
      DEBUG_LogErr(ReturnCode, ErrorExpression, FileName, FunctionName, LineNum);
    }
    return ReturnCode;
  }


  #define GFX(ReturnCode, ErrorCode)                                  \
          CheckFailure(ReturnCode, ErrorCode, #ReturnCode, __FILE__,  \
                       __FUNCTION__, __LINE__)

#else	
  #define CHK_SUCCESS(ReturnCode, SuccessCode) ReturnCode

  #define GFX(ReturnCode, ErrorCode) ReturnCode
#endif

class nsString;
class nsIDeviceContext;


BOOL GetTextExtentPoint32(HPS aPS, const char* aString, int aLength, PSIZEL aSizeL);
BOOL ExtTextOut(HPS aPS, int X, int Y, UINT fuOptions, const RECTL* lprc,
                const char* aString, unsigned int aLength, const int* pDx);

BOOL IsDBCS();

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define MK_RGB(r,g,b) ((r) * 65536) + ((g) * 256) + (b)

#ifdef DEBUG
extern PRLogModuleInfo *gGFXOS2LogModule;
#endif

#endif
