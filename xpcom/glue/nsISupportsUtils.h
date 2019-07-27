




#ifndef nsISupportsUtils_h__
#define nsISupportsUtils_h__

#include "nscore.h"
#include "nsISupportsBase.h"
#include "nsError.h"
#include "nsDebug.h"
#include "nsISupportsImpl.h"
#include "mozilla/TypeTraits.h"





#define NS_ADDREF(_ptr) \
  (_ptr)->AddRef()







#define NS_ADDREF_THIS() \
  AddRef()


extern "C++" {






template<class T>
inline void
ns_if_addref(T aExpr)
{
  if (aExpr) {
    aExpr->AddRef();
  }
}

} 





#define NS_IF_ADDREF(_expr) ns_if_addref(_expr)














#define NS_RELEASE(_ptr)                                                      \
  do {                                                                        \
    (_ptr)->Release();                                                        \
    (_ptr) = 0;                                                               \
  } while (0)




#define NS_RELEASE_THIS() \
    Release()










#define NS_RELEASE2(_ptr, _rc)                                                \
  do {                                                                        \
    _rc = (_ptr)->Release();                                                  \
    if (0 == (_rc)) (_ptr) = 0;                                               \
  } while (0)





#define NS_IF_RELEASE(_ptr)                                                   \
  do {                                                                        \
    if (_ptr) {                                                               \
      (_ptr)->Release();                                                      \
      (_ptr) = 0;                                                             \
    }                                                                         \
  } while (0)














#define NS_ISUPPORTS_CAST(__unambiguousBase, __expr) \
  static_cast<nsISupports*>(static_cast<__unambiguousBase>(__expr))


template<class T, class DestinationType>
inline nsresult
CallQueryInterface(T* aSource, DestinationType** aDestination)
{
  
  
  static_assert(!mozilla::IsSame<T, DestinationType>::value ||
                mozilla::IsSame<DestinationType, nsISupports>::value,
                "don't use CallQueryInterface for compile-time-determinable casts");

  NS_PRECONDITION(aSource, "null parameter");
  NS_PRECONDITION(aDestination, "null parameter");

  return aSource->QueryInterface(NS_GET_TEMPLATE_IID(DestinationType),
                                 reinterpret_cast<void**>(aDestination));
}

#endif 
