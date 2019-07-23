




































#include "windows.h"

DWORD GetPluginsDir(char * path, DWORD maxsize)
{
  if(!path)
    return 0;

  path[0] = '\0';

  DWORD res = GetModuleFileName(NULL, path, maxsize);
  if(res == 0)
    return 0;

  if(path[strlen(path) - 1] == '\\')
    path[lstrlen(path) - 1] = '\0';

  char *p = strrchr(path, '\\');

  if(p)
    *p = '\0';

  strcat(path, "\\plugins");

  res = strlen(path);
  return res;
}

HINSTANCE LoadRealPlugin(char * mimetype)
{
  if(!mimetype || !strlen(mimetype))
    return NULL;

  BOOL bDone = FALSE;
  WIN32_FIND_DATA ffdataStruct;

  char szPath[_MAX_PATH];
  char szFileName[_MAX_PATH];

  GetPluginsDir(szPath, _MAX_PATH);

  strcpy(szFileName, szPath);
  strcat(szFileName, "\\00*");

  HANDLE handle = FindFirstFile(szFileName, &ffdataStruct);
  if(handle == INVALID_HANDLE_VALUE) {
    FindClose(handle);
    return NULL;
  }

  DWORD versize = 0L;
  DWORD zero = 0L;
  char * verbuf = NULL;

  do {
    strcpy(szFileName, szPath);
    strcat(szFileName, "\\");
    strcat(szFileName, ffdataStruct.cFileName);
    if(!(ffdataStruct. dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      versize = GetFileVersionInfoSize(szFileName, &zero);
	    if (versize > 0)
		    verbuf = new char[versize];
      else 
        continue;

      if(!verbuf)
		    continue;

      GetFileVersionInfo(szFileName, NULL, versize, verbuf);

      char *mimetypes = NULL;
      UINT len = 0;

      if(!VerQueryValue(verbuf, "\\StringFileInfo\\040904E4\\MIMEType", (void **)&mimetypes, &len)
         || !mimetypes || !len) {
        delete [] verbuf;
        continue;
      }

      
      mimetypes[len] = '\0';
      char * type = mimetypes;

      BOOL more = TRUE;
      while(more) {
        char * p = strchr(type, '|');
        if(p)
          *p = '\0';
        else
          more = FALSE;

        if(0 == stricmp(mimetype, type)) {
          
          delete [] verbuf;
          FindClose(handle);
          HINSTANCE hLib = LoadLibrary(szFileName);
          return hLib;
        }

        type = p;
        type++;
      }

      delete [] verbuf;
    }

  } while(FindNextFile(handle, &ffdataStruct));

  FindClose(handle);
  return NULL;
}

void UnloadRealPlugin(HINSTANCE hLib)
{
  if(!hLib)
    FreeLibrary(hLib);
}
