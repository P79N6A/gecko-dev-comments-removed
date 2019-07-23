




































#ifndef nsTraceRefcnt_h___
#define nsTraceRefcnt_h___

#include "nsXPCOM.h"


#undef NS_BUILD_REFCNT_LOGGING

#if (defined(DEBUG) || defined(FORCE_BUILD_REFCNT_LOGGING))



#define NS_BUILD_REFCNT_LOGGING 1
#endif



#if defined(NO_BUILD_REFCNT_LOGGING)
#undef NS_BUILD_REFCNT_LOGGING
#endif

#ifdef NS_BUILD_REFCNT_LOGGING

#define NS_LOG_ADDREF(_p, _rc, _type, _size) \
  NS_LogAddRef((_p), (_rc), (_type), (PRUint32) (_size))

#define NS_LOG_RELEASE(_p, _rc, _type) \
  NS_LogRelease((_p), (_rc), (_type))

#define MOZ_DECL_CTOR_COUNTER(_type)

#define MOZ_COUNT_CTOR(_type)                                 \
PR_BEGIN_MACRO                                                \
  NS_LogCtor((void*)this, #_type, sizeof(*this));             \
PR_END_MACRO

#define MOZ_COUNT_DTOR(_type)                                 \
PR_BEGIN_MACRO                                                \
  NS_LogDtor((void*)this, #_type, sizeof(*this));             \
PR_END_MACRO




#define NSCAP_LOG_ASSIGNMENT(_c, _p)                                \
  if (_p)                                                           \
    NS_LogCOMPtrAddRef((_c),static_cast<nsISupports*>(_p))

#define NSCAP_LOG_RELEASE(_c, _p)                                   \
  if (_p)                                                           \
    NS_LogCOMPtrRelease((_c), static_cast<nsISupports*>(_p))

#else 

#define NS_LOG_ADDREF(_p, _rc, _type, _size)
#define NS_LOG_RELEASE(_p, _rc, _type)
#define MOZ_DECL_CTOR_COUNTER(_type)
#define MOZ_COUNT_CTOR(_type)
#define MOZ_COUNT_DTOR(_type)

#endif 

#ifdef __cplusplus

class nsTraceRefcnt {
public:
  inline static void LogAddRef(void* aPtr, nsrefcnt aNewRefCnt,
                               const char* aTypeName, PRUint32 aInstanceSize) {
    NS_LogAddRef(aPtr, aNewRefCnt, aTypeName, aInstanceSize);
  }

  inline static void LogRelease(void* aPtr, nsrefcnt aNewRefCnt,
                                const char* aTypeName) {
    NS_LogRelease(aPtr, aNewRefCnt, aTypeName);
  }

  inline static void LogCtor(void* aPtr, const char* aTypeName,
                             PRUint32 aInstanceSize) {
    NS_LogCtor(aPtr, aTypeName, aInstanceSize);
  }

  inline static void LogDtor(void* aPtr, const char* aTypeName,
                             PRUint32 aInstanceSize) {
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
