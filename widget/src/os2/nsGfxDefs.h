




















#ifndef _nsgfxdefs_h
#define _nsgfxdefs_h



#include "nscore.h"

#define INCL_PM
#define INCL_DOS
#include <os2.h>

#include <uconv.h> 

#define COLOR_CUBE_SIZE 216

void PMERROR(const char *str);

class nsString;
class nsIDeviceContext;


struct nsGfxModuleData
{
   HMODULE hModResources;
   HPS     hpsScreen;
   LONG    lDisplayDepth;

   nsGfxModuleData();
  ~nsGfxModuleData();

   
   
   
   
   char *ConvertFromUcs( const PRUnichar *pText, ULONG ulLength, char *szBuffer, ULONG ulSize);
   char *ConvertFromUcs( const nsString &aStr, char *szBuffer, ULONG ulSize);
   
   const char *ConvertFromUcs( const PRUnichar *pText, ULONG ulLength);
   const char *ConvertFromUcs( const nsString &aStr);

   UconvObject  converter;
   BOOL         supplantConverter;
   PRUint32     renderingHints;
   ULONG        ulCodepage;
   

   void Init();
};

extern nsGfxModuleData gModuleData;

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define MK_RGB(r,g,b) ((r) * 65536) + ((g) * 256) + (b)

#endif
