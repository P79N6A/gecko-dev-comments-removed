





#ifndef LulMain_h
#define LulMain_h

#include "LulPlatformMacros.h"









































namespace lul {


class MOZ_STACK_CLASS TaggedUWord {
public:
  
  
  explicit TaggedUWord(uintptr_t w)
    : mValue(w)
    , mValid(true)
  {}

  
  
  TaggedUWord()
    : mValue(0)
    , mValid(false)
  {}

  
  
  void Add(TaggedUWord other) {
    if (mValid && other.Valid()) {
      mValue += other.Value();
    } else {
      mValue = 0;
      mValid = false;
    }
  }

  
  
  bool IsAligned() const {
    return mValid && (mValue & (sizeof(uintptr_t)-1)) == 0;
  }

  
  uintptr_t Value() const { return mValue; }

  
  bool      Valid() const { return mValid; }

private:
  uintptr_t mValue;
  bool mValid;
};




struct UnwindRegs {
#if defined(LUL_ARCH_arm)
  TaggedUWord r7;
  TaggedUWord r11;
  TaggedUWord r12;
  TaggedUWord r13;
  TaggedUWord r14;
  TaggedUWord r15;
#elif defined(LUL_ARCH_x64) || defined(LUL_ARCH_x86)
  TaggedUWord xbp;
  TaggedUWord xsp;
  TaggedUWord xip;
#else
# error "Unknown plat"
#endif
};







static const size_t N_STACK_BYTES = 32768;


struct StackImage {
  
  
  uintptr_t mStartAvma;
  size_t    mLen;
  uint8_t   mContents[N_STACK_BYTES];
};



template<typename T>
class LULStats {
public:
  LULStats()
    : mContext(0)
    , mCFI(0)
    , mScanned(0)
  {}

  template <typename S>
  LULStats(const LULStats<S>& aOther)
    : mContext(aOther.mContext)
    , mCFI(aOther.mCFI)
    , mScanned(aOther.mScanned)
  {}

  template <typename S>
  LULStats<T>& operator=(const LULStats<S>& aOther)
  {
    mContext = aOther.mContext;
    mCFI     = aOther.mCFI;
    mScanned = aOther.mScanned;
    return *this;
  }

  template <typename S>
  uint32_t operator-(const LULStats<S>& aOther) {
    return (mContext - aOther.mContext) +
           (mCFI - aOther.mCFI) + (mScanned - aOther.mScanned);
  }

  T mContext; 
  T mCFI;     
  T mScanned; 
};




























class PriMap;
class SegArray;
class UniqueStringUniverse;

class LUL {
public:
  
  explicit LUL(void (*aLog)(const char*));

  
  
  
  
  ~LUL();

  
  
  
  
  void EnableUnwinding();

  
  
  
  
  
  
  
  void NotifyAfterMap(uintptr_t aRXavma, size_t aSize,
                      const char* aFileName, const void* aMappedImage);

  
  
  
  
  
  
  
  
  void NotifyExecutableArea(uintptr_t aRXavma, size_t aSize);

  
  
  
  
  
  
  
  
  
  
  void NotifyBeforeUnmap(uintptr_t aAvmaMin, uintptr_t aAvmaMax);

  
  
  
  
  void NotifyBeforeUnmapAll() {
    NotifyBeforeUnmap(0, UINTPTR_MAX);
  }

  
  
  size_t CountMappings();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void Unwind(uintptr_t* aFramePCs,
              uintptr_t* aFrameSPs,
              size_t* aFramesUsed,
              size_t* aScannedFramesAcquired,
              size_t aFramesAvail,
              size_t aScannedFramesAllowed,
              UnwindRegs* aStartRegs, StackImage* aStackImg);

  
  
  void (*mLog)(const char*);

  
  
  LULStats<mozilla::Atomic<uint32_t>> mStats;

  
  
  void MaybeShowStats();

private:
  
  LULStats<uint32_t> mStatsPrevious;

  
  
  bool mAdminMode;

  
  
  
  
  
  
  int mAdminThreadId;

  
  
  
  PriMap* mPriMap;

  
  
  SegArray* mSegArray;

  
  
  
  UniqueStringUniverse* mUSU;
};






void
RunLulUnitTests(int* aNTests, int*aNTestsPassed, LUL* aLUL);

} 

#endif 
