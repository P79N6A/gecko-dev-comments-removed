




































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

    mModule = LoadLibraryExA(modulename, NULL, 0);
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
         intptr_t hookDest,
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
           intptr_t dest)
  {
    byteptr_t tramp = FindTrampolineSpace();
    if (!tramp)
      return 0;

    byteptr_t origBytes = (byteptr_t) origFunction;

    int nBytes = 0;
#if defined(_M_IX86)
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
      } else if (origBytes[nBytes] == 0x6A) {
        
        nBytes += 2;
      } else {
        
        return 0;
      }
    }
#elif defined(_M_X64)
    int pJmp32 = 0;

    while (nBytes < 13) {

      
      if (pJmp32) {
        if (origBytes[nBytes++] != 0x90)
          return 0;

        continue;
      } 
        
      if (origBytes[nBytes] == 0x41) {
        
        nBytes++;

        if ((origBytes[nBytes] & 0xf0) == 0x50) {
          
          nBytes++;
        } else if (origBytes[nBytes] >= 0xb8 && origBytes[nBytes] <= 0xbf) {
          
          nBytes += 5;
        } else {
          return 0;
        }
      } else if (origBytes[nBytes] == 0x45) {
        
        nBytes++;

        if (origBytes[nBytes] == 0x33) {
          
          nBytes += 2;
        } else {
          return 0;
        }
      } else if (origBytes[nBytes] == 0x48) {
        
        nBytes++;

        if (origBytes[nBytes] == 0x81 && (origBytes[nBytes+1] & 0xf8) == 0xe8) {
          
          nBytes += 6;
        } else if (origBytes[nBytes] == 0x83 &&
                  (origBytes[nBytes+1] & 0xf8) == 0xe8) {
          
          nBytes += 3;
        } else if (origBytes[nBytes] == 0x83 &&
                  (origBytes[nBytes+1] & 0xf8) == 0x60) {
          
          nBytes += 5;
        } else if (origBytes[nBytes] == 0x89) {
          
          if ((origBytes[nBytes+1] & 0xc0) == 0x40) {
            if ((origBytes[nBytes+1] & 0x7) == 0x04) {
              
              nBytes += 4;
            } else {
              
              nBytes += 3;
            }
          } else {
            
            return 0;
          }
        } else if (origBytes[nBytes] == 0x8b) {
          
          if ((origBytes[nBytes+1] & 0xc0) == 0x40) {
            if ((origBytes[nBytes+1] & 0x7) == 0x04) {
              
              nBytes += 4;
            } else {
              
              nBytes += 3;
            }
          } else if ((origBytes[nBytes+1] & 0xc0) == 0xc0) {
            
            nBytes += 2;
          } else {
            
            return 0;
          }
        } else {
          
          return 0;
        }
      } else if ((origBytes[nBytes] & 0xf0) == 0x50) {
        
        nBytes++;
      } else if (origBytes[nBytes] == 0x90) {
        
        nBytes++;
      } else if (origBytes[nBytes] == 0xe9) {
        pJmp32 = nBytes;
        
        nBytes += 5;
      } else if (origBytes[nBytes] == 0xff) {
        nBytes++;
        if ((origBytes[nBytes] & 0xf8) == 0xf0) {
          
          nBytes++;
        } else {
          return 0;
        }
      } else {
        return 0;
      }
    }
#else
#error "Unknown processor type"
#endif

    if (nBytes > 100) {
      
      return 0;
    }

    memcpy(tramp, origFunction, nBytes);

    
    byteptr_t trampDest = origBytes + nBytes;

#if defined(_M_IX86)
    tramp[nBytes] = 0xE9; 
    *((intptr_t*)(tramp+nBytes+1)) = (intptr_t)trampDest - (intptr_t)(tramp+nBytes+5); 
#elif defined(_M_X64)
    
    if (pJmp32) {
      
      byteptr_t directJmpAddr = origBytes + pJmp32 + 5 + (*((LONG*)(origBytes+pJmp32+1)));
      
      tramp[pJmp32]   = 0x49;
      tramp[pJmp32+1] = 0xbb;
      *((intptr_t*)(tramp+pJmp32+2)) = (intptr_t)directJmpAddr;

      
      tramp[pJmp32+10] = 0x41;
      tramp[pJmp32+11] = 0xff;
      tramp[pJmp32+12] = 0xe3;
    } else {
      
      tramp[nBytes] = 0x49;
      tramp[nBytes+1] = 0xbb;
      *((intptr_t*)(tramp+nBytes+2)) = (intptr_t)trampDest;

      
      tramp[nBytes+10] = 0x41;
      tramp[nBytes+11] = 0xff;
      tramp[nBytes+12] = 0xe3;
    }
#endif

    
    DWORD op;
    if (!VirtualProtectEx(GetCurrentProcess(), origFunction, nBytes, PAGE_EXECUTE_READWRITE, &op)) {
      
      return 0;
    }

#if defined(_M_IX86)
    
    origBytes[0] = 0xE9; 
    *((intptr_t*)(origBytes+1)) = dest - (intptr_t)(origBytes+5); 
#elif defined(_M_X64)
    
    origBytes[0] = 0x49;
    origBytes[1] = 0xbb;

    *((intptr_t*)(origBytes+2)) = dest;

    
    origBytes[10] = 0x41;
    origBytes[11] = 0xff;
    origBytes[12] = 0xe3;
#endif

    
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
