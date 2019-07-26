





#ifndef mozilla_dom_ToJSValue_h
#define mozilla_dom_ToJSValue_h

#include "mozilla/TypeTraits.h"
#include "mozilla/Assertions.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/TypedArray.h"
#include "jsapi.h"
#include "nsISupports.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {





bool
ToJSValue(JSContext* aCx,
          const nsAString& aArgument,
          JS::MutableHandle<JS::Value> aValue);


inline bool
ToJSValue(JSContext* aCx,
          bool aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  aValue.setBoolean(aArgument);
  return true;
}


inline bool
ToJSValue(JSContext* aCx,
          int32_t aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  aValue.setInt32(aArgument);
  return true;
}

inline bool
ToJSValue(JSContext* aCx,
          uint32_t aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  aValue.setNumber(aArgument);
  return true;
}

inline bool
ToJSValue(JSContext* aCx,
          int64_t aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  aValue.setNumber(double(aArgument));
  return true;
}

inline bool
ToJSValue(JSContext* aCx,
          uint64_t aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  aValue.setNumber(double(aArgument));
  return true;
}



template <class T>
typename EnableIf<IsBaseOf<nsWrapperCache, T>::value &&
                  IsBaseOf<nsISupports, T>::value, bool>::Type
ToJSValue(JSContext* aCx,
          T& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));
  
  MOZ_ASSERT(aArgument.IsDOMBinding());

  return WrapNewBindingObject(aCx, aArgument, aValue);
}


template<typename T>
typename EnableIf<IsBaseOf<AllTypedArraysBase, T>::value, bool>::Type
ToJSValue(JSContext* aCx,
          const TypedArrayCreator<T>& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  JSObject* obj = aArgument.Create(aCx);
  if (!obj) {
    return false;
  }
  aValue.setObject(*obj);
  return true;
}



namespace tojsvalue_detail {
bool
ISupportsToJSValue(JSContext* aCx,
                   nsISupports* aArgument,
                   JS::MutableHandle<JS::Value> aValue);
} 



template <class T>
typename EnableIf<!IsBaseOf<nsWrapperCache, T>::value &&
                  IsBaseOf<nsISupports, T>::value, bool>::Type
ToJSValue(JSContext* aCx,
          T& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  return tojsvalue_detail::ISupportsToJSValue(aCx, &aArgument, aValue);
}


template <template <typename> class SmartPtr, typename T>
bool
ToJSValue(JSContext* aCx,
          const SmartPtr<T>& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  return ToJSValue(aCx, *aArgument.get(), aValue);
}


template <typename T>
bool
ToJSValue(JSContext* aCx,
          const nsTArray<T>& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  JS::AutoValueVector v(aCx);
  if (!v.resize(aArgument.Length())) {
    return false;
  }
  for (uint32_t i = 0; i < aArgument.Length(); ++i) {
    if (!ToJSValue(aCx, aArgument[i], v.handleAt(i))) {
      return false;
    }
  }
  JSObject* arrayObj = JS_NewArrayObject(aCx, v);
  if (!arrayObj) {
    return false;
  }
  aValue.setObject(*arrayObj);
  return true;
}

} 
} 

#endif 
