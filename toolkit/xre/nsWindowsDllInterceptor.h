




#ifndef NS_WINDOWS_DLL_INTERCEPTOR_H_
#define NS_WINDOWS_DLL_INTERCEPTOR_H_
#include <windows.h>
#include <winternl.h>





















































#include <mozilla/StandardInteger.h>

namespace mozilla {
namespace internal {

class WindowsDllNopSpacePatcher
{
  typedef unsigned char *byteptr_t;
  HMODULE mModule;

  
  
  static const size_t maxPatchedFns = 128;
  byteptr_t mPatchedFns[maxPatchedFns];
  int mPatchedFnsLen;

public:
  WindowsDllNopSpacePatcher()
    : mModule(0)
    , mPatchedFnsLen(0)
  {}

  ~WindowsDllNopSpacePatcher()
  {
    

    for (int i = 0; i < mPatchedFnsLen; i++) {
      byteptr_t fn = mPatchedFns[i];

      
      DWORD op;
      if (!VirtualProtectEx(GetCurrentProcess(), fn, 2, PAGE_EXECUTE_READWRITE, &op)) {
        
        continue;
      }

      
      *((uint16_t*)fn) = 0xff8b;

      
      VirtualProtectEx(GetCurrentProcess(), fn, 2, op, &op);

      
      FlushInstructionCache(GetCurrentProcess(),
                             NULL,
                             0);
    }
  }

  void Init(const char *modulename)
  {
    mModule = LoadLibraryExA(modulename, NULL, 0);
    if (!mModule) {
      
      return;
    }
  }

#if defined(_M_IX86)
  bool AddHook(const char *pname, intptr_t hookDest, void **origFunc)
  {
    if (!mModule)
      return false;

    if (mPatchedFnsLen == maxPatchedFns) {
      
      return false;
    }

    byteptr_t fn = reinterpret_cast<byteptr_t>(GetProcAddress(mModule, pname));
    if (!fn) {
      
      return false;
    }
  
    
    
    
    DWORD op;
    if (!VirtualProtectEx(GetCurrentProcess(), fn - 5, 7, PAGE_EXECUTE_READWRITE, &op)) {
      
      return false;
    }

    bool rv = WriteHook(fn, hookDest, origFunc);
    
    
    VirtualProtectEx(GetCurrentProcess(), fn - 5, 7, op, &op);

    if (rv) {
      mPatchedFns[mPatchedFnsLen] = fn;
      mPatchedFnsLen++;
    }

    return rv;
  }

  bool WriteHook(byteptr_t fn, intptr_t hookDest, void **origFunc)
  {
    
    
    
    
    

    for (int i = -5; i <= -1; i++) {
      if (fn[i] != 0x90) 
        return false;
    }

    
    
    
    
    
    
    
    if ((fn[0] != 0x8b && fn[0] != 0x89) || fn[1] != 0xff) {
      return false;
    }

    
    fn[-5] = 0xe9; 
    *((intptr_t*)(fn - 4)) = hookDest - (uintptr_t)(fn); 

    
    
    *origFunc = fn + 2;

    
    *((uint16_t*)(fn)) = 0xf9eb; 

    
    FlushInstructionCache(GetCurrentProcess(),
                           NULL,
                           0);

    return true;
  }
#else
  bool AddHook(const char *pname, intptr_t hookDest, void **origFunc)
  {
    
    return false;
  }
#endif
};

class WindowsDllDetourPatcher
{
  typedef unsigned char *byteptr_t;
public:
  WindowsDllDetourPatcher() 
    : mModule(0), mHookPage(0), mMaxHooks(0), mCurHooks(0)
  {
  }

  ~WindowsDllDetourPatcher()
  {
    int i;
    byteptr_t p;
    for (i = 0, p = mHookPage; i < mCurHooks; i++, p += kHookSize) {
#if defined(_M_IX86)
      size_t nBytes = 1 + sizeof(intptr_t);
#elif defined(_M_X64)
      size_t nBytes = 2 + sizeof(intptr_t);
#else
#error "Unknown processor type"
#endif
      byteptr_t origBytes = *((byteptr_t *)p);
      
      DWORD op;
      if (!VirtualProtectEx(GetCurrentProcess(), origBytes, nBytes, PAGE_EXECUTE_READWRITE, &op)) {
        
        continue;
      }
      
      
      intptr_t dest = (intptr_t)(p + sizeof(void *));
#if defined(_M_IX86)
      *((intptr_t*)(origBytes+1)) = dest - (intptr_t)(origBytes+5); 
#elif defined(_M_X64)
      *((intptr_t*)(origBytes+2)) = dest;
#else
#error "Unknown processor type"
#endif
      
      VirtualProtectEx(GetCurrentProcess(), origBytes, nBytes, op, &op);
    }
  }

  void Init(const char *modulename, int nhooks = 0)
  {
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

    mHookPage = (byteptr_t) VirtualAllocEx(GetCurrentProcess(), NULL, mMaxHooks * kHookSize,
             MEM_COMMIT | MEM_RESERVE,
             PAGE_EXECUTE_READWRITE);

    if (!mHookPage) {
      mModule = 0;
      return;
    }
  }

  bool Initialized()
  {
    return !!mModule;
  }

  void LockHooks()
  {
    if (!mModule)
      return;

    DWORD op;
    VirtualProtectEx(GetCurrentProcess(), mHookPage, mMaxHooks * kHookSize, PAGE_EXECUTE_READ, &op);

    mModule = 0;
  }

  bool AddHook(const char *pname, intptr_t hookDest, void **origFunc)
  {
    if (!mModule)
      return false;

    void *pAddr = (void *) GetProcAddress(mModule, pname);
    if (!pAddr) {
      
      return false;
    }

    CreateTrampoline(pAddr, hookDest, origFunc);
    if (!*origFunc) {
      
      return false;
    }

    return true;
  }

protected:
  const static int kPageSize = 4096;
  const static int kHookSize = 128;

  HMODULE mModule;
  byteptr_t mHookPage;
  int mMaxHooks;
  int mCurHooks;

  void CreateTrampoline(void *origFunction,
                        intptr_t dest,
                        void **outTramp)
  {
    *outTramp = NULL;

    byteptr_t tramp = FindTrampolineSpace();
    if (!tramp)
      return;

    byteptr_t origBytes = (byteptr_t) origFunction;

    int nBytes = 0;
    int pJmp32 = -1;

#if defined(_M_IX86)
    while (nBytes < 5) {
      
      
      
      
      
      if (origBytes[nBytes] >= 0x88 && origBytes[nBytes] <= 0x8B) {
        
        unsigned char b = origBytes[nBytes+1];
        if (((b & 0xc0) == 0xc0) ||
            (((b & 0xc0) == 0x00) &&
             ((b & 0x07) != 0x04) && ((b & 0x07) != 0x05)))
        {
          
          nBytes += 2;
        } else if (((b & 0xc0) == 0x40) && ((b & 0x38) != 0x20)) {
          
          nBytes += 3;
        } else {
          
          return;
        }
      } else if (origBytes[nBytes] == 0xB8) {
        
        nBytes += 5;
      } else if (origBytes[nBytes] == 0x83) {
        
        unsigned char b = origBytes[nBytes+1];
        if ((b & 0xc0) == 0xc0) {
          
          nBytes += 3;
        } else {
          
          return;
        }
      } else if (origBytes[nBytes] == 0x68) {
        
        nBytes += 5;
      } else if ((origBytes[nBytes] & 0xf0) == 0x50) {
        
        nBytes++;
      } else if (origBytes[nBytes] == 0x6A) {
        
        nBytes += 2;
      } else if (origBytes[nBytes] == 0xe9) {
        pJmp32 = nBytes;
        
        nBytes += 5;
      } else {
        
        return;
      }
    }
#elif defined(_M_X64)
    while (nBytes < 13) {

      
      if (pJmp32 >= 0) {
        if (origBytes[nBytes++] != 0x90)
          return;

        continue;
      } 
        
      if (origBytes[nBytes] == 0x41) {
        
        nBytes++;

        if ((origBytes[nBytes] & 0xf0) == 0x50) {
          
          nBytes++;
        } else if (origBytes[nBytes] >= 0xb8 && origBytes[nBytes] <= 0xbf) {
          
          nBytes += 5;
        } else {
          return;
        }
      } else if (origBytes[nBytes] == 0x45) {
        
        nBytes++;

        if (origBytes[nBytes] == 0x33) {
          
          nBytes += 2;
        } else {
          return;
        }
      } else if ((origBytes[nBytes] & 0xfb) == 0x48) {
        
        nBytes++;

        if (origBytes[nBytes] == 0x81 && (origBytes[nBytes+1] & 0xf8) == 0xe8) {
          
          nBytes += 6;
        } else if (origBytes[nBytes] == 0x83 &&
                  (origBytes[nBytes+1] & 0xf8) == 0xe8) {
          
          nBytes += 3;
        } else if (origBytes[nBytes] == 0x83 &&
                  (origBytes[nBytes+1] & 0xf8) == 0x60) {
          
          nBytes += 5;
        } else if ((origBytes[nBytes] & 0xfd) == 0x89) {
          
          if ((origBytes[nBytes+1] & 0xc0) == 0x40) {
            if ((origBytes[nBytes+1] & 0x7) == 0x04) {
              
              nBytes += 4;
            } else {
              
              nBytes += 3;
            }
          } else if (((origBytes[nBytes+1] & 0xc0) == 0xc0) ||
                     (((origBytes[nBytes+1] & 0xc0) == 0x00) &&
                      ((origBytes[nBytes+1] & 0x07) != 0x04) && ((origBytes[nBytes+1] & 0x07) != 0x05))) {
            
            nBytes += 2;
          } else {
            
            return;
          }
        } else {
          
          return;
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
          return;
        }
      } else {
        return;
      }
    }
#else
#error "Unknown processor type"
#endif

    if (nBytes > 100) {
      
      return;
    }

    
    
    *((void **)tramp) = origFunction;
    tramp += sizeof(void *);

    memcpy(tramp, origFunction, nBytes);

    
    byteptr_t trampDest = origBytes + nBytes;

#if defined(_M_IX86)
    if (pJmp32 >= 0) {
      
      
      
      *((intptr_t*)(tramp+pJmp32+1)) += origBytes + pJmp32 - tramp;
    } else {
      tramp[nBytes] = 0xE9; 
      *((intptr_t*)(tramp+nBytes+1)) = (intptr_t)trampDest - (intptr_t)(tramp+nBytes+5); 
    }
#elif defined(_M_X64)
    
    if (pJmp32 >= 0) {
      
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

    
    *outTramp = tramp;

    
    DWORD op;
    if (!VirtualProtectEx(GetCurrentProcess(), origFunction, nBytes, PAGE_EXECUTE_READWRITE, &op)) {
      
      return;
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
  }

  byteptr_t FindTrampolineSpace()
  {
    if (mCurHooks >= mMaxHooks)
      return 0;

    byteptr_t p = mHookPage + mCurHooks*kHookSize;

    mCurHooks++;

    return p;
  }
};

} 

class WindowsDllInterceptor
{
  internal::WindowsDllNopSpacePatcher mNopSpacePatcher;
  internal::WindowsDllDetourPatcher mDetourPatcher;

  const char *mModuleName;
  int mNHooks;

public:
  WindowsDllInterceptor()
    : mModuleName(NULL)
    , mNHooks(0)
  {}

  void Init(const char *moduleName, int nhooks = 0)
  {
    if (mModuleName) {
      return;
    }

    mModuleName = moduleName;
    mNHooks = nhooks;
    mNopSpacePatcher.Init(moduleName);

    
    
  }

  void LockHooks()
  {
    if (mDetourPatcher.Initialized())
      mDetourPatcher.LockHooks();
  }

  bool AddHook(const char *pname, intptr_t hookDest, void **origFunc)
  {
    if (!mModuleName) {
      
      return false;
    }

    if (mNopSpacePatcher.AddHook(pname, hookDest, origFunc)) {
      
      return true;
    }

    if (!mDetourPatcher.Initialized()) {
      
      mDetourPatcher.Init(mModuleName, mNHooks);
    }

    bool rv = mDetourPatcher.AddHook(pname, hookDest, origFunc);
    
    return rv;
  }
};

} 

#endif 
