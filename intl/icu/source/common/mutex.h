


















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

U_NAMESPACE_END

#endif 

