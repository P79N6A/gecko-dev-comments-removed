







































#ifndef nsISupportsUtils_h__
#define nsISupportsUtils_h__

#ifndef nscore_h___
#include "nscore.h"
#endif

#ifndef nsISupportsBase_h__
#include "nsISupportsBase.h"
#endif

#ifndef nsError_h__
#include "nsError.h"
#endif

#ifndef nsDebug_h___
#include "nsDebug.h"
#endif

#ifndef nsISupportsImpl_h__
#include "nsISupportsImpl.h"
#endif








#define NS_NEWXPCOM(_result,_type)                                            \
  PR_BEGIN_MACRO                                                              \
    _result = new _type();                                                    \
  PR_END_MACRO





#define NS_DELETEXPCOM(_ptr)                                                  \
  PR_BEGIN_MACRO                                                              \
    delete (_ptr);                                                            \
  PR_END_MACRO





#define NS_ADDREF(_ptr) \
  (_ptr)->AddRef()







#define NS_ADDREF_THIS() \
  AddRef()


extern "C++" {






template <class T>
inline
nsrefcnt
ns_if_addref( T expr )
{
    return expr ? expr->AddRef() : 0;
}

} 





#define NS_IF_ADDREF(_expr) ns_if_addref(_expr)














#define NS_RELEASE(_ptr)                                                      \
  PR_BEGIN_MACRO                                                              \
    (_ptr)->Release();                                                        \
    (_ptr) = 0;                                                               \
  PR_END_MACRO





#define NS_RELEASE_THIS() \
    Release()









#define NS_RELEASE2(_ptr,_rv)                                                 \
  PR_BEGIN_MACRO                                                              \
    _rv = (_ptr)->Release();                                                  \
    if (0 == (_rv)) (_ptr) = 0;                                               \
  PR_END_MACRO





#define NS_IF_RELEASE(_ptr)                                                   \
  PR_BEGIN_MACRO                                                              \
    if (_ptr) {                                                               \
      (_ptr)->Release();                                                      \
      (_ptr) = 0;                                                             \
    }                                                                         \
  PR_END_MACRO














#define NS_ISUPPORTS_CAST(__unambiguousBase, __expr) \
  static_cast<nsISupports*>(static_cast<__unambiguousBase>(__expr))


template <class T, class DestinationType>
inline
nsresult
CallQueryInterface( T* aSource, DestinationType** aDestination )
{
    NS_PRECONDITION(aSource, "null parameter");
    NS_PRECONDITION(aDestination, "null parameter");
    
    return aSource->QueryInterface(NS_GET_TEMPLATE_IID(DestinationType),
                                   reinterpret_cast<void**>(aDestination));
}

#endif 
