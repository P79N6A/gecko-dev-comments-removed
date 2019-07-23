





































#ifndef NS_WINDOWS_DLL_INTERCEPTOR_H_
#define NS_WINDOWS_DLL_INTERCEPTOR_H_
#include <windows.h>
#include <winternl.h>



















class WindowsDllInterceptor
{
  typedef unsigned char *byteptr_t;
public:
  WindowsDllInterceptor() 
    : mModule(0)
  {
  }

  WindowsDllInterceptor(const char *modulename, int nhooks = 0) {
    Init(modulename, nhooks);
  }

  void Init(const char *modulename, int nhooks = 0) {
    if (mModule)
      return;

    mModule = LoadLibraryEx(modulename, NULL, 0);
    if (!mModule) {
      
      return;
    }

    int hooksPerPage = 4096 / kHookSize;
    if (nhooks == 0)
      nhooks = hooksPerPage;

    mMaxHooks = nhooks + (hooksPerPage % nhooks);
    mCurHooks = 0;

    mHookPage = (byteptr_t) VirtualAllocEx(GetCurrentProcess(), NULL, mMaxHooks * kHookSize,
             MEM_COMMIT | MEM_RESERVE,
             PAGE_EXECUTE_READWRITE);

    if (!mHookPage) {
      mModule = 0;
      return;
    }
  }

  void LockHooks() {
    if (!mModule)
      return;

    DWORD op;
    VirtualProtectEx(GetCurrentProcess(), mHookPage, mMaxHooks * kHookSize, PAGE_EXECUTE_READ, &op);

    mModule = 0;
  }

  bool AddHook(const char *pname,
         void *hookDest,
         void **origFunc)
  {
    if (!mModule)
      return false;

    void *pAddr = (void *) GetProcAddress(mModule, pname);
    if (!pAddr) {
      
      return false;
    }

    void *tramp = CreateTrampoline(pAddr, hookDest);
    if (!tramp) {
      
      return false;
    }

    *origFunc = tramp;

    return true;
  }

protected:
  const static int kPageSize = 4096;
  const static int kHookSize = 128;

  HMODULE mModule;
  byteptr_t mHookPage;
  int mMaxHooks;
  int mCurHooks;

  byteptr_t CreateTrampoline(void *origFunction,
           void *dest)
  {
    byteptr_t tramp = FindTrampolineSpace();
    if (!tramp)
      return 0;

    byteptr_t origBytes = (byteptr_t) origFunction;

    int nBytes = 0;
    while (nBytes < 5) {
      
      
      
      
      
      if (origBytes[nBytes] >= 0x88 && origBytes[nBytes] <= 0x8B) {
        
        unsigned char b = origBytes[nBytes+1];
        if (((b & 0xc0) == 0xc0) ||
            (((b & 0xc0) == 0x00) &&
             ((b & 0x38) != 0x20) && ((b & 0x38) != 0x28)))
        {
          nBytes += 2;
        } else {
          
          return 0;
        }
      } else if (origBytes[nBytes] == 0x68) {
        
        nBytes += 5;
      } else if ((origBytes[nBytes] & 0xf0) == 0x50) {
        
        nBytes++;
      } else {
        
        return 0;
      }
    }

    if (nBytes > 100) {
      
      return 0;
    }

    memcpy(tramp, origFunction, nBytes);

    
    byteptr_t trampDest = origBytes + nBytes;

    tramp[nBytes] = 0xE9; 
    *((intptr_t*)(tramp+nBytes+1)) = (intptr_t)trampDest - (intptr_t)(tramp+nBytes+5); 

    
    DWORD op;
    if (!VirtualProtectEx(GetCurrentProcess(), origFunction, nBytes, PAGE_EXECUTE_READWRITE, &op)) {
      
      return 0;
    }

    
    origBytes[0] = 0xE9; 
    *((intptr_t*)(origBytes+1)) = (intptr_t)dest - (intptr_t)(origBytes+5); 

    
    VirtualProtectEx(GetCurrentProcess(), origFunction, nBytes, op, &op);

    return tramp;
  }

  byteptr_t FindTrampolineSpace() {
    if (mCurHooks >= mMaxHooks)
      return 0;

    byteptr_t p = mHookPage + mCurHooks*kHookSize;

    mCurHooks++;

    return p;
  }
};


#endif 
