





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

    virtual bool defineProperty(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                JS::Handle<jsid> id,
                                JS::MutableHandle<JSPropertyDescriptor> desc,
                                JS::ObjectOpResult &result) const MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, JS::Handle<JSObject*> wrapper,
                     JS::Handle<JSObject*> receiver, JS::Handle<jsid> id,
                     bool strict, JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;

    static const ChromeObjectWrapper singleton;
};

} 

#endif 
