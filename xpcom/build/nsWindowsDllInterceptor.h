





#ifndef NS_WINDOWS_DLL_INTERCEPTOR_H_
#define NS_WINDOWS_DLL_INTERCEPTOR_H_
#include <windows.h>
#include <winternl.h>





















































#include <stdint.h>

namespace mozilla {
namespace internal {

class WindowsDllNopSpacePatcher
{
  typedef unsigned char* byteptr_t;
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
                             nullptr,
                             0);
    }
  }

  void Init(const char* aModuleName)
  {
    mModule = LoadLibraryExA(aModuleName, nullptr, 0);
    if (!mModule) {
      
      return;
    }
  }

#if defined(_M_IX86)
  bool AddHook(const char* aName, intptr_t aHookDest, void** aOrigFunc)
  {
    if (!mModule) {
      return false;
    }

    if (mPatchedFnsLen == maxPatchedFns) {
      
      return false;
    }

    byteptr_t fn = reinterpret_cast<byteptr_t>(GetProcAddress(mModule, aName));
    if (!fn) {
      
      return false;
    }

    fn = ResolveRedirectedAddress(fn);

    
    
    
    DWORD op;
    if (!VirtualProtectEx(GetCurrentProcess(), fn - 5, 7,
                          PAGE_EXECUTE_READWRITE, &op)) {
      
      return false;
    }

    bool rv = WriteHook(fn, aHookDest, aOrigFunc);

    
    VirtualProtectEx(GetCurrentProcess(), fn - 5, 7, op, &op);

    if (rv) {
      mPatchedFns[mPatchedFnsLen] = fn;
      mPatchedFnsLen++;
    }

    return rv;
  }

  bool WriteHook(byteptr_t aFn, intptr_t aHookDest, void** aOrigFunc)
  {
    
    
    
    
    

    for (int i = -5; i <= -1; i++) {
      if (aFn[i] != 0x90 && aFn[i] != 0xcc) { 
        return false;
      }
    }

    
    
    
    
    
    
    
    if ((aFn[0] != 0x8b && aFn[0] != 0x89) || aFn[1] != 0xff) {
      return false;
    }

    
    aFn[-5] = 0xe9; 
    *((intptr_t*)(aFn - 4)) = aHookDest - (uintptr_t)(aFn); 

    
    
    *aOrigFunc = aFn + 2;

    
    *((uint16_t*)(aFn)) = 0xf9eb; 

    
    FlushInstructionCache(GetCurrentProcess(),
                           nullptr,
                           0);

    return true;
  }

private:
  static byteptr_t ResolveRedirectedAddress(const byteptr_t aOriginalFunction)
  {
    
    
    if (aOriginalFunction[0] == 0xff && aOriginalFunction[1] == 0x25) {
      return (byteptr_t)(**((uint32_t**) (aOriginalFunction + 2)));
    }

    return aOriginalFunction;
  }
#else
  bool AddHook(const char* aName, intptr_t aHookDest, void** aOrigFunc)
  {
    
    return false;
  }
#endif
};

class WindowsDllDetourPatcher
{
  typedef unsigned char* byteptr_t;
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
      byteptr_t origBytes = *((byteptr_t*)p);
      
      DWORD op;
      if (!VirtualProtectEx(GetCurrentProcess(), origBytes, nBytes,
                            PAGE_EXECUTE_READWRITE, &op)) {
        
        continue;
      }
      
      
      intptr_t dest = (intptr_t)(p + sizeof(void*));
#if defined(_M_IX86)
      *((intptr_t*)(origBytes + 1)) =
        dest - (intptr_t)(origBytes + 5); 
#elif defined(_M_X64)
      *((intptr_t*)(origBytes + 2)) = dest;
#else
#error "Unknown processor type"
#endif
      
      VirtualProtectEx(GetCurrentProcess(), origBytes, nBytes, op, &op);
    }
  }

  void Init(const char* aModuleName, int aNumHooks = 0)
  {
    if (mModule) {
      return;
    }

    mModule = LoadLibraryExA(aModuleName, nullptr, 0);
    if (!mModule) {
      
      return;
    }

    int hooksPerPage = 4096 / kHookSize;
    if (aNumHooks == 0) {
      aNumHooks = hooksPerPage;
    }

    mMaxHooks = aNumHooks + (hooksPerPage % aNumHooks);

    mHookPage = (byteptr_t)VirtualAllocEx(GetCurrentProcess(), nullptr,
                                          mMaxHooks * kHookSize,
                                          MEM_COMMIT | MEM_RESERVE,
                                          PAGE_EXECUTE_READWRITE);
    if (!mHookPage) {
      mModule = 0;
      return;
    }
  }

  bool Initialized() { return !!mModule; }

  void LockHooks()
  {
    if (!mModule) {
      return;
    }

    DWORD op;
    VirtualProtectEx(GetCurrentProcess(), mHookPage, mMaxHooks * kHookSize,
                     PAGE_EXECUTE_READ, &op);

    mModule = 0;
  }

  bool AddHook(const char* aName, intptr_t aHookDest, void** aOrigFunc)
  {
    if (!mModule) {
      return false;
    }

    void* pAddr = (void*)GetProcAddress(mModule, aName);
    if (!pAddr) {
      
      return false;
    }

    pAddr = ResolveRedirectedAddress((byteptr_t)pAddr);

    CreateTrampoline(pAddr, aHookDest, aOrigFunc);
    if (!*aOrigFunc) {
      
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

  void CreateTrampoline(void* aOrigFunction, intptr_t aDest, void** aOutTramp)
  {
    *aOutTramp = nullptr;

    byteptr_t tramp = FindTrampolineSpace();
    if (!tramp) {
      return;
    }

    byteptr_t origBytes = (byteptr_t)aOrigFunction;

    int nBytes = 0;
    int pJmp32 = -1;

#if defined(_M_IX86)
    while (nBytes < 5) {
      
      
      
      
      
      if (origBytes[nBytes] >= 0x88 && origBytes[nBytes] <= 0x8B) {
        
        unsigned char b = origBytes[nBytes + 1];
        if (((b & 0xc0) == 0xc0) ||
            (((b & 0xc0) == 0x00) &&
             ((b & 0x07) != 0x04) && ((b & 0x07) != 0x05))) {
          
          nBytes += 2;
        } else if ((b & 0xc0) == 0x40) {
          if ((b & 0x07) == 0x04) {
            
            nBytes += 4;
          } else {
            
            nBytes += 3;
          }
        } else {
          
          return;
        }
      } else if (origBytes[nBytes] == 0xB8) {
        
        nBytes += 5;
      } else if (origBytes[nBytes] == 0x83) {
        
        unsigned char b = origBytes[nBytes + 1];
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
      } else if (origBytes[nBytes] == 0xff && origBytes[nBytes + 1] == 0x25) {
        
        nBytes += 6;
      } else {
        
        return;
      }
    }
#elif defined(_M_X64)
    byteptr_t directJmpAddr;

    while (nBytes < 13) {

      
      if (pJmp32 >= 0) {
        if (origBytes[nBytes++] != 0x90) {
          return;
        }

        continue;
      }
      if (origBytes[nBytes] == 0x0f) {
        nBytes++;
        if (origBytes[nBytes] == 0x1f) {
          
          nBytes++;
          if ((origBytes[nBytes] & 0xc0) == 0x40 &&
              (origBytes[nBytes] & 0x7) == 0x04) {
            nBytes += 3;
          } else {
            return;
          }
        } else if (origBytes[nBytes] == 0x05) {
          
          nBytes++;
        } else {
          return;
        }
      } else if (origBytes[nBytes] == 0x40 ||
                 origBytes[nBytes] == 0x41) {
        
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

        if (origBytes[nBytes] == 0x81 &&
            (origBytes[nBytes + 1] & 0xf8) == 0xe8) {
          
          nBytes += 6;
        } else if (origBytes[nBytes] == 0x83 &&
                   (origBytes[nBytes + 1] & 0xf8) == 0xe8) {
          
          nBytes += 3;
        } else if (origBytes[nBytes] == 0x83 &&
                   (origBytes[nBytes + 1] & 0xf8) == 0x60) {
          
          nBytes += 5;
        } else if ((origBytes[nBytes] & 0xfd) == 0x89) {
          
          if ((origBytes[nBytes + 1] & 0xc0) == 0x40) {
            if ((origBytes[nBytes + 1] & 0x7) == 0x04) {
              
              nBytes += 4;
            } else {
              
              nBytes += 3;
            }
          } else if (((origBytes[nBytes + 1] & 0xc0) == 0xc0) ||
                     (((origBytes[nBytes + 1] & 0xc0) == 0x00) &&
                      ((origBytes[nBytes + 1] & 0x07) != 0x04) &&
                      ((origBytes[nBytes + 1] & 0x07) != 0x05))) {
            
            nBytes += 2;
          } else {
            
            return;
          }
        } else if (origBytes[nBytes] == 0xc7) {
          
          if (origBytes[nBytes + 1] == 0x44) {
            
            
            nBytes += 8;
          } else {
            return;
          }
        } else if (origBytes[nBytes] == 0xff) {
          pJmp32 = nBytes - 1;
          
          if ((origBytes[nBytes + 1] & 0xc0) == 0x0 &&
              (origBytes[nBytes + 1] & 0x07) == 0x5) {
            
            
            directJmpAddr =
              (byteptr_t)*((uint64_t*)(origBytes + nBytes + 6 +
                                       (*((int32_t*)(origBytes + nBytes + 2)))));
            nBytes += 6;
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
      } else if (origBytes[nBytes] == 0xb8) {
        
        nBytes += 5;
      } else if (origBytes[nBytes] == 0xc3) {
        
        nBytes++;
      } else if (origBytes[nBytes] == 0xe9) {
        pJmp32 = nBytes;
        
        directJmpAddr = origBytes + pJmp32 + 5 + (*((int32_t*)(origBytes + pJmp32 + 1)));
        
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

    
    
    *((void**)tramp) = aOrigFunction;
    tramp += sizeof(void*);

    memcpy(tramp, aOrigFunction, nBytes);

    
    byteptr_t trampDest = origBytes + nBytes;

#if defined(_M_IX86)
    if (pJmp32 >= 0) {
      
      
      
      *((intptr_t*)(tramp + pJmp32 + 1)) += origBytes - tramp;
    } else {
      tramp[nBytes] = 0xE9; 
      *((intptr_t*)(tramp + nBytes + 1)) =
        (intptr_t)trampDest - (intptr_t)(tramp + nBytes + 5); 
    }
#elif defined(_M_X64)
    
    if (pJmp32 >= 0) {
      
      tramp[pJmp32]   = 0x49;
      tramp[pJmp32 + 1] = 0xbb;
      *((intptr_t*)(tramp + pJmp32 + 2)) = (intptr_t)directJmpAddr;

      
      tramp[pJmp32 + 10] = 0x41;
      tramp[pJmp32 + 11] = 0xff;
      tramp[pJmp32 + 12] = 0xe3;
    } else {
      
      tramp[nBytes] = 0x49;
      tramp[nBytes + 1] = 0xbb;
      *((intptr_t*)(tramp + nBytes + 2)) = (intptr_t)trampDest;

      
      tramp[nBytes + 10] = 0x41;
      tramp[nBytes + 11] = 0xff;
      tramp[nBytes + 12] = 0xe3;
    }
#endif

    
    *aOutTramp = tramp;

    
    DWORD op;
    if (!VirtualProtectEx(GetCurrentProcess(), aOrigFunction, nBytes,
                          PAGE_EXECUTE_READWRITE, &op)) {
      
      return;
    }

#if defined(_M_IX86)
    
    origBytes[0] = 0xE9; 
    *((intptr_t*)(origBytes + 1)) =
      aDest - (intptr_t)(origBytes + 5); 
#elif defined(_M_X64)
    
    origBytes[0] = 0x49;
    origBytes[1] = 0xbb;

    *((intptr_t*)(origBytes + 2)) = aDest;

    
    origBytes[10] = 0x41;
    origBytes[11] = 0xff;
    origBytes[12] = 0xe3;
#endif

    
    VirtualProtectEx(GetCurrentProcess(), aOrigFunction, nBytes, op, &op);
  }

  byteptr_t FindTrampolineSpace()
  {
    if (mCurHooks >= mMaxHooks) {
      return 0;
    }

    byteptr_t p = mHookPage + mCurHooks * kHookSize;

    mCurHooks++;

    return p;
  }

  static void* ResolveRedirectedAddress(const byteptr_t aOriginalFunction)
  {
#if defined(_M_IX86)
    
    
    if (aOriginalFunction[0] == 0xff && aOriginalFunction[1] == 0x25) {
      return (void*)(**((uint32_t**) (aOriginalFunction + 2)));
    }
#elif defined(_M_X64)
    if (aOriginalFunction[0] == 0xe9) {
      
      int32_t offset = *((int32_t*)(aOriginalFunction + 1));
      return aOriginalFunction + 5 + offset;
    }
#endif

    return aOriginalFunction;
  }
};

} 

class WindowsDllInterceptor
{
  internal::WindowsDllNopSpacePatcher mNopSpacePatcher;
  internal::WindowsDllDetourPatcher mDetourPatcher;

  const char* mModuleName;
  int mNHooks;

public:
  WindowsDllInterceptor()
    : mModuleName(nullptr)
    , mNHooks(0)
  {}

  void Init(const char* aModuleName, int aNumHooks = 0)
  {
    if (mModuleName) {
      return;
    }

    mModuleName = aModuleName;
    mNHooks = aNumHooks;
    mNopSpacePatcher.Init(aModuleName);

    
    
  }

  void LockHooks()
  {
    if (mDetourPatcher.Initialized()) {
      mDetourPatcher.LockHooks();
    }
  }

  bool AddHook(const char* aName, intptr_t aHookDest, void** aOrigFunc)
  {
    
    

    if (!mModuleName) {
      return false;
    }

    if (mNopSpacePatcher.AddHook(aName, aHookDest, aOrigFunc)) {
      return true;
    }

    return AddDetour(aName, aHookDest, aOrigFunc);
  }

  bool AddDetour(const char* aName, intptr_t aHookDest, void** aOrigFunc)
  {
    
    

    if (!mModuleName) {
      return false;
    }

    if (!mDetourPatcher.Initialized()) {
      mDetourPatcher.Init(mModuleName, mNHooks);
    }

    return mDetourPatcher.AddHook(aName, aHookDest, aOrigFunc);
  }
};

} 

#endif 
