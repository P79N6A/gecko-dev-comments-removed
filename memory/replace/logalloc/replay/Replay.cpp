





#define MOZ_MEMORY_IMPL
#include "mozmemory_wrap.h"

#ifdef _WIN32


#define NOMINMAX
#include <windows.h>
#include <io.h>
typedef int ssize_t;
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "mozilla/Assertions.h"
#include "FdPrintf.h"

static void
die(const char* message)
{
  
  fprintf(stderr, "%s\n", message);
  exit(1);
}




template <typename T, size_t Len>
class MappedArray
{
public:
  MappedArray(): mPtr(nullptr) {}

  ~MappedArray()
  {
    if (mPtr) {
#ifdef _WIN32
      VirtualFree(mPtr, sizeof(T) * Len, MEM_RELEASE);
#else
      munmap(mPtr, sizeof(T) * Len);
#endif
    }
  }

  T& operator[] (size_t aIndex) const
  {
    if (mPtr) {
      return mPtr[aIndex];
    }

#ifdef _WIN32
    mPtr = reinterpret_cast<T*>(VirtualAlloc(nullptr, sizeof(T) * Len,
             MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (mPtr == nullptr) {
      die("VirtualAlloc error");
    }
#else
    mPtr = reinterpret_cast<T*>(mmap(nullptr, sizeof(T) * Len,
             PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0));
    if (mPtr == MAP_FAILED) {
      die("Mmap error");
    }
#endif
    return mPtr[aIndex];
  }

private:
  mutable T* mPtr;
};


struct MemSlot
{
  void* mPtr;
  size_t mSize;
};










class MemSlotList
{
  static const size_t kGroups = 1024 - 1;
  static const size_t kGroupSize = (1024 * 1024) / sizeof(MemSlot);

  MappedArray<MemSlot, kGroupSize> mSlots[kGroups];
  MappedArray<MemSlotList, 1> mNext;

public:
  MemSlot& operator[] (size_t aIndex) const
  {
    if (aIndex < kGroupSize * kGroups) {
      return mSlots[aIndex / kGroupSize][aIndex % kGroupSize];
    }
    aIndex -= kGroupSize * kGroups;
    return mNext[0][aIndex];
  }
};


class Buffer
{
public:
  Buffer() : mBuf(nullptr), mLength(0) {}

  Buffer(const void* aBuf, size_t aLength)
    : mBuf(reinterpret_cast<const char*>(aBuf)), mLength(aLength)
  {}

  
  template <size_t Size>
  Buffer(const char (&aStr)[Size])
    : mBuf(aStr), mLength(Size - 1)
  {}

  




  Buffer SplitChar(char aNeedle)
  {
    char* buf = const_cast<char*>(mBuf);
    char* c = reinterpret_cast<char*>(memchr(buf, aNeedle, mLength));
    if (!c) {
      return Split(mLength);
    }

    Buffer result = Split(c - buf);
    
    Split(1);
    return result;
  }

  


  Buffer Split(size_t aLength)
  {
    Buffer result(mBuf, std::min(aLength, mLength));
    mLength -= result.mLength;
    mBuf += result.mLength;
    return result;
  }

  

  void Slide(Buffer aOther)
  {
    memmove(const_cast<char*>(aOther.mBuf), mBuf, mLength);
    mBuf = aOther.mBuf;
  }

  
  bool operator ==(Buffer aOther)
  {
    return mLength == aOther.mLength && (mBuf == aOther.mBuf ||
                                         !strncmp(mBuf, aOther.mBuf, mLength));
  }

  
  explicit operator bool() { return mLength; }

  
  const char* get() { return mBuf; }

  

  const char* GetEnd() { return mBuf + mLength; }

  

  void Extend(Buffer aOther)
  {
    MOZ_ASSERT(aOther.mBuf == GetEnd());
    mLength += aOther.mLength;
  }

private:
  const char* mBuf;
  size_t mLength;
};


class FdReader {
public:
  explicit FdReader(int aFd)
    : mFd(aFd)
    , mData(&mRawBuf, 0)
    , mBuf(&mRawBuf, sizeof(mRawBuf))
  {}

  
  Buffer ReadLine()
  {
    while (true) {
      Buffer result = mData.SplitChar('\n');

      












      if (result.GetEnd() != mData.GetEnd()) {
        return result;
      }

      
      mData = result;

      
      mData.Slide(mBuf);

      FillBuffer();

      if (!mData) {
        return Buffer();
      }
    }
  }

private:
  
  void FillBuffer()
  {
    size_t size = mBuf.GetEnd() - mData.GetEnd();
    Buffer remainder(mData.GetEnd(), size);

    ssize_t len = 1;
    while (remainder && len > 0) {
      len = ::read(mFd, const_cast<char*>(remainder.get()), size);
      if (len < 0) {
        die("Read error");
      }
      size -= len;
      mData.Extend(remainder.Split(len));
    }
  }

  
  int mFd;
  

  Buffer mData;
  
  Buffer mBuf;
  
  char mRawBuf[4096];
};

MOZ_BEGIN_EXTERN_C



#define MALLOC_DECL(name, return_type, ...) \
  return_type name ## _impl(__VA_ARGS__);
#define MALLOC_FUNCS MALLOC_FUNCS_MALLOC
#include "malloc_decls.h"

#define MALLOC_DECL(name, return_type, ...) \
  return_type name ## _impl(__VA_ARGS__);
#define MALLOC_FUNCS MALLOC_FUNCS_JEMALLOC
#include "malloc_decls.h"



#if defined(_WIN32) && !defined(MOZ_JEMALLOC3)
void malloc_init_hard(void);
#endif

#ifdef ANDROID


void
MozTagAnonymousMemory(const void* aPtr, size_t aLength, const char* aTag) {}




int
pthread_atfork(void (*aPrepare)(void), void (*aParent)(void),
               void (*aChild)(void))
{
  return 0;
}
#endif

#ifdef MOZ_NUWA_PROCESS
#include <pthread.h>



int
__real_pthread_mutex_lock(pthread_mutex_t* aMutex)
{
  return pthread_mutex_lock(aMutex);
}
#endif

MOZ_END_EXTERN_C

size_t parseNumber(Buffer aBuf)
{
  if (!aBuf) {
    die("Malformed input");
  }

  size_t result = 0;
  for (const char* c = aBuf.get(), *end = aBuf.GetEnd(); c < end; c++) {
    if (*c < '0' || *c > '9') {
      die("Malformed input");
    }
    result *= 10;
    result += *c - '0';
  }
  return result;
}


class Replay
{
public:
  Replay(): mOps(0) {
#ifdef _WIN32
    
    mStdErr = reinterpret_cast<intptr_t>(GetStdHandle(STD_ERROR_HANDLE));
#else
    mStdErr = fileno(stderr);
#endif
  }

  MemSlot& operator[] (size_t index) const
  {
    return mSlots[index];
  }

  void malloc(MemSlot& aSlot, Buffer& aArgs)
  {
    mOps++;
    size_t size = parseNumber(aArgs);
    aSlot.mPtr = ::malloc_impl(size);
    aSlot.mSize = size;
    Commit(aSlot);
  }

  void posix_memalign(MemSlot& aSlot, Buffer& aArgs)
  {
    mOps++;
    size_t alignment = parseNumber(aArgs.SplitChar(','));
    size_t size = parseNumber(aArgs);
    void* ptr;
    if (::posix_memalign_impl(&ptr, alignment, size) == 0) {
      aSlot.mPtr = ptr;
      aSlot.mSize = size;
    } else {
      aSlot.mPtr = nullptr;
      aSlot.mSize = 0;
    }
    Commit(aSlot);
  }

  void aligned_alloc(MemSlot& aSlot, Buffer& aArgs)
  {
    mOps++;
    size_t alignment = parseNumber(aArgs.SplitChar(','));
    size_t size = parseNumber(aArgs);
    aSlot.mPtr = ::aligned_alloc_impl(alignment, size);
    aSlot.mSize = size;
    Commit(aSlot);
  }

  void calloc(MemSlot& aSlot, Buffer& aArgs)
  {
    mOps++;
    size_t num = parseNumber(aArgs.SplitChar(','));
    size_t size = parseNumber(aArgs);
    aSlot.mPtr = ::calloc_impl(num, size);
    aSlot.mSize = size * num;
    Commit(aSlot);
  }

  void realloc(MemSlot& aSlot, Buffer& aArgs)
  {
    mOps++;
    Buffer dummy = aArgs.SplitChar('#');
    if (dummy) {
      die("Malformed input");
    }
    size_t slot_id = parseNumber(aArgs.SplitChar(','));
    size_t size = parseNumber(aArgs);
    MemSlot& old_slot = (*this)[slot_id];
    void* old_ptr = old_slot.mPtr;
    old_slot.mPtr = nullptr;
    old_slot.mSize = 0;
    aSlot.mPtr = ::realloc_impl(old_ptr, size);
    aSlot.mSize = size;
    Commit(aSlot);
  }

  void free(Buffer& aArgs)
  {
    mOps++;
    Buffer dummy = aArgs.SplitChar('#');
    if (dummy) {
      die("Malformed input");
    }
    size_t slot_id = parseNumber(aArgs);
    MemSlot& slot = (*this)[slot_id];
    ::free_impl(slot.mPtr);
    slot.mPtr = nullptr;
    slot.mSize = 0;
  }

  void memalign(MemSlot& aSlot, Buffer& aArgs)
  {
    mOps++;
    size_t alignment = parseNumber(aArgs.SplitChar(','));
    size_t size = parseNumber(aArgs);
    aSlot.mPtr = ::memalign_impl(alignment, size);
    aSlot.mSize = size;
    Commit(aSlot);
  }

  void valloc(MemSlot& aSlot, Buffer& aArgs)
  {
    mOps++;
    size_t size = parseNumber(aArgs);
    aSlot.mPtr = ::valloc_impl(size);
    aSlot.mSize = size;
    Commit(aSlot);
  }

  void jemalloc_stats(Buffer& aArgs)
  {
    if (aArgs) {
      die("Malformed input");
    }
    jemalloc_stats_t stats;
    ::jemalloc_stats_impl(&stats);
    FdPrintf(mStdErr,
             "#%zu mapped: %zu; allocated: %zu; waste: %zu; dirty: %zu; "
             "bookkeep: %zu; binunused: %zu\n", mOps, stats.mapped,
             stats.allocated, stats.waste, stats.page_cache,
             stats.bookkeeping, stats.bin_unused);
    

  }

private:
  void Commit(MemSlot& aSlot)
  {
    memset(aSlot.mPtr, 0x5a, aSlot.mSize);
  }

  intptr_t mStdErr;
  size_t mOps;
  MemSlotList mSlots;
};


int
main()
{
  size_t first_pid = 0;
  FdReader reader(0);
  Replay replay;

#if defined(_WIN32) && !defined(MOZ_JEMALLOC3)
  malloc_init_hard();
#endif

  











  while (true) {
    Buffer line = reader.ReadLine();

    if (!line) {
      break;
    }

    size_t pid = parseNumber(line.SplitChar(' '));
    if (!first_pid) {
      first_pid = pid;
    }

    

    if (first_pid != pid) {
      continue;
    }

    Buffer func = line.SplitChar('(');
    Buffer args = line.SplitChar(')');

    
    if (func == Buffer("jemalloc_stats")) {
      replay.jemalloc_stats(args);
      continue;
    } else if (func == Buffer("free")) {
      replay.free(args);
      continue;
    }

    
    Buffer dummy = line.SplitChar('=');
    Buffer dummy2 = line.SplitChar('#');
    if (dummy || dummy2) {
      die("Malformed input");
    }

    size_t slot_id = parseNumber(line);
    MemSlot& slot = replay[slot_id];

    if (func == Buffer("malloc")) {
      replay.malloc(slot, args);
    } else if (func == Buffer("posix_memalign")) {
      replay.posix_memalign(slot, args);
    } else if (func == Buffer("aligned_alloc")) {
      replay.aligned_alloc(slot, args);
    } else if (func == Buffer("calloc")) {
      replay.calloc(slot, args);
    } else if (func == Buffer("realloc")) {
      replay.realloc(slot, args);
    } else if (func == Buffer("memalign")) {
      replay.memalign(slot, args);
    } else if (func == Buffer("valloc")) {
      replay.valloc(slot, args);
    } else {
      die("Malformed input");
    }
  }

  return 0;
}
