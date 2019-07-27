





#ifndef __CrossOriginWrapper_h__
#define __CrossOriginWrapper_h__

#include "mozilla/Attributes.h"

#include "jswrapper.h"

namespace xpc {

class WaiveXrayWrapper : public js::CrossCompartmentWrapper {
  public:
    explicit MOZ_CONSTEXPR WaiveXrayWrapper(unsigned flags) : js::CrossCompartmentWrapper(flags) { }

    virtual bool getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                          JS::Handle<jsid> id,
                                          JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;
    virtual bool getPrototype(JSContext *cx, JS::Handle<JSObject*> wrapper,
                              JS::MutableHandle<JSObject*> protop) const MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<JSObject*> receiver,
                     JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp) const MOZ_OVERRIDE;
    virtual bool call(JSContext *cx, JS::Handle<JSObject*> wrapper,
                      const JS::CallArgs &args) const MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JS::Handle<JSObject*> wrapper,
                           const JS::CallArgs &args) const MOZ_OVERRIDE;

    virtual bool enumerate(JSContext *cx, JS::Handle<JSObject*> proxy,
                           JS::MutableHandle<JSObject*> objp) const MOZ_OVERRIDE;
    virtual bool nativeCall(JSContext *cx, JS::IsAcceptableThis test,
                            JS::NativeImpl impl, JS::CallArgs args) const MOZ_OVERRIDE;
    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;

    static const WaiveXrayWrapper singleton;
};

}

#endif
