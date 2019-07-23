




































#include "xp.h"

#include "profile.h"
#include "strconv.h"

static char szDefaultString[] = PROFILE_DEFAULT_STRING;

static BOOL isTrueNumericValue(LPSTR szString)
{
  char szPrefix[] = PROFILE_NUMERIC_PREFIX;
  return (strncmp(szString, szPrefix, strlen(szPrefix)) == 0);
}

static LPSTR convertStringToLPSTR(LPSTR szString)
{
  if(szString == NULL)
    return NULL;

  if(isTrueNumericValue(szString))
  {
    char szPrefix[] = PROFILE_NUMERIC_PREFIX;
    return (LPSTR)atol(szString + strlen(szPrefix));
  }
  else
    return szString;
}

DWORD convertStringToLPSTR1(DWORD * pdw1)
{
  if(pdw1 == NULL)
    return 0L;

  DWORD dwRet = (isTrueNumericValue((LPSTR)(*pdw1)) ? (fTNV1) : 0);

  *pdw1 = (DWORD)convertStringToLPSTR((LPSTR)(*pdw1));

  return dwRet;
}

DWORD convertStringToLPSTR2(DWORD * pdw1, DWORD * pdw2)
{
  if((pdw1 == NULL) || (pdw2 == NULL))
    return 0L;

  DWORD dwRet =   (isTrueNumericValue((LPSTR)(*pdw1)) ? (fTNV1) : 0)
                | (isTrueNumericValue((LPSTR)(*pdw2)) ? (fTNV2) : 0);
  
  *pdw1 = (DWORD)convertStringToLPSTR((LPSTR)(*pdw1));
  *pdw2 = (DWORD)convertStringToLPSTR((LPSTR)(*pdw2));

  return dwRet;
}

DWORD convertStringToLPSTR3(DWORD * pdw1, DWORD * pdw2, DWORD * pdw3)
{
  if((pdw1 == NULL) || (pdw2 == NULL) || (pdw3 == NULL))
    return 0L;

  DWORD dwRet =   (isTrueNumericValue((LPSTR)(*pdw1)) ? (fTNV1) : 0)
                | (isTrueNumericValue((LPSTR)(*pdw2)) ? (fTNV2) : 0)
                | (isTrueNumericValue((LPSTR)(*pdw3)) ? (fTNV3) : 0);

  *pdw1 = (DWORD)convertStringToLPSTR((LPSTR)(*pdw1));
  *pdw2 = (DWORD)convertStringToLPSTR((LPSTR)(*pdw2));
  *pdw3 = (DWORD)convertStringToLPSTR((LPSTR)(*pdw3));

  return dwRet;
}


static DWORD convertStringToDWORD(LPSTR szString)
{
  if(szString == NULL)
    return 0L;

  if(strcmp(szString, szDefaultString) == 0)
    return DEFAULT_DWARG_VALUE;
  else
    return (DWORD)atol(szString);
}

void convertStringToDWORD1(DWORD * pdw1)
{
  if(pdw1 == NULL)
    return;
  *pdw1 = convertStringToDWORD((LPSTR)(*pdw1));
}

void convertStringToDWORD2(DWORD * pdw1, DWORD * pdw2)
{
  if((pdw1 == NULL) || (pdw2 == NULL))
    return;
  *pdw1 = convertStringToDWORD((LPSTR)(*pdw1));
  *pdw2 = convertStringToDWORD((LPSTR)(*pdw2));
}

void convertStringToDWORD3(DWORD * pdw1, DWORD * pdw2, DWORD * pdw3)
{
  if((pdw1 == NULL) || (pdw2 == NULL) || (pdw3 == NULL))
    return;
  *pdw1 = convertStringToDWORD((LPSTR)(*pdw1));
  *pdw2 = convertStringToDWORD((LPSTR)(*pdw2));
  *pdw3 = convertStringToDWORD((LPSTR)(*pdw3));
}

void convertStringToDWORD4(DWORD * pdw1, DWORD * pdw2, DWORD * pdw3, DWORD * pdw4)
{
  if((pdw1 == NULL) || (pdw2 == NULL) || (pdw3 == NULL) || (pdw4 == NULL))
    return;

  *pdw1 = convertStringToDWORD((LPSTR)(*pdw1));
  *pdw2 = convertStringToDWORD((LPSTR)(*pdw2));
  *pdw3 = convertStringToDWORD((LPSTR)(*pdw3));
  *pdw4 = convertStringToDWORD((LPSTR)(*pdw4));
}

static NPBool convertStringToBOOL(LPSTR szString)
{
  if(szString == NULL)
    return (NPBool)NULL;

  if(isTrueNumericValue(szString))
  {
    char szPrefix[] = PROFILE_NUMERIC_PREFIX;
    return (NPBool)atol(szString + strlen(szPrefix));
  }
  else
  {
    NPBool npb = (stricmp(szString, "TRUE") == 0) ? TRUE : FALSE;
    return npb;
  }
}

DWORD convertStringToBOOL1(DWORD * pdw1)
{
  if(pdw1 == NULL)
    return 0L;

  DWORD dwRet = (isTrueNumericValue((LPSTR)(*pdw1)) ? (fTNV1) : 0);

  *pdw1 = (DWORD)convertStringToBOOL((LPSTR)(*pdw1));

  return dwRet;
}

static NPReason convertStringToNPReason(LPSTR szString)
{
  if(szString == NULL)
    return NPRES_DONE;

  if(isTrueNumericValue(szString))
  {
    char szPrefix[] = PROFILE_NUMERIC_PREFIX;
    return (NPReason)atol(szString + strlen(szPrefix));
  }
  else
  {
    if(stricmp(ENTRY_NPRES_DONE, szString) == 0)
      return NPRES_DONE;
    else if(stricmp(ENTRY_NPRES_NETWORK_ERR, szString) == 0)
      return NPRES_NETWORK_ERR;
    else if(stricmp(ENTRY_NPRES_USER_BREAK, szString) == 0)
      return NPRES_USER_BREAK;
    else
      return NPRES_DONE;
  }
}

DWORD convertStringToNPReason1(DWORD * pdw1)
{
  if(pdw1 == NULL)
    return 0L;

  *pdw1 = (DWORD)convertStringToNPReason((LPSTR)(*pdw1));

  return 0L;
}

static NPNVariable convertStringToNPNVariable(LPSTR szString)
{
  if(szString == NULL)
    return (NPNVariable)0;

  if(isTrueNumericValue(szString))
  {
    char szPrefix[] = PROFILE_NUMERIC_PREFIX;
    return (NPNVariable)atol(szString + strlen(szPrefix));
  }
  else
  {
    if(stricmp(ENTRY_NPNVXDISPLAY, szString) == 0)
      return NPNVxDisplay;
    else if(stricmp(ENTRY_NPNVXTAPPCONTEXT, szString) == 0)
      return NPNVxtAppContext;
    else if(stricmp(ENTRY_NPNVNETSCAPEWINDOW, szString) == 0)
      return NPNVnetscapeWindow;
    else if(stricmp(ENTRY_NPNVJAVASCRIPTENABLEDBOOL, szString) == 0)
      return NPNVjavascriptEnabledBool;
    else if(stricmp(ENTRY_NPNVASDENABLEDBOOL, szString) == 0)
      return NPNVasdEnabledBool;
    else if(stricmp(ENTRY_NPNVISOFFLINEBOOL, szString) == 0)
      return NPNVisOfflineBool;
    else
      return (NPNVariable)0;
  }
}

DWORD convertStringToNPNVariable1(DWORD * pdw1)
{
  if(pdw1 == NULL)
    return 0L;

  DWORD dwRet = (isTrueNumericValue((LPSTR)(*pdw1)) ? (fTNV1) : 0);
  
  *pdw1 = (DWORD)convertStringToNPNVariable((LPSTR)(*pdw1));

  return dwRet;
}

static NPPVariable convertStringToNPPVariable(LPSTR szString)
{
  if(szString == NULL)
    return (NPPVariable)0;

  if(isTrueNumericValue(szString))
  {
    char szPrefix[] = PROFILE_NUMERIC_PREFIX;
    return (NPPVariable)atol(szString + strlen(szPrefix));
  }
  else
  {
    if(stricmp(ENTRY_NPPVPLUGINNAMESTRING, szString) == 0)
      return NPPVpluginNameString;
    else if(stricmp(ENTRY_NPPVPLUGINDESCRIPTIONSTRING, szString) == 0)
      return NPPVpluginDescriptionString;
    else if(stricmp(ENTRY_NPPVPLUGINWINDOWBOOL, szString) == 0)
      return NPPVpluginWindowBool;
    else if(stricmp(ENTRY_NPPVPLUGINTRANSPARENTBOOL, szString) == 0)
      return NPPVpluginTransparentBool;
    else if(stricmp(ENTRY_NPPVPLUGINKEEPLIBRARYINMEMORY, szString) == 0)
      return NPPVpluginKeepLibraryInMemory;
    else if(stricmp(ENTRY_NPPVPLUGINWINDOWSIZE, szString) == 0)
      return NPPVpluginWindowSize;
    else
      return (NPPVariable)0;
  }
}

DWORD convertStringToNPPVariable1(DWORD * pdw1)
{
  if(pdw1 == NULL)
    return 0L;

  DWORD dwRet = (isTrueNumericValue((LPSTR)(*pdw1)) ? (fTNV1) : 0);

  *pdw1 = (DWORD)convertStringToNPPVariable((LPSTR)(*pdw1));

  return dwRet;
}

NPByteRange * convertStringToNPByteRangeList(LPSTR szString)
{
  NPByteRange **brNextFromPrev, *brList = 0;
  if(szString) {
    int offset = -1, len = -1;
    char *p = szString;
    while (EOF != sscanf((const char*)p, "%d-%d", &offset, &len)) {
      if (offset == -1 || len == -1)
        break;
      NPByteRange *brCurr = new NPByteRange;
      brCurr->offset = offset;
      brCurr->length = len;
      brCurr->next = 0;
      if (!brList)
        brList = brCurr;
      else
        *brNextFromPrev = brCurr;
      
      brNextFromPrev = &brCurr->next;
      
      if (!(p = strchr(p, ',')))
        break;
      while(*(++p) == ' '); 
    } 
  }
  return brList;
}