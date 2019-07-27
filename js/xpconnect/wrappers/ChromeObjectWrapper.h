





#ifndef __ChromeObjectWrapper_h__
#define __ChromeObjectWrapper_h__

#include "mozilla/Attributes.h"

#include "FilteringWrapper.h"

namespace xpc {

struct ExposedPropertiesOnly;






#define ChromeObjectWrapperBase \
  FilteringWrapper<js::CrossCompartmentSecurityWrapper, ExposedPropertiesOnly>

class ChromeObjectWrapper : public ChromeObjectWrapperBase
{
  public:
    MOZ_CONSTEXPR ChromeObjectWrapper() : ChromeObjectWrapperBase(0) {}

    
    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                JS::Handle<jsid> id,
                                JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, JS::Handle<JSObject*> wrapper,
                     JS::Handle<JSObject*> receiver, JS::Handle<jsid> id,
                     bool strict, JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;

    virtual bool has(JSContext *cx, JS::Handle<JSObject*> wrapper,
                     JS::Handle<jsid> id, bool *bp) const MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<JSObject*> receiver,
                     JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;

    virtual bool call(JSContext *cx, JS::Handle<JSObject*> wrapper,
                      const JS::CallArgs &args) const MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JS::Handle<JSObject*> wrapper,
                           const JS::CallArgs &args) const MOZ_OVERRIDE;

    virtual bool objectClassIs(JS::Handle<JSObject*> obj, js::ESClassValue classValue,
                               JSContext *cx) const MOZ_OVERRIDE;

    virtual bool enter(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<jsid> id,
                       js::Wrapper::Action act, bool *bp) const MOZ_OVERRIDE;

    
    
    
    
    
    
    

    static const ChromeObjectWrapper singleton;
};

} 

#endif 
