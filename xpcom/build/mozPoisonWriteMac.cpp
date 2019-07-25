





#include "mozilla/mozPoisonWrite.h"
#include "mozilla/Util.h"
#include "nsTraceRefcntImpl.h"
#include "mozilla/Assertions.h"
#include "mozilla/Scoped.h"
#include "mozilla/Mutex.h"
#include "nsStackWalk.h"
#include "mach_override.h"
#include <sys/stat.h>
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

typedef ssize_t (*write_t)(int fd, const void *buf, size_t count);
void AbortOnBadWrite(int fd, const void *wbuf, size_t count);
template<FuncData &foo>
ssize_t wrap_write_temp(int fd, const void *buf, size_t count) {
    AbortOnBadWrite(fd, buf, count);
    write_t old_write = (write_t) foo.Buffer;
    return old_write(fd, buf, count);
}





#define DEFINE_F_DATA(X)                                             \
ssize_t wrap_ ## X (int fd, const void *buf, size_t count);          \
FuncData X ## _data = { 0, (void*) wrap_ ## X, (void*) X };          \
ssize_t wrap_ ## X (int fd, const void *buf, size_t count) {         \
    return wrap_write_temp<X ## _data>(fd, buf, count);              \
}


#define DEFINE_F_DATA_DYN(X, NAME)                                   \
ssize_t wrap_ ## X (int fd, const void *buf, size_t count);          \
FuncData X ## _data = { NAME, (void*) wrap_ ## X };                  \
ssize_t wrap_ ## X (int fd, const void *buf, size_t count) {         \
    return wrap_write_temp<X ## _data>(fd, buf, count);              \
}


#define DEFINE_F_DATA_ABORT(X)                                       \
void wrap_ ## X() { abort(); }                                       \
FuncData X ## _data = { 0, (void*) wrap_ ## X, (void*) X }


#define DEFINE_F_DATA_ABORT_DYN(X, NAME)                             \
void wrap_ ## X() { abort(); }                                       \
FuncData X ## _data = { NAME, (void*) wrap_ ## X }

DEFINE_F_DATA_ABORT(aio_write);
DEFINE_F_DATA_ABORT(pwrite);


DEFINE_F_DATA_ABORT_DYN(writev_NOCANCEL_UNIX2003, "writev$NOCANCEL$UNIX2003");
DEFINE_F_DATA_ABORT_DYN(writev_UNIX2003, "writev$UNIX2003");
DEFINE_F_DATA_ABORT_DYN(pwrite_NOCANCEL_UNIX2003, "pwrite$NOCANCEL$UNIX2003");
DEFINE_F_DATA_ABORT_DYN(pwrite_UNIX2003, "pwrite$UNIX2003");


DEFINE_F_DATA_ABORT_DYN(writev_NOCANCEL, "writev$NOCANCEL");
DEFINE_F_DATA_ABORT_DYN(pwrite_NOCANCEL, "pwrite$NOCANCEL");

DEFINE_F_DATA(write);

typedef ssize_t (*writev_t)(int fd, const struct iovec *iov, int iovcnt);
ssize_t wrap_writev(int fd, const struct iovec *iov, int iovcnt);
FuncData writev_data = { 0, (void*) wrap_writev, (void*) writev };
ssize_t wrap_writev(int fd, const struct iovec *iov, int iovcnt) {
    AbortOnBadWrite(fd, 0, iovcnt);
    writev_t old_write = (writev_t) writev_data.Buffer;
    return old_write(fd, iov, iovcnt);
}


DEFINE_F_DATA_DYN(write_NOCANCEL_UNIX2003, "write$NOCANCEL$UNIX2003");
DEFINE_F_DATA_DYN(write_UNIX2003, "write$UNIX2003");


DEFINE_F_DATA_DYN(write_NOCANCEL, "write$NOCANCEL");

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

std::vector<int>& getDebugFDs() {
    
    
    static std::vector<int> *DebugFDs = new std::vector<int>();
    return *DebugFDs;
}

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

bool PoisoningDisabled = false;
void AbortOnBadWrite(int fd, const void *wbuf, size_t count) {
    if (PoisoningDisabled)
        return;

    
    if (count == 0)
        return;

    
    if(fd == 1 || fd == 2)
        return;

    struct stat buf;
    int rv = fstat(fd, &buf);
    MOZ_ASSERT(rv == 0);

    
    if ((buf.st_mode & S_IFMT) == S_IFIFO)
        return;

    MyAutoLock lockedScope;

    
    std::vector<int> &Vec = getDebugFDs();
    if (std::find(Vec.begin(), Vec.end(), fd) != Vec.end())
        return;

    
    
    MOZ_ASSERT(wbuf);

    
    
    
    
    void *wbuf2 = malloc(count);
    MOZ_ASSERT(wbuf2);
    off_t pos = lseek(fd, 0, SEEK_CUR);
    MOZ_ASSERT(pos != -1);
    ssize_t r = read(fd, wbuf2, count);
    MOZ_ASSERT(r == count);
    int cmp = memcmp(wbuf, wbuf2, count);
    MOZ_ASSERT(cmp == 0);
    free(wbuf2);
    off_t pos2 = lseek(fd, pos, SEEK_SET);
    MOZ_ASSERT(pos2 == pos);
}




void (*OldCleanup)();
extern "C" void (*__cleanup)();
void FinalCleanup() {
    if (OldCleanup)
        OldCleanup();
    delete &getDebugFDs();
    PR_DestroyLock(MyAutoLock::getDebugFDsLock());
}

} 

extern "C" {
    void MozillaRegisterDebugFD(int fd) {
        MyAutoLock lockedScope;
        std::vector<int> &Vec = getDebugFDs();
        MOZ_ASSERT(std::find(Vec.begin(), Vec.end(), fd) == Vec.end());
        Vec.push_back(fd);
    }
    void MozillaUnRegisterDebugFD(int fd) {
        MyAutoLock lockedScope;
        std::vector<int> &Vec = getDebugFDs();
        std::vector<int>::iterator i = std::find(Vec.begin(), Vec.end(), fd);
        MOZ_ASSERT(i != Vec.end());
        Vec.erase(i);
    }
}

namespace mozilla {
void PoisonWrite() {
    
#ifndef DEBUG
    return;
#endif

    PoisoningDisabled = false;
    OldCleanup = __cleanup;
    __cleanup = FinalCleanup;

    for (int i = 0; i < NumFunctions; ++i) {
        FuncData *d = Functions[i];
        if (!d->Function)
            d->Function = dlsym(RTLD_DEFAULT, d->Name);
        if (!d->Function)
            continue;
        mach_error_t t = mach_override_ptr(d->Function, d->Wrapper,
                                           &d->Buffer);
        MOZ_ASSERT(t == err_none);
    }
}
void DisableWritePoisoning() {
    PoisoningDisabled = true;
}
}
