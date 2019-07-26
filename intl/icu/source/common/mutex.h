


















#ifndef MUTEX_H
#define MUTEX_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "umutex.h"

U_NAMESPACE_BEGIN























class U_COMMON_API Mutex : public UMemory {
public:
  inline Mutex(UMutex *mutex = NULL);
  inline ~Mutex();

private:
  UMutex   *fMutex;

  Mutex(const Mutex &other); 
  Mutex &operator=(const Mutex &other); 
};

inline Mutex::Mutex(UMutex *mutex)
  : fMutex(mutex)
{
  umtx_lock(fMutex);
}

inline Mutex::~Mutex()
{
  umtx_unlock(fMutex);
}









typedef void *InstantiatorFn(const void *context, UErrorCode &errorCode);









struct SimpleSingleton {
    void *fInstance;

    






    void *getInstance(InstantiatorFn *instantiator, const void *context,
                      void *&duplicate,
                      UErrorCode &errorCode);
    




    void reset() { fInstance=NULL; }
};

#define STATIC_SIMPLE_SINGLETON(name) static SimpleSingleton name={ NULL }






template<typename T>
class SimpleSingletonWrapper {
public:
    SimpleSingletonWrapper(SimpleSingleton &s) : singleton(s) {}
    void deleteInstance() {
        delete (T *)singleton.fInstance;
        singleton.reset();
    }
    T *getInstance(InstantiatorFn *instantiator, const void *context,
                   UErrorCode &errorCode) {
        void *duplicate;
        T *instance=(T *)singleton.getInstance(instantiator, context, duplicate, errorCode);
        delete (T *)duplicate;
        return instance;
    }
private:
    SimpleSingleton &singleton;
};







struct TriStateSingleton {
    void *fInstance;
    UErrorCode fErrorCode;

    








    void *getInstance(InstantiatorFn *instantiator, const void *context,
                      void *&duplicate,
                      UErrorCode &errorCode);
    




    void reset();
};

#define STATIC_TRI_STATE_SINGLETON(name) static TriStateSingleton name={ NULL, U_ZERO_ERROR }






template<typename T>
class TriStateSingletonWrapper {
public:
    TriStateSingletonWrapper(TriStateSingleton &s) : singleton(s) {}
    void deleteInstance() {
        delete (T *)singleton.fInstance;
        singleton.reset();
    }
    T *getInstance(InstantiatorFn *instantiator, const void *context,
                   UErrorCode &errorCode) {
        void *duplicate;
        T *instance=(T *)singleton.getInstance(instantiator, context, duplicate, errorCode);
        delete (T *)duplicate;
        return instance;
    }
private:
    TriStateSingleton &singleton;
};

U_NAMESPACE_END

#endif 

