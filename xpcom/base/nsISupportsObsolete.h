



































#ifndef nsISupportsObsolete_h__
#define nsISupportsObsolete_h__

#include "prcmon.h"




#define NS_INIT_REFCNT() NS_INIT_ISUPPORTS()
















#define NS_FREE_XPCOM_ISUPPORTS_POINTER_ARRAY(size, array)                    \
    NS_FREE_XPCOM_POINTER_ARRAY((size), (array), NS_IF_RELEASE)








#define NS_METHOD_GETTER(_method, _type, _member) \
_method(_type* aResult) \
{\
    if (!aResult) return NS_ERROR_NULL_POINTER; \
    *aResult = _member; \
    return NS_OK; \
}
    
#define NS_METHOD_SETTER(_method, _type, _member) \
_method(_type aResult) \
{ \
    _member = aResult; \
    return NS_OK; \
}





#define NS_METHOD_GETTER_STR(_method,_member)   \
_method(char* *aString)                         \
{                                               \
    if (!aString) return NS_ERROR_NULL_POINTER; \
    if (!(*aString = PL_strdup(_member)))       \
      return NS_ERROR_OUT_OF_MEMORY;            \
    return NS_OK;                               \
}

#define NS_METHOD_SETTER_STR(_method, _member) \
_method(const char *aString)                   \
{                                              \
    if (_member) PR_Free(_member);             \
    if (!aString)                              \
      _member = nsnull;                        \
    else if (!(_member = PL_strdup(aString)))  \
      return NS_ERROR_OUT_OF_MEMORY;           \
    return NS_OK;                              \
}







































   





#define NS_IMPL_CLASS_GETTER(_method, _type, _member) \
NS_IMETHOD NS_METHOD_GETTER(_method, _type, _member)

#define NS_IMPL_CLASS_SETTER(_method, _type, _member) \
NS_IMETHOD NS_METHOD_SETTER(_method, _type, _member)

#define NS_IMPL_CLASS_GETSET(_postfix, _type, _member) \
NS_IMPL_CLASS_GETTER(Get##_postfix, _type, _member) \
NS_IMPL_CLASS_SETTER(Set##_postfix, _type, _member)


#define NS_IMPL_CLASS_GETTER_STR(_method, _member) \
NS_IMETHOD NS_METHOD_GETTER_STR(_method, _member)

#define NS_IMPL_CLASS_SETTER_STR(_method, _member) \
NS_IMETHOD NS_METHOD_SETTER_STR(_method, _member)

#define NS_IMPL_CLASS_GETSET_STR(_postfix, _member) \
NS_IMPL_CLASS_GETTER_STR(Get##_postfix, _member) \
NS_IMPL_CLASS_SETTER_STR(Set##_postfix, _member)




#define NS_IMPL_GETTER(_method, _type, _member) \
NS_IMETHODIMP NS_METHOD_GETTER(_method, _type, _member)

#define NS_IMPL_SETTER(_method, _type, _member) \
NS_IMETHODIMP NS_METHOD_SETTER(_method, _type, _member)

#define NS_IMPL_GETSET(_class, _postfix, _type, _member) \
NS_IMPL_GETTER(_class::Get##_postfix, _type, _member) \
NS_IMPL_SETTER(_class::Set##_postfix, _type, _member)


#define NS_IMPL_GETTER_STR(_method, _member) \
NS_IMETHODIMP NS_METHOD_GETTER_STR(_method, _member)

#define NS_IMPL_SETTER_STR(_method, _member) \
NS_IMETHODIMP NS_METHOD_SETTER_STR(_method, _member)

#define NS_IMPL_GETSET_STR(_class, _postfix, _member) \
NS_IMPL_GETTER_STR(_class::Get##_postfix, _member) \
NS_IMPL_SETTER_STR(_class::Set##_postfix, _member)








#define NS_ISTHREADSAFE_IID                                                   \
  { 0x88210890, 0x47a6, 0x11d2,                                               \
    {0xbe, 0xc3, 0x00, 0x80, 0x5f, 0x8a, 0x66, 0xdc} }

#define NS_LOCK_INSTANCE()                                                    \
  PR_CEnterMonitor((void*)this)
#define NS_UNLOCK_INSTANCE()                                                  \
  PR_CExitMonitor((void*)this)











#if defined(NS_DEBUG)
#define NS_VERIFY_THREADSAFE_INTERFACE(_iface)                                \
 if (NULL != (_iface)) {                                                      \
   nsISupports* tmp;                                                          \
   static NS_DEFINE_IID(kIsThreadsafeIID, NS_ISTHREADSAFE_IID);               \
   NS_PRECONDITION((NS_OK == _iface->QueryInterface(kIsThreadsafeIID,         \
                                                    (void**)&tmp)),           \
                   "Interface is not threadsafe");                            \
 }
#else
#define NS_VERIFY_THREADSAFE_INTERFACE(_iface)
#endif





#endif
