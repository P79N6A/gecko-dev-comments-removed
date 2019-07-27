





#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef XP_WIN
#if defined(MOZ_OPTIMIZE) && !defined(MOZ_PROFILING)
#error "Optimized, DMD-enabled builds on Windows must be built with --enable-profiling"
#endif
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

#include "nscore.h"
#include "nsStackWalk.h"

#include "js/HashTable.h"
#include "js/Vector.h"

#include "mozilla/Assertions.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/JSONWriter.h"
#include "mozilla/Likely.h"
#include "mozilla/MemoryReporting.h"



#include "CodeAddressService.h"







#define MOZ_REPLACE_ONLY_MEMALIGN 1

#ifndef PAGE_SIZE
#define DMD_DEFINED_PAGE_SIZE
#ifdef XP_WIN
#define PAGE_SIZE GetPageSize()
static long GetPageSize()
{
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwPageSize;
}
#else 
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#endif 
#endif 
#include "replace_malloc.h"
#undef MOZ_REPLACE_ONLY_MEMALIGN
#ifdef DMD_DEFINED_PAGE_SIZE
#undef DMD_DEFINED_PAGE_SIZE
#undef PAGE_SIZE
#endif 

#include "DMD.h"

namespace mozilla {
namespace dmd {

class DMDBridge : public ReplaceMallocBridge
{
  virtual DMDFuncs* GetDMDFuncs() override;
};

static DMDBridge* gDMDBridge;
static DMDFuncs gDMDFuncs;

DMDFuncs*
DMDBridge::GetDMDFuncs()
{
  return &gDMDFuncs;
}

inline void
StatusMsg(const char* aFmt, ...)
{
  va_list ap;
  va_start(ap, aFmt);
  gDMDFuncs.StatusMsg(aFmt, ap);
  va_end(ap);
}





#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(T) \
  T(const T&);                      \
  void operator=(const T&)
#endif

static const malloc_table_t* gMallocTable = nullptr;


static bool gIsDMDInitialized = false;















class InfallibleAllocPolicy
{
  static void ExitOnFailure(const void* aP);

public:
  static void* malloc_(size_t aSize)
  {
    void* p = gMallocTable->malloc(aSize);
    ExitOnFailure(p);
    return p;
  }

  template <typename T>
  static T* pod_malloc(size_t aNumElems)
  {
    if (aNumElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
      return nullptr;
    void* p = gMallocTable->malloc(aNumElems * sizeof(T));
    ExitOnFailure(p);
    return (T*)p;
  }

  static void* calloc_(size_t aSize)
  {
    void* p = gMallocTable->calloc(1, aSize);
    ExitOnFailure(p);
    return p;
  }

  template <typename T>
  static T* pod_calloc(size_t aNumElems)
  {
    void* p = gMallocTable->calloc(aNumElems, sizeof(T));
    ExitOnFailure(p);
    return (T*)p;
  }

  
  static void* realloc_(void* aPtr, size_t aNewSize)
  {
    void* p = gMallocTable->realloc(aPtr, aNewSize);
    ExitOnFailure(p);
    return p;
  }

  
  template <typename T>
  static T* pod_realloc(T* aPtr, size_t aOldSize, size_t aNewSize)
  {
    if (aNewSize & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
      return nullptr;
    return (T*)InfallibleAllocPolicy::realloc_((void *)aPtr, aNewSize * sizeof(T));
  }

  static void* memalign_(size_t aAlignment, size_t aSize)
  {
    void* p = gMallocTable->memalign(aAlignment, aSize);
    ExitOnFailure(p);
    return p;
  }

  static void free_(void* aPtr) { gMallocTable->free(aPtr); }

  static char* strdup_(const char* aStr)
  {
    char* s = (char*) InfallibleAllocPolicy::malloc_(strlen(aStr) + 1);
    strcpy(s, aStr);
    return s;
  }

  template <class T>
  static T* new_()
  {
    void* mem = malloc_(sizeof(T));
    ExitOnFailure(mem);
    return new (mem) T;
  }

  template <class T, typename P1>
  static T* new_(P1 p1)
  {
    void* mem = malloc_(sizeof(T));
    ExitOnFailure(mem);
    return new (mem) T(p1);
  }

  template <class T>
  static void delete_(T *p)
  {
    if (p) {
      p->~T();
      InfallibleAllocPolicy::free_(p);
    }
  }

  static void reportAllocOverflow() { ExitOnFailure(nullptr); }
};


static size_t
MallocSizeOf(const void* aPtr)
{
  return gMallocTable->malloc_usable_size(const_cast<void*>(aPtr));
}

void
DMDFuncs::StatusMsg(const char* aFmt, va_list aAp)
{
#ifdef ANDROID
#ifdef MOZ_B2G_LOADER
  
  
  if (gIsDMDInitialized)
#endif
    __android_log_vprint(ANDROID_LOG_INFO, "DMD", aFmt, aAp);
#else
  
  char* fmt = (char*) InfallibleAllocPolicy::malloc_(strlen(aFmt) + 64);
  sprintf(fmt, "DMD[%d] %s", getpid(), aFmt);
  vfprintf(stderr, fmt, aAp);
  InfallibleAllocPolicy::free_(fmt);
#endif
}

 void
InfallibleAllocPolicy::ExitOnFailure(const void* aP)
{
  if (!aP) {
    MOZ_CRASH("DMD out of memory; aborting");
  }
}

static double
Percent(size_t part, size_t whole)
{
  return (whole == 0) ? 0 : 100 * (double)part / whole;
}


static char*
Show(size_t n, char* buf, size_t buflen)
{
  int nc = 0, i = 0, lasti = buflen - 2;
  buf[lasti + 1] = '\0';
  if (n == 0) {
    buf[lasti - i] = '0';
    i++;
  } else {
    while (n > 0) {
      if (((i - nc) % 3) == 0 && i != 0) {
        buf[lasti - i] = ',';
        i++;
        nc++;
      }
      buf[lasti - i] = static_cast<char>((n % 10) + '0');
      i++;
      n /= 10;
    }
  }
  int firstCharIndex = lasti - i + 1;

  MOZ_ASSERT(firstCharIndex >= 0);
  return &buf[firstCharIndex];
}





class Options
{
  template <typename T>
  struct NumOption
  {
    const T mDefault;
    const T mMax;
    T       mActual;
    NumOption(T aDefault, T aMax)
      : mDefault(aDefault), mMax(aMax), mActual(aDefault)
    {}
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  enum Mode
  {
    
    
    
    Live,

    
    
    
    DarkMatter,

    
    
    
    Cumulative
  };

  char* mDMDEnvVar;   

  Mode mMode;
  NumOption<size_t> mSampleBelowSize;
  NumOption<uint32_t> mMaxFrames;
  bool mShowDumpStats;

  void BadArg(const char* aArg);
  static const char* ValueIfMatch(const char* aArg, const char* aOptionName);
  static bool GetLong(const char* aArg, const char* aOptionName,
                      long aMin, long aMax, long* aValue);
  static bool GetBool(const char* aArg, const char* aOptionName, bool* aValue);

public:
  explicit Options(const char* aDMDEnvVar);

  bool IsLiveMode()       const { return mMode == Live; }
  bool IsDarkMatterMode() const { return mMode == DarkMatter; }
  bool IsCumulativeMode() const { return mMode == Cumulative; }

  const char* DMDEnvVar() const { return mDMDEnvVar; }

  size_t SampleBelowSize() const { return mSampleBelowSize.mActual; }
  size_t MaxFrames()       const { return mMaxFrames.mActual; }
  size_t ShowDumpStats()   const { return mShowDumpStats; }
};

static Options *gOptions;







#ifdef XP_WIN

class MutexBase
{
  CRITICAL_SECTION mCS;

  DISALLOW_COPY_AND_ASSIGN(MutexBase);

public:
  MutexBase()  { InitializeCriticalSection(&mCS); }
  ~MutexBase() { DeleteCriticalSection(&mCS); }

  void Lock()   { EnterCriticalSection(&mCS); }
  void Unlock() { LeaveCriticalSection(&mCS); }
};

#else

#include <pthread.h>
#include <sys/types.h>

class MutexBase
{
  pthread_mutex_t mMutex;

  DISALLOW_COPY_AND_ASSIGN(MutexBase);

public:
  MutexBase() { pthread_mutex_init(&mMutex, nullptr); }

  void Lock()   { pthread_mutex_lock(&mMutex); }
  void Unlock() { pthread_mutex_unlock(&mMutex); }
};

#endif

class Mutex : private MutexBase
{
  bool mIsLocked;

  DISALLOW_COPY_AND_ASSIGN(Mutex);

public:
  Mutex()
    : mIsLocked(false)
  {}

  void Lock()
  {
    MutexBase::Lock();
    MOZ_ASSERT(!mIsLocked);
    mIsLocked = true;
  }

  void Unlock()
  {
    MOZ_ASSERT(mIsLocked);
    mIsLocked = false;
    MutexBase::Unlock();
  }

  bool IsLocked() { return mIsLocked; }
};






static Mutex* gStateLock = nullptr;

class AutoLockState
{
  DISALLOW_COPY_AND_ASSIGN(AutoLockState);

public:
  AutoLockState()  { gStateLock->Lock(); }
  ~AutoLockState() { gStateLock->Unlock(); }
};

class AutoUnlockState
{
  DISALLOW_COPY_AND_ASSIGN(AutoUnlockState);

public:
  AutoUnlockState()  { gStateLock->Unlock(); }
  ~AutoUnlockState() { gStateLock->Lock(); }
};





#ifdef XP_WIN

#define DMD_TLS_INDEX_TYPE              DWORD
#define DMD_CREATE_TLS_INDEX(i_)        do {                                  \
                                          (i_) = TlsAlloc();                  \
                                        } while (0)
#define DMD_DESTROY_TLS_INDEX(i_)       TlsFree((i_))
#define DMD_GET_TLS_DATA(i_)            TlsGetValue((i_))
#define DMD_SET_TLS_DATA(i_, v_)        TlsSetValue((i_), (v_))

#else

#include <pthread.h>

#define DMD_TLS_INDEX_TYPE               pthread_key_t
#define DMD_CREATE_TLS_INDEX(i_)         pthread_key_create(&(i_), nullptr)
#define DMD_DESTROY_TLS_INDEX(i_)        pthread_key_delete((i_))
#define DMD_GET_TLS_DATA(i_)             pthread_getspecific((i_))
#define DMD_SET_TLS_DATA(i_, v_)         pthread_setspecific((i_), (v_))

#endif

static DMD_TLS_INDEX_TYPE gTlsIndex;

class Thread
{
  
  friend class InfallibleAllocPolicy;

  
  
  
  
  bool mBlockIntercepts;

  Thread()
    : mBlockIntercepts(false)
  {}

  DISALLOW_COPY_AND_ASSIGN(Thread);

public:
  static Thread* Fetch();

  bool BlockIntercepts()
  {
    MOZ_ASSERT(!mBlockIntercepts);
    return mBlockIntercepts = true;
  }

  bool UnblockIntercepts()
  {
    MOZ_ASSERT(mBlockIntercepts);
    return mBlockIntercepts = false;
  }

  bool InterceptsAreBlocked() const { return mBlockIntercepts; }
};

 Thread*
Thread::Fetch()
{
  Thread* t = static_cast<Thread*>(DMD_GET_TLS_DATA(gTlsIndex));

  if (MOZ_UNLIKELY(!t)) {
    
    
    t = InfallibleAllocPolicy::new_<Thread>();
    DMD_SET_TLS_DATA(gTlsIndex, t);
  }

  return t;
}



class AutoBlockIntercepts
{
  Thread* const mT;

  DISALLOW_COPY_AND_ASSIGN(AutoBlockIntercepts);

public:
  explicit AutoBlockIntercepts(Thread* aT)
    : mT(aT)
  {
    mT->BlockIntercepts();
  }
  ~AutoBlockIntercepts()
  {
    MOZ_ASSERT(mT->InterceptsAreBlocked());
    mT->UnblockIntercepts();
  }
};





class StringTable
{
public:
  StringTable() { (void)mSet.init(64); }

  const char*
  Intern(const char* aString)
  {
    StringHashSet::AddPtr p = mSet.lookupForAdd(aString);
    if (p) {
      return *p;
    }

    const char* newString = InfallibleAllocPolicy::strdup_(aString);
    (void)mSet.add(p, newString);
    return newString;
  }

  size_t
  SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    size_t n = 0;
    n += mSet.sizeOfExcludingThis(aMallocSizeOf);
    for (auto r = mSet.all(); !r.empty(); r.popFront()) {
      n += aMallocSizeOf(r.front());
    }
    return n;
  }

private:
  struct StringHasher
  {
      typedef const char* Lookup;

      static uint32_t hash(const char* const& aS)
      {
          return HashString(aS);
      }

      static bool match(const char* const& aA, const char* const& aB)
      {
          return strcmp(aA, aB) == 0;
      }
  };

  typedef js::HashSet<const char*, StringHasher, InfallibleAllocPolicy> StringHashSet;

  StringHashSet mSet;
};

class StringAlloc
{
public:
  static char* copy(const char* aString)
  {
    return InfallibleAllocPolicy::strdup_(aString);
  }
  static void free(char* aString)
  {
    InfallibleAllocPolicy::free_(aString);
  }
};

struct DescribeCodeAddressLock
{
  static void Unlock() { gStateLock->Unlock(); }
  static void Lock() { gStateLock->Lock(); }
  static bool IsLocked() { return gStateLock->IsLocked(); }
};

typedef CodeAddressService<StringTable, StringAlloc, DescribeCodeAddressLock>
  CodeAddressService;





class StackTrace
{
public:
  static const uint32_t MaxFrames = 24;

private:
  uint32_t mLength;             
  const void* mPcs[MaxFrames];  
                                
                                

public:
  StackTrace() : mLength(0) {}

  uint32_t Length() const { return mLength; }
  const void* Pc(uint32_t i) const
  {
    MOZ_ASSERT(i < mLength);
    return mPcs[i];
  }

  uint32_t Size() const { return mLength * sizeof(mPcs[0]); }

  
  
  static const StackTrace* Get(Thread* aT);

  

  typedef StackTrace* Lookup;

  static uint32_t hash(const StackTrace* const& aSt)
  {
    return mozilla::HashBytes(aSt->mPcs, aSt->Size());
  }

  static bool match(const StackTrace* const& aA,
                    const StackTrace* const& aB)
  {
    return aA->mLength == aB->mLength &&
           memcmp(aA->mPcs, aB->mPcs, aA->Size()) == 0;
  }

private:
  static void StackWalkCallback(uint32_t aFrameNumber, void* aPc, void* aSp,
                                void* aClosure)
  {
    StackTrace* st = (StackTrace*) aClosure;
    MOZ_ASSERT(st->mLength < MaxFrames);
    st->mPcs[st->mLength] = aPc;
    st->mLength++;
    MOZ_ASSERT(st->mLength == aFrameNumber);
  }
};

typedef js::HashSet<StackTrace*, StackTrace, InfallibleAllocPolicy>
        StackTraceTable;
static StackTraceTable* gStackTraceTable = nullptr;

typedef js::HashSet<const StackTrace*, js::DefaultHasher<const StackTrace*>,
                    InfallibleAllocPolicy>
        StackTraceSet;

typedef js::HashSet<const void*, js::DefaultHasher<const void*>,
                    InfallibleAllocPolicy>
        PointerSet;
typedef js::HashMap<const void*, uint32_t, js::DefaultHasher<const void*>,
                    InfallibleAllocPolicy>
        PointerIdMap;


static uint32_t gGCStackTraceTableWhenSizeExceeds = 4 * 1024;

 const StackTrace*
StackTrace::Get(Thread* aT)
{
  MOZ_ASSERT(gStateLock->IsLocked());
  MOZ_ASSERT(aT->InterceptsAreBlocked());

  
  
  
  
  
  
  
  nsresult rv;
  StackTrace tmp;
  {
    AutoUnlockState unlock;
    uint32_t skipFrames = 2;
    rv = NS_StackWalk(StackWalkCallback, skipFrames,
                      gOptions->MaxFrames(), &tmp, 0, nullptr);
  }

  if (rv == NS_OK) {
    
  } else if (rv == NS_ERROR_NOT_IMPLEMENTED || rv == NS_ERROR_FAILURE) {
    tmp.mLength = 0;
  } else if (rv == NS_ERROR_UNEXPECTED) {
    
    
    
    
    
    
    
    
    
    
    MOZ_CRASH("unexpected case in StackTrace::Get()");
  } else {
    MOZ_CRASH("impossible case in StackTrace::Get()");
  }

  StackTraceTable::AddPtr p = gStackTraceTable->lookupForAdd(&tmp);
  if (!p) {
    StackTrace* stnew = InfallibleAllocPolicy::new_<StackTrace>(tmp);
    (void)gStackTraceTable->add(p, stnew);
  }
  return *p;
}










template <typename T>
class TaggedPtr
{
  union
  {
    T         mPtr;
    uintptr_t mUint;
  };

  static const uintptr_t kTagMask = uintptr_t(0x1);
  static const uintptr_t kPtrMask = ~kTagMask;

  static bool IsTwoByteAligned(T aPtr)
  {
    return (uintptr_t(aPtr) & kTagMask) == 0;
  }

public:
  TaggedPtr()
    : mPtr(nullptr)
  {}

  TaggedPtr(T aPtr, bool aBool)
    : mPtr(aPtr)
  {
    MOZ_ASSERT(IsTwoByteAligned(aPtr));
    uintptr_t tag = uintptr_t(aBool);
    MOZ_ASSERT(tag <= kTagMask);
    mUint |= (tag & kTagMask);
  }

  void Set(T aPtr, bool aBool)
  {
    MOZ_ASSERT(IsTwoByteAligned(aPtr));
    mPtr = aPtr;
    uintptr_t tag = uintptr_t(aBool);
    MOZ_ASSERT(tag <= kTagMask);
    mUint |= (tag & kTagMask);
  }

  T Ptr() const { return reinterpret_cast<T>(mUint & kPtrMask); }

  bool Tag() const { return bool(mUint & kTagMask); }
};



class LiveBlock
{
  const void*  mPtr;
  const size_t mReqSize;    

  
  
  
  
  TaggedPtr<const StackTrace* const>
    mAllocStackTrace_mIsSampled;

  
  
  
  
  
  
  
  
  
  
  
  
  mutable TaggedPtr<const StackTrace*> mReportStackTrace_mReportedOnAlloc[2];

public:
  LiveBlock(const void* aPtr, size_t aReqSize,
            const StackTrace* aAllocStackTrace, bool aIsSampled)
    : mPtr(aPtr)
    , mReqSize(aReqSize)
    , mAllocStackTrace_mIsSampled(aAllocStackTrace, aIsSampled)
    , mReportStackTrace_mReportedOnAlloc()     
  {
    MOZ_ASSERT(aAllocStackTrace);
  }

  const void* Address() const { return mPtr; }

  size_t ReqSize() const { return mReqSize; }

  
  size_t SlopSize() const
  {
    return IsSampled() ? 0 : MallocSizeOf(mPtr) - mReqSize;
  }

  size_t UsableSize() const
  {
    return IsSampled() ? mReqSize : MallocSizeOf(mPtr);
  }

  bool IsSampled() const
  {
    return mAllocStackTrace_mIsSampled.Tag();
  }

  const StackTrace* AllocStackTrace() const
  {
    return mAllocStackTrace_mIsSampled.Ptr();
  }

  const StackTrace* ReportStackTrace1() const
  {
    MOZ_ASSERT(gOptions->IsDarkMatterMode());
    return mReportStackTrace_mReportedOnAlloc[0].Ptr();
  }

  const StackTrace* ReportStackTrace2() const
  {
    MOZ_ASSERT(gOptions->IsDarkMatterMode());
    return mReportStackTrace_mReportedOnAlloc[1].Ptr();
  }

  bool ReportedOnAlloc1() const
  {
    MOZ_ASSERT(gOptions->IsDarkMatterMode());
    return mReportStackTrace_mReportedOnAlloc[0].Tag();
  }

  bool ReportedOnAlloc2() const
  {
    MOZ_ASSERT(gOptions->IsDarkMatterMode());
    return mReportStackTrace_mReportedOnAlloc[1].Tag();
  }

  void AddStackTracesToTable(StackTraceSet& aStackTraces) const
  {
    aStackTraces.put(AllocStackTrace());  
    if (gOptions->IsDarkMatterMode()) {
      const StackTrace* st;
      if ((st = ReportStackTrace1())) {     
        aStackTraces.put(st);
      }
      if ((st = ReportStackTrace2())) {     
        aStackTraces.put(st);
      }
    }
  }

  uint32_t NumReports() const
  {
    MOZ_ASSERT(gOptions->IsDarkMatterMode());
    if (ReportStackTrace2()) {
      MOZ_ASSERT(ReportStackTrace1());
      return 2;
    }
    if (ReportStackTrace1()) {
      return 1;
    }
    return 0;
  }

  
  void Report(Thread* aT, bool aReportedOnAlloc) const
  {
    MOZ_ASSERT(gOptions->IsDarkMatterMode());
    
    uint32_t numReports = NumReports();
    if (numReports < 2) {
      mReportStackTrace_mReportedOnAlloc[numReports].Set(StackTrace::Get(aT),
                                                         aReportedOnAlloc);
    }
  }

  void UnreportIfNotReportedOnAlloc() const
  {
    MOZ_ASSERT(gOptions->IsDarkMatterMode());
    if (!ReportedOnAlloc1() && !ReportedOnAlloc2()) {
      mReportStackTrace_mReportedOnAlloc[0].Set(nullptr, 0);
      mReportStackTrace_mReportedOnAlloc[1].Set(nullptr, 0);

    } else if (!ReportedOnAlloc1() && ReportedOnAlloc2()) {
      
      mReportStackTrace_mReportedOnAlloc[0] =
        mReportStackTrace_mReportedOnAlloc[1];
      mReportStackTrace_mReportedOnAlloc[1].Set(nullptr, 0);

    } else if (ReportedOnAlloc1() && !ReportedOnAlloc2()) {
      mReportStackTrace_mReportedOnAlloc[1].Set(nullptr, 0);
    }
  }

  

  typedef const void* Lookup;

  static uint32_t hash(const void* const& aPtr)
  {
    return mozilla::HashGeneric(aPtr);
  }

  static bool match(const LiveBlock& aB, const void* const& aPtr)
  {
    return aB.mPtr == aPtr;
  }
};

typedef js::HashSet<LiveBlock, LiveBlock, InfallibleAllocPolicy> LiveBlockTable;
static LiveBlockTable* gLiveBlockTable = nullptr;


class DeadBlock
{
  const size_t mReqSize;    
  const size_t mSlopSize;   

  
  
  TaggedPtr<const StackTrace* const>
    mAllocStackTrace_mIsSampled;

public:
  DeadBlock()
    : mReqSize(0)
    , mSlopSize(0)
    , mAllocStackTrace_mIsSampled(nullptr, 0)
  {}

  explicit DeadBlock(const LiveBlock& aLb)
    : mReqSize(aLb.ReqSize())
    , mSlopSize(aLb.SlopSize())
    , mAllocStackTrace_mIsSampled(aLb.AllocStackTrace(), aLb.IsSampled())
  {
    MOZ_ASSERT(AllocStackTrace());
    MOZ_ASSERT_IF(IsSampled(), SlopSize() == 0);
  }

  ~DeadBlock() {}

  size_t ReqSize()    const { return mReqSize; }
  size_t SlopSize()   const { return mSlopSize; }
  size_t UsableSize() const { return mReqSize + mSlopSize; }

  bool IsSampled() const
  {
    return mAllocStackTrace_mIsSampled.Tag();
  }

  const StackTrace* AllocStackTrace() const
  {
    return mAllocStackTrace_mIsSampled.Ptr();
  }

  void AddStackTracesToTable(StackTraceSet& aStackTraces) const
  {
    aStackTraces.put(AllocStackTrace());  
  }

  

  typedef DeadBlock Lookup;

  static uint32_t hash(const DeadBlock& aB)
  {
    return mozilla::HashGeneric(aB.ReqSize(),
                                aB.SlopSize(),
                                aB.IsSampled(),
                                aB.AllocStackTrace());
  }

  static bool match(const DeadBlock& aA, const DeadBlock& aB)
  {
    return aA.ReqSize() == aB.ReqSize() &&
           aA.SlopSize() == aB.SlopSize() &&
           aA.IsSampled() == aB.IsSampled() &&
           aA.AllocStackTrace() == aB.AllocStackTrace();
  }
};



typedef js::HashMap<DeadBlock, size_t, DeadBlock, InfallibleAllocPolicy>
  DeadBlockTable;
static DeadBlockTable* gDeadBlockTable = nullptr;


void MaybeAddToDeadBlockTable(const DeadBlock& aDb)
{
  if (gOptions->IsCumulativeMode() && aDb.AllocStackTrace()) {
    AutoLockState lock;
    if (DeadBlockTable::AddPtr p = gDeadBlockTable->lookupForAdd(aDb)) {
      p->value() += 1;
    } else {
      gDeadBlockTable->add(p, aDb, 1);
    }
  }
}



static void
GatherUsedStackTraces(StackTraceSet& aStackTraces)
{
  MOZ_ASSERT(gStateLock->IsLocked());
  MOZ_ASSERT(Thread::Fetch()->InterceptsAreBlocked());

  aStackTraces.finish();
  aStackTraces.init(512);

  for (auto r = gLiveBlockTable->all(); !r.empty(); r.popFront()) {
    r.front().AddStackTracesToTable(aStackTraces);
  }

  for (auto r = gDeadBlockTable->all(); !r.empty(); r.popFront()) {
    r.front().key().AddStackTracesToTable(aStackTraces);
  }
}


static void
GCStackTraces()
{
  MOZ_ASSERT(gStateLock->IsLocked());
  MOZ_ASSERT(Thread::Fetch()->InterceptsAreBlocked());

  StackTraceSet usedStackTraces;
  GatherUsedStackTraces(usedStackTraces);

  
  
  for (StackTraceTable::Enum e(*gStackTraceTable); !e.empty(); e.popFront()) {
    StackTrace* const& st = e.front();
    if (!usedStackTraces.has(st)) {
      e.removeFront();
      InfallibleAllocPolicy::delete_(st);
    }
  }

  
  
  gGCStackTraceTableWhenSizeExceeds = 2 * gStackTraceTable->count();
}





static size_t gSmallBlockActualSizeCounter = 0;

static void
AllocCallback(void* aPtr, size_t aReqSize, Thread* aT)
{
  if (!aPtr) {
    return;
  }

  AutoLockState lock;
  AutoBlockIntercepts block(aT);

  size_t actualSize = gMallocTable->malloc_usable_size(aPtr);
  size_t sampleBelowSize = gOptions->SampleBelowSize();

  if (actualSize < sampleBelowSize) {
    
    
    
    
    gSmallBlockActualSizeCounter += actualSize;
    if (gSmallBlockActualSizeCounter >= sampleBelowSize) {
      gSmallBlockActualSizeCounter -= sampleBelowSize;

      LiveBlock b(aPtr, sampleBelowSize, StackTrace::Get(aT),
                   true);
      (void)gLiveBlockTable->putNew(aPtr, b);
    }
  } else {
    
    LiveBlock b(aPtr, aReqSize, StackTrace::Get(aT),  false);
    (void)gLiveBlockTable->putNew(aPtr, b);
  }
}

static void
FreeCallback(void* aPtr, Thread* aT, DeadBlock* aDeadBlock)
{
  if (!aPtr) {
    return;
  }

  AutoLockState lock;
  AutoBlockIntercepts block(aT);

  if (LiveBlockTable::Ptr lb = gLiveBlockTable->lookup(aPtr)) {
    if (gOptions->IsCumulativeMode()) {
      
      new (aDeadBlock) DeadBlock(*lb);
    }
    gLiveBlockTable->remove(lb);
  } else {
    
    
    
  }

  if (gStackTraceTable->count() > gGCStackTraceTableWhenSizeExceeds) {
    GCStackTraces();
  }
}





static void Init(const malloc_table_t* aMallocTable);

}   
}   

void
replace_init(const malloc_table_t* aMallocTable)
{
  mozilla::dmd::Init(aMallocTable);
}

ReplaceMallocBridge*
replace_get_bridge()
{
  return mozilla::dmd::gDMDBridge;
}

void*
replace_malloc(size_t aSize)
{
  using namespace mozilla::dmd;

  if (!gIsDMDInitialized) {
    
    
    
    
    return gMallocTable->malloc(aSize);
  }

  Thread* t = Thread::Fetch();
  if (t->InterceptsAreBlocked()) {
    
    
    return InfallibleAllocPolicy::malloc_(aSize);
  }

  
  void* ptr = gMallocTable->malloc(aSize);
  AllocCallback(ptr, aSize, t);
  return ptr;
}

void*
replace_calloc(size_t aCount, size_t aSize)
{
  using namespace mozilla::dmd;

  if (!gIsDMDInitialized) {
    return gMallocTable->calloc(aCount, aSize);
  }

  Thread* t = Thread::Fetch();
  if (t->InterceptsAreBlocked()) {
    return InfallibleAllocPolicy::calloc_(aCount * aSize);
  }

  void* ptr = gMallocTable->calloc(aCount, aSize);
  AllocCallback(ptr, aCount * aSize, t);
  return ptr;
}

void*
replace_realloc(void* aOldPtr, size_t aSize)
{
  using namespace mozilla::dmd;

  if (!gIsDMDInitialized) {
    return gMallocTable->realloc(aOldPtr, aSize);
  }

  Thread* t = Thread::Fetch();
  if (t->InterceptsAreBlocked()) {
    return InfallibleAllocPolicy::realloc_(aOldPtr, aSize);
  }

  
  if (!aOldPtr) {
    return replace_malloc(aSize);
  }

  
  
  
  
  DeadBlock db;
  FreeCallback(aOldPtr, t, &db);
  void* ptr = gMallocTable->realloc(aOldPtr, aSize);
  if (ptr) {
    AllocCallback(ptr, aSize, t);
    MaybeAddToDeadBlockTable(db);
  } else {
    
    
    
    
    
    
    AllocCallback(aOldPtr, gMallocTable->malloc_usable_size(aOldPtr), t);
  }
  return ptr;
}

void*
replace_memalign(size_t aAlignment, size_t aSize)
{
  using namespace mozilla::dmd;

  if (!gIsDMDInitialized) {
    return gMallocTable->memalign(aAlignment, aSize);
  }

  Thread* t = Thread::Fetch();
  if (t->InterceptsAreBlocked()) {
    return InfallibleAllocPolicy::memalign_(aAlignment, aSize);
  }

  void* ptr = gMallocTable->memalign(aAlignment, aSize);
  AllocCallback(ptr, aSize, t);
  return ptr;
}

void
replace_free(void* aPtr)
{
  using namespace mozilla::dmd;

  if (!gIsDMDInitialized) {
    gMallocTable->free(aPtr);
    return;
  }

  Thread* t = Thread::Fetch();
  if (t->InterceptsAreBlocked()) {
    return InfallibleAllocPolicy::free_(aPtr);
  }

  
  
  
  DeadBlock db;
  FreeCallback(aPtr, t, &db);
  MaybeAddToDeadBlockTable(db);
  gMallocTable->free(aPtr);
}

namespace mozilla {
namespace dmd {








const char*
Options::ValueIfMatch(const char* aArg, const char* aOptionName)
{
  MOZ_ASSERT(!isspace(*aArg));  
  size_t optionLen = strlen(aOptionName);
  if (strncmp(aArg, aOptionName, optionLen) == 0 && aArg[optionLen] == '=' &&
      aArg[optionLen + 1]) {
    return aArg + optionLen + 1;
  }
  return nullptr;
}



bool
Options::GetLong(const char* aArg, const char* aOptionName,
                 long aMin, long aMax, long* aValue)
{
  if (const char* optionValue = ValueIfMatch(aArg, aOptionName)) {
    char* endPtr;
    *aValue = strtol(optionValue, &endPtr,  10);
    if (!*endPtr && aMin <= *aValue && *aValue <= aMax &&
        *aValue != LONG_MIN && *aValue != LONG_MAX) {
      return true;
    }
  }
  return false;
}



bool
Options::GetBool(const char* aArg, const char* aOptionName, bool* aValue)
{
  if (const char* optionValue = ValueIfMatch(aArg, aOptionName)) {
    if (strcmp(optionValue, "yes") == 0) {
      *aValue = true;
      return true;
    }
    if (strcmp(optionValue, "no") == 0) {
      *aValue = false;
      return true;
    }
  }
  return false;
}









Options::Options(const char* aDMDEnvVar)
  : mDMDEnvVar(aDMDEnvVar ? InfallibleAllocPolicy::strdup_(aDMDEnvVar)
                          : nullptr)
  , mMode(DarkMatter)
  , mSampleBelowSize(4093, 100 * 100 * 1000)
  , mMaxFrames(StackTrace::MaxFrames, StackTrace::MaxFrames)
  , mShowDumpStats(false)
{
  
  
  
  char* e = mDMDEnvVar;
  if (e && strcmp(e, "1") != 0) {
    bool isEnd = false;
    while (!isEnd) {
      
      while (isspace(*e)) {
        e++;
      }

      
      const char* arg = e;

      
      
      while (!isspace(*e) && *e != '\0') {
        e++;
      }
      char replacedChar = *e;
      isEnd = replacedChar == '\0';
      *e = '\0';

      
      long myLong;
      bool myBool;
      if (strcmp(arg, "--mode=live") == 0) {
        mMode = Options::Live;
      } else if (strcmp(arg, "--mode=dark-matter") == 0) {
        mMode = Options::DarkMatter;
      } else if (strcmp(arg, "--mode=cumulative") == 0) {
        mMode = Options::Cumulative;

      } else if (GetLong(arg, "--sample-below", 1, mSampleBelowSize.mMax,
                 &myLong)) {
        mSampleBelowSize.mActual = myLong;

      } else if (GetLong(arg, "--max-frames", 1, mMaxFrames.mMax, &myLong)) {
        mMaxFrames.mActual = myLong;

      } else if (GetBool(arg, "--show-dump-stats", &myBool)) {
        mShowDumpStats = myBool;

      } else if (strcmp(arg, "") == 0) {
        
        MOZ_ASSERT(isEnd);

      } else {
        BadArg(arg);
      }

      
      *e = replacedChar;
    }
  }
}

void
Options::BadArg(const char* aArg)
{
  StatusMsg("\n");
  StatusMsg("Bad entry in the $DMD environment variable: '%s'.\n", aArg);
  StatusMsg("\n");
  StatusMsg("$DMD must be a whitespace-separated list of |--option=val|\n");
  StatusMsg("entries.\n");
  StatusMsg("\n");
  StatusMsg("The following options are allowed;  defaults are shown in [].\n");
  StatusMsg("  --mode=<mode>                Profiling mode [dark-matter]\n");
  StatusMsg("      where <mode> is one of: live, dark-matter, cumulative\n");
  StatusMsg("  --sample-below=<1..%d> Sample blocks smaller than this [%d]\n",
            int(mSampleBelowSize.mMax),
            int(mSampleBelowSize.mDefault));
  StatusMsg("                               (prime numbers are recommended)\n");
  StatusMsg("  --max-frames=<1..%d>         Max. depth of stack traces [%d]\n",
            int(mMaxFrames.mMax),
            int(mMaxFrames.mDefault));
  StatusMsg("  --show-dump-stats=<yes|no>   Show stats about dumps? [no]\n");
  StatusMsg("\n");
  exit(1);
}





#ifdef XP_MACOSX
static void
NopStackWalkCallback(uint32_t aFrameNumber, void* aPc, void* aSp,
                     void* aClosure)
{
}
#endif





static void
Init(const malloc_table_t* aMallocTable)
{
  gMallocTable = aMallocTable;
  gDMDBridge = InfallibleAllocPolicy::new_<DMDBridge>();

  
  const char* e = getenv("DMD");

  if (e) {
    StatusMsg("$DMD = '%s'\n", e);
  } else {
    StatusMsg("$DMD is undefined\n", e);
  }

  
  gOptions = InfallibleAllocPolicy::new_<Options>(e);

#ifdef XP_MACOSX
  
  
  
  
  
  
  (void)NS_StackWalk(NopStackWalkCallback,  0,
                      1, nullptr, 0, nullptr);
#endif

  gStateLock = InfallibleAllocPolicy::new_<Mutex>();

  gSmallBlockActualSizeCounter = 0;

  DMD_CREATE_TLS_INDEX(gTlsIndex);

  {
    AutoLockState lock;

    gStackTraceTable = InfallibleAllocPolicy::new_<StackTraceTable>();
    gStackTraceTable->init(8192);

    gLiveBlockTable = InfallibleAllocPolicy::new_<LiveBlockTable>();
    gLiveBlockTable->init(8192);

    
    
    
    gDeadBlockTable = InfallibleAllocPolicy::new_<DeadBlockTable>();
    gDeadBlockTable->init(gOptions->IsCumulativeMode() ? 8192 : 4);
  }

  gIsDMDInitialized = true;
}





static void
ReportHelper(const void* aPtr, bool aReportedOnAlloc)
{
  if (!gOptions->IsDarkMatterMode() || !aPtr) {
    return;
  }

  Thread* t = Thread::Fetch();

  AutoBlockIntercepts block(t);
  AutoLockState lock;

  if (LiveBlockTable::Ptr p = gLiveBlockTable->lookup(aPtr)) {
    p->Report(t, aReportedOnAlloc);
  } else {
    
    
    
    
  }
}

void
DMDFuncs::Report(const void* aPtr)
{
  ReportHelper(aPtr,  false);
}

void
DMDFuncs::ReportOnAlloc(const void* aPtr)
{
  ReportHelper(aPtr,  true);
}








static const int kOutputVersionNumber = 4;










static void
SizeOfInternal(Sizes* aSizes)
{
  MOZ_ASSERT(gStateLock->IsLocked());
  MOZ_ASSERT(Thread::Fetch()->InterceptsAreBlocked());

  aSizes->Clear();

  StackTraceSet usedStackTraces;
  GatherUsedStackTraces(usedStackTraces);

  for (auto r = gStackTraceTable->all(); !r.empty(); r.popFront()) {
    StackTrace* const& st = r.front();

    if (usedStackTraces.has(st)) {
      aSizes->mStackTracesUsed += MallocSizeOf(st);
    } else {
      aSizes->mStackTracesUnused += MallocSizeOf(st);
    }
  }

  aSizes->mStackTraceTable =
    gStackTraceTable->sizeOfIncludingThis(MallocSizeOf);

  aSizes->mLiveBlockTable = gLiveBlockTable->sizeOfIncludingThis(MallocSizeOf);

  aSizes->mDeadBlockTable = gDeadBlockTable->sizeOfIncludingThis(MallocSizeOf);
}

void
DMDFuncs::SizeOf(Sizes* aSizes)
{
  aSizes->Clear();

  AutoBlockIntercepts block(Thread::Fetch());
  AutoLockState lock;
  SizeOfInternal(aSizes);
}

void
DMDFuncs::ClearReports()
{
  if (!gOptions->IsDarkMatterMode()) {
    return;
  }

  AutoLockState lock;

  
  
  
  for (auto r = gLiveBlockTable->all(); !r.empty(); r.popFront()) {
    r.front().UnreportIfNotReportedOnAlloc();
  }
}

class ToIdStringConverter final
{
public:
  ToIdStringConverter() : mNextId(0) { mIdMap.init(512); }

  
  
  const char* ToIdString(const void* aPtr)
  {
    uint32_t id;
    PointerIdMap::AddPtr p = mIdMap.lookupForAdd(aPtr);
    if (!p) {
      id = mNextId++;
      (void)mIdMap.add(p, aPtr, id);
    } else {
      id = p->value();
    }
    return Base32(id);
  }

  size_t sizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return mIdMap.sizeOfExcludingThis(aMallocSizeOf);
  }

private:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  char* Base32(uint32_t aN)
  {
    static const char digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef";

    char* b = mIdBuf + kIdBufLen - 1;
    *b = '\0';
    do {
      b--;
      if (b == mIdBuf) {
        MOZ_CRASH("Base32 buffer too small");
      }
      *b = digits[aN % 32];
      aN /= 32;
    } while (aN);

    return b;
  }

  PointerIdMap mIdMap;
  uint32_t mNextId;
  static const size_t kIdBufLen = 16;
  char mIdBuf[kIdBufLen];
};

static void
AnalyzeImpl(UniquePtr<JSONWriteFunc> aWriter)
{
  AutoBlockIntercepts block(Thread::Fetch());
  AutoLockState lock;

  
  auto locService = InfallibleAllocPolicy::new_<CodeAddressService>();

  StackTraceSet usedStackTraces;
  usedStackTraces.init(512);

  PointerSet usedPcs;
  usedPcs.init(512);

  size_t iscSize;

  static int analysisCount = 1;
  StatusMsg("Dump %d {\n", analysisCount++);

  JSONWriter writer(Move(aWriter));
  writer.Start();
  {
    writer.IntProperty("version", kOutputVersionNumber);

    writer.StartObjectProperty("invocation");
    {
      const char* var = gOptions->DMDEnvVar();
      if (var) {
        writer.StringProperty("dmdEnvVar", var);
      } else {
        writer.NullProperty("dmdEnvVar");
      }

      const char* mode;
      if (gOptions->IsLiveMode()) {
        mode = "live";
      } else if (gOptions->IsDarkMatterMode()) {
        mode = "dark-matter";
      } else if (gOptions->IsCumulativeMode()) {
        mode = "cumulative";
      } else {
        MOZ_ASSERT(false);
        mode = "(unknown DMD mode)";
      }
      writer.StringProperty("mode", mode);

      writer.IntProperty("sampleBelowSize", gOptions->SampleBelowSize());
    }
    writer.EndObject();

    StatusMsg("  Constructing the heap block list...\n");

    ToIdStringConverter isc;

    writer.StartArrayProperty("blockList");
    {
      
      for (auto r = gLiveBlockTable->all(); !r.empty(); r.popFront()) {
        const LiveBlock& b = r.front();
        b.AddStackTracesToTable(usedStackTraces);

        writer.StartObjectElement(writer.SingleLineStyle);
        {
          if (!b.IsSampled()) {
            writer.IntProperty("req", b.ReqSize());
            if (b.SlopSize() > 0) {
              writer.IntProperty("slop", b.SlopSize());
            }
          }
          writer.StringProperty("alloc", isc.ToIdString(b.AllocStackTrace()));
          if (gOptions->IsDarkMatterMode() && b.NumReports() > 0) {
            MOZ_ASSERT(gOptions->IsDarkMatterMode());
            writer.StartArrayProperty("reps");
            {
              if (b.ReportStackTrace1()) {
                writer.StringElement(isc.ToIdString(b.ReportStackTrace1()));
              }
              if (b.ReportStackTrace2()) {
                writer.StringElement(isc.ToIdString(b.ReportStackTrace2()));
              }
            }
            writer.EndArray();
          }
        }
        writer.EndObject();
      }

      
      for (auto r = gDeadBlockTable->all(); !r.empty(); r.popFront()) {
        const DeadBlock& b = r.front().key();
        b.AddStackTracesToTable(usedStackTraces);

        size_t num = r.front().value();
        MOZ_ASSERT(num > 0);

        writer.StartObjectElement(writer.SingleLineStyle);
        {
          if (!b.IsSampled()) {
            writer.IntProperty("req", b.ReqSize());
            if (b.SlopSize() > 0) {
              writer.IntProperty("slop", b.SlopSize());
            }
          }
          writer.StringProperty("alloc", isc.ToIdString(b.AllocStackTrace()));

          if (num > 1) {
            writer.IntProperty("num", num);
          }
        }
        writer.EndObject();
      }
    }
    writer.EndArray();

    StatusMsg("  Constructing the stack trace table...\n");

    writer.StartObjectProperty("traceTable");
    {
      for (auto r = usedStackTraces.all(); !r.empty(); r.popFront()) {
        const StackTrace* const st = r.front();
        writer.StartArrayProperty(isc.ToIdString(st), writer.SingleLineStyle);
        {
          for (uint32_t i = 0; i < st->Length(); i++) {
            const void* pc = st->Pc(i);
            writer.StringElement(isc.ToIdString(pc));
            usedPcs.put(pc);
          }
        }
        writer.EndArray();
      }
    }
    writer.EndObject();

    StatusMsg("  Constructing the stack frame table...\n");

    writer.StartObjectProperty("frameTable");
    {
      static const size_t locBufLen = 1024;
      char locBuf[locBufLen];

      for (PointerSet::Enum e(usedPcs); !e.empty(); e.popFront()) {
        const void* const pc = e.front();

        
        
        locService->GetLocation(0, pc, locBuf, locBufLen);
        writer.StringProperty(isc.ToIdString(pc), locBuf);
      }
    }
    writer.EndObject();

    iscSize = isc.sizeOfExcludingThis(MallocSizeOf);
  }
  writer.End();

  if (gOptions->ShowDumpStats()) {
    Sizes sizes;
    SizeOfInternal(&sizes);

    static const size_t kBufLen = 64;
    char buf1[kBufLen];
    char buf2[kBufLen];
    char buf3[kBufLen];

    StatusMsg("  Execution measurements {\n");

    StatusMsg("    Data structures that persist after Dump() ends {\n");

    StatusMsg("      Used stack traces:    %10s bytes\n",
      Show(sizes.mStackTracesUsed, buf1, kBufLen));

    StatusMsg("      Unused stack traces:  %10s bytes\n",
      Show(sizes.mStackTracesUnused, buf1, kBufLen));

    StatusMsg("      Stack trace table:    %10s bytes (%s entries, %s used)\n",
      Show(sizes.mStackTraceTable,       buf1, kBufLen),
      Show(gStackTraceTable->capacity(), buf2, kBufLen),
      Show(gStackTraceTable->count(),    buf3, kBufLen));

    StatusMsg("      Live block table:     %10s bytes (%s entries, %s used)\n",
      Show(sizes.mLiveBlockTable,       buf1, kBufLen),
      Show(gLiveBlockTable->capacity(), buf2, kBufLen),
      Show(gLiveBlockTable->count(),    buf3, kBufLen));

    StatusMsg("      Dead block table:     %10s bytes (%s entries, %s used)\n",
      Show(sizes.mDeadBlockTable,       buf1, kBufLen),
      Show(gDeadBlockTable->capacity(), buf2, kBufLen),
      Show(gDeadBlockTable->count(),    buf3, kBufLen));

    StatusMsg("    }\n");
    StatusMsg("    Data structures that are destroyed after Dump() ends {\n");

    StatusMsg("      Location service:      %10s bytes\n",
      Show(locService->SizeOfIncludingThis(MallocSizeOf), buf1, kBufLen));
    StatusMsg("      Used stack traces set: %10s bytes\n",
      Show(usedStackTraces.sizeOfExcludingThis(MallocSizeOf), buf1, kBufLen));
    StatusMsg("      Used PCs set:          %10s bytes\n",
      Show(usedPcs.sizeOfExcludingThis(MallocSizeOf), buf1, kBufLen));
    StatusMsg("      Pointer ID map:        %10s bytes\n",
      Show(iscSize, buf1, kBufLen));

    StatusMsg("    }\n");
    StatusMsg("    Counts {\n");

    size_t hits   = locService->NumCacheHits();
    size_t misses = locService->NumCacheMisses();
    size_t requests = hits + misses;
    StatusMsg("      Location service:    %10s requests\n",
      Show(requests, buf1, kBufLen));

    size_t count    = locService->CacheCount();
    size_t capacity = locService->CacheCapacity();
    StatusMsg("      Location service cache:  "
      "%4.1f%% hit rate, %.1f%% occupancy at end\n",
      Percent(hits, requests), Percent(count, capacity));

    StatusMsg("    }\n");
    StatusMsg("  }\n");
  }

  InfallibleAllocPolicy::delete_(locService);

  StatusMsg("}\n");
}

void
DMDFuncs::Analyze(UniquePtr<JSONWriteFunc> aWriter)
{
  AnalyzeImpl(Move(aWriter));
  ClearReports();
}





void
DMDFuncs::ResetEverything(const char* aOptions)
{
  AutoLockState lock;

  
  InfallibleAllocPolicy::delete_(gOptions);
  gOptions = InfallibleAllocPolicy::new_<Options>(aOptions);

  
  gLiveBlockTable->clear();
  gDeadBlockTable->clear();
  gSmallBlockActualSizeCounter = 0;
}

}   
}   
