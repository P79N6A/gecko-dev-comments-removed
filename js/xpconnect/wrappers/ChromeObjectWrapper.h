






#ifndef __ChromeObjectWrapper_h__
#define __ChromeObjectWrapper_h__

#include "mozilla/Attributes.h"

#include "FilteringWrapper.h"
#include "AccessCheck.h"

namespace xpc {






#define ChromeObjectWrapperBase \
  FilteringWrapper<js::CrossCompartmentSecurityWrapper, ExposedPropertiesOnly>

class ChromeObjectWrapper : public ChromeObjectWrapperBase
{
  public:
    ChromeObjectWrapper() : ChromeObjectWrapperBase(0) {}

    
    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id, js::PropertyDescriptor *desc,
                                       unsigned flags) MOZ_OVERRIDE;
    virtual bool has(JSContext *cx, JS::Handle<JSObject*> wrapper,
                     JS::Handle<jsid> id, bool *bp) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<JSObject*> receiver,
                     JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp) MOZ_OVERRIDE;

    virtual bool objectClassIs(JS::Handle<JSObject*> obj, js::ESClassValue classValue,
                               JSContext *cx) MOZ_OVERRIDE;

    virtual bool enter(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<jsid> id,
                       js::Wrapper::Action act, bool *bp) MOZ_OVERRIDE;

    
    
    
    
    
    
    

    static ChromeObjectWrapper singleton;
};

} 

#endif 
