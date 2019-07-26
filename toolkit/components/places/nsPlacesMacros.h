




#include "prtypes.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"

#ifndef __FUNCTION__
#define __FUNCTION__ __func__
#endif



#define NOTIFY_OBSERVERS(canFire, cache, array, type, method)                  \
  PR_BEGIN_MACRO                                                               \
  if (canFire) {                                                               \
    const nsCOMArray<type> &entries = cache.GetEntries();                      \
    for (int32_t idx = 0; idx < entries.Count(); ++idx)                        \
        entries[idx]->method;                                                  \
    ENUMERATE_WEAKARRAY(array, type, method)                                   \
  }                                                                            \
  PR_END_MACRO;

#define PLACES_FACTORY_SINGLETON_IMPLEMENTATION(_className, _sInstance)        \
  _className * _className::_sInstance = nullptr;                                \
                                                                               \
  _className *                                                                 \
  _className::GetSingleton()                                                   \
  {                                                                            \
    if (_sInstance) {                                                          \
      NS_ADDREF(_sInstance);                                                   \
      return _sInstance;                                                       \
    }                                                                          \
    _sInstance = new _className();                                             \
    if (_sInstance) {                                                          \
      NS_ADDREF(_sInstance);                                                   \
      if (NS_FAILED(_sInstance->Init())) {                                     \
        NS_RELEASE(_sInstance);                                                \
        _sInstance = nullptr;                                                   \
      }                                                                        \
    }                                                                          \
    return _sInstance;                                                         \
  }

#define PLACES_WARN_DEPRECATED()                                               \
  PR_BEGIN_MACRO                                                               \
  nsCString msg = NS_LITERAL_CSTRING(__FUNCTION__);                            \
  msg.AppendLiteral(" is deprecated and will be removed in the next version.");\
  NS_WARNING(msg.get());                                                       \
  nsCOMPtr<nsIConsoleService> cs = do_GetService(NS_CONSOLESERVICE_CONTRACTID);\
  if (cs) {                                                                    \
    nsCOMPtr<nsIScriptError> e = do_CreateInstance(NS_SCRIPTERROR_CONTRACTID); \
    if (e && NS_SUCCEEDED(e->Init(NS_ConvertUTF8toUTF16(msg), EmptyString(),   \
                                  EmptyString(), 0, 0,                         \
                                  nsIScriptError::errorFlag, "Places"))) {     \
      cs->LogMessage(e);                                                       \
    }                                                                          \
  }                                                                            \
  PR_END_MACRO
