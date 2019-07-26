





#include "DMD.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef XP_WIN
#error "Windows not supported yet, sorry."



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
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
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

enum Mode {
  Normal,   
  Test,     
  Stress    
};
static Mode gMode = Normal;









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
    char* s = (char*) gMallocTable->malloc(strlen(aStr) + 1);
    ExitOnFailure(s);
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

static const size_t kNoSize = size_t(-1);






#ifdef XP_WIN

#include <windows.h>

class MutexBase
{
  HANDLE mMutex;

  DISALLOW_COPY_AND_ASSIGN(MutexBase);

public:
  MutexBase()
    : mMutex(CreateMutexW(nullptr, false, nullptr))
  {
    MOZ_ASSERT(mMutex);
  }

  ~MutexBase()
  {
    if (mMutex) {
      CloseHandle(mMutex);
    }
  }

  void Lock()
  {
    if (mMutex) {
      WaitForSingleObject(mMutex, INFINITE);
    }
  }

  void Unlock()
  {
    if (mMutex) {
      ReleaseMutex(mMutex);
    }
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
    : mMutex(PTHREAD_MUTEX_INITIALIZER)
  {}

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
#define DMD_CREATE_TLS_INDEX(i_)        PR_BEGIN_MACRO                        \
                                          (i_) = TlsAlloc();                  \
                                        PR_END_MACRO
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

  bool blockIntercepts()
  {
    MOZ_ASSERT(!mBlockIntercepts);
    return mBlockIntercepts = true;
  }

  bool unblockIntercepts()
  {
    MOZ_ASSERT(mBlockIntercepts);
    return mBlockIntercepts = false;
  }

  bool interceptsAreBlocked() const
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
    mT->blockIntercepts();
  }
  ~AutoBlockIntercepts()
  {
    MOZ_ASSERT(mT->interceptsAreBlocked());
    mT->unblockIntercepts();
  }
};





static void
PcInfo(const void* aPc, nsCodeAddressDetails* aDetails)
{
  
  
  
  
  
  {
    AutoUnlockState unlock;
    (void)NS_DescribeCodeAddress(const_cast<void*>(aPc), aDetails);
  }
  if (!aDetails->function[0]) {
    strcpy(aDetails->function, "???");
  }
}

class StackTrace
{
  static const uint32_t MaxDepth = 24;

  uint32_t mLength;             
  void* mPcs[MaxDepth];         

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

  void Print(const Writer& aWriter) const;

  

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

    
    
    if (st->mLength < MaxDepth) {
      st->mPcs[st->mLength] = aPc;
      st->mLength++;
    }
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

void
StackTrace::Print(const Writer& aWriter) const
{
  if (mLength == 0) {
    W("   (empty)\n");
    return;
  }

  if (gMode == Test) {
    
    W("   (stack omitted due to test mode)\n");
    return;
  }

  for (uint32_t i = 0; i < mLength; i++) {
    nsCodeAddressDetails details;
    void* pc = mPcs[i];
    PcInfo(pc, &details);
    if (details.function[0]) {
      W("   %s[%s +0x%X] %p\n", details.function, details.library,
        details.loffset, pc);
    }
  }
}

 const StackTrace*
StackTrace::Get(Thread* aT)
{
  MOZ_ASSERT(gStateLock->IsLocked());
  MOZ_ASSERT(aT->interceptsAreBlocked());

  
  
  
  
  
  StackTrace tmp;
  {
#ifdef XP_WIN
    AutoUnlockState unlock;
#endif
    
    
    
    uint32_t skip = (gMode == Test) ? 2 : 3;
    nsresult rv = NS_StackWalk(StackWalkCallback, skip, &tmp, 0, nullptr);
    if (NS_FAILED(rv) || tmp.mLength == 0) {
      tmp.mLength = 0;
    }
  }

  StackTraceTable::AddPtr p = gStackTraceTable->lookupForAdd(&tmp);
  if (!p) {
    StackTrace* stnew = InfallibleAllocPolicy::new_<StackTrace>(tmp);
    (void)gStackTraceTable->add(p, stnew);
  }
  return *p;
}





static const char* gUnreportedName = "unreported";


class LiveBlock
{
  const void*  mPtr;

  
  
  static const size_t kReqBits = sizeof(size_t) * 8 - 1;    
  const size_t mReqSize:kReqBits; 
  const size_t mSampled:1;        

public:
  const StackTrace* const mAllocStackTrace;     

  
  
  
  
  
  
  
  
  
  
private:
  mutable const StackTrace* mReportStackTrace;  
  mutable const char*       mReporterName;      
  mutable bool              mReportedOnAlloc;   
                                                

public:
  LiveBlock(const void* aPtr, size_t aReqSize,
            const StackTrace* aAllocStackTrace, bool aSampled)
    : mPtr(aPtr),
      mReqSize(aReqSize),
      mSampled(aSampled),
      mAllocStackTrace(aAllocStackTrace),
      mReportStackTrace(nullptr),
      mReporterName(gUnreportedName),
      mReportedOnAlloc(false)
 {
    if (mReqSize != aReqSize)
    {
      MOZ_CRASH();              
    }
    MOZ_ASSERT(IsSane());
  }

  bool IsSane() const
  {
    bool hasReporterName = mReporterName != gUnreportedName;
    return mAllocStackTrace &&
           (( mReportStackTrace &&  hasReporterName) ||
            (!mReportStackTrace && !hasReporterName && !mReportedOnAlloc));
  }

  size_t ReqSize() const { return mReqSize; }

  
  size_t SlopSize() const
  {
    return mSampled ? 0 : MallocSizeOf(mPtr) - mReqSize;
  }

  size_t UsableSize() const
  {
    return mSampled ? mReqSize : MallocSizeOf(mPtr);
  }

  bool IsSampled() const { return mSampled; }

  bool IsReported() const
  {
    MOZ_ASSERT(IsSane());
    bool isRep = mReporterName != gUnreportedName;
    return isRep;
  }

  const StackTrace* ReportStackTrace() const { return mReportStackTrace; }
  const char* ReporterName() const { return mReporterName; }

  
  void Report(Thread* aT, const char* aReporterName, bool aReportedOnAlloc)
       const;

  void UnreportIfNotReportedOnAlloc() const;

  

  typedef const void* Lookup;

  static uint32_t hash(const void* const& aPc)
  {
    return mozilla::HashGeneric(aPc);
  }

  static bool match(const LiveBlock& aB, const void* const& aPtr)
  {
    return aB.mPtr == aPtr;
  }
};


typedef js::HashSet<LiveBlock, LiveBlock, InfallibleAllocPolicy> LiveBlockTable;
static LiveBlockTable* gLiveBlockTable = nullptr;





static size_t gSampleBelowSize = 0;
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

  if (actualSize < gSampleBelowSize) {
    
    
    
    
    gSmallBlockActualSizeCounter += actualSize;
    if (gSmallBlockActualSizeCounter >= gSampleBelowSize) {
      gSmallBlockActualSizeCounter -= gSampleBelowSize;

      LiveBlock b(aPtr, gSampleBelowSize, StackTrace::Get(aT),
                   true);
      (void)gLiveBlockTable->putNew(aPtr, b);
    }
  } else {
    
    LiveBlock b(aPtr, aReqSize, StackTrace::Get(aT),  false);
    (void)gLiveBlockTable->putNew(aPtr, b);
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

  gLiveBlockTable->remove(aPtr);
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
  if (t->interceptsAreBlocked()) {
    
    
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
  if (t->interceptsAreBlocked()) {
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
  if (t->interceptsAreBlocked()) {
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
  if (t->interceptsAreBlocked()) {
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
  if (t->interceptsAreBlocked()) {
    return InfallibleAllocPolicy::free_(aPtr);
  }

  
  
  
  FreeCallback(aPtr, t);
  gMallocTable->free(aPtr);
}

namespace mozilla {
namespace dmd {





class LiveBlockKey
{
public:
  const StackTrace* const mAllocStackTrace;   
protected:
  const StackTrace* const mReportStackTrace;  
  const char*       const mReporterName;      

public:
  LiveBlockKey(const LiveBlock& aB)
    : mAllocStackTrace(aB.mAllocStackTrace),
      mReportStackTrace(aB.ReportStackTrace()),
      mReporterName(aB.ReporterName())
  {
    MOZ_ASSERT(IsSane());
  }

  bool IsSane() const
  {
    bool hasReporterName = mReporterName != gUnreportedName;
    return mAllocStackTrace &&
           (( mReportStackTrace &&  hasReporterName) ||
            (!mReportStackTrace && !hasReporterName));
  }

  bool IsReported() const
  {
    MOZ_ASSERT(IsSane());
    bool isRep = mReporterName != gUnreportedName;
    return isRep;
  }

  
  
  
  
  
  
  
  

  typedef LiveBlockKey Lookup;

  static uint32_t hash(const LiveBlockKey& aKey)
  {
    return mozilla::HashGeneric(aKey.mAllocStackTrace,
                                aKey.mReportStackTrace,
                                aKey.mReporterName);
  }

  static bool match(const LiveBlockKey& aA, const LiveBlockKey& aB)
  {
    return aA.mAllocStackTrace  == aB.mAllocStackTrace &&
           aA.mReportStackTrace == aB.mReportStackTrace &&
           aA.mReporterName     == aB.mReporterName;
  }
};

class DoubleReportBlockKey
{
public:
  const StackTrace* const mAllocStackTrace;     

protected:
  
  
  const StackTrace* const mReportStackTrace1;   
  const StackTrace* const mReportStackTrace2;   
  const char*       const mReporterName1;       
  const char*       const mReporterName2;       

public:
  DoubleReportBlockKey(const StackTrace* aAllocStackTrace,
                       const StackTrace* aReportStackTrace1,
                       const StackTrace* aReportStackTrace2,
                       const char* aReporterName1,
                       const char* aReporterName2)
    : mAllocStackTrace(aAllocStackTrace),
      mReportStackTrace1(aReportStackTrace1),
      mReportStackTrace2(aReportStackTrace2),
      mReporterName1(aReporterName1),
      mReporterName2(aReporterName2)
  {
    MOZ_ASSERT(IsSane());
  }

  bool IsSane() const
  {
    return mAllocStackTrace &&
           mReportStackTrace1 &&
           mReportStackTrace2 &&
           mReporterName1 != gUnreportedName &&
           mReporterName2 != gUnreportedName;
  }

  
  
  
  

  typedef DoubleReportBlockKey Lookup;

  static uint32_t hash(const DoubleReportBlockKey& aKey)
  {
    return mozilla::HashGeneric(aKey.mAllocStackTrace,
                                aKey.mReportStackTrace1,
                                aKey.mReportStackTrace2,
                                aKey.mReporterName1,
                                aKey.mReporterName2);
  }

  static bool match(const DoubleReportBlockKey& aA,
                    const DoubleReportBlockKey& aB)
  {
    return aA.mAllocStackTrace   == aB.mAllocStackTrace &&
           aA.mReportStackTrace1 == aB.mReportStackTrace1 &&
           aA.mReportStackTrace2 == aB.mReportStackTrace2 &&
           aA.mReporterName1     == aB.mReporterName1 &&
           aA.mReporterName2     == aB.mReporterName2;
  }
};

class GroupSize
{
  static const size_t kReqBits = sizeof(size_t) * 8 - 1;  

  size_t mReq;              
  size_t mSlop:kReqBits;    
  size_t mSampled:1;        
                            
public:
  GroupSize()
    : mReq(0),
      mSlop(0),
      mSampled(false)
  {}

  size_t Req()    const { return mReq; }
  size_t Slop()   const { return mSlop; }
  size_t Usable() const { return mReq + mSlop; }

  bool IsSampled() const { return mSampled; }

  void Add(const LiveBlock& aB)
  {
    mReq  += aB.ReqSize();
    mSlop += aB.SlopSize();
    mSampled = mSampled || aB.IsSampled();
  }

  void Add(const GroupSize& aGroupSize)
  {
    mReq  += aGroupSize.Req();
    mSlop += aGroupSize.Slop();
    mSampled = mSampled || aGroupSize.IsSampled();
  }

  static int Cmp(const GroupSize& aA, const GroupSize& aB)
  {
    
    if (aA.Usable() > aB.Usable()) return -1;
    if (aA.Usable() < aB.Usable()) return  1;

    
    if (!aA.mSampled &&  aB.mSampled) return -1;
    if ( aA.mSampled && !aB.mSampled) return  1;

    return 0;
  }
};

class BlockGroup
{
protected:
  
  
  
  mutable uint32_t  mNumBlocks;     
  mutable GroupSize mGroupSize;     

public:
  BlockGroup()
    : mNumBlocks(0),
      mGroupSize()
  {}

  const GroupSize& GroupSize() const { return mGroupSize; }

  
  void Add(const LiveBlock& aB) const
  {
    mNumBlocks++;
    mGroupSize.Add(aB);
  }

  static const char* const kName;   
};

const char* const BlockGroup::kName = "block";


class LiveBlockGroup : public LiveBlockKey, public BlockGroup
{
  friend class FrameGroup;      

public:
  explicit LiveBlockGroup(const LiveBlockKey& aKey)
    : LiveBlockKey(aKey),
      BlockGroup()
  {}

  void Print(const Writer& aWriter, uint32_t aM, uint32_t aN,
             const char* aStr, const char* astr,
             size_t aCategoryUsableSize, size_t aCumulativeUsableSize,
             size_t aTotalUsableSize) const;

  static int QsortCmp(const void* aA, const void* aB)
  {
    const LiveBlockGroup* const a =
      *static_cast<const LiveBlockGroup* const*>(aA);
    const LiveBlockGroup* const b =
      *static_cast<const LiveBlockGroup* const*>(aB);

    return GroupSize::Cmp(a->mGroupSize, b->mGroupSize);
  }
};

typedef js::HashSet<LiveBlockGroup, LiveBlockGroup, InfallibleAllocPolicy>
        LiveBlockGroupTable;

void
LiveBlockGroup::Print(const Writer& aWriter, uint32_t aM, uint32_t aN,
                      const char* aStr, const char* astr,
                      size_t aCategoryUsableSize, size_t aCumulativeUsableSize,
                      size_t aTotalUsableSize) const
{
  bool showTilde = mGroupSize.IsSampled();

  W("%s: %s block%s in block group %s of %s\n",
    aStr,
    Show(mNumBlocks, gBuf1, kBufLen, showTilde), Plural(mNumBlocks),
    Show(aM, gBuf2, kBufLen),
    Show(aN, gBuf3, kBufLen));

  W(" %s bytes (%s requested / %s slop)\n",
    Show(mGroupSize.Usable(), gBuf1, kBufLen, showTilde),
    Show(mGroupSize.Req(),    gBuf2, kBufLen, showTilde),
    Show(mGroupSize.Slop(),   gBuf3, kBufLen, showTilde));

  W(" %4.2f%% of the heap (%4.2f%% cumulative); "
    " %4.2f%% of %s (%4.2f%% cumulative)\n",
    Percent(mGroupSize.Usable(), aTotalUsableSize),
    Percent(aCumulativeUsableSize, aTotalUsableSize),
    Percent(mGroupSize.Usable(), aCategoryUsableSize),
    astr,
    Percent(aCumulativeUsableSize, aCategoryUsableSize));

  W(" Allocated at\n");
  mAllocStackTrace->Print(aWriter);

  if (IsReported()) {
    W("\n Reported by '%s' at\n", mReporterName);
    mReportStackTrace->Print(aWriter);
  }

  W("\n");
}



class DoubleReportBlockGroup : public DoubleReportBlockKey, public BlockGroup
{
public:
  explicit DoubleReportBlockGroup(const DoubleReportBlockKey& aKey)
    : DoubleReportBlockKey(aKey),
      BlockGroup()
  {}

  void Print(const Writer& aWriter, uint32_t aM, uint32_t aN,
             const char* aStr, const char* astr,
             size_t aCategoryUsableSize, size_t aCumulativeUsableSize,
             size_t aTotalUsableSize) const;

  static int QsortCmp(const void* aA, const void* aB)
  {
    const DoubleReportBlockGroup* const a =
      *static_cast<const DoubleReportBlockGroup* const*>(aA);
    const DoubleReportBlockGroup* const b =
      *static_cast<const DoubleReportBlockGroup* const*>(aB);

    return GroupSize::Cmp(a->mGroupSize, b->mGroupSize);
  }
};

typedef js::HashSet<DoubleReportBlockGroup, DoubleReportBlockGroup,
                    InfallibleAllocPolicy> DoubleReportBlockGroupTable;
DoubleReportBlockGroupTable* gDoubleReportBlockGroupTable = nullptr;

void
DoubleReportBlockGroup::Print(const Writer& aWriter, uint32_t aM, uint32_t aN,
                              const char* aStr, const char* astr,
                              size_t aCategoryUsableSize,
                              size_t aCumulativeUsableSize,
                              size_t aTotalUsableSize) const
{
  bool showTilde = mGroupSize.IsSampled();

  W("%s: %s block%s in block group %s of %s\n",
    aStr,
    Show(mNumBlocks, gBuf1, kBufLen, showTilde), Plural(mNumBlocks),
    Show(aM, gBuf2, kBufLen),
    Show(aN, gBuf3, kBufLen));

  W(" %s bytes (%s requested / %s slop)\n",
    Show(mGroupSize.Usable(), gBuf1, kBufLen, showTilde),
    Show(mGroupSize.Req(),    gBuf2, kBufLen, showTilde),
    Show(mGroupSize.Slop(),   gBuf3, kBufLen, showTilde));

  W(" Allocated at\n");
  mAllocStackTrace->Print(aWriter);

  W("\n Previously reported by '%s' at\n", mReporterName1);
  mReportStackTrace1->Print(aWriter);

  W("\n Now reported by '%s' at\n", mReporterName2);
  mReportStackTrace2->Print(aWriter);

  W("\n");
}







class FrameGroup
{
  
  
  const void* const mPc;
  mutable size_t    mNumBlocks;
  mutable size_t    mNumBlockGroups;
  mutable GroupSize mGroupSize;

public:
  explicit FrameGroup(const void* aPc)
    : mPc(aPc),
      mNumBlocks(0),
      mNumBlockGroups(0),
      mGroupSize()
  {}

  const GroupSize& GroupSize() const { return mGroupSize; }

  
  void Add(const LiveBlockGroup& aBg) const
  {
    mNumBlocks += aBg.mNumBlocks;
    mNumBlockGroups++;
    mGroupSize.Add(aBg.mGroupSize);
  }

  void Print(const Writer& aWriter, uint32_t aM, uint32_t aN,
             const char* aStr, const char* astr,
             size_t aCategoryUsableSize, size_t aCumulativeUsableSize,
             size_t aTotalUsableSize) const;

  static int QsortCmp(const void* aA, const void* aB)
  {
    const FrameGroup* const a = *static_cast<const FrameGroup* const*>(aA);
    const FrameGroup* const b = *static_cast<const FrameGroup* const*>(aB);

    return GroupSize::Cmp(a->mGroupSize, b->mGroupSize);
  }

  static const char* const kName;   

  

  typedef const void* Lookup;

  static uint32_t hash(const void* const& aPc)
  {
    return mozilla::HashGeneric(aPc);
  }

  static bool match(const FrameGroup& aFg, const void* const& aPc)
  {
    return aFg.mPc == aPc;
  }
};

const char* const FrameGroup::kName = "frame";

typedef js::HashSet<FrameGroup, FrameGroup, InfallibleAllocPolicy>
        FrameGroupTable;

void
FrameGroup::Print(const Writer& aWriter, uint32_t aM, uint32_t aN,
                  const char* aStr, const char* astr,
                  size_t aCategoryUsableSize, size_t aCumulativeUsableSize,
                  size_t aTotalUsableSize) const
{
  (void)aCumulativeUsableSize;

  bool showTilde = mGroupSize.IsSampled();

  nsCodeAddressDetails details;
  PcInfo(mPc, &details);

  W("%s: %s block%s and %s block group%s in frame group %s of %s\n",
    aStr,
    Show(mNumBlocks, gBuf1, kBufLen, showTilde), Plural(mNumBlocks),
    Show(mNumBlockGroups, gBuf2, kBufLen, showTilde), Plural(mNumBlockGroups),
    Show(aM, gBuf3, kBufLen),
    Show(aN, gBuf4, kBufLen));

  W(" %s bytes (%s requested / %s slop)\n",
    Show(mGroupSize.Usable(), gBuf1, kBufLen, showTilde),
    Show(mGroupSize.Req(),    gBuf2, kBufLen, showTilde),
    Show(mGroupSize.Slop(),   gBuf3, kBufLen, showTilde));

  W(" %4.2f%% of the heap;  %4.2f%% of %s\n",
    Percent(mGroupSize.Usable(), aTotalUsableSize),
    Percent(mGroupSize.Usable(), aCategoryUsableSize),
    astr);

  W(" PC is\n");
  W("   %s[%s +0x%X] %p\n\n", details.function, details.library,
    details.loffset, mPc);
}





static void RunTestMode(FILE* fp);
static void RunStressMode(FILE* fp);

static const char* gDMDEnvVar = nullptr;




static const char*
OptionValueIfMatch(const char* aArg, const char* aOptionName)
{
  MOZ_ASSERT(!isspace(*aArg));  
  size_t optionLen = strlen(aOptionName);
  if (strncmp(aArg, aOptionName, optionLen) == 0 && aArg[optionLen] == '=' &&
      aArg[optionLen + 1]) {
    return aArg + optionLen + 1;
  }
  return nullptr;
}



static bool
OptionLong(const char* aArg, const char* aOptionName, long aMin, long aMax,
           long* aN)
{
  if (const char* optionValue = OptionValueIfMatch(aArg, aOptionName)) {
    char* endPtr;
    *aN = strtol(optionValue, &endPtr,  10);
    if (!*endPtr && aMin <= *aN && *aN <= aMax &&
        *aN != LONG_MIN && *aN != LONG_MAX) {
      return true;
    }
  }
  return false;
}

static const size_t gMaxSampleBelowSize = 100 * 1000 * 1000;    















static const size_t gDefaultSampleBelowSize = 4093;

static void
BadArg(const char* aArg)
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
  StatusMsg("  --sample-below=<1..%d> Sample blocks smaller than this [%d]\n"
            "                         (prime numbers recommended).\n",
            int(gMaxSampleBelowSize), int(gDefaultSampleBelowSize));
  StatusMsg("  --mode=<normal|test|stress>   Which mode to run in? [normal]\n");
  StatusMsg("\n");
  exit(1);
}


static FILE*
OpenTestOrStressFile(const char* aFilename)
{
  FILE* fp = fopen(aFilename, "w");
  if (!fp) {
    StatusMsg("can't create %s file: %s\n", aFilename, strerror(errno));
    exit(1);
  }
  return fp;
}





static void
Init(const malloc_table_t* aMallocTable)
{
  MOZ_ASSERT(!gIsDMDRunning);

  gMallocTable = aMallocTable;

  
  gMode = Normal;
  gSampleBelowSize = gDefaultSampleBelowSize;

  
  
  

  char* e = getenv("DMD");

  StatusMsg("$DMD = '%s'\n", e);

  if (!e || strcmp(e, "") == 0 || strcmp(e, "0") == 0) {
    StatusMsg("DMD is not enabled\n");
    return;
  }

  
  gDMDEnvVar = e = InfallibleAllocPolicy::strdup_(e);

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
      if (OptionLong(arg, "--sample-below", 1, gMaxSampleBelowSize, &myLong)) {
        gSampleBelowSize = myLong;

      } else if (strcmp(arg, "--mode=normal") == 0) {
        gMode = Normal;
      } else if (strcmp(arg, "--mode=test")   == 0) {
        gMode = Test;
      } else if (strcmp(arg, "--mode=stress") == 0) {
        gMode = Stress;

      } else if (strcmp(arg, "") == 0) {
        
        MOZ_ASSERT(isEnd);

      } else {
        BadArg(arg);
      }

      
      *e = replacedChar;
    }
  }

  

  StatusMsg("DMD is enabled\n");

  gStateLock = InfallibleAllocPolicy::new_<Mutex>();

  gSmallBlockActualSizeCounter = 0;

  DMD_CREATE_TLS_INDEX(gTlsIndex);

  gStackTraceTable = InfallibleAllocPolicy::new_<StackTraceTable>();
  gStackTraceTable->init(8192);

  gLiveBlockTable = InfallibleAllocPolicy::new_<LiveBlockTable>();
  gLiveBlockTable->init(8192);

  gDoubleReportBlockGroupTable =
    InfallibleAllocPolicy::new_<DoubleReportBlockGroupTable>();
  gDoubleReportBlockGroupTable->init(0);

  if (gMode == Test) {
    
    
    
    FILE* fp = OpenTestOrStressFile("test.dmd");
    gIsDMDRunning = true;

    StatusMsg("running test mode...\n");
    RunTestMode(fp);
    StatusMsg("finished test mode\n");
    fclose(fp);
    exit(0);
  }

  if (gMode == Stress) {
    FILE* fp = OpenTestOrStressFile("stress.dmd");
    gIsDMDRunning = true;

    StatusMsg("running stress mode...\n");
    RunStressMode(fp);
    StatusMsg("finished stress mode\n");
    fclose(fp);
    exit(0);
  }

  gIsDMDRunning = true;
}





void
LiveBlock::Report(Thread* aT, const char* aReporterName, bool aOnAlloc) const
{
  if (IsReported()) {
    DoubleReportBlockKey doubleReportKey(mAllocStackTrace,
                                         mReportStackTrace, StackTrace::Get(aT),
                                         mReporterName, aReporterName);
    DoubleReportBlockGroupTable::AddPtr p =
      gDoubleReportBlockGroupTable->lookupForAdd(doubleReportKey);
    if (!p) {
      DoubleReportBlockGroup bg(doubleReportKey);
      (void)gDoubleReportBlockGroupTable->add(p, bg);
    }
    p->Add(*this);

  } else {
    mReporterName     = aReporterName;
    mReportStackTrace = StackTrace::Get(aT);
    mReportedOnAlloc  = aOnAlloc;
  }
}

void
LiveBlock::UnreportIfNotReportedOnAlloc() const
{
  if (!mReportedOnAlloc) {
    mReporterName     = gUnreportedName;
    mReportStackTrace = nullptr;
  }
}

static void
ReportHelper(const void* aPtr, const char* aReporterName, bool aOnAlloc)
{
  if (!gIsDMDRunning || !aPtr) {
    return;
  }

  Thread* t = Thread::Fetch();

  AutoBlockIntercepts block(t);
  AutoLockState lock;

  if (LiveBlockTable::Ptr p = gLiveBlockTable->lookup(aPtr)) {
    p->Report(t, aReporterName, aOnAlloc);
  } else {
    
    
    
    
  }
}

MOZ_EXPORT void
Report(const void* aPtr, const char* aReporterName)
{
  ReportHelper(aPtr, aReporterName,  false);
}

MOZ_EXPORT void
ReportOnAlloc(const void* aPtr, const char* aReporterName)
{
  ReportHelper(aPtr, aReporterName,  true);
}






template <class TGroup>
static void
PrintSortedGroups(const Writer& aWriter, const char* aStr, const char* astr,
                  const js::HashSet<TGroup, TGroup, InfallibleAllocPolicy>& aTGroupTable,
                  size_t aCategoryUsableSize, size_t aTotalUsableSize)
{
  const char* name = TGroup::kName;
  StatusMsg("  creating and sorting %s %s group array...\n", astr, name);

  
  js::Vector<const TGroup*, 0, InfallibleAllocPolicy> tgArray;
  tgArray.reserve(aTGroupTable.count());
  typedef js::HashSet<TGroup, TGroup, InfallibleAllocPolicy> TGroupTable;
  for (typename TGroupTable::Range r = aTGroupTable.all();
       !r.empty();
       r.popFront()) {
    tgArray.infallibleAppend(&r.front());
  }
  qsort(tgArray.begin(), tgArray.length(), sizeof(tgArray[0]),
        TGroup::QsortCmp);

  WriteTitle("%s %ss\n", aStr, name);

  if (tgArray.length() == 0) {
    W("(none)\n\n");
    return;
  }

  
  
  
  static const uint32_t MaxTGroups = 1000;
  uint32_t numTGroups = tgArray.length();

  StatusMsg("  printing %s %s group array...\n", astr, name);
  size_t cumulativeUsableSize = 0;
  for (uint32_t i = 0; i < numTGroups; i++) {
    const TGroup* tg = tgArray[i];
    cumulativeUsableSize += tg->GroupSize().Usable();
    if (i < MaxTGroups) {
      tg->Print(aWriter, i+1, numTGroups, aStr, astr, aCategoryUsableSize,
                cumulativeUsableSize, aTotalUsableSize);
    } else if (i == MaxTGroups) {
      W("%s: stopping after %s %s groups\n\n", aStr,
        Show(MaxTGroups, gBuf1, kBufLen), name);
    }
  }

  MOZ_ASSERT(aCategoryUsableSize == kNoSize ||
             aCategoryUsableSize == cumulativeUsableSize);
}

static void
PrintSortedBlockAndFrameGroups(const Writer& aWriter,
                               const char* aStr, const char* astr,
                               const LiveBlockGroupTable& aLiveBlockGroupTable,
                               size_t aCategoryUsableSize,
                               size_t aTotalUsableSize)
{
  PrintSortedGroups(aWriter, aStr, astr, aLiveBlockGroupTable,
                    aCategoryUsableSize, aTotalUsableSize);

  
  
  if (gMode == Test) {
    return;
  }

  FrameGroupTable frameGroupTable;
  frameGroupTable.init(2048);
  for (LiveBlockGroupTable::Range r = aLiveBlockGroupTable.all();
       !r.empty();
       r.popFront()) {
    const LiveBlockGroup& bg = r.front();
    const StackTrace* st = bg.mAllocStackTrace;

    
    
    StackTrace sorted(*st);
    sorted.Sort();              
    void* prevPc = (void*)intptr_t(-1);
    for (uint32_t i = 0; i < sorted.Length(); i++) {
      void* pc = sorted.Pc(i);
      if (pc == prevPc) {
        continue;               
      }
      prevPc = pc;

      FrameGroupTable::AddPtr p = frameGroupTable.lookupForAdd(pc);
      if (!p) {
        FrameGroup fg(pc);
        (void)frameGroupTable.add(p, fg);
      }
      p->Add(bg);
    }
  }
  PrintSortedGroups(aWriter, aStr, astr, frameGroupTable, kNoSize,
                    aTotalUsableSize);
}





MOZ_EXPORT void
SizeOf(Sizes* aSizes)
{
  aSizes->mStackTraces = 0;
  for (StackTraceTable::Range r = gStackTraceTable->all();
       !r.empty();
       r.popFront()) {
    StackTrace* const& st = r.front();
    aSizes->mStackTraces += MallocSizeOf(st);
  }

  aSizes->mStackTraceTable =
    gStackTraceTable->sizeOfIncludingThis(MallocSizeOf);

  aSizes->mLiveBlockTable = gLiveBlockTable->sizeOfIncludingThis(MallocSizeOf);

  aSizes->mDoubleReportTable =
    gDoubleReportBlockGroupTable->sizeOfIncludingThis(MallocSizeOf);
}

static void
ClearState()
{
  
  
  for (LiveBlockTable::Range r = gLiveBlockTable->all();
       !r.empty();
       r.popFront()) {
    r.front().UnreportIfNotReportedOnAlloc();
  }

  
  gDoubleReportBlockGroupTable->finish();
  gDoubleReportBlockGroupTable->init();
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

  StatusMsg("  gathering live block groups...\n");

  LiveBlockGroupTable unreportedLiveBlockGroupTable;
  (void)unreportedLiveBlockGroupTable.init(1024);
  size_t unreportedUsableSize = 0;

  LiveBlockGroupTable reportedLiveBlockGroupTable;
  (void)reportedLiveBlockGroupTable.init(1024);
  size_t reportedUsableSize = 0;

  bool anyBlocksSampled = false;

  for (LiveBlockTable::Range r = gLiveBlockTable->all();
       !r.empty();
       r.popFront()) {
    const LiveBlock& b = r.front();

    size_t& size = !b.IsReported() ? unreportedUsableSize : reportedUsableSize;
    size += b.UsableSize();

    LiveBlockGroupTable& table = !b.IsReported()
                               ? unreportedLiveBlockGroupTable
                               : reportedLiveBlockGroupTable;
    LiveBlockKey liveKey(b);
    LiveBlockGroupTable::AddPtr p = table.lookupForAdd(liveKey);
    if (!p) {
      LiveBlockGroup bg(b);
      (void)table.add(p, bg);
    }
    p->Add(b);

    anyBlocksSampled = anyBlocksSampled || b.IsSampled();
  }
  size_t totalUsableSize = unreportedUsableSize + reportedUsableSize;

  WriteTitle("Invocation\n");
  W("$DMD = '%s'\n", gDMDEnvVar);
  W("Sample-below size = %lld\n\n", (long long)(gSampleBelowSize));

  PrintSortedGroups(aWriter, "Double-reported", "double-reported",
                    *gDoubleReportBlockGroupTable, kNoSize, kNoSize);

  PrintSortedBlockAndFrameGroups(aWriter, "Unreported", "unreported",
                                 unreportedLiveBlockGroupTable,
                                 unreportedUsableSize, totalUsableSize);

  PrintSortedBlockAndFrameGroups(aWriter, "Reported", "reported",
                                 reportedLiveBlockGroupTable,
                                 reportedUsableSize, totalUsableSize);

  bool showTilde = anyBlocksSampled;
  WriteTitle("Summary\n");
  W("Total:      %s bytes\n",
    Show(totalUsableSize, gBuf1, kBufLen, showTilde));
  W("Reported:   %s bytes (%5.2f%%)\n",
    Show(reportedUsableSize, gBuf1, kBufLen, showTilde),
    Percent(reportedUsableSize, totalUsableSize));
  W("Unreported: %s bytes (%5.2f%%)\n",
    Show(unreportedUsableSize, gBuf1, kBufLen, showTilde),
    Percent(unreportedUsableSize, totalUsableSize));

  W("\n");

  
  if (gMode != Test) {
    Sizes sizes;
    SizeOf(&sizes);

    WriteTitle("Execution measurements\n");

    W("Data structures that persist after Dump() ends:\n");

    W("  Stack traces:        %10s bytes\n",
      Show(sizes.mStackTraces, gBuf1, kBufLen));

    W("  Stack trace table:   %10s bytes (%s entries, %s used)\n",
      Show(sizes.mStackTraceTable,       gBuf1, kBufLen),
      Show(gStackTraceTable->capacity(), gBuf2, kBufLen),
      Show(gStackTraceTable->count(),    gBuf3, kBufLen));

    W("  Live block table:    %10s bytes (%s entries, %s used)\n",
      Show(sizes.mLiveBlockTable,       gBuf1, kBufLen),
      Show(gLiveBlockTable->capacity(), gBuf2, kBufLen),
      Show(gLiveBlockTable->count(),    gBuf3, kBufLen));

    W("\nData structures that are cleared after Dump() ends:\n");

    W("  Double-report table: %10s bytes (%s entries, %s used)\n",
      Show(sizes.mDoubleReportTable,                 gBuf1, kBufLen),
      Show(gDoubleReportBlockGroupTable->capacity(), gBuf2, kBufLen),
      Show(gDoubleReportBlockGroupTable->count(),    gBuf3, kBufLen));

    size_t unreportedSize =
      unreportedLiveBlockGroupTable.sizeOfIncludingThis(MallocSizeOf);
    W("  Unreported table:    %10s bytes (%s entries, %s used)\n",
      Show(unreportedSize,                           gBuf1, kBufLen),
      Show(unreportedLiveBlockGroupTable.capacity(), gBuf2, kBufLen),
      Show(unreportedLiveBlockGroupTable.count(),    gBuf3, kBufLen));

    size_t reportedSize =
      reportedLiveBlockGroupTable.sizeOfIncludingThis(MallocSizeOf);
    W("  Reported table:      %10s bytes (%s entries, %s used)\n",
      Show(reportedSize,                           gBuf1, kBufLen),
      Show(reportedLiveBlockGroupTable.capacity(), gBuf2, kBufLen),
      Show(reportedLiveBlockGroupTable.count(),    gBuf3, kBufLen));

    W("\n");
  }

  ClearState();

  StatusMsg("}\n");
}







void foo()
{
   char* a[6];
   for (int i = 0; i < 6; i++) {
      a[i] = (char*) malloc(128 - 16*i);
   }

   for (int i = 0; i <= 1; i++)
      Report(a[i], "a01");              
   Report(a[2], "a23");                 
   Report(a[3], "a23");                 
   
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

  
  gSampleBelowSize = 1;

  
  Dump(writer);

  
  
  int i;
  char* a;
  for (i = 0; i < 10; i++) {
      a = (char*) malloc(100);
      UseItOrLoseIt(a);
  }
  free(a);

  
  
  
  char* a2 = (char*) malloc(0);
  Report(a2, "a2");

  
  
  
  char* b = new char[10];
  ReportOnAlloc(b, "b");

  
  
  
  char* b2 = new char;
  ReportOnAlloc(b2, "b2");
  free(b2);

  
  
  char* c = (char*) calloc(10, 3);
  Report(c, "c");
  for (int i = 0; i < 3; i++) {
    Report(c, "c");
  }

  
  
  Report((void*)(intptr_t)i, "d");

  
  
  
  char* e = (char*) malloc(4096);
  e = (char*) realloc(e, 4097);
  Report(e, "e");

  
  
  
  char* e2 = (char*) realloc(nullptr, 1024);
  e2 = (char*) realloc(e2, 512);
  Report(e2, "e2");

  
  
  
  
  char* e3 = (char*) realloc(nullptr, 1024);
  e3 = (char*) realloc(e3, 0);
  MOZ_ASSERT(e3);
  Report(e3, "a2");

  
  
  char* f = (char*) malloc(64);
  free(f);

  
  
  Report((void*)(intptr_t)0x0, "zero");

  
  
  foo();
  foo();

  
  
  
  


  



  void* z = valloc(1);                  
  UseItOrLoseIt(z);


  
  Dump(writer);

  

  Report(a2, "a2b");
  Report(a2, "a2b");
  free(c);
  free(e);
  Report(e2, "e2b");
  free(e3);


  free(z);

  
  Dump(writer);

  

  
  gLiveBlockTable->clear();

  
  
  gSmallBlockActualSizeCounter = 0;
  gSampleBelowSize = 128;

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

  
  gSampleBelowSize = 1;

  stress1(); stress1(); stress1(); stress1(); stress1();
  stress1(); stress1(); stress1(); stress1(); stress1();

  Dump(writer);
}

}   
}   
