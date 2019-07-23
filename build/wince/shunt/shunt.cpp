





































 
#include "include/mozce_shunt.h"
#include "time_conversions.h"
#include <stdlib.h>
#include "Windows.h"


void putenv_copy(const char *k, const char *v);





typedef struct MOZCE_SHUNT_SPECIAL_FOLDER_INFO
{
  int   nFolder;
  char *folderEnvName;
} MozceShuntSpecialFolderInfo;



#define CSIDL_DESKTOP            0x0000
#define CSIDL_PROGRAMS           0x0002      // \Windows\Start Menu\Programs
#define CSIDL_PERSONAL           0x0005
#define CSIDL_WINDOWS            0x0024      // \Windows
#define CSIDL_PROGRAM_FILES      0x0026      // \Program Files

#define CSIDL_APPDATA            0x001A      // NOT IN SHELLAPI.H header file
#define CSIDL_PROFILE            0x0028      // NOT IN SHELLAPI.H header file

MozceShuntSpecialFolderInfo mozceSpecialFoldersToEnvVars[] = {
  { CSIDL_APPDATA,  "APPDATA" },
  { CSIDL_PROGRAM_FILES, "ProgramFiles" },
  { CSIDL_WINDOWS,  "windir" },

  
  
  
  
  
  
  
  
  

  { NULL, NULL }
};

static void InitializeSpecialFolderEnvVars()
{
  MozceShuntSpecialFolderInfo *p = mozceSpecialFoldersToEnvVars;
  while ( p && p->nFolder && p->folderEnvName ) {
    WCHAR wPath[MAX_PATH];
    char  cPath[MAX_PATH];
    if ( SHGetSpecialFolderPath(NULL, wPath, p->nFolder, FALSE) )
      if ( 0 != WideCharToMultiByte(CP_ACP, 0, wPath, -1, cPath, MAX_PATH, 0, 0) )
        putenv_copy(p->folderEnvName, cPath);
    p++;
  }
}





char* strerror(int inErrno)
{
  return "Unknown Error";
}

int errno = 0;






unsigned short * _wgetcwd(unsigned short * dir, unsigned long size)
{
  unsigned short tmp[MAX_PATH] = {0};
  GetEnvironmentVariableW(L"CWD", tmp, size);
  if (tmp && tmp[0]) {
    if (wcslen(tmp) > size)
      return 0;
    if (!dir)
      dir = (unsigned short*)malloc(sizeof(unsigned short) * (wcslen(tmp) + 1));
    wcscpy(dir, tmp);
    return dir;
  }
  unsigned long i;
  if (!dir)
      dir = (unsigned short*)malloc(sizeof(unsigned short) * (MAX_PATH + 1));
  GetModuleFileName(GetModuleHandle (NULL), dir, MAX_PATH);
  for (i = _tcslen(dir); i && dir[i] != TEXT('\\'); i--) {}
  dir[i + 1] = TCHAR('\0');
  SetEnvironmentVariableW(L"CWD", dir);
  return dir;
}

unsigned short *_wfullpath( unsigned short *absPath, const unsigned short *relPath, unsigned long maxLength )
{
  if(absPath == NULL){
    absPath = (unsigned short *)malloc(maxLength*sizeof(unsigned short));
  }
  unsigned short cwd[MAX_PATH];
  if (NULL == _wgetcwd( cwd, MAX_PATH))
    return NULL;

  unsigned long len = wcslen(cwd);
  if(!(cwd[len-1] == TCHAR('/') || cwd[len-1] == TCHAR('\\'))&& len< maxLength){
    cwd[len] = TCHAR('\\');
    cwd[++len] = TCHAR('\0');
  }
  if(len+wcslen(relPath) < maxLength){
#if (_WIN32_WCE > 300)
    if ( 0 < CeGetCanonicalPathName(relPath[0] == L'\\'? relPath : 
                                                         wcscat(cwd,relPath), 
                                    absPath, maxLength, 0))
      return absPath;
#else
    #error Need CeGetCanonicalPathName to build.
    
#endif
  }
  return NULL;
}

int _wchdir(const WCHAR* path) {
  return SetEnvironmentVariableW(L"CWD", path);
}

int _unlink(const char *filename)
{
  unsigned short wname[MAX_PATH];
  
  MultiByteToWideChar(CP_ACP,
                      0,
                      filename,
                      strlen(filename)+1,
                      wname,
                      MAX_PATH );
  return DeleteFileW(wname)?0:-1;
}

void abort(void)
{
#if defined(DEBUG)
  DebugBreak();
#endif
  TerminateProcess((HANDLE) GetCurrentProcessId(), 3);
}





#define localeconv __not_supported_on_device_localeconv
#include <locale.h>
#undef localeconv

extern "C" {
  struct lconv * localeconv(void);
}

static struct lconv s_locale_conv =
  {
    ".",   
    ",",   
    "333", 
    "$",   
    "$",   
    "",    
    "",    
    "",    
    "+",   
    "-",   
    '2',   
    '2',   
    1,     
    1,     
    1,     
    1,     
    1,     
    1,     
  };

struct lconv * localeconv(void)
{
  return &s_locale_conv;
}






BOOL WINAPI DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
  
  switch( fdwReason ) 
  { 
    case DLL_PROCESS_ATTACH:
      
      
      InitializeSpecialFolderEnvVars();
      break;

    case DLL_THREAD_ATTACH:
      
      break;

    case DLL_THREAD_DETACH:
      
      break;

    case DLL_PROCESS_DETACH:
      
      break;
  }
  return TRUE;  
}
