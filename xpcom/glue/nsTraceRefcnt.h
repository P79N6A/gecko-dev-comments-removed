



#ifndef nsTraceRefcnt_h___
#define nsTraceRefcnt_h___

#include "nsXPCOM.h"

#ifdef NS_BUILD_REFCNT_LOGGING

#define NS_LOG_ADDREF(_p, _rc, _type, _size) \
  NS_LogAddRef((_p), (_rc), (_type), (uint32_t) (_size))

#define NS_LOG_RELEASE(_p, _rc, _type) \
  NS_LogRelease((_p), (_rc), (_type))

#define MOZ_COUNT_CTOR(_type)                                 \
do {                                                          \
  NS_LogCtor((void*)this, #_type, sizeof(*this));             \
} while (0)

#define MOZ_COUNT_CTOR_INHERITED(_type, _base)                    \
do {                                                              \
  NS_LogCtor((void*)this, #_type, sizeof(*this) - sizeof(_base)); \
} while (0)

#define MOZ_COUNT_DTOR(_type)                                 \
do {                                                          \
  NS_LogDtor((void*)this, #_type, sizeof(*this));             \
} while (0)

#define MOZ_COUNT_DTOR_INHERITED(_type, _base)                    \
do {                                                              \
  NS_LogDtor((void*)this, #_type, sizeof(*this) - sizeof(_base)); \
} while (0)




#define NSCAP_LOG_ASSIGNMENT(_c, _p)                                \
  if (_p)                                                           \
    NS_LogCOMPtrAddRef((_c),static_cast<nsISupports*>(_p))

#define NSCAP_LOG_RELEASE(_c, _p)                                   \
  if (_p)                                                           \
    NS_LogCOMPtrRelease((_c), static_cast<nsISupports*>(_p))

#else 

#define NS_LOG_ADDREF(_p, _rc, _type, _size)
#define NS_LOG_RELEASE(_p, _rc, _type)
#define MOZ_COUNT_CTOR(_type)
#define MOZ_COUNT_CTOR_INHERITED(_type, _base)
#define MOZ_COUNT_DTOR(_type)
#define MOZ_COUNT_DTOR_INHERITED(_type, _base)

#endif 

#ifdef __cplusplus

class nsTraceRefcnt {
public:
  inline static void LogAddRef(void* aPtr, nsrefcnt aNewRefCnt,
                               const char* aTypeName, uint32_t aInstanceSize) {
    NS_LogAddRef(aPtr, aNewRefCnt, aTypeName, aInstanceSize);
  }

  inline static void LogRelease(void* aPtr, nsrefcnt aNewRefCnt,
                                const char* aTypeName) {
    NS_LogRelease(aPtr, aNewRefCnt, aTypeName);
  }

  inline static void LogCtor(void* aPtr, const char* aTypeName,
                             uint32_t aInstanceSize) {
    NS_LogCtor(aPtr, aTypeName, aInstanceSize);
  }

  inline static void LogDtor(void* aPtr, const char* aTypeName,
                             uint32_t aInstanceSize) {
    NS_LogDtor(aPtr, aTypeName, aInstanceSize);
  }

  inline static void LogAddCOMPtr(void *aCOMPtr, nsISupports *aObject) {
    NS_LogCOMPtrAddRef(aCOMPtr, aObject);
  }

  inline static void LogReleaseCOMPtr(void *aCOMPtr, nsISupports *aObject) {
    NS_LogCOMPtrRelease(aCOMPtr, aObject);
  }
};

#endif 

#endif 
