





































#ifndef __NSDOMWORKERMACROS_H__
#define __NSDOMWORKERMACROS_H__


#define NS_IMPL_THREADSAFE_DOM_CI_GETINTERFACES(_class)                       \
NS_IMETHODIMP                                                                 \
_class::GetInterfaces(PRUint32* _count, nsIID*** _array)                      \
{                                                                             \
  return NS_CI_INTERFACE_GETTER_NAME(_class)(_count, _array);                 \
}                                                                             \

#define NS_IMPL_THREADSAFE_DOM_CI_HELPER(_class)                              \
NS_IMETHODIMP                                                                 \
_class::GetHelperForLanguage(PRUint32 _language, nsISupports** _retval)       \
{                                                                             \
  *_retval = nsnull;                                                          \
  return NS_OK;                                                               \
}

#define NS_IMPL_THREADSAFE_DOM_CI_ALL_THE_REST(_class)                        \
NS_IMETHODIMP                                                                 \
_class::GetContractID(char** _contractID)                                     \
{                                                                             \
  *_contractID = nsnull;                                                      \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassDescription(char** _classDescription)                         \
{                                                                             \
  *_classDescription = nsnull;                                                \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassID(nsCID** _classID)                                          \
{                                                                             \
  *_classID = nsnull;                                                         \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetImplementationLanguage(PRUint32* _language)                        \
{                                                                             \
  *_language = nsIProgrammingLanguage::CPLUSPLUS;                             \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetFlags(PRUint32* _flags)                                            \
{                                                                             \
  *_flags = nsIClassInfo::THREADSAFE | nsIClassInfo::DOM_OBJECT;              \
  return NS_OK;                                                               \
}                                                                             \
                                                                              \
NS_IMETHODIMP                                                                 \
_class::GetClassIDNoAlloc(nsCID* _classIDNoAlloc)                             \
{                                                                             \
  return NS_ERROR_NOT_AVAILABLE;                                              \
}

#define NS_IMPL_THREADSAFE_DOM_CI(_class)                                     \
NS_IMPL_THREADSAFE_DOM_CI_GETINTERFACES(_class)                               \
NS_IMPL_THREADSAFE_DOM_CI_HELPER(_class)                                      \
NS_IMPL_THREADSAFE_DOM_CI_ALL_THE_REST(_class)

#define NS_FORWARD_NSICLASSINFO_NOGETINTERFACES(_to)                          \
  NS_IMETHOD GetHelperForLanguage(PRUint32 aLanguage, nsISupports** _retval)  \
    { return _to GetHelperForLanguage(aLanguage, _retval); }                  \
  NS_IMETHOD GetContractID(char** aContractID)                                \
    { return _to GetContractID(aContractID); }                                \
  NS_IMETHOD GetClassDescription(char** aClassDescription)                    \
    { return _to GetClassDescription(aClassDescription); }                    \
  NS_IMETHOD GetClassID(nsCID** aClassID)                                     \
    { return _to GetClassID(aClassID); }                                      \
  NS_IMETHOD GetImplementationLanguage(PRUint32* aImplementationLanguage)     \
    { return _to GetImplementationLanguage(aImplementationLanguage); }        \
  NS_IMETHOD GetFlags(PRUint32* aFlags)                                       \
    { return _to GetFlags(aFlags); }                                          \
  NS_IMETHOD GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)                        \
    { return _to GetClassIDNoAlloc(aClassIDNoAlloc); }

#define NS_DECL_NSICLASSINFO_GETINTERFACES                                    \
  NS_IMETHOD GetInterfaces(PRUint32* aCount, nsIID*** aArray);


#define NS_FORWARD_NSISUPPORTS(_to)                                           \
  NS_IMETHOD QueryInterface(const nsIID& uuid, void** result) {               \
    return _to QueryInterface(uuid, result);                                  \
  }                                                                           \
  NS_IMETHOD_(nsrefcnt) AddRef(void) { return _to AddRef(); }                 \
  NS_IMETHOD_(nsrefcnt) Release(void) { return _to Release(); }

#define JSON_PRIMITIVE_PROPNAME                                               \
  "primitive"

#endif 
