





#ifndef CodeAddressService_h__
#define CodeAddressService_h__

#include "mozilla/Assertions.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Types.h"

#include "nsStackWalk.h"

#ifdef XP_WIN
#define snprintf _snprintf
#endif

namespace mozilla {













template <class StringTable,
          class StringAlloc,
          class DescribeCodeAddressLock>
class CodeAddressService
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  StringTable mLibraryStrings;

  struct Entry
  {
    const void* mPc;
    char*       mFunction;  
    const char* mLibrary;   
                            
    ptrdiff_t   mLOffset;
    char*       mFileName;  
    uint32_t    mLineNo:31;
    uint32_t    mInUse:1;   

    Entry()
      : mPc(0), mFunction(nullptr), mLibrary(nullptr), mLOffset(0),
        mFileName(nullptr), mLineNo(0), mInUse(0)
    {}

    ~Entry()
    {
      
      StringAlloc::free(mFunction);
      StringAlloc::free(mFileName);
    }

    void Replace(const void* aPc, const char* aFunction,
                 const char* aLibrary, ptrdiff_t aLOffset,
                 const char* aFileName, unsigned long aLineNo)
    {
      mPc = aPc;

      
      StringAlloc::free(mFunction);
      mFunction =
        !aFunction[0] ? nullptr : StringAlloc::copy(aFunction);
      StringAlloc::free(mFileName);
      mFileName =
        !aFileName[0] ? nullptr : StringAlloc::copy(aFileName);


      mLibrary = aLibrary;
      mLOffset = aLOffset;
      mLineNo = aLineNo;

      mInUse = 1;
    }

    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
    {
      
      size_t n = 0;
      n += aMallocSizeOf(mFunction);
      n += aMallocSizeOf(mFileName);
      return n;
    }
  };

  
  
  
  
  
  
  static const size_t kNumEntries = 1 << 12;
  static const size_t kMask = kNumEntries - 1;
  Entry mEntries[kNumEntries];

  size_t mNumCacheHits;
  size_t mNumCacheMisses;

public:
  CodeAddressService()
    : mEntries(), mNumCacheHits(0), mNumCacheMisses(0)
  {
  }

  void GetLocation(const void* aPc, char* aBuf, size_t aBufLen)
  {
    MOZ_ASSERT(DescribeCodeAddressLock::IsLocked());

    uint32_t index = HashGeneric(aPc) & kMask;
    MOZ_ASSERT(index < kNumEntries);
    Entry& entry = mEntries[index];

    if (!entry.mInUse || entry.mPc != aPc) {
      mNumCacheMisses++;

      
      
      
      
      
      nsCodeAddressDetails details;
      {
        DescribeCodeAddressLock::Unlock();
        (void)NS_DescribeCodeAddress(const_cast<void*>(aPc), &details);
        DescribeCodeAddressLock::Lock();
      }

      const char* library = mLibraryStrings.Intern(details.library);
      entry.Replace(aPc, details.function, library, details.loffset,
                    details.filename, details.lineno);

    } else {
      mNumCacheHits++;
    }

    MOZ_ASSERT(entry.mPc == aPc);

    uintptr_t entryPc = (uintptr_t)(entry.mPc);
    
    
    if (!entry.mFunction && !entry.mLibrary[0] && entry.mLOffset == 0) {
      snprintf(aBuf, aBufLen, "??? 0x%" PRIxPTR, entryPc);
    } else {
      
      const char* entryFunction = entry.mFunction ? entry.mFunction : "???";
      if (entry.mFileName) {
        
        snprintf(aBuf, aBufLen, "%s (%s:%u) 0x%" PRIxPTR,
                 entryFunction, entry.mFileName, entry.mLineNo, entryPc);
      } else {
        
        
        
        snprintf(aBuf, aBufLen, "%s[%s +0x%" PRIXPTR "] 0x%" PRIxPTR,
                 entryFunction, entry.mLibrary, entry.mLOffset, entryPc);
      }
    }
  }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    size_t n = aMallocSizeOf(this);
    for (uint32_t i = 0; i < kNumEntries; i++) {
      n += mEntries[i].SizeOfExcludingThis(aMallocSizeOf);
    }

    n += mLibraryStrings.SizeOfExcludingThis(aMallocSizeOf);

    return n;
  }

  size_t CacheCapacity() const { return kNumEntries; }

  size_t CacheCount() const
  {
    size_t n = 0;
    for (size_t i = 0; i < kNumEntries; i++) {
      if (mEntries[i].mInUse) {
        n++;
      }
    }
    return n;
  }

  size_t NumCacheHits()   const { return mNumCacheHits; }
  size_t NumCacheMisses() const { return mNumCacheMisses; }
};

} 

#endif 
