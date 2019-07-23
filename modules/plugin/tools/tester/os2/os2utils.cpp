




































#define INCL_DOSMODULEMGR
#include <os2.h>

#include "plugbase.h"

void GetModulePath(HMODULE aInstance, char * aPath, int aSize)
{
  char sz[_MAX_PATH];
  DosQueryModuleName(aInstance, sizeof(sz), sz);
  char * p = strrchr(sz, '\\');
  if(p != NULL) {
    *++p = '\0';
    if((int)strlen(sz) < aSize) 
      strcpy(aPath, sz);
    else
      strcpy(aPath, "");
  }
  else
    strcpy(aPath, "");
}

static char szINIFile[] = NPAPI_INI_FILE_NAME;

void GetINIFileName(HMODULE aInstance, char * aName, int aSize)
{
  GetModulePath(aInstance, aName, aSize);
  strcat(aName, szINIFile);
}
