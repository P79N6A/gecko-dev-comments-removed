







































#include "nscore.h"
#include "windows.h"
#include "imagehlp.h"
#include "stdio.h"
#include "nsStackFrameWin.h"









PR_BEGIN_EXTERN_C

SYMSETOPTIONSPROC _SymSetOptions;

SYMINITIALIZEPROC _SymInitialize;

SYMCLEANUPPROC _SymCleanup;

STACKWALKPROC _StackWalk;

SYMFUNCTIONTABLEACCESSPROC _SymFunctionTableAccess;

SYMGETMODULEBASEPROC _SymGetModuleBase;

SYMGETSYMFROMADDRPROC _SymGetSymFromAddr;

SYMLOADMODULE _SymLoadModule;

SYMUNDNAME _SymUnDName;

SYMGETMODULEINFO _SymGetModuleInfo;

ENUMLOADEDMODULES _EnumerateLoadedModules;

SYMGETLINEFROMADDRPROC _SymGetLineFromAddr;

PR_END_EXTERN_C




PRBool
EnsureImageHlpInitialized()
{
  static PRBool gInitialized = PR_FALSE;

  if (! gInitialized) {
    HMODULE module = ::LoadLibrary("IMAGEHLP.DLL");
    if (!module) return PR_FALSE;

    _SymSetOptions = (SYMSETOPTIONSPROC) ::GetProcAddress(module, "SymSetOptions");
    if (!_SymSetOptions) return PR_FALSE;

    _SymInitialize = (SYMINITIALIZEPROC) ::GetProcAddress(module, "SymInitialize");
    if (!_SymInitialize) return PR_FALSE;

    _SymCleanup = (SYMCLEANUPPROC)GetProcAddress(module, "SymCleanup");
    if (!_SymCleanup) return PR_FALSE;

    _StackWalk = (STACKWALKPROC)GetProcAddress(module, "StackWalk");
    if (!_StackWalk) return PR_FALSE;

    _SymFunctionTableAccess = (SYMFUNCTIONTABLEACCESSPROC) GetProcAddress(module, "SymFunctionTableAccess");
    if (!_SymFunctionTableAccess) return PR_FALSE;

    _SymGetModuleBase = (SYMGETMODULEBASEPROC)GetProcAddress(module, "SymGetModuleBase");
    if (!_SymGetModuleBase) return PR_FALSE;

    _SymGetSymFromAddr = (SYMGETSYMFROMADDRPROC)GetProcAddress(module, "SymGetSymFromAddr");
    if (!_SymGetSymFromAddr) return PR_FALSE;

    _SymLoadModule = (SYMLOADMODULE)GetProcAddress(module, "SymLoadModule");
    if (!_SymLoadModule) return PR_FALSE;

    _SymUnDName = (SYMUNDNAME)GetProcAddress(module, "SymUnDName");
    if (!_SymUnDName) return PR_FALSE;

    _SymGetModuleInfo = (SYMGETMODULEINFO)GetProcAddress(module, "SymGetModuleInfo");
    if (!_SymGetModuleInfo) return PR_FALSE;

    _EnumerateLoadedModules = (ENUMLOADEDMODULES)GetProcAddress(module, "EnumerateLoadedModules");
    if (!_EnumerateLoadedModules) return PR_FALSE;

    _SymGetLineFromAddr = (SYMGETLINEFROMADDRPROC)GetProcAddress(module, "SymGetLineFromAddr");
    if (!_SymGetLineFromAddr) return PR_FALSE;

    gInitialized = PR_TRUE;
  }

  return gInitialized;
} 




static BOOL CALLBACK callbackEspecial(LPSTR aModuleName, ULONG aModuleBase, ULONG aModuleSize, PVOID aUserContext)
{
    BOOL retval = TRUE;
    DWORD addr = (DWORD)aUserContext;

    




    const BOOL addressIncreases = TRUE;
    
    


    if(addressIncreases
       ? (addr >= aModuleBase && addr <= (aModuleBase + aModuleSize))
       : (addr <= aModuleBase && addr >= (aModuleBase - aModuleSize))
        )
    {
        BOOL loadRes = FALSE;
        HANDLE process = GetCurrentProcess();
                
        loadRes = _SymLoadModule(process, NULL, aModuleName, NULL, aModuleBase, aModuleSize);
        PR_ASSERT(FALSE != loadRes);
    }

    return retval;
}










BOOL SymGetModuleInfoEspecial(HANDLE aProcess, DWORD aAddr, PIMAGEHLP_MODULE aModuleInfo, PIMAGEHLP_LINE aLineInfo)
{
    BOOL retval = FALSE;

    


    aModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULE);
    if (nsnull != aLineInfo) {
      aLineInfo->SizeOfStruct = sizeof(IMAGEHLP_LINE);
    }

    



    retval = _SymGetModuleInfo(aProcess, aAddr, aModuleInfo);

    if (FALSE == retval) {
        BOOL enumRes = FALSE;

        



        enumRes = _EnumerateLoadedModules(aProcess, callbackEspecial, (PVOID)aAddr);
        if(FALSE != enumRes)
        {
            



            retval = _SymGetModuleInfo(aProcess, aAddr, aModuleInfo);
        }
    }

    



    if (FALSE != retval && nsnull != aLineInfo && nsnull != _SymGetLineFromAddr) {
      DWORD displacement = 0;
      BOOL lineRes = FALSE;

      lineRes = _SymGetLineFromAddr(aProcess, aAddr, &displacement, aLineInfo);
    }

    return retval;
}

PRBool
EnsureSymInitialized()
{  
  static PRBool gInitialized = PR_FALSE;

  if (! gInitialized) {
    if (! EnsureImageHlpInitialized())
      return PR_FALSE;
    _SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    gInitialized = _SymInitialize(GetCurrentProcess(), 0, TRUE);
  }
  return gInitialized;
}















void
DumpStackToFile(FILE* aStream) 
{
  HANDLE myProcess = ::GetCurrentProcess();
  HANDLE myThread = ::GetCurrentThread();
  BOOL ok;

  ok = EnsureSymInitialized();
  if (! ok)
    return;

  
  
  
  CONTEXT context;
  context.ContextFlags = CONTEXT_FULL;
  ok = GetThreadContext(myThread, &context);
  if (! ok)
    return;

  
  STACKFRAME frame;
  memset(&frame, 0, sizeof(frame));
  frame.AddrPC.Offset    = context.Eip;
  frame.AddrPC.Mode      = AddrModeFlat;
  frame.AddrStack.Offset = context.Esp;
  frame.AddrStack.Mode   = AddrModeFlat;
  frame.AddrFrame.Offset = context.Ebp;
  frame.AddrFrame.Mode   = AddrModeFlat;

  
  int skip = 2;
  while (1) {
    ok = _StackWalk(IMAGE_FILE_MACHINE_I386,
                   myProcess,
                   myThread,
                   &frame,
                   &context,
                   0,                        
                   _SymFunctionTableAccess,  
                   _SymGetModuleBase,        
                   0);                       

    if (!ok) {
      LPVOID lpMsgBuf;
      FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
        );
      fprintf(aStream, "### ERROR: WalkStack: %s", lpMsgBuf);
      fflush(aStream);
      LocalFree( lpMsgBuf );
    }
    if (!ok || frame.AddrPC.Offset == 0)
      break;

    if (skip-- > 0)
      continue;

    
    
    
    
    IMAGEHLP_MODULE modInfo;
    modInfo.SizeOfStruct = sizeof(modInfo);
    BOOL modInfoRes = TRUE;
    modInfoRes = SymGetModuleInfoEspecial(myProcess, frame.AddrPC.Offset, &modInfo, nsnull);

    char buf[sizeof(IMAGEHLP_SYMBOL) + 512];
    PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL) buf;
    symbol->SizeOfStruct = sizeof(buf);
    symbol->MaxNameLength = 512;

    DWORD displacement;
    ok = _SymGetSymFromAddr(myProcess,
                            frame.AddrPC.Offset,
                            &displacement,
                            symbol);

    if (ok) {
      fprintf(aStream, "%s+0x%08X\n", symbol->Name, displacement);
    }
    else {
      fprintf(aStream, "0x%08X\n", frame.AddrPC.Offset);
    }
  }
}

