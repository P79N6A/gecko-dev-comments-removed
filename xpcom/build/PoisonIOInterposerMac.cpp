





#include "PoisonIOInterposer.h"
#include "mach_override.h"

#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/IOInterposer.h"
#include "mozilla/Mutex.h"
#include "mozilla/ProcessedStack.h"
#include "mozilla/Scoped.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Util.h"
#include "nsPrintfCString.h"
#include "nsStackWalk.h"
#include "nsTraceRefcntImpl.h"
#include "plstr.h"
#include "prio.h"

#include <vector>
#include <algorithm>
#include <string.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <aio.h>
#include <dlfcn.h>

namespace {

using namespace mozilla;


static bool sIsEnabled = false;


static bool sOnlyReportDirtyWrites = false;


bool IsValidWrite(int fd, const void *wbuf, size_t count);
bool IsIPCWrite(int fd, const struct stat &buf);







class MacIOAutoObservation : public IOInterposeObserver::Observation
{
public:
  MacIOAutoObservation(IOInterposeObserver::Operation aOp,
                       const char* aReference, int aFd)
    : mShouldObserve(sIsEnabled && IOInterposer::IsObservedOperation(aOp) &&
                     !IsDebugFile(aFd))
  {
    if (mShouldObserve) {
      mOperation = aOp;
      mReference = aReference;
      mStart = TimeStamp::Now();
    }
  }

  MacIOAutoObservation(IOInterposeObserver::Operation aOp,
                       const char* aReference, int aFd, const void *aBuf,
                       size_t aCount)
    : mShouldObserve(sIsEnabled && IOInterposer::IsObservedOperation(aOp) &&
                     !IsDebugFile(aFd))
  {
    if (mShouldObserve) {
      mShouldObserve = IsValidWrite(aFd, aBuf, aCount);
      if (mShouldObserve) {
        mOperation = aOp;
        mReference = aReference;
        mStart = TimeStamp::Now();
      }
    }
  }

  ~MacIOAutoObservation()
  {
    if (mShouldObserve) {
      mEnd = TimeStamp::Now();

      
      IOInterposer::Report(*this);
    }
  }

private:
  bool                mShouldObserve;
};





bool IsIPCWrite(int fd, const struct stat &buf) {
  if ((buf.st_mode & S_IFMT) == S_IFIFO) {
    return true;
  }

  if ((buf.st_mode & S_IFMT) != S_IFSOCK) {
    return false;
  }

  sockaddr_storage address;
  socklen_t len = sizeof(address);
  if (getsockname(fd, (sockaddr*) &address, &len) != 0) {
    return true; 
  }

  return address.ss_family == AF_UNIX;
}


bool IsValidWrite(int fd, const void *wbuf, size_t count)
{
  
  if (count == 0) {
    return false;
  }

  {
    struct stat buf;
    int rv = fstat(fd, &buf);
    if (rv != 0) {
      return true;
    }

    if (IsIPCWrite(fd, buf)) {
      return false;
    }
  }

  
  
  if (!wbuf) {
    return true;
  }

  
  if(!sOnlyReportDirtyWrites) {
    return true;
  }

  
  
  
  
  ScopedFreePtr<void> wbuf2(malloc(count));
  if (!wbuf2) {
    return true;
  }
  off_t pos = lseek(fd, 0, SEEK_CUR);
  if (pos == -1) {
    return true;
  }
  ssize_t r = read(fd, wbuf2, count);
  if (r < 0 || (size_t)r != count) {
    return true;
  }
  int cmp = memcmp(wbuf, wbuf2, count);
  if (cmp != 0) {
    return true;
  }
  off_t pos2 = lseek(fd, pos, SEEK_SET);
  if (pos2 != pos) {
    return true;
  }

  
  return false;
}




struct FuncData {
  const char *Name;      
  const void *Wrapper;   
  void *Function;        
  void *Buffer;          
                         
};


typedef ssize_t (*aio_write_t)(struct aiocb *aiocbp);
ssize_t wrap_aio_write(struct aiocb *aiocbp);
FuncData aio_write_data = { 0, (void*) wrap_aio_write, (void*) aio_write };
ssize_t wrap_aio_write(struct aiocb *aiocbp) {
  const char* ref = "aio_write";
  MacIOAutoObservation timer(IOInterposeObserver::OpWrite, ref,
                             aiocbp->aio_fildes);

  aio_write_t old_write = (aio_write_t) aio_write_data.Buffer;
  return old_write(aiocbp);
}



typedef ssize_t (*pwrite_t)(int fd, const void *buf, size_t nbyte, off_t offset);
template<FuncData &foo>
ssize_t wrap_pwrite_temp(int fd, const void *buf, size_t nbyte, off_t offset) {
  const char* ref = "pwrite_*";
  MacIOAutoObservation timer(IOInterposeObserver::OpWrite, ref, fd);
  pwrite_t old_write = (pwrite_t) foo.Buffer;
  return old_write(fd, buf, nbyte, offset);
}


#define DEFINE_PWRITE_DATA(X, NAME)                                        \
FuncData X ## _data = { NAME, (void*) wrap_pwrite_temp<X ## _data> };      \


DEFINE_PWRITE_DATA(pwrite, "pwrite")

DEFINE_PWRITE_DATA(pwrite_NOCANCEL_UNIX2003, "pwrite$NOCANCEL$UNIX2003");
DEFINE_PWRITE_DATA(pwrite_UNIX2003, "pwrite$UNIX2003");

DEFINE_PWRITE_DATA(pwrite_NOCANCEL, "pwrite$NOCANCEL");


typedef ssize_t (*writev_t)(int fd, const struct iovec *iov, int iovcnt);
template<FuncData &foo>
ssize_t wrap_writev_temp(int fd, const struct iovec *iov, int iovcnt) {
  const char* ref = "pwrite_*";
  MacIOAutoObservation timer(IOInterposeObserver::OpWrite, ref, fd, nullptr,
                             iovcnt);
  writev_t old_write = (writev_t) foo.Buffer;
  return old_write(fd, iov, iovcnt);
}


#define DEFINE_WRITEV_DATA(X, NAME)                                   \
FuncData X ## _data = { NAME, (void*) wrap_writev_temp<X ## _data> }; \


DEFINE_WRITEV_DATA(writev, "writev");

DEFINE_WRITEV_DATA(writev_NOCANCEL_UNIX2003, "writev$NOCANCEL$UNIX2003");
DEFINE_WRITEV_DATA(writev_UNIX2003, "writev$UNIX2003");

DEFINE_WRITEV_DATA(writev_NOCANCEL, "writev$NOCANCEL");

typedef ssize_t (*write_t)(int fd, const void *buf, size_t count);
template<FuncData &foo>
ssize_t wrap_write_temp(int fd, const void *buf, size_t count) {
  const char* ref = "pwrite_*";
  MacIOAutoObservation timer(IOInterposeObserver::OpWrite, ref, fd, buf,
                             count);
  write_t old_write = (write_t) foo.Buffer;
  return old_write(fd, buf, count);
}


#define DEFINE_WRITE_DATA(X, NAME)                                   \
FuncData X ## _data = { NAME, (void*) wrap_write_temp<X ## _data> }; \


DEFINE_WRITE_DATA(write, "write");

DEFINE_WRITE_DATA(write_NOCANCEL_UNIX2003, "write$NOCANCEL$UNIX2003");
DEFINE_WRITE_DATA(write_UNIX2003, "write$UNIX2003");

DEFINE_WRITE_DATA(write_NOCANCEL, "write$NOCANCEL");

FuncData *Functions[] = { &aio_write_data,

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
                          &writev_NOCANCEL_data};

const int NumFunctions = ArrayLength(Functions);

} 



namespace mozilla {

void InitPoisonIOInterposer() {
  
  sIsEnabled = true;

  
  static bool WritesArePoisoned = false;
  if (WritesArePoisoned) {
    return;
  }
  WritesArePoisoned = true;

  
  MozillaRegisterDebugFD(1);
  MozillaRegisterDebugFD(2);

  for (int i = 0; i < NumFunctions; ++i) {
    FuncData *d = Functions[i];
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

void OnlyReportDirtyWrites() {
  sOnlyReportDirtyWrites = true;
}

void ClearPoisonIOInterposer() {
  
  
  sIsEnabled = false;
}

} 
