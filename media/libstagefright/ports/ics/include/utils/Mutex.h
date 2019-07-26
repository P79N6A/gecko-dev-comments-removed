















#ifndef _LIBS_UTILS_MUTEX_H
#define _LIBS_UTILS_MUTEX_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#include <utils/Errors.h>

#ifdef _MSC_VER
#define __attribute__(X)
#endif


namespace android {


class Condition;







class Mutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };
    
                Mutex();
                Mutex(const char* name);
                Mutex(int type, const char* name = NULL);
                ~Mutex();

    
    status_t    lock();
    void        unlock();

    
    status_t    tryLock();

    
    
    class Autolock {
    public:
        inline Autolock(Mutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline Autolock(Mutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~Autolock() { mLock.unlock(); }
    private:
        Mutex& mLock;
    };

private:
    friend class Condition;
    
    
                Mutex(const Mutex&);
    Mutex&      operator = (const Mutex&);
};



inline Mutex::Mutex() {
}
inline Mutex::Mutex(__attribute__((unused)) const char* name) {
}
inline Mutex::Mutex(int type, __attribute__((unused)) const char* name) {
}
inline Mutex::~Mutex() {
}
inline status_t Mutex::lock() {
  return OK;
}
inline void Mutex::unlock() {
}
inline status_t Mutex::tryLock() {
  return OK;
}








 
typedef Mutex::Autolock AutoMutex;


}; 


#ifdef _MSC_VER
#undef __attribute__
#endif

#endif
