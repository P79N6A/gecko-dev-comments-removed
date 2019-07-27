





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

    virtual bool defineProperty(JSContext* cx, JS::Handle<JSObject*> wrapper,
                                JS::Handle<jsid> id,
                                JS::Handle<JSPropertyDescriptor> desc,
                                JS::ObjectOpResult& result) const override;
    virtual bool set(JSContext* cx, JS::HandleObject wrapper, JS::HandleId id,
                     JS::HandleValue v, JS::HandleValue receiver,
                     JS::ObjectOpResult& result) const override;

    static const ChromeObjectWrapper singleton;
};

} 

#endif 
