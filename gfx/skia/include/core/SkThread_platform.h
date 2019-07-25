








#ifndef SkThread_platform_DEFINED
#define SkThread_platform_DEFINED

#if defined(ANDROID) && !defined(SK_BUILD_FOR_ANDROID_NDK)

#include <utils/threads.h>
#include <utils/Atomic.h>

#define sk_atomic_inc(addr)     android_atomic_inc(addr)
#define sk_atomic_dec(addr)     android_atomic_dec(addr)

class SkMutex : android::Mutex {
public:
    
    
    SkMutex(bool isGlobal = true) {}
    ~SkMutex() {}

    void    acquire() { this->lock(); }
    void    release() { this->unlock(); }
};

#else




SK_API int32_t sk_atomic_inc(int32_t* addr);




SK_API int32_t sk_atomic_dec(int32_t* addr);

class SkMutex {
public:
    
    
    SkMutex(bool isGlobal = true);
    ~SkMutex();

    void    acquire();
    void    release();

private:
    bool fIsGlobal;
    enum {
        kStorageIntCount = 64
    };
    uint32_t    fStorage[kStorageIntCount];
};

#endif

#endif
