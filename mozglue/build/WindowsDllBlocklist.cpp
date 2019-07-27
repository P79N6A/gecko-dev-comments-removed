




#include <windows.h>
#include <winternl.h>
#include <io.h>

#pragma warning( push )
#pragma warning( disable : 4275 4530 ) // See msvc-stl-wrapper.template.h
#include <map>
#pragma warning( pop )

#define MOZ_NO_MOZALLOC
#include "nsAutoPtr.h"

#include "nsWindowsDllInterceptor.h"
#include "mozilla/WindowsVersion.h"
#include "nsWindowsHelpers.h"

using namespace mozilla;

#define ALL_VERSIONS   ((unsigned long long)-1LL)





#define UNVERSIONED    ((unsigned long long)0LL)



#define MAKE_VERSION(a,b,c,d)\
  ((a##ULL << 48) + (b##ULL << 32) + (c##ULL << 16) + d##ULL)

struct DllBlockInfo {
  
  
  const char *name;

  
  
  
  
  
  
  
  
  
  
  
  unsigned long long maxVersion;

  enum {
    FLAGS_DEFAULT = 0,
    BLOCK_WIN8PLUS_ONLY = 1,
    BLOCK_XP_ONLY = 2,
    USE_TIMESTAMP = 4,
  } flags;
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

  
  {"babyfox.dll", ALL_VERSIONS},

  
  {"sprotector.dll", ALL_VERSIONS},

  
  {"qipcap.dll", MAKE_VERSION(7, 6, 815, 1)},

  
  { "mozdllblockingtest.dll", ALL_VERSIONS },
  { "mozdllblockingtest_versioned.dll", 0x0000000400000000ULL },

  
  { "mfflac.dll", ALL_VERSIONS },

  
  { "rlnx.dll", MAKE_VERSION(1, 3, 334, 9) },
  { "pmnx.dll", MAKE_VERSION(1, 3, 334, 9) },
  { "opnx.dll", MAKE_VERSION(1, 3, 334, 9) },
  { "prnx.dll", MAKE_VERSION(1, 3, 334, 9) },

  
  
  { "beid35cardlayer.dll", MAKE_VERSION(3, 5, 6, 6968) },

  
  { "bitguard.dll", ALL_VERSIONS },

  
  
  { "atkdx11disp.dll", ALL_VERSIONS },

  
  { "spvc32.dll", ALL_VERSIONS },

  
  { "fs_ccf_ni_umh32.dll", MAKE_VERSION(1, 42, 101, 0), DllBlockInfo::BLOCK_XP_ONLY },

  
  { "libinject.dll", UNVERSIONED },
  { "libinject2.dll", 0x537DDC93, DllBlockInfo::USE_TIMESTAMP },
  { "libredir2.dll", 0x5385B7ED, DllBlockInfo::USE_TIMESTAMP },

  
  { "rf-firefox-22.dll", ALL_VERSIONS },

  
  { "dtwxsvc.dll", 0x53153234, DllBlockInfo::USE_TIMESTAMP },

  
  { "activedetect32.dll", UNVERSIONED },
  { "activedetect64.dll", UNVERSIONED },
  { "windowsapihookdll32.dll", UNVERSIONED },
  { "windowsapihookdll64.dll", UNVERSIONED },

  
  { "rndlnpshimswf.dll", ALL_VERSIONS },
  { "rndlmainbrowserrecordplugin.dll", ALL_VERSIONS },

  
  { "ycwebcamerasource.ax", MAKE_VERSION(2, 0, 0, 1611) },

  
  { "vwcsource.ax", MAKE_VERSION(1, 5, 0, 0) },

  { nullptr, 0 }
};

#ifndef STATUS_DLL_NOT_FOUND
#define STATUS_DLL_NOT_FOUND ((DWORD)0xC0000135L)
#endif


#undef DEBUG_very_verbose

static const char kBlockedDllsParameter[] = "BlockedDllList=";
static const int kBlockedDllsParameterLen =
  sizeof(kBlockedDllsParameter) - 1;

static const char kBlocklistInitFailedParameter[] = "BlocklistInitFailed=1\n";
static const int kBlocklistInitFailedParameterLen =
  sizeof(kBlocklistInitFailedParameter) - 1;

static const char kUser32BeforeBlocklistParameter[] = "User32BeforeBlocklist=1\n";
static const int kUser32BeforeBlocklistParameterLen =
  sizeof(kUser32BeforeBlocklistParameter) - 1;

static DWORD sThreadLoadingXPCOMModule;
static bool sBlocklistInitFailed;
static bool sUser32BeforeBlocklist;


void
printf_stderr(const char *fmt, ...)
{
  if (IsDebuggerPresent()) {
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    buf[sizeof(buf) - 1] = '\0';
    va_end(args);
    OutputDebugStringA(buf);
  }

  FILE *fp = _fdopen(_dup(2), "a");
  if (!fp)
      return;

  va_list args;
  va_start(args, fmt);
  vfprintf(fp, fmt, args);
  va_end(args);

  fclose(fp);
}

namespace {

typedef NTSTATUS (NTAPI *LdrLoadDll_func) (PWCHAR filePath, PULONG flags, PUNICODE_STRING moduleFileName, PHANDLE handle);

static LdrLoadDll_func stub_LdrLoadDll = 0;

template <class T>
struct RVAMap {
  RVAMap(HANDLE map, DWORD offset) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    DWORD alignedOffset = (offset / info.dwAllocationGranularity) *
                          info.dwAllocationGranularity;

    MOZ_ASSERT(offset - alignedOffset < info.dwAllocationGranularity, "Wtf");

    mRealView = ::MapViewOfFile(map, FILE_MAP_READ, 0, alignedOffset,
                                sizeof(T) + (offset - alignedOffset));

    mMappedView = mRealView ? reinterpret_cast<T*>((char*)mRealView + (offset - alignedOffset)) :
                              nullptr;
  }
  ~RVAMap() {
    if (mRealView) {
      ::UnmapViewOfFile(mRealView);
    }
  }
  operator const T*() const { return mMappedView; }
  const T* operator->() const { return mMappedView; }
private:
  const T* mMappedView;
  void* mRealView;
};

bool
CheckASLR(const wchar_t* path)
{
  bool retval = false;

  HANDLE file = ::CreateFileW(path, GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
  if (file != INVALID_HANDLE_VALUE) {
    HANDLE map = ::CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0,
                                      nullptr);
    if (map) {
      RVAMap<IMAGE_DOS_HEADER> peHeader(map, 0);
      if (peHeader) {
        RVAMap<IMAGE_NT_HEADERS> ntHeader(map, peHeader->e_lfanew);
        if (ntHeader) {
          
          if (ntHeader->OptionalHeader.SizeOfCode == 0) {
            retval = true;
          }
          
          else if ((ntHeader->OptionalHeader.DllCharacteristics &
                    IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE) != 0) {
            retval = true;
          }
        }
      }
      ::CloseHandle(map);
    }
    ::CloseHandle(file);
  }

  return retval;
}

DWORD
GetTimestamp(const wchar_t* path)
{
  DWORD timestamp = 0;

  HANDLE file = ::CreateFileW(path, GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
  if (file != INVALID_HANDLE_VALUE) {
    HANDLE map = ::CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0,
                                      nullptr);
    if (map) {
      RVAMap<IMAGE_DOS_HEADER> peHeader(map, 0);
      if (peHeader) {
        RVAMap<IMAGE_NT_HEADERS> ntHeader(map, peHeader->e_lfanew);
        if (ntHeader) {
          timestamp = ntHeader->FileHeader.TimeDateStamp;
        }
      }
      ::CloseHandle(map);
    }
    ::CloseHandle(file);
  }

  return timestamp;
}



static CRITICAL_SECTION sLock;











class ReentrancySentinel
{
public:
  explicit ReentrancySentinel(const char* dllName)
  {
    DWORD currentThreadId = GetCurrentThreadId();
    AutoCriticalSection lock(&sLock);
    mPreviousDllName = (*sThreadMap)[currentThreadId];

    
    
    mReentered = mPreviousDllName && !stricmp(mPreviousDllName, dllName);
    (*sThreadMap)[currentThreadId] = dllName;
  }
    
  ~ReentrancySentinel()
  {
    DWORD currentThreadId = GetCurrentThreadId();
    AutoCriticalSection lock(&sLock);
    (*sThreadMap)[currentThreadId] = mPreviousDllName;
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
  static std::map<DWORD, const char*>* sThreadMap;

  const char* mPreviousDllName;
  bool mReentered;
};

std::map<DWORD, const char*>* ReentrancySentinel::sThreadMap;






class DllBlockSet
{
public:
  static void Add(const char* name, unsigned long long version);

  
  
  static void Write(HANDLE file);

private:
  DllBlockSet(const char* name, unsigned long long version)
    : mName(name)
    , mVersion(version)
    , mNext(nullptr)
  {
  }

  const char* mName; 
  unsigned long long mVersion;
  DllBlockSet* mNext;

  static DllBlockSet* gFirst;
};

DllBlockSet* DllBlockSet::gFirst;

void
DllBlockSet::Add(const char* name, unsigned long long version)
{
  AutoCriticalSection lock(&sLock);
  for (DllBlockSet* b = gFirst; b; b = b->mNext) {
    if (0 == strcmp(b->mName, name) && b->mVersion == version) {
      return;
    }
  }
  
  DllBlockSet* n = new DllBlockSet(name, version);
  n->mNext = gFirst;
  gFirst = n;
}

void
DllBlockSet::Write(HANDLE file)
{
  AutoCriticalSection lock(&sLock);
  DWORD nBytes;

  
  
  MOZ_SEH_TRY {
    for (DllBlockSet* b = gFirst; b; b = b->mNext) {
      
      WriteFile(file, b->mName, strlen(b->mName), &nBytes, nullptr);
      if (b->mVersion != -1) {
        WriteFile(file, ",", 1, &nBytes, nullptr);
        uint16_t parts[4];
        parts[0] = b->mVersion >> 48;
        parts[1] = (b->mVersion >> 32) & 0xFFFF;
        parts[2] = (b->mVersion >> 16) & 0xFFFF;
        parts[3] = b->mVersion & 0xFFFF;
        for (int p = 0; p < 4; ++p) {
          char buf[32];
          ltoa(parts[p], buf, 10);
          WriteFile(file, buf, strlen(buf), &nBytes, nullptr);
          if (p != 3) {
            WriteFile(file, ".", 1, &nBytes, nullptr);
          }
        }
      }
      WriteFile(file, ";", 1, &nBytes, nullptr);
    }
  }
  MOZ_SEH_EXCEPT (EXCEPTION_EXECUTE_HANDLER) { }
}

static
wchar_t* getFullPath (PWCHAR filePath, wchar_t* fname)
{
  
  
  
  
  PWCHAR sanitizedFilePath = (intptr_t(filePath) < 1024) ? nullptr : filePath;

  
  DWORD pathlen = SearchPathW(sanitizedFilePath, fname, L".dll", 0, nullptr,
                              nullptr);
  if (pathlen == 0) {
    return nullptr;
  }

  wchar_t* full_fname = new wchar_t[pathlen+1];
  if (!full_fname) {
    
    return nullptr;
  }

  
  SearchPathW(sanitizedFilePath, fname, L".dll", pathlen + 1, full_fname,
              nullptr);
  return full_fname;
}


static wchar_t* lastslash(wchar_t* s, int len)
{
  for (wchar_t* c = s + len - 1; c >= s; --c) {
    if (*c == L'\\' || *c == L'/') {
      return c;
    }
  }
  return nullptr;
}

static NTSTATUS NTAPI
patched_LdrLoadDll (PWCHAR filePath, PULONG flags, PUNICODE_STRING moduleFileName, PHANDLE handle)
{
  
#define DLLNAME_MAX 128
  char dllName[DLLNAME_MAX+1];
  wchar_t *dll_part;
  char *dot;
  DllBlockInfo *info;

  int len = moduleFileName->Length / 2;
  wchar_t *fname = moduleFileName->Buffer;
  nsAutoArrayPtr<wchar_t> full_fname;

  
  
  
  
  if (moduleFileName->MaximumLength < moduleFileName->Length+2 ||
      fname[len] != 0)
  {
#ifdef DEBUG
    printf_stderr("LdrLoadDll: non-null terminated string found!\n");
#endif
    goto continue_loading;
  }

  dll_part = lastslash(fname, len);
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

  
  
  dot = strchr(dllName, '.');
  if (dot && (strchr(dot+1, '.') == dot+13)) {
    char * end = nullptr;
    _strtoui64(dot+1, &end, 16);
    if (end == dot+13) {
      return STATUS_DLL_NOT_FOUND;
    }
  }
  
  if (dot && ((dot - dllName) >= 16)) {
    char * current = dllName;
    while (current < dot && isxdigit(*current)) {
      current++;
    }
    if (current == dot) {
      return STATUS_DLL_NOT_FOUND;
    }
  }

  
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

    if ((info->flags == DllBlockInfo::BLOCK_WIN8PLUS_ONLY) &&
        !IsWin8OrLater()) {
      goto continue_loading;
    }

    if ((info->flags == DllBlockInfo::BLOCK_XP_ONLY) &&
        IsWin2003OrLater()) {
      goto continue_loading;
    }

    unsigned long long fVersion = ALL_VERSIONS;

    if (info->maxVersion != ALL_VERSIONS) {
      ReentrancySentinel sentinel(dllName);
      if (sentinel.BailOut()) {
        goto continue_loading;
      }

      full_fname = getFullPath(filePath, fname);
      if (!full_fname) {
        
        printf_stderr("LdrLoadDll: Blocking load of '%s' (SearchPathW didn't find it?)\n", dllName);
        return STATUS_DLL_NOT_FOUND;
      }

      if (info->flags & DllBlockInfo::USE_TIMESTAMP) {
        fVersion = GetTimestamp(full_fname);
        if (fVersion > info->maxVersion) {
          load_ok = true;
        }
      } else {
        DWORD zero;
        DWORD infoSize = GetFileVersionInfoSizeW(full_fname, &zero);

        

        if (infoSize != 0) {
          nsAutoArrayPtr<unsigned char> infoData(new unsigned char[infoSize]);
          VS_FIXEDFILEINFO *vInfo;
          UINT vInfoLen;

          if (GetFileVersionInfoW(full_fname, 0, infoSize, infoData) &&
              VerQueryValueW(infoData, L"\\", (LPVOID*) &vInfo, &vInfoLen))
          {
            fVersion =
              ((unsigned long long)vInfo->dwFileVersionMS) << 32 |
              ((unsigned long long)vInfo->dwFileVersionLS);

            
            
            if (fVersion > info->maxVersion)
              load_ok = true;
          }
        }
      }
    }

    if (!load_ok) {
      printf_stderr("LdrLoadDll: Blocking load of '%s' -- see http://www.mozilla.com/en-US/blocklist/\n", dllName);
      DllBlockSet::Add(info->name, fVersion);
      return STATUS_DLL_NOT_FOUND;
    }
  }

continue_loading:
#ifdef DEBUG_very_verbose
  printf_stderr("LdrLoadDll: continuing load... ('%S')\n", moduleFileName->Buffer);
#endif

  if (GetCurrentThreadId() == sThreadLoadingXPCOMModule) {
    
    full_fname = getFullPath(filePath, fname);
    if (!full_fname) {
      
      printf_stderr("LdrLoadDll: Blocking load of '%s' (SearchPathW didn't find it?)\n", dllName);
      return STATUS_DLL_NOT_FOUND;
    }

    if (IsVistaOrLater() && !CheckASLR(full_fname)) {
      printf_stderr("LdrLoadDll: Blocking load of '%s'.  XPCOM components must support ASLR.\n", dllName);
      return STATUS_DLL_NOT_FOUND;
    }
  }

  return stub_LdrLoadDll(filePath, flags, moduleFileName, handle);
}

WindowsDllInterceptor NtDllIntercept;

} 

NS_EXPORT void
DllBlocklist_Initialize()
{
  if (GetModuleHandleA("user32.dll")) {
    sUser32BeforeBlocklist = true;
  }

  NtDllIntercept.Init("ntdll.dll");

  ReentrancySentinel::InitializeStatics();

  
  
  
  bool ok = NtDllIntercept.AddDetour("LdrLoadDll", reinterpret_cast<intptr_t>(patched_LdrLoadDll), (void**) &stub_LdrLoadDll);

  if (!ok) {
    sBlocklistInitFailed = true;
#ifdef DEBUG
    printf_stderr ("LdrLoadDll hook failed, no dll blocklisting active\n");
#endif
  }
}

NS_EXPORT void
DllBlocklist_SetInXPCOMLoadOnMainThread(bool inXPCOMLoadOnMainThread)
{
  if (inXPCOMLoadOnMainThread) {
    MOZ_ASSERT(sThreadLoadingXPCOMModule == 0, "Only one thread should be doing this");
    sThreadLoadingXPCOMModule = GetCurrentThreadId();
  } else {
    sThreadLoadingXPCOMModule = 0;
  }
}

NS_EXPORT void
DllBlocklist_WriteNotes(HANDLE file)
{
  DWORD nBytes;

  WriteFile(file, kBlockedDllsParameter, kBlockedDllsParameterLen, &nBytes, nullptr);
  DllBlockSet::Write(file);
  WriteFile(file, "\n", 1, &nBytes, nullptr);

  if (sBlocklistInitFailed) {
    WriteFile(file, kBlocklistInitFailedParameter,
              kBlocklistInitFailedParameterLen, &nBytes, nullptr);
  }

  if (sUser32BeforeBlocklist) {
    WriteFile(file, kUser32BeforeBlocklistParameter,
              kUser32BeforeBlocklistParameterLen, &nBytes, nullptr);
  }
}
