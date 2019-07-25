




#ifndef mozilla_dom_workers_dombindinginlines_h__
#define mozilla_dom_workers_dombindinginlines_h__

#include "mozilla/dom/XMLHttpRequestBinding.h"
#include "mozilla/dom/XMLHttpRequestUploadBinding.h"

BEGIN_WORKERS_NAMESPACE

class XMLHttpRequest;
class XMLHttpRequestUpload;

namespace {

template <class T>
struct WrapPrototypeTraits
{ };



#define SPECIALIZE_PROTO_TRAITS(_class)                                        \
  template <>                                                                  \
  struct WrapPrototypeTraits<_class>                                           \
  {                                                                            \
    static inline JSClass*                                                     \
    GetJSClass()                                                               \
    {                                                                          \
      using namespace mozilla::dom;                                            \
      return _class##Binding_workers::Class.ToJSClass();                       \
    }                                                                          \
                                                                               \
    static inline JSObject*                                                    \
    GetProtoObject(JSContext* aCx, JSObject* aGlobal)                          \
    {                                                                          \
      using namespace mozilla::dom;                                            \
      return _class##Binding_workers::GetProtoObject(aCx, aGlobal, aGlobal);   \
    }                                                                          \
  };

SPECIALIZE_PROTO_TRAITS(XMLHttpRequest)
SPECIALIZE_PROTO_TRAITS(XMLHttpRequestUpload)

#undef SPECIALIZE_PROTO_TRAITS

} 

template <class T>
inline JSObject*
Wrap(JSContext* aCx, JSObject* aGlobal, nsRefPtr<T>& aConcreteObject)
{
  MOZ_ASSERT(aCx);

  if (!aGlobal) {
    aGlobal = JS_GetGlobalForScopeChain(aCx);
    if (!aGlobal) {
      return NULL;
    }
  }

  JSObject* proto = WrapPrototypeTraits<T>::GetProtoObject(aCx, aGlobal);
  if (!proto) {
    return NULL;
  }

  JSObject* wrapper =
    JS_NewObject(aCx, WrapPrototypeTraits<T>::GetJSClass(), proto, aGlobal);
  if (!wrapper) {
    return NULL;
  }

  js::SetReservedSlot(wrapper, DOM_OBJECT_SLOT,
                      PRIVATE_TO_JSVAL(aConcreteObject));

  aConcreteObject->SetIsDOMBinding();
  aConcreteObject->SetWrapper(wrapper);

  NS_ADDREF(aConcreteObject.get());
  return wrapper;
}

END_WORKERS_NAMESPACE

#endif 
