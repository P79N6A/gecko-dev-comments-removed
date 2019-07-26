






#ifndef __nsDebugHelpWin32_h__
#define __nsDebugHelpWin32_h__

#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_X64))
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <imagehlp.h>
  #include <crtdbg.h>
#else
  #error "nsDebugHelpWin32.h should only be included in Win32 x86/x64 builds"
#endif








#ifdef DHW_IMPLEMENT_GLOBALS
#define DHW_DECLARE_FUN_GLOBAL(name_) decltype(name_)* dhw##name_
#else
#define DHW_DECLARE_FUN_GLOBAL(name_) extern decltype(name_)* dhw##name_
#endif





#define DHW_ORIGINAL(name_, hooker_) \
    ((decltype(name_)*) hooker_ . GetOriginalFunction())




#ifndef _WIN64
DHW_DECLARE_FUN_GLOBAL(EnumerateLoadedModules);
#else
DHW_DECLARE_FUN_GLOBAL(EnumerateLoadedModules64);
#endif

DHW_DECLARE_FUN_GLOBAL(ImageDirectoryEntryToData);



extern bool
dhwEnsureImageHlpInitialized();



class DHWImportHooker
{
public: 

    DHWImportHooker(const char* aModuleName,
                    const char* aFunctionName,
                    PROC aHook,
                    bool aExcludeOurModule = false);
                    
    ~DHWImportHooker();

    PROC GetOriginalFunction()  {return mOriginal;}

    bool PatchAllModules();
    bool PatchOneModule(HMODULE aModule, const char* name);
    static bool ModuleLoaded(HMODULE aModule, DWORD flags);


    
    
    
    

    static DHWImportHooker &getLoadLibraryWHooker();
    static DHWImportHooker &getLoadLibraryExWHooker();
    static DHWImportHooker &getLoadLibraryAHooker();
    static DHWImportHooker &getLoadLibraryExAHooker();
    static DHWImportHooker &getGetProcAddressHooker();

    static HMODULE WINAPI LoadLibraryA(PCSTR path);

private:
    DHWImportHooker* mNext;
    const char*      mModuleName;
    const char*      mFunctionName;
    PROC             mOriginal;
    PROC             mHook;
    HMODULE          mIgnoreModule;
    bool             mHooking;

private:
    static PRLock* gLock;
    static DHWImportHooker* gHooks;
    static decltype(GetProcAddress)* gRealGetProcAddress;
    
    static HMODULE WINAPI LoadLibraryW(PCWSTR path);
    static HMODULE WINAPI LoadLibraryExW(PCWSTR path, HANDLE file, DWORD flags);
    static HMODULE WINAPI LoadLibraryExA(PCSTR path, HANDLE file, DWORD flags);

    static FARPROC WINAPI GetProcAddress(HMODULE aModule, PCSTR aFunctionName);
};






#if 0 

class DHWAllocationSizeDebugHook
{
public:
    virtual bool AllocHook(size_t size) = 0;
    virtual bool ReallocHook(size_t size, size_t sizeOld) = 0;
    virtual bool FreeHook(size_t size) = 0;
};

extern bool dhwSetAllocationSizeDebugHook(DHWAllocationSizeDebugHook* hook);
extern bool dhwClearAllocationSizeDebugHook();


#endif 

#endif 
