



































#include "nsGfxDefs.h"

#ifdef DEBUG
  #include <string.h>
  #include <ctype.h>




  #include <stdio.h>
  #define DPRINTF printf
#else
  #include <stdio.h>
  #define DPRINTF printf
#endif

#include "nsDeviceContextSpecOS2.h"

#include <stdlib.h>

BOOL GetTextExtentPoint32(HPS aPS, const char* aString, int aLength, PSIZEL aSizeL)
{
  BOOL rc = TRUE;
  POINTL ptls[5];

  aSizeL->cx = 0;

  while(aLength > 0 && rc == TRUE) {
    ULONG thislen = min(aLength, 512);
    rc = GFX (::GpiQueryTextBox(aPS, thislen, (PCH)aString, 5, ptls), FALSE);
    aSizeL->cx += ptls[TXTBOX_CONCAT].x;
    aLength -= thislen;
    aString += thislen;
  }

  aSizeL->cy = ptls[TXTBOX_TOPLEFT].y - ptls[TXTBOX_BOTTOMLEFT].y;
  return rc;
}

BOOL ExtTextOut(HPS aPS, int X, int Y, UINT fuOptions, const RECTL* lprc,
                const char* aString, unsigned int aLength, const int* pSpacing)
{
  long rc = GPI_OK;
  POINTL ptl = {X, Y};

  GFX (::GpiMove(aPS, &ptl), FALSE);

  
  while (aLength > 0 && rc == GPI_OK) {
    ULONG ulChunkLen = min(aLength, 512);
    if (pSpacing) {
      rc = GFX (::GpiCharStringPos(aPS, nsnull, CHS_VECTOR, ulChunkLen,
                                   (PCH)aString, (PLONG)pSpacing), GPI_ERROR);
      pSpacing += ulChunkLen;
    } else {
      rc = GFX (::GpiCharString(aPS, ulChunkLen, (PCH)aString), GPI_ERROR);
    }
    aLength -= ulChunkLen;
    aString += ulChunkLen;
  }

  if (rc == GPI_OK)
    return TRUE;
  else
    return FALSE;
}

static BOOL bIsDBCS;
static BOOL bIsDBCSSet = FALSE;

BOOL IsDBCS()
{
  if (!bIsDBCSSet) {
    
    APIRET rc;
    COUNTRYCODE ctrycodeInfo = {0};
    CHAR        achDBCSInfo[12] = {0};                  
    ctrycodeInfo.country  = 0;                          
    ctrycodeInfo.codepage = 0;                          

    rc = DosQueryDBCSEnv(sizeof(achDBCSInfo), &ctrycodeInfo, achDBCSInfo);
    if (rc == NO_ERROR)
    {
        
        
        if (achDBCSInfo[0] != 0 || achDBCSInfo[1] != 0 ||
            achDBCSInfo[2] != 0 || achDBCSInfo[3] != 0)
        {
           bIsDBCS = TRUE;
        }
        else
        {
           bIsDBCS = FALSE;
        }
    } else {
       bIsDBCS = FALSE;
    } 
    bIsDBCSSet = TRUE;
  } 
  return bIsDBCS;
}





#ifdef DEBUG
void DEBUG_LogErr(long ReturnCode, const char* ErrorExpression,
                  const char* FileName, const char* FunctionName, long LineNum)
{
   char TempBuf [300];

   strcpy (TempBuf, ErrorExpression);
   char* APIName = TempBuf;

   char* ch = strstr (APIName , "(");                 
   if (ch != NULL)                                    
   {
      while (isspace (*--ch)) {}                      
      *++ch = '\0';

      if (APIName [0] == ':' && APIName [1] == ':')   
         APIName += 2;

      while (isspace (*APIName))                      
         APIName++;
   }


   USHORT ErrorCode = ERRORIDERROR (::WinGetLastError(0));

   printf("GFX_Err: %s = 0x%X, 0x%X (%s - %s,  line %ld)\n", APIName, ReturnCode,
          ErrorCode, FileName, FunctionName, LineNum);
}
#endif

void PMERROR( const char *api)
{
   ERRORID eid = ::WinGetLastError(0);
   USHORT usError = ERRORIDERROR(eid);
   DPRINTF ( "%s failed, error = 0x%X\n", api, usError);
}

