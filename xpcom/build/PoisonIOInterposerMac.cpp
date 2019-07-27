





#include "PoisonIOInterposer.h"
#include "mach_override.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/IOInterposer.h"
#include "mozilla/Mutex.h"
#include "mozilla/ProcessedStack.h"
#include "mozilla/Scoped.h"
#include "mozilla/Telemetry.h"
#include "nsPrintfCString.h"
#include "nsStackWalk.h"
#include "nsTraceRefcnt.h"
#include "plstr.h"
#include "prio.h"

#include <algorithm>
#include <vector>

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <aio.h>
#include <dlfcn.h>
#include <fcntl.h>

#ifdef MOZ_REPLACE_MALLOC
#include "replace_malloc_bridge.h"
#endif

namespace {

using namespace mozilla;


static bool sIsEnabled = false;


static bool sOnlyReportDirtyWrites = false;


bool IsValidWrite(int aFd, const void* aWbuf, size_t aCount);
bool IsIPCWrite(int aFd, const struct stat& aBuf);







class MacIOAutoObservation : public IOInterposeObserver::Observation
{
public:
  MacIOAutoObservation(IOInterposeObserver::Operation aOp, int aFd)
    : IOInterposeObserver::Observation(aOp, sReference, sIsEnabled &&
                                       !IsDebugFile(aFd))
    , mFd(aFd)
    , mHasQueriedFilename(false)
    , mFilename(nullptr)
  {
  }

  MacIOAutoObservation(IOInterposeObserver::Operation aOp, int aFd,
                       const void* aBuf, size_t aCount)
    : IOInterposeObserver::Observation(aOp, sReference, sIsEnabled &&
                                       !IsDebugFile(aFd) &&
                                       IsValidWrite(aFd, aBuf, aCount))
    , mFd(aFd)
    , mHasQueriedFilename(false)
    , mFilename(nullptr)
  {
  }

  
  const char16_t* Filename() override;

  ~MacIOAutoObservation()
  {
    Report();
    if (mFilename) {
      free(mFilename);
      mFilename = nullptr;
    }
  }

private:
  int                 mFd;
  bool                mHasQueriedFilename;
  char16_t*           mFilename;
  static const char*  sReference;
};

const char* MacIOAutoObservation::sReference = "PoisonIOInterposer";


const char16_t*
MacIOAutoObservation::Filename()
{
  
  if (mHasQueriedFilename) {
    return mFilename;
  }
  char filename[MAXPATHLEN];
  if (fcntl(mFd, F_GETPATH, filename) != -1) {
    mFilename = UTF8ToNewUnicode(nsDependentCString(filename));
  } else {
    mFilename = nullptr;
  }
  mHasQueriedFilename = true;

  
  return mFilename;
}





bool
IsIPCWrite(int aFd, const struct stat& aBuf)
{
  if ((aBuf.st_mode & S_IFMT) == S_IFIFO) {
    return true;
  }

  if ((aBuf.st_mode & S_IFMT) != S_IFSOCK) {
    return false;
  }

  sockaddr_storage address;
  socklen_t len = sizeof(address);
  if (getsockname(aFd, (sockaddr*)&address, &len) != 0) {
    return true; 
  }

  return address.ss_family == AF_UNIX;
}


bool
IsValidWrite(int aFd, const void* aWbuf, size_t aCount)
{
  
  if (aCount == 0) {
    return false;
  }

  {
    struct stat buf;
    int rv = fstat(aFd, &buf);
    if (rv != 0) {
      return true;
    }

    if (IsIPCWrite(aFd, buf)) {
      return false;
    }
  }

  
  
  if (!aWbuf) {
    return true;
  }

  
  if (!sOnlyReportDirtyWrites) {
    return true;
  }

  
  
  
  
  ScopedFreePtr<void> wbuf2(malloc(aCount));
  if (!wbuf2) {
    return true;
  }
  off_t pos = lseek(aFd, 0, SEEK_CUR);
  if (pos == -1) {
    return true;
  }
  ssize_t r = read(aFd, wbuf2, aCount);
  if (r < 0 || (size_t)r != aCount) {
    return true;
  }
  int cmp = memcmp(aWbuf, wbuf2, aCount);
  if (cmp != 0) {
    return true;
  }
  off_t pos2 = lseek(aFd, pos, SEEK_SET);
  if (pos2 != pos) {
    return true;
  }

  
  return false;
}




struct FuncData
{
  const char* Name;      
  const void* Wrapper;   
  void* Function;        
  void* Buffer;          
                         
};


typedef ssize_t (*aio_write_t)(struct aiocb* aAioCbp);
ssize_t wrap_aio_write(struct aiocb* aAioCbp);
FuncData aio_write_data = { 0, (void*)wrap_aio_write, (void*)aio_write };
ssize_t
wrap_aio_write(struct aiocb* aAioCbp)
{
  MacIOAutoObservation timer(IOInterposeObserver::OpWrite,
                             aAioCbp->aio_fildes);

  aio_write_t old_write = (aio_write_t)aio_write_data.Buffer;
  return old_write(aAioCbp);
}



typedef ssize_t (*pwrite_t)(int aFd, const void* buf, size_t aNumBytes,
                            off_t aOffset);
template<FuncData& foo>
ssize_t
wrap_pwrite_temp(int aFd, const void* aBuf, size_t aNumBytes, off_t aOffset)
{
  MacIOAutoObservation timer(IOInterposeObserver::OpWrite, aFd);
  pwrite_t old_write = (pwrite_t)foo.Buffer;
  return old_write(aFd, aBuf, aNumBytes, aOffset);
}


#define DEFINE_PWRITE_DATA(X, NAME)                                        \
FuncData X ## _data = { NAME, (void*) wrap_pwrite_temp<X ## _data> };      \


DEFINE_PWRITE_DATA(pwrite, "pwrite")

DEFINE_PWRITE_DATA(pwrite_NOCANCEL_UNIX2003, "pwrite$NOCANCEL$UNIX2003");
DEFINE_PWRITE_DATA(pwrite_UNIX2003, "pwrite$UNIX2003");

DEFINE_PWRITE_DATA(pwrite_NOCANCEL, "pwrite$NOCANCEL");


typedef ssize_t (*writev_t)(int aFd, const struct iovec* aIov, int aIovCount);
template<FuncData& foo>
ssize_t
wrap_writev_temp(int aFd, const struct iovec* aIov, int aIovCount)
{
  MacIOAutoObservation timer(IOInterposeObserver::OpWrite, aFd, nullptr,
                             aIovCount);
  writev_t old_write = (writev_t)foo.Buffer;
  return old_write(aFd, aIov, aIovCount);
}


#define DEFINE_WRITEV_DATA(X, NAME)                                   \
FuncData X ## _data = { NAME, (void*) wrap_writev_temp<X ## _data> }; \


DEFINE_WRITEV_DATA(writev, "writev");

DEFINE_WRITEV_DATA(writev_NOCANCEL_UNIX2003, "writev$NOCANCEL$UNIX2003");
DEFINE_WRITEV_DATA(writev_UNIX2003, "writev$UNIX2003");

DEFINE_WRITEV_DATA(writev_NOCANCEL, "writev$NOCANCEL");

typedef ssize_t (*write_t)(int aFd, const void* aBuf, size_t aCount);
template<FuncData& foo>
ssize_t
wrap_write_temp(int aFd, const void* aBuf, size_t aCount)
{
  MacIOAutoObservation timer(IOInterposeObserver::OpWrite, aFd, aBuf, aCount);
  write_t old_write = (write_t)foo.Buffer;
  return old_write(aFd, aBuf, aCount);
}


#define DEFINE_WRITE_DATA(X, NAME)                                   \
FuncData X ## _data = { NAME, (void*) wrap_write_temp<X ## _data> }; \


DEFINE_WRITE_DATA(write, "write");

DEFINE_WRITE_DATA(write_NOCANCEL_UNIX2003, "write$NOCANCEL$UNIX2003");
DEFINE_WRITE_DATA(write_UNIX2003, "write$UNIX2003");

DEFINE_WRITE_DATA(write_NOCANCEL, "write$NOCANCEL");

FuncData* Functions[] = {
  &aio_write_data,

  &pwrite_data,
  &pwrite_NOCANCEL_UNIX2003_data,
  &pwrite_UNIX2003_data,
  &pwrite_NOCANCEL_data,

  &write_data,
  &write_NOCANCEL_UNIX2003_data,
  &write_UNIX2003_data,
  &write_NOCANCEL_data,

  &writev_data,
  &writev_NOCANCEL_UNIX2003_data,
  &writev_UNIX2003_data,
  &writev_NOCANCEL_data
};

const int NumFunctions = ArrayLength(Functions);

} 



namespace mozilla {

void
InitPoisonIOInterposer()
{
  
  sIsEnabled = true;

  
  static bool WritesArePoisoned = false;
  if (WritesArePoisoned) {
    return;
  }
  WritesArePoisoned = true;

  
  MozillaRegisterDebugFD(1);
  MozillaRegisterDebugFD(2);

#ifdef MOZ_REPLACE_MALLOC
  
  
  
  static DebugFdRegistry registry;
  ReplaceMalloc::InitDebugFd(registry);
#endif

  for (int i = 0; i < NumFunctions; ++i) {
    FuncData* d = Functions[i];
    if (!d->Function) {
      d->Function = dlsym(RTLD_DEFAULT, d->Name);
    }
    if (!d->Function) {
      continue;
    }
    DebugOnly<mach_error_t> t = mach_override_ptr(d->Function, d->Wrapper,
                                                  &d->Buffer);
    MOZ_ASSERT(t == err_none);
  }
}

void
OnlyReportDirtyWrites()
{
  sOnlyReportDirtyWrites = true;
}

void
ClearPoisonIOInterposer()
{
  
  
  sIsEnabled = false;
}

} 
