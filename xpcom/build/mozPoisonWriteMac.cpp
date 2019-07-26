





#include "mozilla/mozPoisonWrite.h"
#include "mozPoisonWriteBase.h"
#include "mozilla/Util.h"
#include "nsTraceRefcntImpl.h"
#include "mozilla/Assertions.h"
#include "mozilla/Scoped.h"
#include "mozilla/Mutex.h"
#include "mozilla/Telemetry.h"
#include "mozilla/ProcessedStack.h"
#include "nsStackWalk.h"
#include "nsPrintfCString.h"
#include "mach_override.h"
#include "prio.h"
#include "plstr.h"
#include "nsCOMPtr.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "mozilla/SHA1.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <vector>
#include <algorithm>
#include <string.h>
#include <sys/uio.h>
#include <aio.h>
#include <dlfcn.h>

namespace {
using namespace mozilla;

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
    ValidWriteAssert(0);
    aio_write_t old_write = (aio_write_t) aio_write_data.Buffer;
    return old_write(aiocbp);
}



typedef ssize_t (*pwrite_t)(int fd, const void *buf, size_t nbyte, off_t offset);
template<FuncData &foo>
ssize_t wrap_pwrite_temp(int fd, const void *buf, size_t nbyte, off_t offset) {
    ValidWriteAssert(0);
    pwrite_t old_write = (pwrite_t) foo.Buffer;
    return old_write(fd, buf, nbyte, offset);
}


#define DEFINE_PWRITE_DATA(X, NAME)                                        \
FuncData X ## _data = { NAME, (void*) wrap_pwrite_temp<X ## _data> };      \


DEFINE_PWRITE_DATA(pwrite, "pwrite")

DEFINE_PWRITE_DATA(pwrite_NOCANCEL_UNIX2003, "pwrite$NOCANCEL$UNIX2003");
DEFINE_PWRITE_DATA(pwrite_UNIX2003, "pwrite$UNIX2003");

DEFINE_PWRITE_DATA(pwrite_NOCANCEL, "pwrite$NOCANCEL");

void AbortOnBadWrite(int fd, const void *wbuf, size_t count);

typedef ssize_t (*writev_t)(int fd, const struct iovec *iov, int iovcnt);
template<FuncData &foo>
ssize_t wrap_writev_temp(int fd, const struct iovec *iov, int iovcnt) {
    AbortOnBadWrite(fd, 0, iovcnt);
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
    AbortOnBadWrite(fd, buf, count);
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

struct AutoLockTraits {
    typedef PRLock *type;
    const static type empty() {
      return NULL;
    }
    const static void release(type aL) {
        PR_Unlock(aL);
    }
};

class MyAutoLock : public Scoped<AutoLockTraits> {
public:
    static PRLock *getDebugFDsLock() {
        
        
        static PRLock *Lock = PR_NewLock();
        return Lock;
    }

    MyAutoLock() : Scoped<AutoLockTraits>(getDebugFDsLock()) {
        PR_Lock(get());
    }
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

void AbortOnBadWrite(int fd, const void *wbuf, size_t count) {
    if (!PoisonWriteEnabled())
        return;

    
    if (count == 0)
        return;

    
    if(fd == 1 || fd == 2)
        return;

    struct stat buf;
    int rv = fstat(fd, &buf);
    if (!ValidWriteAssert(rv == 0))
        return;

    if (IsIPCWrite(fd, buf))
        return;

    {
        MyAutoLock lockedScope;

        
        std::vector<int> &Vec = getDebugFDs();
        if (std::find(Vec.begin(), Vec.end(), fd) != Vec.end())
            return;
    }

    
    
    if (!ValidWriteAssert(wbuf))
        return;

    
    
    
    
    ScopedFreePtr<void> wbuf2(malloc(count));
    if (!ValidWriteAssert(wbuf2))
        return;
    off_t pos = lseek(fd, 0, SEEK_CUR);
    if (!ValidWriteAssert(pos != -1))
        return;
    ssize_t r = read(fd, wbuf2, count);
    if (!ValidWriteAssert(r == count))
        return;
    int cmp = memcmp(wbuf, wbuf2, count);
    if (!ValidWriteAssert(cmp == 0))
        return;
    off_t pos2 = lseek(fd, pos, SEEK_SET);
    if (!ValidWriteAssert(pos2 == pos))
        return;
}
} 

namespace mozilla {
void PoisonWrite() {
    
    static bool WritesArePoisoned = false;
    MOZ_ASSERT(!WritesArePoisoned);
    if (WritesArePoisoned)
        return;
    WritesArePoisoned = true;

    if (!PoisonWriteEnabled())
        return;

    for (int i = 0; i < NumFunctions; ++i) {
        FuncData *d = Functions[i];
        if (!d->Function)
            d->Function = dlsym(RTLD_DEFAULT, d->Name);
        if (!d->Function)
            continue;
        DebugOnly<mach_error_t> t = mach_override_ptr(d->Function, d->Wrapper,
                                           &d->Buffer);
        MOZ_ASSERT(t == err_none);
    }
}
}
