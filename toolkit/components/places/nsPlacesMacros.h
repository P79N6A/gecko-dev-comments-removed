




#include "prtypes.h"



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
