






































#ifndef nsStackFrameWin_h___
#define nsStackFrameWin_h___


#if defined(_WIN32) && defined(_M_IX86) 
#include "nspr.h"
#include <windows.h>
#include <imagehlp.h>







PR_BEGIN_EXTERN_C

typedef DWORD (__stdcall *SYMSETOPTIONSPROC)(DWORD);
extern SYMSETOPTIONSPROC _SymSetOptions;

typedef BOOL (__stdcall *SYMINITIALIZEPROC)(HANDLE, LPSTR, BOOL);
extern SYMINITIALIZEPROC _SymInitialize;

typedef BOOL (__stdcall *SYMCLEANUPPROC)(HANDLE);
extern SYMCLEANUPPROC _SymCleanup;

typedef BOOL (__stdcall *STACKWALKPROC)(DWORD,
                                        HANDLE,
                                        HANDLE,
                                        LPSTACKFRAME,
                                        LPVOID,
                                        PREAD_PROCESS_MEMORY_ROUTINE,
                                        PFUNCTION_TABLE_ACCESS_ROUTINE,
                                        PGET_MODULE_BASE_ROUTINE,
                                        PTRANSLATE_ADDRESS_ROUTINE);
extern  STACKWALKPROC _StackWalk;

typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESSPROC)(HANDLE, DWORD);
extern  SYMFUNCTIONTABLEACCESSPROC _SymFunctionTableAccess;

typedef DWORD (__stdcall *SYMGETMODULEBASEPROC)(HANDLE, DWORD);
extern  SYMGETMODULEBASEPROC _SymGetModuleBase;

typedef BOOL (__stdcall *SYMGETSYMFROMADDRPROC)(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);
extern  SYMGETSYMFROMADDRPROC _SymGetSymFromAddr;

typedef DWORD ( __stdcall *SYMLOADMODULE)(HANDLE, HANDLE, PSTR, PSTR, DWORD, DWORD);
extern  SYMLOADMODULE _SymLoadModule;

typedef DWORD ( __stdcall *SYMUNDNAME)(PIMAGEHLP_SYMBOL, PSTR, DWORD);
extern  SYMUNDNAME _SymUnDName;

typedef DWORD ( __stdcall *SYMGETMODULEINFO)( HANDLE, DWORD, PIMAGEHLP_MODULE);
extern  SYMGETMODULEINFO _SymGetModuleInfo;

typedef BOOL ( __stdcall *ENUMLOADEDMODULES)( HANDLE, PENUMLOADED_MODULES_CALLBACK, PVOID);
extern  ENUMLOADEDMODULES _EnumerateLoadedModules;

typedef BOOL (__stdcall *SYMGETLINEFROMADDRPROC)(HANDLE, DWORD, PDWORD, PIMAGEHLP_LINE);
extern  SYMGETLINEFROMADDRPROC _SymGetLineFromAddr;

PRBool EnsureSymInitialized();

PRBool EnsureImageHlpInitialized();











BOOL SymGetModuleInfoEspecial(HANDLE aProcess, DWORD aAddr, PIMAGEHLP_MODULE aModuleInfo, PIMAGEHLP_LINE aLineInfo);


void DumpStackToFile(FILE* out);

PR_END_EXTERN_C

#endif 

#endif 


