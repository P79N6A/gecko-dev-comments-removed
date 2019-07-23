




































#include <windows.h>

#include "plugbase.h"

void GetModulePath(HINSTANCE aInstance, char * aPath, int aSize)
{
  char sz[_MAX_PATH];
  GetModuleFileName(aInstance, sz, sizeof(sz));
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

void GetINIFileName(HINSTANCE aInstance, char * aName, int aSize)
{
  GetModulePath(aInstance, aName, aSize);
  strcat(aName, szINIFile);
}
