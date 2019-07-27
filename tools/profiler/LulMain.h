





#ifndef LulMain_h
#define LulMain_h

#include <pthread.h>   

#include <map>

#include "LulPlatformMacros.h"
#include "LulRWLock.h"









































namespace lul {


class TaggedUWord {
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




























class PriMap;
class SegArray;
class CFICache;

class LUL {
public:
  
  explicit LUL(void (*aLog)(const char*));

  
  
  
  
  ~LUL();

  
  
  
  
  
  
  
  
  
  void NotifyAfterMap(uintptr_t aRXavma, size_t aSize,
                      const char* aFileName, const void* aMappedImage);

  
  
  
  
  
  
  
  void NotifyExecutableArea(uintptr_t aRXavma, size_t aSize);

  
  
  
  
  
  
  
  
  
  void NotifyBeforeUnmap(uintptr_t aAvmaMin, uintptr_t aAvmaMax);

  
  
  
  void NotifyBeforeUnmapAll() {
    NotifyBeforeUnmap(0, UINTPTR_MAX);
  }

  
  
  size_t CountMappings();

  
  
  void RegisterUnwinderThread();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void Unwind(uintptr_t* aFramePCs,
              uintptr_t* aFrameSPs,
              size_t* aFramesUsed,
              size_t* aScannedFramesAcquired,
              size_t aFramesAvail,
              size_t aScannedFramesAllowed,
              UnwindRegs* aStartRegs, StackImage* aStackImg);

  
  
  void (*mLog)(const char*);

private:
  
  
  void InvalidateCFICaches();

  
  LulRWLock* mRWlock;

  
  
  
  
  
  
  PriMap* mPriMap;

  
  
  
  
  
  SegArray* mSegArray;

  
  
  
  
  
  
  
  
  
  
  std::map<pthread_t, CFICache*> mCaches;
};






void
RunLulUnitTests(int* aNTests, int*aNTestsPassed, LUL* aLUL);

} 

#endif 
