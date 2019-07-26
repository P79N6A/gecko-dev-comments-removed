





#ifndef mozilla_dom_WindowNamedPropertiesHandler_h
#define mozilla_dom_WindowNamedPropertiesHandler_h

#include "mozilla/dom/DOMJSProxyHandler.h"

namespace mozilla {
namespace dom {

class WindowNamedPropertiesHandler : public BaseDOMProxyHandler
{
public:
  WindowNamedPropertiesHandler() : BaseDOMProxyHandler(nullptr)
  {
    setHasPrototype(true);
  }
  virtual bool
  preventExtensions(JSContext* aCx, JS::Handle<JSObject*> aProxy) MOZ_OVERRIDE
  {
    
    JS_ReportErrorNumber(aCx, js_GetErrorMessage, nullptr,
                         JSMSG_CANT_CHANGE_EXTENSIBILITY);
    return false;
  }
  virtual bool
  getOwnPropertyDescriptor(JSContext* aCx, JS::Handle<JSObject*> aProxy,
                           JS::Handle<jsid> aId,
                           JS::MutableHandle<JSPropertyDescriptor> aDesc) MOZ_OVERRIDE;
  virtual bool
  defineProperty(JSContext* aCx, JS::Handle<JSObject*> aProxy,
                 JS::Handle<jsid> aId,
                 JS::MutableHandle<JSPropertyDescriptor> aDesc) MOZ_OVERRIDE;
  virtual bool
  ownPropNames(JSContext* aCx, JS::Handle<JSObject*> aProxy, unsigned flags,
               JS::AutoIdVector& aProps) MOZ_OVERRIDE;
  virtual bool
  delete_(JSContext* aCx, JS::Handle<JSObject*> aProxy, JS::Handle<jsid> aId,
          bool* aBp) MOZ_OVERRIDE;
  virtual bool
  isExtensible(JSContext* aCx, JS::Handle<JSObject*> aProxy,
               bool* aIsExtensible) MOZ_OVERRIDE
  {
    *aIsExtensible = true;
    return true;
  }
  virtual const char*
  className(JSContext *aCx, JS::Handle<JSObject*> aProxy) MOZ_OVERRIDE
  {
    return "WindowProperties";
  }

  static WindowNamedPropertiesHandler*
  getInstance()
  {
    static WindowNamedPropertiesHandler instance;
    return &instance;
  }

  
  static void
  Install(JSContext *aCx, JS::Handle<JSObject*> aProto);
};

} 
} 

#endif 
