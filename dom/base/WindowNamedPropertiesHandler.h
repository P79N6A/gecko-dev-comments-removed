





#ifndef mozilla_dom_WindowNamedPropertiesHandler_h
#define mozilla_dom_WindowNamedPropertiesHandler_h

#include "mozilla/dom/DOMJSProxyHandler.h"

namespace mozilla {
namespace dom {

class WindowNamedPropertiesHandler : public BaseDOMProxyHandler
{
public:
  MOZ_CONSTEXPR WindowNamedPropertiesHandler()
    : BaseDOMProxyHandler(nullptr,  true)
  {
  }
  virtual bool
  getOwnPropDescriptor(JSContext* aCx, JS::Handle<JSObject*> aProxy,
                       JS::Handle<jsid> aId,
                       bool ,
                       JS::MutableHandle<JSPropertyDescriptor> aDesc)
                       const override;
  virtual bool
  defineProperty(JSContext* aCx, JS::Handle<JSObject*> aProxy,
                 JS::Handle<jsid> aId,
                 JS::Handle<JSPropertyDescriptor> aDesc,
                 JS::ObjectOpResult &result) const override;
  virtual bool
  ownPropNames(JSContext* aCx, JS::Handle<JSObject*> aProxy, unsigned flags,
               JS::AutoIdVector& aProps) const override;
  virtual bool
  delete_(JSContext* aCx, JS::Handle<JSObject*> aProxy, JS::Handle<jsid> aId,
          JS::ObjectOpResult &aResult) const override;
  virtual bool
  preventExtensions(JSContext* aCx, JS::Handle<JSObject*> aProxy,
                    JS::ObjectOpResult& aResult) const override
  {
    return aResult.failCantPreventExtensions();
  }
  virtual bool
  isExtensible(JSContext* aCx, JS::Handle<JSObject*> aProxy,
               bool* aIsExtensible) const override
  {
    *aIsExtensible = true;
    return true;
  }
  virtual const char*
  className(JSContext *aCx, JS::Handle<JSObject*> aProxy) const override
  {
    return "WindowProperties";
  }

  static const WindowNamedPropertiesHandler*
  getInstance()
  {
    static const WindowNamedPropertiesHandler instance;
    return &instance;
  }

  
  
  static JSObject*
  Create(JSContext *aCx, JS::Handle<JSObject*> aProto);
};

} 
} 

#endif 
