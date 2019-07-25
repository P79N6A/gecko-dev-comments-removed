






































#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_X64))

#define DHW_IMPLEMENT_GLOBALS
#include <stdio.h>
#include "prtypes.h"
#include "prprf.h"
#include "prlog.h"
#include "plstr.h"
#include "prlock.h"
#include "nscore.h"
#include "nsDebugHelpWin32.h"
#else
#error "nsDebugHelpWin32.cpp should only be built in Win32 x86/x64 builds"
#endif






PRLock*           DHWImportHooker::gLock  = nsnull;
DHWImportHooker*  DHWImportHooker::gHooks = nsnull;
GETPROCADDRESS    DHWImportHooker::gRealGetProcAddress = nsnull;


static bool
dhwEnsureImageHlpInitialized()
{
  static bool gInitialized = false;
  static bool gTried       = false;

  if (!gInitialized && !gTried) {
    gTried = PR_TRUE;
    HMODULE module = ::LoadLibrary("DBGHELP.DLL");
    if (!module) {
      DWORD dw = GetLastError();
      printf("DumpStack Error: DBGHELP.DLL wasn't found. GetLastError() returned 0x%8.8X\n"
             "                 This DLL is needed for succeessfully implementing trace-malloc.\n"
             "                 This dll ships by default on Win2k. Disabling trace-malloc functionality.\n"
             , dw);
      return PR_FALSE;
    }

#define INIT_PROC(typename_, name_) \
    dhw##name_ = (typename_) ::GetProcAddress(module, #name_); \
    if(!dhw##name_) return PR_FALSE;

#ifdef _WIN64
    INIT_PROC(ENUMERATELOADEDMODULES64, EnumerateLoadedModules64);
#else
    INIT_PROC(ENUMERATELOADEDMODULES, EnumerateLoadedModules);
#endif
    INIT_PROC(IMAGEDIRECTORYENTRYTODATA, ImageDirectoryEntryToData);

#undef INIT_PROC

    gInitialized = PR_TRUE;
  }

  return gInitialized;
} 


DHWImportHooker&
DHWImportHooker::getGetProcAddressHooker()
{
  static DHWImportHooker gGetProcAddress("Kernel32.dll", "GetProcAddress",
                                           (PROC)DHWImportHooker::GetProcAddress);
  return gGetProcAddress;
}
 

DHWImportHooker&
DHWImportHooker::getLoadLibraryWHooker()
{
  static DHWImportHooker gLoadLibraryW("Kernel32.dll", "LoadLibraryW",
                                         (PROC)DHWImportHooker::LoadLibraryW);
  return gLoadLibraryW;
}

DHWImportHooker&
DHWImportHooker::getLoadLibraryExWHooker()
{
  static DHWImportHooker gLoadLibraryExW("Kernel32.dll", "LoadLibraryExW",
                                         (PROC)DHWImportHooker::LoadLibraryExW);
  return gLoadLibraryExW;
}

DHWImportHooker&
DHWImportHooker::getLoadLibraryAHooker()
{
  static DHWImportHooker gLoadLibraryA("Kernel32.dll", "LoadLibraryA",
                                         (PROC)DHWImportHooker::LoadLibraryA);
  return gLoadLibraryA;
}

DHWImportHooker&
DHWImportHooker::getLoadLibraryExAHooker()
{
  static DHWImportHooker gLoadLibraryExA("Kernel32.dll", "LoadLibraryExA",
                                           (PROC)DHWImportHooker::LoadLibraryExA);
  return gLoadLibraryExA;
}


static HMODULE ThisModule()
{
    MEMORY_BASIC_INFORMATION info;
    return VirtualQuery(ThisModule, &info, sizeof(info)) ? 
                            (HMODULE) info.AllocationBase : nsnull;
}

DHWImportHooker::DHWImportHooker(const char* aModuleName,
                                 const char* aFunctionName,
                                 PROC aHook,
                                 bool aExcludeOurModule )
    :   mNext(nsnull),
        mModuleName(aModuleName),
        mFunctionName(aFunctionName),
        mOriginal(nsnull),
        mHook(aHook),
        mIgnoreModule(aExcludeOurModule ? ThisModule() : nsnull),
        mHooking(PR_TRUE)
{
    

    if(!gLock)
        gLock = PR_NewLock();
    PR_Lock(gLock);

    dhwEnsureImageHlpInitialized(); 

    if(!gRealGetProcAddress)
        gRealGetProcAddress = ::GetProcAddress;

    mOriginal = gRealGetProcAddress(::GetModuleHandleA(aModuleName), 
                                    aFunctionName),
 
    mNext = gHooks;
    gHooks = this;

    PatchAllModules();

    PR_Unlock(gLock);
}   

DHWImportHooker::~DHWImportHooker()
{
    PR_Lock(gLock);

    mHooking = PR_FALSE;
    PatchAllModules();

    for (DHWImportHooker **cur = &gHooks;
         (PR_ASSERT(*cur), *cur); 
         cur = &(*cur)->mNext)
    {
        if (*cur == this)
        {
            *cur = mNext;
            break;
        }
    }

    if(!gHooks)
    {
        PRLock* theLock = gLock;
        gLock = nsnull;
        PR_Unlock(theLock);
        PR_DestroyLock(theLock);
    }
    if (gLock)
        PR_Unlock(gLock);
}    

#ifdef _WIN64
static BOOL CALLBACK ModuleEnumCallback(PCSTR ModuleName,
                                        DWORD64 ModuleBase,
                                        ULONG ModuleSize,
                                        PVOID UserContext)
#else
static BOOL CALLBACK ModuleEnumCallback(PCSTR ModuleName,
                                        ULONG ModuleBase,
                                        ULONG ModuleSize,
                                        PVOID UserContext)
#endif
{
    
    DHWImportHooker* self = (DHWImportHooker*) UserContext;
    HMODULE aModule = (HMODULE) ModuleBase;
    return self->PatchOneModule(aModule, ModuleName);
}

bool 
DHWImportHooker::PatchAllModules()
{
    
    
    
    
#ifdef _WIN64
    return dhwEnumerateLoadedModules64(::GetCurrentProcess(),
               (PENUMLOADED_MODULES_CALLBACK64)ModuleEnumCallback, this);
#else
    return dhwEnumerateLoadedModules(::GetCurrentProcess(), 
               (PENUMLOADED_MODULES_CALLBACK)ModuleEnumCallback, this);
#endif
}    
                                
bool 
DHWImportHooker::PatchOneModule(HMODULE aModule, const char* name)
{
    if(aModule == mIgnoreModule)
    {
        return PR_TRUE;
    }

    

    PIMAGE_IMPORT_DESCRIPTOR desc;
    uint32 size;

    desc = (PIMAGE_IMPORT_DESCRIPTOR) 
        dhwImageDirectoryEntryToData(aModule, PR_TRUE, 
                                     IMAGE_DIRECTORY_ENTRY_IMPORT, &size);

    if(!desc)
    {
        return PR_TRUE;
    }

    for(; desc->Name; desc++)
    {
        const char* entryModuleName = (const char*)
            ((char*)aModule + desc->Name);
        if(!lstrcmpi(entryModuleName, mModuleName))
            break;
    }

    if(!desc->Name)
    {
        return PR_TRUE;
    }

    PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)
        ((char*) aModule + desc->FirstThunk);

    for(; thunk->u1.Function; thunk++)
    {
        PROC original;
        PROC replacement;
        
        if(mHooking)
        {
            original = mOriginal;
            replacement = mHook;  
        } 
        else
        {
            original = mHook;  
            replacement = mOriginal;
        }   

        PROC* ppfn = (PROC*) &thunk->u1.Function;
        if(*ppfn == original)
        {
            DWORD dwDummy;
            VirtualProtect(ppfn, sizeof(ppfn), PAGE_EXECUTE_READWRITE, &dwDummy);
            BOOL result = WriteProcessMemory(GetCurrentProcess(), 
                               ppfn, &replacement, sizeof(replacement), nsnull);
            if (!result) 
            {
              printf("failure name %s  func %x\n",name,*ppfn);
              DWORD error = GetLastError();
              return PR_TRUE;
            }
            else
            {
              
              DWORD filler = result+1;
              return result;
            }
        }

    }
    return PR_TRUE;
}

bool 
DHWImportHooker::ModuleLoaded(HMODULE aModule, DWORD flags)
{
    
    if(aModule && !(flags & LOAD_LIBRARY_AS_DATAFILE))
    {
        PR_Lock(gLock);
        
        
        for(DHWImportHooker* cur = gHooks; cur; cur = cur->mNext)
            cur->PatchAllModules();
        PR_Unlock(gLock);
    }
    return PR_TRUE;
}


HMODULE WINAPI 
DHWImportHooker::LoadLibraryW(PCWSTR path)
{
    
    DHW_DECLARE_FUN_TYPE(HMODULE, __stdcall, LOADLIBRARYW_, (PCWSTR));
    HMODULE hmod = DHW_ORIGINAL(LOADLIBRARYW_, getLoadLibraryWHooker())(path);
    ModuleLoaded(hmod, 0);
    return hmod;
}



HMODULE WINAPI 
DHWImportHooker::LoadLibraryExW(PCWSTR path, HANDLE file, DWORD flags)
{
    
    DHW_DECLARE_FUN_TYPE(HMODULE, __stdcall, LOADLIBRARYEXW_, (PCWSTR, HANDLE, DWORD));
    HMODULE hmod = DHW_ORIGINAL(LOADLIBRARYEXW_, getLoadLibraryExWHooker())(path, file, flags);
    ModuleLoaded(hmod, flags);
    return hmod;
}    


HMODULE WINAPI 
DHWImportHooker::LoadLibraryA(PCSTR path)
{
    

    DHW_DECLARE_FUN_TYPE(HMODULE, __stdcall, LOADLIBRARYA_, (PCSTR));
    HMODULE hmod = DHW_ORIGINAL(LOADLIBRARYA_, getLoadLibraryAHooker())(path);
    ModuleLoaded(hmod, 0);
    return hmod;
}


HMODULE WINAPI 
DHWImportHooker::LoadLibraryExA(PCSTR path, HANDLE file, DWORD flags)
{
    
    DHW_DECLARE_FUN_TYPE(HMODULE, __stdcall, LOADLIBRARYEXA_, (PCSTR, HANDLE, DWORD));
    HMODULE hmod = DHW_ORIGINAL(LOADLIBRARYEXA_, getLoadLibraryExAHooker())(path, file, flags);
    ModuleLoaded(hmod, flags);
    return hmod;
}     

FARPROC WINAPI 
DHWImportHooker::GetProcAddress(HMODULE aModule, PCSTR aFunctionName)
{
    FARPROC pfn = gRealGetProcAddress(aModule, aFunctionName);
    
    if(pfn)
    {
        PR_Lock(gLock);
        for(DHWImportHooker* cur = gHooks; cur; cur = cur->mNext)
        {
            if(pfn == cur->mOriginal)
            {
                pfn = cur->mHook;
                break;
            }    
        }
        PR_Unlock(gLock);
    }
    return pfn;
}


