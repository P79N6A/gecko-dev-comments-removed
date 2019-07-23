







































#ifndef __nsDebugHelpWin32_h__
#define __nsDebugHelpWin32_h__

#if defined(_WIN32) && defined(_M_IX86)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <imagehlp.h>
  #include <crtdbg.h>
#else
  #error "nsDebugHelpWin32.h should only be included in Win32 x86 builds"
#endif








#define DHW_DECLARE_FUN_TYPE(retval_, conv_, typename_, args_) \
    typedef retval_ ( conv_ * typename_ ) args_ ;

#ifdef DHW_IMPLEMENT_GLOBALS
#define DHW_DECLARE_FUN_GLOBAL(typename_, name_) typename_ dhw##name_
#else
#define DHW_DECLARE_FUN_GLOBAL(typename_, name_) extern typename_ dhw##name_
#endif

#define DHW_DECLARE_FUN_PROTO(retval_, conv_, name_, args_) \
    retval_ conv_ name_ args_

#define DHW_DECLARE_FUN_STATIC_PROTO(retval_, name_, args_) \
    static retval_ conv_ name_ args_

#define DHW_DECLARE_FUN_TYPE_AND_PROTO(name_, retval_, conv_, typename_, args_) \
    DHW_DECLARE_FUN_TYPE(retval_, conv_, typename_, args_); \
    DHW_DECLARE_FUN_PROTO(retval_, conv_, name_, args_)

#define DHW_DECLARE_FUN_TYPE_AND_STATIC_PROTO(name_, retval_, conv_, typename_, args_) \
    DHW_DECLARE_FUN_TYPE(retval_, conv_, typename_, args_); \
    DHW_DECLARE_FUN_STATIC_PROTO(retval_, conv_, name_, args_)

#define DHW_DECLARE_FUN_TYPE_AND_GLOBAL(typename_, name_, retval_, conv_, args_) \
    DHW_DECLARE_FUN_TYPE(retval_, conv_, typename_, args_); \
    DHW_DECLARE_FUN_GLOBAL(typename_, name_)





#define DHW_DECLARE_ORIGINAL(type_, name_, hooker_) \
    type_ name_ = (type_) hooker_ . GetOriginalFunction()

#define DHW_DECLARE_ORIGINAL_PTR(type_, name_, hooker_) \
    type_ name_ = (type_) hooker_ -> GetOriginalFunction()

#define DHW_ORIGINAL(type_, hooker_) \
    ((type_) hooker_ . GetOriginalFunction())

#define DHW_ORIGINAL_PTR(type_, hooker_) \
    ((type_) hooker_ -> GetOriginalFunction())



#if 0
DHW_DECLARE_FUN_TYPE_AND_GLOBAL(SYMINITIALIZEPROC, SymInitialize, \
                                BOOL, __stdcall, (HANDLE, LPSTR, BOOL));

DHW_DECLARE_FUN_TYPE_AND_GLOBAL(SYMSETOPTIONS, SymSetOptions, \
                                DWORD, __stdcall, (DWORD));

DHW_DECLARE_FUN_TYPE_AND_GLOBAL(SYMGETOPTIONS, SymGetOptions, \
                                DWORD, __stdcall, ());

DHW_DECLARE_FUN_TYPE_AND_GLOBAL(SYMGETMODULEINFO, SymGetModuleInfo, \
                                BOOL, __stdcall, (HANDLE, DWORD, PIMAGEHLP_MODULE));

DHW_DECLARE_FUN_TYPE_AND_GLOBAL(SYMGETSYMFROMADDRPROC, SymGetSymFromAddr, \
                                BOOL, __stdcall, (HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL));

#endif

DHW_DECLARE_FUN_TYPE_AND_GLOBAL(ENUMERATELOADEDMODULES, EnumerateLoadedModules, \
                                BOOL, __stdcall, (HANDLE, PENUMLOADED_MODULES_CALLBACK, PVOID));

DHW_DECLARE_FUN_TYPE_AND_GLOBAL(IMAGEDIRECTORYENTRYTODATA, ImageDirectoryEntryToData, \
                                PVOID, __stdcall, (PVOID, BOOL, USHORT, PULONG));






































extern PRBool
dhwEnsureImageHlpInitialized();



DHW_DECLARE_FUN_TYPE(FARPROC, __stdcall, GETPROCADDRESS, (HMODULE, PCSTR));

class DHWImportHooker
{
public: 

    DHWImportHooker(const char* aModuleName,
                    const char* aFunctionName,
                    PROC aHook,
                    PRBool aExcludeOurModule = PR_FALSE);
                    
    ~DHWImportHooker();

    PROC GetOriginalFunction()  {return mOriginal;}

    PRBool PatchAllModules();
    PRBool PatchOneModule(HMODULE aModule, const char* name);
    static PRBool ModuleLoaded(HMODULE aModule, DWORD flags);


    
    
    
    

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
    PRBool           mHooking;

private:
    static PRLock* gLock;
    static DHWImportHooker* gHooks;
    static GETPROCADDRESS gRealGetProcAddress;
    
    static HMODULE WINAPI LoadLibraryW(PCWSTR path);
    static HMODULE WINAPI LoadLibraryExW(PCWSTR path, HANDLE file, DWORD flags);
    static HMODULE WINAPI LoadLibraryExA(PCSTR path, HANDLE file, DWORD flags);

    static FARPROC WINAPI GetProcAddress(HMODULE aModule, PCSTR aFunctionName);
};






#if 0 

class DHWAllocationSizeDebugHook
{
public:
    virtual PRBool AllocHook(size_t size) = 0;
    virtual PRBool ReallocHook(size_t size, size_t sizeOld) = 0;
    virtual PRBool FreeHook(size_t size) = 0;
};

extern PRBool dhwSetAllocationSizeDebugHook(DHWAllocationSizeDebugHook* hook);
extern PRBool dhwClearAllocationSizeDebugHook();


#endif 

#endif 
