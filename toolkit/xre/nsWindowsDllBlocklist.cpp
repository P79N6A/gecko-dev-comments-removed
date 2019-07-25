




































#include <windows.h>
#include <winternl.h>

#include <stdio.h>
#include <string.h>

#include <map>

#ifdef XRE_WANT_DLL_BLOCKLIST
#define XRE_SetupDllBlocklist SetupDllBlocklist
#else
#include "nsXULAppAPI.h"
#endif

#include "nsAutoPtr.h"

#include "prlog.h"

#include "nsWindowsDllInterceptor.h"

#if defined(MOZ_CRASHREPORTER) && !defined(NO_BLOCKLIST_CRASHREPORTER)
#include "nsExceptionHandler.h"
#endif

#define ALL_VERSIONS   ((unsigned long long)-1LL)





#define UNVERSIONED    ((unsigned long long)0LL)



#define MAKE_VERSION(a,b,c,d)\
  ((a##ULL << 48) + (b##ULL << 32) + (c##ULL << 16) + d##ULL)

struct DllBlockInfo {
  
  
  const char *name;

  
  
  
  
  
  
  
  
  unsigned long long maxVersion;
};

static DllBlockInfo sWindowsDllBlocklist[] = {
  
  
  
  
  
  
  { "npffaddon.dll", ALL_VERSIONS},

  
  {"avgrsstx.dll", MAKE_VERSION(8,5,0,401)},
  
  
  {"calc.dll", MAKE_VERSION(1,0,0,1)},

  
  {"hook.dll", ALL_VERSIONS},
  
  
  
  {"googledesktopnetwork3.dll", UNVERSIONED},

  
  {"rdolib.dll", MAKE_VERSION(6,0,88,4)},

  
  {"fgjk4wvb.dll", MAKE_VERSION(8,8,8,8)},
  
  
  {"radhslib.dll", UNVERSIONED},

  
  
  {"vksaver.dll", MAKE_VERSION(2,2,2,0)},

  
  {"rlxf.dll", MAKE_VERSION(1,2,323,1)},

  
  
  {"psicon.dll", ALL_VERSIONS},

  
  {"accelerator.dll", MAKE_VERSION(3,2,1,6)},

  
  {"rf-firefox.dll", MAKE_VERSION(7,6,1,0)},
  {"roboform.dll", MAKE_VERSION(7,6,1,0)},

  
  { "mozdllblockingtest.dll", ALL_VERSIONS },
  { "mozdllblockingtest_versioned.dll", 0x0000000400000000ULL },

  { NULL, 0 }
};

#ifndef STATUS_DLL_NOT_FOUND
#define STATUS_DLL_NOT_FOUND ((DWORD)0xC0000135L)
#endif


#undef DEBUG_very_verbose

namespace {

typedef NTSTATUS (NTAPI *LdrLoadDll_func) (PWCHAR filePath, PULONG flags, PUNICODE_STRING moduleFileName, PHANDLE handle);

static LdrLoadDll_func stub_LdrLoadDll = 0;











class ReentrancySentinel
{
public:
  explicit ReentrancySentinel(const char* dllName)
  {
    DWORD currentThreadId = GetCurrentThreadId();
    EnterCriticalSection(&sLock);
    mPreviousDllName = (*sThreadMap)[currentThreadId];

    
    
    mReentered = mPreviousDllName && !stricmp(mPreviousDllName, dllName);
    (*sThreadMap)[currentThreadId] = dllName;
    LeaveCriticalSection(&sLock);
  }
    
  ~ReentrancySentinel()
  {
    DWORD currentThreadId = GetCurrentThreadId();
    EnterCriticalSection(&sLock);
    (*sThreadMap)[currentThreadId] = mPreviousDllName;
    LeaveCriticalSection(&sLock);
  }

  bool BailOut() const
  {
    return mReentered;
  };
    
  static void InitializeStatics()
  {
    InitializeCriticalSection(&sLock);
    sThreadMap = new std::map<DWORD, const char*>;
  }

private:
  static CRITICAL_SECTION sLock;
  static std::map<DWORD, const char*>* sThreadMap;

  const char* mPreviousDllName;
  bool mReentered;
};

CRITICAL_SECTION ReentrancySentinel::sLock;
std::map<DWORD, const char*>* ReentrancySentinel::sThreadMap;

static NTSTATUS NTAPI
patched_LdrLoadDll (PWCHAR filePath, PULONG flags, PUNICODE_STRING moduleFileName, PHANDLE handle)
{
  
#define DLLNAME_MAX 128
  char dllName[DLLNAME_MAX+1];
  wchar_t *dll_part;
  DllBlockInfo *info;

  int len = moduleFileName->Length / 2;
  wchar_t *fname = moduleFileName->Buffer;

  
  
  
  
  if (moduleFileName->MaximumLength < moduleFileName->Length+2 ||
      fname[len] != 0)
  {
#ifdef DEBUG
    printf_stderr("LdrLoadDll: non-null terminated string found!\n");
#endif
    goto continue_loading;
  }

  dll_part = wcsrchr(fname, L'\\');
  if (dll_part) {
    dll_part = dll_part + 1;
    len -= dll_part - fname;
  } else {
    dll_part = fname;
  }

#ifdef DEBUG_very_verbose
  printf_stderr("LdrLoadDll: dll_part '%S' %d\n", dll_part, len);
#endif

  
  
  
  if (len > DLLNAME_MAX) {
#ifdef DEBUG
    printf_stderr("LdrLoadDll: len too long! %d\n", len);
#endif
    goto continue_loading;
  }

  
  for (int i = 0; i < len; i++) {
    wchar_t c = dll_part[i];

    if (c > 0x7f) {
      
      
      goto continue_loading;
    }

    
    if (c >= 'A' && c <= 'Z')
      c += 'a' - 'A';

    dllName[i] = (char) c;
  }

  dllName[len] = 0;

#ifdef DEBUG_very_verbose
  printf_stderr("LdrLoadDll: dll name '%s'\n", dllName);
#endif

  
  info = &sWindowsDllBlocklist[0];
  while (info->name) {
    if (strcmp(info->name, dllName) == 0)
      break;

    info++;
  }

  if (info->name) {
    bool load_ok = false;

#ifdef DEBUG_very_verbose
    printf_stderr("LdrLoadDll: info->name: '%s'\n", info->name);
#endif

    if (info->maxVersion != ALL_VERSIONS) {
      ReentrancySentinel sentinel(dllName);
      if (sentinel.BailOut()) {
        goto continue_loading;
      }

      
      
      
      
      PWCHAR sanitizedFilePath = (intptr_t(filePath) < 1024) ? NULL : filePath;

      
      DWORD pathlen = SearchPathW(sanitizedFilePath, fname, L".dll", 0, NULL, NULL);
      if (pathlen == 0) {
        
        printf_stderr("LdrLoadDll: Blocking load of '%s' (SearchPathW didn't find it?)\n", dllName);
        return STATUS_DLL_NOT_FOUND;
      }

      wchar_t *full_fname = (wchar_t*) malloc(sizeof(wchar_t)*(pathlen+1));
      if (!full_fname) {
        
        return STATUS_DLL_NOT_FOUND;
      }

      
      SearchPathW(sanitizedFilePath, fname, L".dll", pathlen+1, full_fname, NULL);

      DWORD zero;
      DWORD infoSize = GetFileVersionInfoSizeW(full_fname, &zero);

      

      if (infoSize != 0) {
        nsAutoArrayPtr<unsigned char> infoData(new unsigned char[infoSize]);
        VS_FIXEDFILEINFO *vInfo;
        UINT vInfoLen;

        if (GetFileVersionInfoW(full_fname, 0, infoSize, infoData) &&
            VerQueryValueW(infoData, L"\\", (LPVOID*) &vInfo, &vInfoLen))
        {
          unsigned long long fVersion =
            ((unsigned long long)vInfo->dwFileVersionMS) << 32 |
            ((unsigned long long)vInfo->dwFileVersionLS);

          
          
          if (fVersion > info->maxVersion)
            load_ok = true;
        }
      }

      free(full_fname);
    }

    if (!load_ok) {
      printf_stderr("LdrLoadDll: Blocking load of '%s' -- see http://www.mozilla.com/en-US/blocklist/\n", dllName);
      return STATUS_DLL_NOT_FOUND;
    }
  }

continue_loading:
#ifdef DEBUG_very_verbose
  printf_stderr("LdrLoadDll: continuing load... ('%S')\n", moduleFileName->Buffer);
#endif

  NS_SetHasLoadedNewDLLs();

  return stub_LdrLoadDll(filePath, flags, moduleFileName, handle);
}

WindowsDllInterceptor NtDllIntercept;

} 

void
XRE_SetupDllBlocklist()
{
  NtDllIntercept.Init("ntdll.dll");

  ReentrancySentinel::InitializeStatics();

  bool ok = NtDllIntercept.AddHook("LdrLoadDll", reinterpret_cast<intptr_t>(patched_LdrLoadDll), (void**) &stub_LdrLoadDll);

#ifdef DEBUG
  if (!ok)
    printf_stderr ("LdrLoadDll hook failed, no dll blocklisting active\n");
#endif

#if defined(MOZ_CRASHREPORTER) && !defined(NO_BLOCKLIST_CRASHREPORTER)
  if (!ok) {
    CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING("DllBlockList Failed\n"));
  }
#endif
}
