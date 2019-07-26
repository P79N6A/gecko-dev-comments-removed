





#include "DMD.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef XP_WIN
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
#include "mozilla/Likely.h"





#define MOZ_REPLACE_ONLY_MEMALIGN 1
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
#include "replace_malloc.h"
#undef MOZ_REPLACE_ONLY_MEMALIGN
#undef PAGE_SIZE

namespace mozilla {
namespace dmd {





#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(T) \
  T(const T&);                      \
  void operator=(const T&)
#endif

static const malloc_table_t* gMallocTable = nullptr;


static bool gIsDMDRunning = false;









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

  static void* calloc_(size_t aSize)
  {
    void* p = gMallocTable->calloc(1, aSize);
    ExitOnFailure(p);
    return p;
  }

  
  static void* realloc_(void* aPtr, size_t aNewSize)
  {
    void* p = gMallocTable->realloc(aPtr, aNewSize);
    ExitOnFailure(p);
    return p;
  }

  
  static void* realloc_(void* aPtr, size_t aOldSize, size_t aNewSize)
  {
    return InfallibleAllocPolicy::realloc_(aPtr, aNewSize);
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

static void
StatusMsg(const char* aFmt, ...)
{
  va_list ap;
  va_start(ap, aFmt);
#ifdef ANDROID
  __android_log_vprint(ANDROID_LOG_INFO, "DMD", aFmt, ap);
#else
  
  char* fmt = (char*) InfallibleAllocPolicy::malloc_(strlen(aFmt) + 64);
  sprintf(fmt, "DMD[%d] %s", getpid(), aFmt);
  vfprintf(stderr, fmt, ap);
  InfallibleAllocPolicy::free_(fmt);
#endif
  va_end(ap);
}

 void
InfallibleAllocPolicy::ExitOnFailure(const void* aP)
{
  if (!aP) {
    StatusMsg("out of memory;  aborting\n");
    MOZ_CRASH();
  }
}

void
Writer::Write(const char* aFmt, ...) const
{
  va_list ap;
  va_start(ap, aFmt);
  mWriterFun(mWriteState, aFmt, ap);
  va_end(ap);
}

#define W(...) aWriter.Write(__VA_ARGS__);

#define WriteTitle(...)                                                       \
  W("------------------------------------------------------------------\n");  \
  W(__VA_ARGS__);                                                             \
  W("------------------------------------------------------------------\n\n");

MOZ_EXPORT void
FpWrite(void* aWriteState, const char* aFmt, va_list aAp)
{
  FILE* fp = static_cast<FILE*>(aWriteState);
  vfprintf(fp, aFmt, aAp);
}

static double
Percent(size_t part, size_t whole)
{
  return (whole == 0) ? 0 : 100 * (double)part / whole;
}




static char*
Show(size_t n, char* buf, size_t buflen, bool addTilde = false)
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

  if (addTilde) {
    firstCharIndex--;
    buf[firstCharIndex] = '~';
  }

  MOZ_ASSERT(firstCharIndex >= 0);
  return &buf[firstCharIndex];
}

static const char*
Plural(size_t aN)
{
  return aN == 1 ? "" : "s";
}


static const size_t kBufLen = 64;
static char gBuf1[kBufLen];
static char gBuf2[kBufLen];
static char gBuf3[kBufLen];
static char gBuf4[kBufLen];





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

  enum Mode {
    Normal,   
    Test,     
    Stress    
  };

  char* mDMDEnvVar;   

  NumOption<size_t>   mSampleBelowSize;
  NumOption<uint32_t> mMaxFrames;
  NumOption<uint32_t> mMaxRecords;
  Mode mMode;

  void BadArg(const char* aArg);
  static const char* ValueIfMatch(const char* aArg, const char* aOptionName);
  static bool GetLong(const char* aArg, const char* aOptionName,
                      long aMin, long aMax, long* aN);

public:
  Options(const char* aDMDEnvVar);

  const char* DMDEnvVar() const { return mDMDEnvVar; }

  size_t SampleBelowSize() const { return mSampleBelowSize.mActual; }
  size_t MaxFrames()       const { return mMaxFrames.mActual; }
  size_t MaxRecords()      const { return mMaxRecords.mActual; }

  void SetSampleBelowSize(size_t aN) { mSampleBelowSize.mActual = aN; }

  bool IsTestMode()   const { return mMode == Test; }
  bool IsStressMode() const { return mMode == Stress; }
};

static Options *gOptions;







#ifdef XP_WIN

class MutexBase
{
  CRITICAL_SECTION mCS;

  DISALLOW_COPY_AND_ASSIGN(MutexBase);

public:
  MutexBase()
  {
    InitializeCriticalSection(&mCS);
  }

  ~MutexBase()
  {
    DeleteCriticalSection(&mCS);
  }

  void Lock()
  {
    EnterCriticalSection(&mCS);
  }

  void Unlock()
  {
    LeaveCriticalSection(&mCS);
  }
};

#else

#include <pthread.h>
#include <sys/types.h>

class MutexBase
{
  pthread_mutex_t mMutex;

  DISALLOW_COPY_AND_ASSIGN(MutexBase);

public:
  MutexBase()
  {
    pthread_mutex_init(&mMutex, NULL);
  }

  void Lock()
  {
    pthread_mutex_lock(&mMutex);
  }

  void Unlock()
  {
    pthread_mutex_unlock(&mMutex);
  }
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

  bool IsLocked()
  {
    return mIsLocked;
  }
};



static Mutex* gStateLock = nullptr;

class AutoLockState
{
  DISALLOW_COPY_AND_ASSIGN(AutoLockState);

public:
  AutoLockState()
  {
    gStateLock->Lock();
  }
  ~AutoLockState()
  {
    gStateLock->Unlock();
  }
};

class AutoUnlockState
{
  DISALLOW_COPY_AND_ASSIGN(AutoUnlockState);

public:
  AutoUnlockState()
  {
    gStateLock->Unlock();
  }
  ~AutoUnlockState()
  {
    gStateLock->Lock();
  }
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

  bool InterceptsAreBlocked() const
  {
    return mBlockIntercepts;
  }
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
  AutoBlockIntercepts(Thread* aT)
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






class LocationService
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

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

  typedef js::HashSet<const char*, StringHasher, InfallibleAllocPolicy>
          StringTable;

  StringTable mLibraryStrings;

  struct Entry
  {
    static const void* const kUnused;

    const void* mPc;        
    char*       mFunction;  
    const char* mLibrary;   
                            
    ptrdiff_t   mLOffset;

    Entry()
      : mPc(kUnused), mFunction(nullptr), mLibrary(nullptr), mLOffset(0)
    {}

    ~Entry()
    {
      
      InfallibleAllocPolicy::free_(mFunction);
    }

    void Replace(const void* aPc, const char* aFunction, const char* aLibrary,
                 ptrdiff_t aLOffset)
    {
      mPc = aPc;

      
      InfallibleAllocPolicy::free_(mFunction);
      mFunction =
        !aFunction[0] ? nullptr : InfallibleAllocPolicy::strdup_(aFunction);

      mLibrary = aLibrary;
      mLOffset = aLOffset;
    }

    size_t SizeOfExcludingThis() {
      
      return MallocSizeOf(mFunction);
    }
  };

  
  
  
  
  
  
  static const size_t kNumEntries = 1 << 12;
  static const size_t kMask = kNumEntries - 1;
  Entry mEntries[kNumEntries];

  size_t mNumCacheHits;
  size_t mNumCacheMisses;

public:
  LocationService()
    : mEntries(), mNumCacheHits(0), mNumCacheMisses(0)
  {
    (void)mLibraryStrings.init(64);
  }

  void WriteLocation(const Writer& aWriter, const void* aPc)
  {
    MOZ_ASSERT(gStateLock->IsLocked());

    uint32_t index = HashGeneric(aPc) & kMask;
    MOZ_ASSERT(index < kNumEntries);
    Entry& entry = mEntries[index];

    MOZ_ASSERT(aPc != Entry::kUnused);
    if (entry.mPc != aPc) {
      mNumCacheMisses++;

      
      
      
      
      
      nsCodeAddressDetails details;
      {
        AutoUnlockState unlock;
        (void)NS_DescribeCodeAddress(const_cast<void*>(aPc), &details);
      }

      
      const char* library = nullptr;
      StringTable::AddPtr p = mLibraryStrings.lookupForAdd(details.library);
      if (!p) {
        library = InfallibleAllocPolicy::strdup_(details.library);
        (void)mLibraryStrings.add(p, library);
      } else {
        library = *p;
      }

      entry.Replace(aPc, details.function, library, details.loffset);

    } else {
      mNumCacheHits++;
    }

    MOZ_ASSERT(entry.mPc == aPc);

    
    
    if (!entry.mFunction && !entry.mLibrary[0] && entry.mLOffset == 0) {
      W("   ??? %p\n", entry.mPc);
    } else {
      
      W("   %s[%s +0x%X] %p\n", entry.mFunction ? entry.mFunction : "???",
        entry.mLibrary, entry.mLOffset, entry.mPc);
    }
  }

  size_t SizeOfIncludingThis()
  {
    size_t n = MallocSizeOf(this);
    for (uint32_t i = 0; i < kNumEntries; i++) {
      n += mEntries[i].SizeOfExcludingThis();
    }

    n += mLibraryStrings.sizeOfExcludingThis(MallocSizeOf);
    for (StringTable::Range r = mLibraryStrings.all();
         !r.empty();
         r.popFront()) {
      n += MallocSizeOf(r.front());
    }

    return n;
  }

  size_t CacheCapacity() const { return kNumEntries; }

  size_t CacheCount() const
  {
    size_t n = 0;
    for (size_t i = 0; i < kNumEntries; i++) {
      if (mEntries[i].mPc != Entry::kUnused) {
        n++;
      }
    }
    return n;
  }

  size_t NumCacheHits()   const { return mNumCacheHits; }
  size_t NumCacheMisses() const { return mNumCacheMisses; }
};


const void* const LocationService::Entry::kUnused =
  reinterpret_cast<const void* const>(intptr_t(-1));





class StackTrace
{
public:
  static const uint32_t MaxFrames = 24;

private:
  uint32_t mLength;         
  void* mPcs[MaxFrames];    
                            
                            

public:
  StackTrace() : mLength(0) {}

  uint32_t Length() const { return mLength; }
  void* Pc(uint32_t i) const { MOZ_ASSERT(i < mLength); return mPcs[i]; }

  uint32_t Size() const { return mLength * sizeof(mPcs[0]); }

  
  
  static const StackTrace* Get(Thread* aT);

  void Sort()
  {
    qsort(mPcs, mLength, sizeof(mPcs[0]), StackTrace::QsortCmp);
  }

  void Print(const Writer& aWriter, LocationService* aLocService) const;

  

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
  static void StackWalkCallback(void* aPc, void* aSp, void* aClosure)
  {
    StackTrace* st = (StackTrace*) aClosure;
    MOZ_ASSERT(st->mLength < MaxFrames);
    st->mPcs[st->mLength] = aPc;
    st->mLength++;
  }

  static int QsortCmp(const void* aA, const void* aB)
  {
    const void* const a = *static_cast<const void* const*>(aA);
    const void* const b = *static_cast<const void* const*>(aB);
    if (a < b) return -1;
    if (a > b) return  1;
    return 0;
  }
};

typedef js::HashSet<StackTrace*, StackTrace, InfallibleAllocPolicy>
        StackTraceTable;
static StackTraceTable* gStackTraceTable = nullptr;


static uint32_t gGCStackTraceTableWhenSizeExceeds = 4 * 1024;

void
StackTrace::Print(const Writer& aWriter, LocationService* aLocService) const
{
  if (mLength == 0) {
    W("   (empty)\n");  
    return;
  }

  for (uint32_t i = 0; i < mLength; i++) {
    aLocService->WriteLocation(aWriter, Pc(i));
  }
}

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
    
    
    
    
    
    
    
    
    
    
    MOZ_CRASH();
  } else {
    MOZ_CRASH();  
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


class Block
{
  const void*  mPtr;
  const size_t mReqSize;    

  
  
  TaggedPtr<const StackTrace* const>
    mAllocStackTrace_mSampled;

  
  
  
  
  
  
  
  
  
  
  mutable TaggedPtr<const StackTrace*> mReportStackTrace_mReportedOnAlloc[2];

public:
  Block(const void* aPtr, size_t aReqSize, const StackTrace* aAllocStackTrace,
        bool aSampled)
    : mPtr(aPtr),
      mReqSize(aReqSize),
      mAllocStackTrace_mSampled(aAllocStackTrace, aSampled),
      mReportStackTrace_mReportedOnAlloc()     
  {
    MOZ_ASSERT(aAllocStackTrace);
  }

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
    return mAllocStackTrace_mSampled.Tag();
  }

  const StackTrace* AllocStackTrace() const
  {
    return mAllocStackTrace_mSampled.Ptr();
  }

  const StackTrace* ReportStackTrace1() const {
    return mReportStackTrace_mReportedOnAlloc[0].Ptr();
  }

  const StackTrace* ReportStackTrace2() const {
    return mReportStackTrace_mReportedOnAlloc[1].Ptr();
  }

  bool ReportedOnAlloc1() const {
    return mReportStackTrace_mReportedOnAlloc[0].Tag();
  }

  bool ReportedOnAlloc2() const {
    return mReportStackTrace_mReportedOnAlloc[1].Tag();
  }

  uint32_t NumReports() const {
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
    
    uint32_t numReports = NumReports();
    if (numReports < 2) {
      mReportStackTrace_mReportedOnAlloc[numReports].Set(StackTrace::Get(aT),
                                                         aReportedOnAlloc);
    }
  }

  void UnreportIfNotReportedOnAlloc() const
  {
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

  static bool match(const Block& aB, const void* const& aPtr)
  {
    return aB.mPtr == aPtr;
  }
};

typedef js::HashSet<Block, Block, InfallibleAllocPolicy> BlockTable;
static BlockTable* gBlockTable = nullptr;

typedef js::HashSet<const StackTrace*, js::DefaultHasher<const StackTrace*>,
                    InfallibleAllocPolicy>
        StackTraceSet;



static void
GatherUsedStackTraces(StackTraceSet& aStackTraces)
{
  MOZ_ASSERT(gStateLock->IsLocked());
  MOZ_ASSERT(Thread::Fetch()->InterceptsAreBlocked());

  aStackTraces.finish();
  aStackTraces.init(1024);

  for (BlockTable::Range r = gBlockTable->all(); !r.empty(); r.popFront()) {
    const Block& b = r.front();
    aStackTraces.put(b.AllocStackTrace());
    aStackTraces.put(b.ReportStackTrace1());
    aStackTraces.put(b.ReportStackTrace2());
  }

  
  
  aStackTraces.remove(nullptr);
}


static void
GCStackTraces()
{
  MOZ_ASSERT(gStateLock->IsLocked());
  MOZ_ASSERT(Thread::Fetch()->InterceptsAreBlocked());

  StackTraceSet usedStackTraces;
  GatherUsedStackTraces(usedStackTraces);

  
  
  for (StackTraceTable::Enum e(*gStackTraceTable);
       !e.empty();
       e.popFront()) {
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
  MOZ_ASSERT(gIsDMDRunning);

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

      Block b(aPtr, sampleBelowSize, StackTrace::Get(aT),  true);
      (void)gBlockTable->putNew(aPtr, b);
    }
  } else {
    
    Block b(aPtr, aReqSize, StackTrace::Get(aT),  false);
    (void)gBlockTable->putNew(aPtr, b);
  }
}

static void
FreeCallback(void* aPtr, Thread* aT)
{
  MOZ_ASSERT(gIsDMDRunning);

  if (!aPtr) {
    return;
  }

  AutoLockState lock;
  AutoBlockIntercepts block(aT);

  gBlockTable->remove(aPtr);

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

void*
replace_malloc(size_t aSize)
{
  using namespace mozilla::dmd;

  if (!gIsDMDRunning) {
    
    
    
    
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

  if (!gIsDMDRunning) {
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

  if (!gIsDMDRunning) {
    return gMallocTable->realloc(aOldPtr, aSize);
  }

  Thread* t = Thread::Fetch();
  if (t->InterceptsAreBlocked()) {
    return InfallibleAllocPolicy::realloc_(aOldPtr, aSize);
  }

  
  if (!aOldPtr) {
    return replace_malloc(aSize);
  }

  
  
  
  
  FreeCallback(aOldPtr, t);
  void* ptr = gMallocTable->realloc(aOldPtr, aSize);
  if (ptr) {
    AllocCallback(ptr, aSize, t);
  } else {
    
    
    
    
    AllocCallback(aOldPtr, gMallocTable->malloc_usable_size(aOldPtr), t);
  }
  return ptr;
}

void*
replace_memalign(size_t aAlignment, size_t aSize)
{
  using namespace mozilla::dmd;

  if (!gIsDMDRunning) {
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

  if (!gIsDMDRunning) {
    gMallocTable->free(aPtr);
    return;
  }

  Thread* t = Thread::Fetch();
  if (t->InterceptsAreBlocked()) {
    return InfallibleAllocPolicy::free_(aPtr);
  }

  
  
  
  FreeCallback(aPtr, t);
  gMallocTable->free(aPtr);
}

namespace mozilla {
namespace dmd {





class TraceRecordKey
{
public:
  const StackTrace* const mAllocStackTrace;   
protected:
  const StackTrace* const mReportStackTrace1; 
  const StackTrace* const mReportStackTrace2; 

public:
  TraceRecordKey(const Block& aB)
    : mAllocStackTrace(aB.AllocStackTrace()),
      mReportStackTrace1(aB.ReportStackTrace1()),
      mReportStackTrace2(aB.ReportStackTrace2())
  {
    MOZ_ASSERT(mAllocStackTrace);
  }

  

  typedef TraceRecordKey Lookup;

  static uint32_t hash(const TraceRecordKey& aKey)
  {
    return mozilla::HashGeneric(aKey.mAllocStackTrace,
                                aKey.mReportStackTrace1,
                                aKey.mReportStackTrace2);
  }

  static bool match(const TraceRecordKey& aA, const TraceRecordKey& aB)
  {
    return aA.mAllocStackTrace   == aB.mAllocStackTrace &&
           aA.mReportStackTrace1 == aB.mReportStackTrace1 &&
           aA.mReportStackTrace2 == aB.mReportStackTrace2;
  }
};

class RecordSize
{
  static const size_t kReqBits = sizeof(size_t) * 8 - 1;  

  size_t mReq;              
  size_t mSlop:kReqBits;    
  size_t mSampled:1;        
                            
public:
  RecordSize()
    : mReq(0),
      mSlop(0),
      mSampled(false)
  {}

  size_t Req()    const { return mReq; }
  size_t Slop()   const { return mSlop; }
  size_t Usable() const { return mReq + mSlop; }

  bool IsSampled() const { return mSampled; }

  void Add(const Block& aB)
  {
    mReq  += aB.ReqSize();
    mSlop += aB.SlopSize();
    mSampled = mSampled || aB.IsSampled();
  }

  void Add(const RecordSize& aRecordSize)
  {
    mReq  += aRecordSize.Req();
    mSlop += aRecordSize.Slop();
    mSampled = mSampled || aRecordSize.IsSampled();
  }

  static int Cmp(const RecordSize& aA, const RecordSize& aB)
  {
    
    if (aA.Usable() > aB.Usable()) return -1;
    if (aA.Usable() < aB.Usable()) return  1;

    
    if (aA.Req() > aB.Req()) return -1;
    if (aA.Req() < aB.Req()) return  1;

    
    if (!aA.mSampled &&  aB.mSampled) return -1;
    if ( aA.mSampled && !aB.mSampled) return  1;

    return 0;
  }
};


class TraceRecord : public TraceRecordKey
{
  
  
  
  mutable uint32_t    mNumBlocks; 
  mutable RecordSize mRecordSize; 

public:
  explicit TraceRecord(const TraceRecordKey& aKey)
    : TraceRecordKey(aKey),
      mNumBlocks(0),
      mRecordSize()
  {}

  uint32_t NumBlocks() const { return mNumBlocks; }

  const RecordSize& GetRecordSize() const { return mRecordSize; }

  
  void Add(const Block& aB) const
  {
    mNumBlocks++;
    mRecordSize.Add(aB);
  }

  
  static const char* const kRecordKind;
  static bool recordsOverlap() { return false; }

  void Print(const Writer& aWriter, LocationService* aLocService,
             uint32_t aM, uint32_t aN, const char* aStr, const char* astr,
             size_t aCategoryUsableSize, size_t aCumulativeUsableSize,
             size_t aTotalUsableSize) const;

  static int QsortCmp(const void* aA, const void* aB)
  {
    const TraceRecord* const a = *static_cast<const TraceRecord* const*>(aA);
    const TraceRecord* const b = *static_cast<const TraceRecord* const*>(aB);

    return RecordSize::Cmp(a->mRecordSize, b->mRecordSize);
  }
};

const char* const TraceRecord::kRecordKind = "trace";

typedef js::HashSet<TraceRecord, TraceRecord, InfallibleAllocPolicy>
        TraceRecordTable;

void
TraceRecord::Print(const Writer& aWriter, LocationService* aLocService,
                   uint32_t aM, uint32_t aN, const char* aStr, const char* astr,
                   size_t aCategoryUsableSize, size_t aCumulativeUsableSize,
                   size_t aTotalUsableSize) const
{
  bool showTilde = mRecordSize.IsSampled();

  W("%s: %s block%s in stack trace record %s of %s\n",
    aStr,
    Show(mNumBlocks, gBuf1, kBufLen, showTilde), Plural(mNumBlocks),
    Show(aM, gBuf2, kBufLen),
    Show(aN, gBuf3, kBufLen));

  W(" %s bytes (%s requested / %s slop)\n",
    Show(mRecordSize.Usable(), gBuf1, kBufLen, showTilde),
    Show(mRecordSize.Req(),    gBuf2, kBufLen, showTilde),
    Show(mRecordSize.Slop(),   gBuf3, kBufLen, showTilde));

  W(" %4.2f%% of the heap (%4.2f%% cumulative); "
    " %4.2f%% of %s (%4.2f%% cumulative)\n",
    Percent(mRecordSize.Usable(), aTotalUsableSize),
    Percent(aCumulativeUsableSize, aTotalUsableSize),
    Percent(mRecordSize.Usable(), aCategoryUsableSize),
    astr,
    Percent(aCumulativeUsableSize, aCategoryUsableSize));

  W(" Allocated at\n");
  mAllocStackTrace->Print(aWriter, aLocService);

  if (mReportStackTrace1) {
    W("\n Reported at\n");
    mReportStackTrace1->Print(aWriter, aLocService);
  }
  if (mReportStackTrace2) {
    W("\n Reported again at\n");
    mReportStackTrace2->Print(aWriter, aLocService);
  }

  W("\n");
}







class FrameRecord
{
  
  
  const void* const  mPc;
  mutable size_t     mNumBlocks;
  mutable size_t     mNumTraceRecords;
  mutable RecordSize mRecordSize;

public:
  explicit FrameRecord(const void* aPc)
    : mPc(aPc),
      mNumBlocks(0),
      mNumTraceRecords(0),
      mRecordSize()
  {}

  const RecordSize& GetRecordSize() const { return mRecordSize; }

  
  void Add(const TraceRecord& aTr) const
  {
    mNumBlocks += aTr.NumBlocks();
    mNumTraceRecords++;
    mRecordSize.Add(aTr.GetRecordSize());
  }

  void Print(const Writer& aWriter, LocationService* aLocService,
             uint32_t aM, uint32_t aN, const char* aStr, const char* astr,
             size_t aCategoryUsableSize, size_t aCumulativeUsableSize,
             size_t aTotalUsableSize) const;

  static int QsortCmp(const void* aA, const void* aB)
  {
    const FrameRecord* const a = *static_cast<const FrameRecord* const*>(aA);
    const FrameRecord* const b = *static_cast<const FrameRecord* const*>(aB);

    return RecordSize::Cmp(a->mRecordSize, b->mRecordSize);
  }

  
  static const char* const kRecordKind;
  static bool recordsOverlap() { return true; }

  

  typedef const void* Lookup;

  static uint32_t hash(const void* const& aPc)
  {
    return mozilla::HashGeneric(aPc);
  }

  static bool match(const FrameRecord& aFr, const void* const& aPc)
  {
    return aFr.mPc == aPc;
  }
};

const char* const FrameRecord::kRecordKind = "frame";

typedef js::HashSet<FrameRecord, FrameRecord, InfallibleAllocPolicy>
        FrameRecordTable;

void
FrameRecord::Print(const Writer& aWriter, LocationService* aLocService,
                   uint32_t aM, uint32_t aN, const char* aStr, const char* astr,
                   size_t aCategoryUsableSize, size_t aCumulativeUsableSize,
                   size_t aTotalUsableSize) const
{
  (void)aCumulativeUsableSize;

  bool showTilde = mRecordSize.IsSampled();

  W("%s: %s block%s from %s stack trace record%s in stack frame record %s of %s\n",
    aStr,
    Show(mNumBlocks, gBuf1, kBufLen, showTilde), Plural(mNumBlocks),
    Show(mNumTraceRecords, gBuf2, kBufLen, showTilde), Plural(mNumTraceRecords),
    Show(aM, gBuf3, kBufLen),
    Show(aN, gBuf4, kBufLen));

  W(" %s bytes (%s requested / %s slop)\n",
    Show(mRecordSize.Usable(), gBuf1, kBufLen, showTilde),
    Show(mRecordSize.Req(),    gBuf2, kBufLen, showTilde),
    Show(mRecordSize.Slop(),   gBuf3, kBufLen, showTilde));

  W(" %4.2f%% of the heap;  %4.2f%% of %s\n",
    Percent(mRecordSize.Usable(), aTotalUsableSize),
    Percent(mRecordSize.Usable(), aCategoryUsableSize),
    astr);

  W(" PC is\n");
  aLocService->WriteLocation(aWriter, mPc);
  W("\n");
}








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
                 long aMin, long aMax, long* aN)
{
  if (const char* optionValue = ValueIfMatch(aArg, aOptionName)) {
    char* endPtr;
    *aN = strtol(optionValue, &endPtr,  10);
    if (!*endPtr && aMin <= *aN && *aN <= aMax &&
        *aN != LONG_MIN && *aN != LONG_MAX) {
      return true;
    }
  }
  return false;
}









Options::Options(const char* aDMDEnvVar)
  : mDMDEnvVar(InfallibleAllocPolicy::strdup_(aDMDEnvVar)),
    mSampleBelowSize(4093, 100 * 100 * 1000),
    mMaxFrames(StackTrace::MaxFrames, StackTrace::MaxFrames),
    mMaxRecords(1000, 1000000),
    mMode(Normal)
{
  char* e = mDMDEnvVar;
  if (strcmp(e, "1") != 0) {
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
      if (GetLong(arg, "--sample-below", 1, mSampleBelowSize.mMax, &myLong)) {
        mSampleBelowSize.mActual = myLong;

      } else if (GetLong(arg, "--max-frames", 1, mMaxFrames.mMax, &myLong)) {
        mMaxFrames.mActual = myLong;

      } else if (GetLong(arg, "--max-records", 1, mMaxRecords.mMax, &myLong)) {
        mMaxRecords.mActual = myLong;

      } else if (strcmp(arg, "--mode=normal") == 0) {
        mMode = Options::Normal;
      } else if (strcmp(arg, "--mode=test")   == 0) {
        mMode = Options::Test;
      } else if (strcmp(arg, "--mode=stress") == 0) {
        mMode = Options::Stress;

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
  StatusMsg("Valid values of $DMD are:\n");
  StatusMsg("- undefined or \"\" or \"0\", which disables DMD, or\n");
  StatusMsg("- \"1\", which enables it with the default options, or\n");
  StatusMsg("- a whitespace-separated list of |--option=val| entries, which\n");
  StatusMsg("  enables it with non-default options.\n");
  StatusMsg("\n");
  StatusMsg("The following options are allowed;  defaults are shown in [].\n");
  StatusMsg("  --sample-below=<1..%d> Sample blocks smaller than this [%d]\n",
            int(mSampleBelowSize.mMax),
            int(mSampleBelowSize.mDefault));
  StatusMsg("                               (prime numbers are recommended)\n");
  StatusMsg("  --max-frames=<1..%d>         Max. depth of stack traces [%d]\n",
            int(mMaxFrames.mMax),
            int(mMaxFrames.mDefault));
  StatusMsg("  --max-records=<1..%u>   Max. number of records printed [%u]\n",
            mMaxRecords.mMax,
            mMaxRecords.mDefault);
  StatusMsg("  --mode=<normal|test|stress>  Mode of operation [normal]\n");
  StatusMsg("\n");
  exit(1);
}





#ifdef XP_MACOSX
static void
NopStackWalkCallback(void* aPc, void* aSp, void* aClosure)
{
}
#endif


static FILE*
OpenOutputFile(const char* aFilename)
{
  FILE* fp = fopen(aFilename, "w");
  if (!fp) {
    StatusMsg("can't create %s file: %s\n", aFilename, strerror(errno));
    exit(1);
  }
  return fp;
}

static void RunTestMode(FILE* fp);
static void RunStressMode(FILE* fp);





static void
Init(const malloc_table_t* aMallocTable)
{
  MOZ_ASSERT(!gIsDMDRunning);

  gMallocTable = aMallocTable;

  
  
  

  char* e = getenv("DMD");
  StatusMsg("$DMD = '%s'\n", e);

  if (!e || strcmp(e, "") == 0 || strcmp(e, "0") == 0) {
    StatusMsg("DMD is not enabled\n");
    return;
  }

  
  gOptions = InfallibleAllocPolicy::new_<Options>(e);

  StatusMsg("DMD is enabled\n");

#ifdef XP_MACOSX
  
  
  
  
  
  
  (void)NS_StackWalk(NopStackWalkCallback,  0,
                      1, nullptr, 0, nullptr);
#endif

  gStateLock = InfallibleAllocPolicy::new_<Mutex>();

  gSmallBlockActualSizeCounter = 0;

  DMD_CREATE_TLS_INDEX(gTlsIndex);

  gStackTraceTable = InfallibleAllocPolicy::new_<StackTraceTable>();
  gStackTraceTable->init(8192);

  gBlockTable = InfallibleAllocPolicy::new_<BlockTable>();
  gBlockTable->init(8192);

  if (gOptions->IsTestMode()) {
    
    
    
    FILE* fp = OpenOutputFile("test.dmd");
    gIsDMDRunning = true;

    StatusMsg("running test mode...\n");
    RunTestMode(fp);
    StatusMsg("finished test mode\n");
    fclose(fp);
    exit(0);
  }

  if (gOptions->IsStressMode()) {
    FILE* fp = OpenOutputFile("stress.dmd");
    gIsDMDRunning = true;

    StatusMsg("running stress mode...\n");
    RunStressMode(fp);
    StatusMsg("finished stress mode\n");
    fclose(fp);
    exit(0);
  }

  gIsDMDRunning = true;
}





static void
ReportHelper(const void* aPtr, bool aReportedOnAlloc)
{
  if (!gIsDMDRunning || !aPtr) {
    return;
  }

  Thread* t = Thread::Fetch();

  AutoBlockIntercepts block(t);
  AutoLockState lock;

  if (BlockTable::Ptr p = gBlockTable->lookup(aPtr)) {
    p->Report(t, aReportedOnAlloc);
  } else {
    
    
    
    
  }
}

MOZ_EXPORT void
Report(const void* aPtr)
{
  ReportHelper(aPtr,  false);
}

MOZ_EXPORT void
ReportOnAlloc(const void* aPtr)
{
  ReportHelper(aPtr,  true);
}






template <class Record>
static void
PrintSortedRecords(const Writer& aWriter, LocationService* aLocService,
                   const char* aStr, const char* astr,
                   const js::HashSet<Record, Record, InfallibleAllocPolicy>&
                         aRecordTable,
                   size_t aCategoryUsableSize, size_t aTotalUsableSize)
{
  const char* kind = Record::kRecordKind;
  StatusMsg("  creating and sorting %s stack %s record array...\n", astr, kind);

  
  js::Vector<const Record*, 0, InfallibleAllocPolicy> recordArray;
  recordArray.reserve(aRecordTable.count());
  typedef js::HashSet<Record, Record, InfallibleAllocPolicy> RecordTable;
  for (typename RecordTable::Range r = aRecordTable.all();
       !r.empty();
       r.popFront()) {
    recordArray.infallibleAppend(&r.front());
  }
  qsort(recordArray.begin(), recordArray.length(), sizeof(recordArray[0]),
        Record::QsortCmp);

  WriteTitle("%s stack %s records\n", aStr, kind);

  if (recordArray.length() == 0) {
    W("(none)\n\n");
    return;
  }

  StatusMsg("  printing %s stack %s record array...\n", astr, kind);
  size_t cumulativeUsableSize = 0;

  
  
  
  uint32_t numRecords = recordArray.length();
  uint32_t maxRecords = gOptions->MaxRecords();
  for (uint32_t i = 0; i < numRecords; i++) {
    const Record* r = recordArray[i];
    cumulativeUsableSize += r->GetRecordSize().Usable();
    if (i < maxRecords) {
      r->Print(aWriter, aLocService, i+1, numRecords, aStr, astr,
               aCategoryUsableSize, cumulativeUsableSize, aTotalUsableSize);
    } else if (i == maxRecords) {
      W("%s: stopping after %s stack %s records\n\n", aStr,
        Show(maxRecords, gBuf1, kBufLen), kind);
    }
  }

  
  MOZ_ASSERT_IF(!Record::recordsOverlap(),
                aCategoryUsableSize == cumulativeUsableSize);
}

static void
PrintSortedTraceAndFrameRecords(const Writer& aWriter,
                                LocationService* aLocService,
                                const char* aStr, const char* astr,
                                const TraceRecordTable& aTraceRecordTable,
                                size_t aCategoryUsableSize,
                                size_t aTotalUsableSize)
{
  PrintSortedRecords(aWriter, aLocService, aStr, astr, aTraceRecordTable,
                     aCategoryUsableSize, aTotalUsableSize);

  FrameRecordTable frameRecordTable;
  (void)frameRecordTable.init(2048);
  for (TraceRecordTable::Range r = aTraceRecordTable.all();
       !r.empty();
       r.popFront()) {
    const TraceRecord& tr = r.front();
    const StackTrace* st = tr.mAllocStackTrace;

    
    
    StackTrace sorted(*st);
    sorted.Sort();              
    void* prevPc = (void*)intptr_t(-1);
    for (uint32_t i = 0; i < sorted.Length(); i++) {
      void* pc = sorted.Pc(i);
      if (pc == prevPc) {
        continue;               
      }
      prevPc = pc;

      FrameRecordTable::AddPtr p = frameRecordTable.lookupForAdd(pc);
      if (!p) {
        FrameRecord fr(pc);
        (void)frameRecordTable.add(p, fr);
      }
      p->Add(tr);
    }
  }

  PrintSortedRecords(aWriter, aLocService, aStr, astr, frameRecordTable,
                     aCategoryUsableSize, aTotalUsableSize);
}









static void
SizeOfInternal(Sizes* aSizes)
{
  MOZ_ASSERT(gStateLock->IsLocked());
  MOZ_ASSERT(Thread::Fetch()->InterceptsAreBlocked());

  aSizes->Clear();

  if (!gIsDMDRunning) {
    return;
  }

  StackTraceSet usedStackTraces;
  GatherUsedStackTraces(usedStackTraces);

  for (StackTraceTable::Range r = gStackTraceTable->all();
       !r.empty();
       r.popFront()) {
    StackTrace* const& st = r.front();

    if (usedStackTraces.has(st)) {
      aSizes->mStackTracesUsed += MallocSizeOf(st);
    } else {
      aSizes->mStackTracesUnused += MallocSizeOf(st);
    }
  }

  aSizes->mStackTraceTable =
    gStackTraceTable->sizeOfIncludingThis(MallocSizeOf);

  aSizes->mBlockTable = gBlockTable->sizeOfIncludingThis(MallocSizeOf);
}

MOZ_EXPORT void
SizeOf(Sizes* aSizes)
{
  aSizes->Clear();

  if (!gIsDMDRunning) {
    return;
  }

  AutoBlockIntercepts block(Thread::Fetch());
  AutoLockState lock;
  SizeOfInternal(aSizes);
}

MOZ_EXPORT void
ClearReports()
{
  
  
  
  for (BlockTable::Range r = gBlockTable->all(); !r.empty(); r.popFront()) {
    r.front().UnreportIfNotReportedOnAlloc();
  }
}

MOZ_EXPORT void
Dump(Writer aWriter)
{
  if (!gIsDMDRunning) {
    const char* msg = "cannot Dump();  DMD was not enabled at startup\n";
    StatusMsg("%s", msg);
    W("%s", msg);
    return;
  }

  AutoBlockIntercepts block(Thread::Fetch());
  AutoLockState lock;

  static int dumpCount = 1;
  StatusMsg("Dump %d {\n", dumpCount++);

  StatusMsg("  gathering stack trace records...\n");

  TraceRecordTable unreportedTraceRecordTable;
  (void)unreportedTraceRecordTable.init(1024);
  size_t unreportedUsableSize = 0;
  size_t unreportedNumBlocks = 0;

  TraceRecordTable onceReportedTraceRecordTable;
  (void)onceReportedTraceRecordTable.init(1024);
  size_t onceReportedUsableSize = 0;
  size_t onceReportedNumBlocks = 0;

  TraceRecordTable twiceReportedTraceRecordTable;
  (void)twiceReportedTraceRecordTable.init(0);
  size_t twiceReportedUsableSize = 0;
  size_t twiceReportedNumBlocks = 0;

  bool anyBlocksSampled = false;

  for (BlockTable::Range r = gBlockTable->all(); !r.empty(); r.popFront()) {
    const Block& b = r.front();

    TraceRecordTable* table;
    uint32_t numReports = b.NumReports();
    if (numReports == 0) {
      unreportedUsableSize += b.UsableSize();
      unreportedNumBlocks++;
      table = &unreportedTraceRecordTable;
    } else if (numReports == 1) {
      onceReportedUsableSize += b.UsableSize();
      onceReportedNumBlocks++;
      table = &onceReportedTraceRecordTable;
    } else {
      MOZ_ASSERT(numReports == 2);
      twiceReportedUsableSize += b.UsableSize();
      twiceReportedNumBlocks++;
      table = &twiceReportedTraceRecordTable;
    }
    TraceRecordKey key(b);
    TraceRecordTable::AddPtr p = table->lookupForAdd(key);
    if (!p) {
      TraceRecord tr(b);
      (void)table->add(p, tr);
    }
    p->Add(b);

    anyBlocksSampled = anyBlocksSampled || b.IsSampled();
  }
  size_t totalUsableSize =
    unreportedUsableSize + onceReportedUsableSize + twiceReportedUsableSize;
  size_t totalNumBlocks =
    unreportedNumBlocks + onceReportedNumBlocks + twiceReportedNumBlocks;

  WriteTitle("Invocation\n");
  W("$DMD = '%s'\n", gOptions->DMDEnvVar());
  W("Sample-below size = %lld\n\n",
    (long long)(gOptions->SampleBelowSize()));

  
  LocationService* locService = InfallibleAllocPolicy::new_<LocationService>();

  PrintSortedRecords(aWriter, locService, "Twice-reported", "twice-reported",
                     twiceReportedTraceRecordTable, twiceReportedUsableSize,
                     totalUsableSize);

  PrintSortedTraceAndFrameRecords(aWriter, locService,
                                  "Unreported", "unreported",
                                  unreportedTraceRecordTable,
                                  unreportedUsableSize, totalUsableSize);

  PrintSortedTraceAndFrameRecords(aWriter, locService,
                                 "Once-reported", "once-reported",
                                 onceReportedTraceRecordTable,
                                 onceReportedUsableSize, totalUsableSize);

  bool showTilde = anyBlocksSampled;
  WriteTitle("Summary\n");

  W("Total:          %12s bytes (%6.2f%%) in %7s blocks (%6.2f%%)\n",
    Show(totalUsableSize, gBuf1, kBufLen, showTilde),
    100.0,
    Show(totalNumBlocks,  gBuf2, kBufLen, showTilde),
    100.0);

  W("Unreported:     %12s bytes (%6.2f%%) in %7s blocks (%6.2f%%)\n",
    Show(unreportedUsableSize, gBuf1, kBufLen, showTilde),
    Percent(unreportedUsableSize, totalUsableSize),
    Show(unreportedNumBlocks, gBuf2, kBufLen, showTilde),
    Percent(unreportedNumBlocks, totalNumBlocks));

  W("Once-reported:  %12s bytes (%6.2f%%) in %7s blocks (%6.2f%%)\n",
    Show(onceReportedUsableSize, gBuf1, kBufLen, showTilde),
    Percent(onceReportedUsableSize, totalUsableSize),
    Show(onceReportedNumBlocks, gBuf2, kBufLen, showTilde),
    Percent(onceReportedNumBlocks, totalNumBlocks));

  W("Twice-reported: %12s bytes (%6.2f%%) in %7s blocks (%6.2f%%)\n",
    Show(twiceReportedUsableSize, gBuf1, kBufLen, showTilde),
    Percent(twiceReportedUsableSize, totalUsableSize),
    Show(twiceReportedNumBlocks, gBuf2, kBufLen, showTilde),
    Percent(twiceReportedNumBlocks, totalNumBlocks));

  W("\n");

  
  if (!gOptions->IsTestMode()) {
    Sizes sizes;
    SizeOfInternal(&sizes);

    WriteTitle("Execution measurements\n");

    W("Data structures that persist after Dump() ends:\n");

    W("  Used stack traces:    %10s bytes\n",
      Show(sizes.mStackTracesUsed, gBuf1, kBufLen));

    W("  Unused stack traces:  %10s bytes\n",
      Show(sizes.mStackTracesUnused, gBuf1, kBufLen));

    W("  Stack trace table:    %10s bytes (%s entries, %s used)\n",
      Show(sizes.mStackTraceTable,       gBuf1, kBufLen),
      Show(gStackTraceTable->capacity(), gBuf2, kBufLen),
      Show(gStackTraceTable->count(),    gBuf3, kBufLen));

    W("  Block table:          %10s bytes (%s entries, %s used)\n",
      Show(sizes.mBlockTable,       gBuf1, kBufLen),
      Show(gBlockTable->capacity(), gBuf2, kBufLen),
      Show(gBlockTable->count(),    gBuf3, kBufLen));

    W("\nData structures that are destroyed after Dump() ends:\n");

    size_t unreportedSize =
      unreportedTraceRecordTable.sizeOfIncludingThis(MallocSizeOf);
    W("  Unreported table:     %10s bytes (%s entries, %s used)\n",
      Show(unreportedSize,                        gBuf1, kBufLen),
      Show(unreportedTraceRecordTable.capacity(), gBuf2, kBufLen),
      Show(unreportedTraceRecordTable.count(),    gBuf3, kBufLen));

    size_t onceReportedSize =
      onceReportedTraceRecordTable.sizeOfIncludingThis(MallocSizeOf);
    W("  Once-reported table:  %10s bytes (%s entries, %s used)\n",
      Show(onceReportedSize,                        gBuf1, kBufLen),
      Show(onceReportedTraceRecordTable.capacity(), gBuf2, kBufLen),
      Show(onceReportedTraceRecordTable.count(),    gBuf3, kBufLen));

    size_t twiceReportedSize =
      twiceReportedTraceRecordTable.sizeOfIncludingThis(MallocSizeOf);
    W("  Twice-reported table: %10s bytes (%s entries, %s used)\n",
      Show(twiceReportedSize,                        gBuf1, kBufLen),
      Show(twiceReportedTraceRecordTable.capacity(), gBuf2, kBufLen),
      Show(twiceReportedTraceRecordTable.count(),    gBuf3, kBufLen));

    W("  Location service:     %10s bytes\n",
      Show(locService->SizeOfIncludingThis(), gBuf1, kBufLen));

    W("\nCounts:\n");

    size_t hits   = locService->NumCacheHits();
    size_t misses = locService->NumCacheMisses();
    size_t requests = hits + misses;
    W("  Location service:    %10s requests\n",
      Show(requests, gBuf1, kBufLen));

    size_t count    = locService->CacheCount();
    size_t capacity = locService->CacheCapacity();
    W("  Location service cache:  %4.1f%% hit rate, %.1f%% occupancy at end\n",
      Percent(hits, requests), Percent(count, capacity));

    W("\n");
  }

  InfallibleAllocPolicy::delete_(locService);

  ClearReports();

  StatusMsg("}\n");
}







void foo()
{
   char* a[6];
   for (int i = 0; i < 6; i++) {
      a[i] = (char*) malloc(128 - 16*i);
   }

   for (int i = 0; i <= 1; i++)
      Report(a[i]);                     
   Report(a[2]);                        
   Report(a[3]);                        
   
}


static void
UseItOrLoseIt(void* a)
{
  if (a == 0) {
    fprintf(stderr, "UseItOrLoseIt: %p\n", a);
  }
}




static void
RunTestMode(FILE* fp)
{
  Writer writer(FpWrite, fp);

  
  gOptions->SetSampleBelowSize(1);

  
  Dump(writer);

  
  
  int i;
  char* a;
  for (i = 0; i < 10; i++) {
      a = (char*) malloc(100);
      UseItOrLoseIt(a);
  }
  free(a);

  
  
  
  char* a2 = (char*) malloc(0);
  Report(a2);

  
  
  
  char* b = new char[10];
  ReportOnAlloc(b);

  
  
  
  char* b2 = new char;
  ReportOnAlloc(b2);
  free(b2);

  
  
  char* c = (char*) calloc(10, 3);
  Report(c);
  for (int i = 0; i < 3; i++) {
    Report(c);
  }

  
  
  Report((void*)(intptr_t)i);

  
  
  
  char* e = (char*) malloc(4096);
  e = (char*) realloc(e, 4097);
  Report(e);

  
  
  
  char* e2 = (char*) realloc(nullptr, 1024);
  e2 = (char*) realloc(e2, 512);
  Report(e2);

  
  
  
  
  char* e3 = (char*) realloc(nullptr, 1023);

  MOZ_ASSERT(e3);
  Report(e3);

  
  
  char* f = (char*) malloc(64);
  free(f);

  
  
  Report((void*)(intptr_t)0x0);

  
  
  foo();
  foo();

  
  
  char* g1 = (char*) malloc(77);
  ReportOnAlloc(g1);
  ReportOnAlloc(g1);

  
  
  char* g2 = (char*) malloc(78);
  Report(g2);
  ReportOnAlloc(g2);

  
  
  char* g3 = (char*) malloc(79);
  ReportOnAlloc(g3);
  Report(g3);

  
  
  
  


  



  




  
  Dump(writer);

  

  Report(a2);
  Report(a2);
  free(c);
  free(e);
  Report(e2);
  free(e3);




  
  Dump(writer);

  

  
  gBlockTable->clear();

  gOptions->SetSampleBelowSize(128);

  char* s;

  
  
  s = (char*) malloc(128);
  UseItOrLoseIt(s);

  
  s = (char*) malloc(144);
  UseItOrLoseIt(s);

  
  for (int i = 0; i < 16; i++) {
    s = (char*) malloc(8);
    UseItOrLoseIt(s);
  }
  MOZ_ASSERT(gSmallBlockActualSizeCounter == 0);

  
  for (int i = 0; i < 15; i++) {
    s = (char*) malloc(8);
    UseItOrLoseIt(s);
  }
  MOZ_ASSERT(gSmallBlockActualSizeCounter == 120);

  
  s = (char*) malloc(256);
  UseItOrLoseIt(s);
  MOZ_ASSERT(gSmallBlockActualSizeCounter == 120);

  
  s = (char*) malloc(96);
  UseItOrLoseIt(s);
  MOZ_ASSERT(gSmallBlockActualSizeCounter == 88);

  
  for (int i = 0; i < 5; i++) {
    s = (char*) malloc(8);
    UseItOrLoseIt(s);
  }
  MOZ_ASSERT(gSmallBlockActualSizeCounter == 0);

  
  
  
  for (int i = 1; i <= 8; i++) {
    s = (char*) malloc(i * 16);
    UseItOrLoseIt(s);
  }
  MOZ_ASSERT(gSmallBlockActualSizeCounter == 64);

  
  

  Dump(writer);
}





MOZ_NEVER_INLINE static void
stress5()
{
  for (int i = 0; i < 10; i++) {
    void* x = malloc(64);
    UseItOrLoseIt(x);
    if (i & 1) {
      free(x);
    }
  }
}

MOZ_NEVER_INLINE static void
stress4()
{
  stress5(); stress5(); stress5(); stress5(); stress5();
  stress5(); stress5(); stress5(); stress5(); stress5();
}

MOZ_NEVER_INLINE static void
stress3()
{
  for (int i = 0; i < 10; i++) {
    stress4();
  }
}

MOZ_NEVER_INLINE static void
stress2()
{
  stress3(); stress3(); stress3(); stress3(); stress3();
  stress3(); stress3(); stress3(); stress3(); stress3();
}

MOZ_NEVER_INLINE static void
stress1()
{
  for (int i = 0; i < 10; i++) {
    stress2();
  }
}








static void
RunStressMode(FILE* fp)
{
  Writer writer(FpWrite, fp);

  
  gOptions->SetSampleBelowSize(1);

  stress1(); stress1(); stress1(); stress1(); stress1();
  stress1(); stress1(); stress1(); stress1(); stress1();

  Dump(writer);
}

}   
}   
