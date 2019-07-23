




















#ifndef _nsgfxdefs_h
#define _nsgfxdefs_h



#include "nscore.h"

#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DEV
#include <os2.h>
#include "prlog.h"

#include <uconv.h> 

#define COLOR_CUBE_SIZE 216

void PMERROR(const char *str);








#ifdef DEBUG
  extern void GFX_LogErr (unsigned ReturnCode, const char* ErrorExpression, const char* FileName, const char* FunctionName, long LineNum);

  inline long GFX_Check (long ReturnCode, long ErrorCode, const char* ErrorExpression, const char* FileName, const char* FunctionName, long LineNum) 
  { 
    if (ReturnCode == ErrorCode)
      GFX_LogErr (ErrorCode, ErrorExpression, FileName, FunctionName, LineNum);

    return ReturnCode ; 
  }

  #ifdef XP_OS2_VACPP
    #define GFX(ReturnCode, ErrorCode)\
            GFX_Check (ReturnCode, ErrorCode, #ReturnCode, __FILE__, __FUNCTION__, __LINE__)
  #else
    #define GFX(ReturnCode, ErrorCode)\
            GFX_Check (ReturnCode, ErrorCode, #ReturnCode, __FILE__, NULL, __LINE__)
  #endif

#else	
  #define GFX(ReturnCode, ErrorCode) ReturnCode
#endif

class nsString;
class nsIDeviceContext;

struct nsUconvInfo
{
  char*    mCharset;
  PRUint16 mCodePage;
  UconvObject  mConverter;
};

static nsUconvInfo gUconvInfo[15   ] = 
{
  { "DEFAULT",     0,    NULL },
  { "ANSI",        1252, NULL },
  { "EASTEUROPE",  1250, NULL },
  { "RUSSIAN",     1251, NULL },
  { "GREEK",       1253, NULL },
  { "TURKISH",     1254, NULL },
  { "HEBREW",      862,  NULL },
  { "ARABIC",      864,  NULL },
  { "BALTIC",      1257, NULL },
  { "THAI",        874,  NULL },
  { "SHIFTJIS",    932,  NULL },
  { "GB2312",      936,  NULL },
  { "HANGEUL",     949,  NULL },
  { "CHINESEBIG5", 950,  NULL },
  { "JOHAB",       1361, NULL }
};



struct nsGfxModuleData
{
   HMODULE hModResources;
   HPS     hpsScreen;
   LONG    lDisplayDepth;

   nsGfxModuleData();
  ~nsGfxModuleData();

   void Init();
};

int WideCharToMultiByte( int CodePage, const PRUnichar *pText, ULONG ulLength, char* szBuffer, ULONG ulSize );
int MultiByteToWideChar( int CodePage, const char*pText, ULONG ulLength, PRUnichar *szBuffer, ULONG ulSize );
BOOL GetTextExtentPoint32(HPS aPS, const char* aString, int aLength, PSIZEL aSizeL);
BOOL ExtTextOut(HPS aPS, int X, int Y, UINT fuOptions, const RECTL* lprc,
                const char* aString, unsigned int aLength, const int* pDx);

BOOL IsDBCS();

extern nsGfxModuleData gModuleData;

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define MK_RGB(r,g,b) ((r) * 65536) + ((g) * 256) + (b)

#ifdef DEBUG
extern PRLogModuleInfo *gGFXOS2LogModule;
#endif

#endif
