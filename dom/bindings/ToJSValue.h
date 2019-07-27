





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





MOZ_WARN_UNUSED_RESULT bool
ToJSValue(JSContext* aCx,
          const nsAString& aArgument,
          JS::MutableHandle<JS::Value> aValue);






template<typename T>
MOZ_WARN_UNUSED_RESULT
typename EnableIf<IsSame<T, bool>::value, bool>::Type
ToJSValue(JSContext* aCx,
          T aArgument,
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


inline bool
ToJSValue(JSContext* aCx,
          float aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  aValue.setNumber(aArgument);
  return true;
}

inline bool
ToJSValue(JSContext* aCx,
          double aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  aValue.setNumber(aArgument);
  return true;
}


MOZ_WARN_UNUSED_RESULT inline bool
ToJSValue(JSContext* aCx,
          CallbackObject& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  aValue.setObject(*aArgument.Callback());

  return MaybeWrapValue(aCx, aValue);
}



template <class T>
MOZ_WARN_UNUSED_RESULT
typename EnableIf<IsBaseOf<nsWrapperCache, T>::value, bool>::Type
ToJSValue(JSContext* aCx,
          T& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));
  
  MOZ_ASSERT(aArgument.IsDOMBinding());

  return GetOrCreateDOMReflector(aCx, aArgument, aValue);
}


template<typename T>
MOZ_WARN_UNUSED_RESULT
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



template <class T>
MOZ_WARN_UNUSED_RESULT
typename EnableIf<!IsBaseOf<nsWrapperCache, T>::value &&
                  !IsBaseOf<CallbackObject, T>::value &&
                  IsBaseOf<nsISupports, T>::value, bool>::Type
ToJSValue(JSContext* aCx,
          T& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  qsObjectHelper helper(ToSupports(&aArgument), nullptr);
  JS::Rooted<JSObject*> scope(aCx, JS::CurrentGlobalOrNull(aCx));
  return XPCOMObjectToJsval(aCx, scope, helper, nullptr, true, aValue);
}


template <typename T>
MOZ_WARN_UNUSED_RESULT bool
ToJSValue(JSContext* aCx,
          const nsCOMPtr<T>& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  return ToJSValue(aCx, *aArgument.get(), aValue);
}

template <typename T>
MOZ_WARN_UNUSED_RESULT bool
ToJSValue(JSContext* aCx,
          const nsRefPtr<T>& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  return ToJSValue(aCx, *aArgument.get(), aValue);
}


template <class T>
MOZ_WARN_UNUSED_RESULT
typename EnableIf<IsBaseOf<DictionaryBase, T>::value, bool>::Type
ToJSValue(JSContext* aCx,
          const T& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  return aArgument.ToObjectInternal(aCx, aValue);
}


MOZ_WARN_UNUSED_RESULT inline bool
ToJSValue(JSContext* aCx, JS::Handle<JS::Value> aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  aValue.set(aArgument);
  return MaybeWrapValue(aCx, aValue);
}


MOZ_WARN_UNUSED_RESULT inline bool
ToJSValue(JSContext* aCx, const JS::Heap<JS::Value>& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  aValue.set(aArgument);
  return MaybeWrapValue(aCx, aValue);
}


MOZ_WARN_UNUSED_RESULT inline bool
ToJSValue(JSContext* aCx, const JS::Rooted<JS::Value>& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  aValue.set(aArgument);
  return MaybeWrapValue(aCx, aValue);
}



MOZ_WARN_UNUSED_RESULT inline bool
ToJSValue(JSContext* aCx, const JS::Rooted<JSObject*>& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  aValue.setObjectOrNull(aArgument);
  return MaybeWrapObjectOrNullValue(aCx, aValue);
}



MOZ_WARN_UNUSED_RESULT bool
ToJSValue(JSContext* aCx,
          nsresult aArgument,
          JS::MutableHandle<JS::Value> aValue);




MOZ_WARN_UNUSED_RESULT bool
ToJSValue(JSContext* aCx,
          ErrorResult& aArgument,
          JS::MutableHandle<JS::Value> aValue);


template <typename T>
MOZ_WARN_UNUSED_RESULT
typename EnableIf<IsPointer<T>::value, bool>::Type
ToJSValue(JSContext* aCx,
          T aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  return ToJSValue(aCx, *aArgument, aValue);
}


template <typename T>
MOZ_WARN_UNUSED_RESULT bool
ToJSValue(JSContext* aCx,
          T* aArguments,
          size_t aLength,
          JS::MutableHandle<JS::Value> aValue)
{
  
  MOZ_ASSERT(JS::CurrentGlobalOrNull(aCx));

  JS::AutoValueVector v(aCx);
  if (!v.resize(aLength)) {
    return false;
  }
  for (size_t i = 0; i < aLength; ++i) {
    if (!ToJSValue(aCx, aArguments[i], v[i])) {
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

template <typename T>
MOZ_WARN_UNUSED_RESULT bool
ToJSValue(JSContext* aCx,
          const nsTArray<T>& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  return ToJSValue(aCx, aArgument.Elements(),
                   aArgument.Length(), aValue);
}

template <typename T>
MOZ_WARN_UNUSED_RESULT bool
ToJSValue(JSContext* aCx,
          const FallibleTArray<T>& aArgument,
          JS::MutableHandle<JS::Value> aValue)
{
  return ToJSValue(aCx, aArgument.Elements(),
                   aArgument.Length(), aValue);
}

template <typename T, int N>
MOZ_WARN_UNUSED_RESULT bool
ToJSValue(JSContext* aCx,
          const T(&aArgument)[N],
          JS::MutableHandle<JS::Value> aValue)
{
  return ToJSValue(aCx, aArgument, N, aValue);
}

} 
} 

#endif 
