





#ifndef __CrossOriginWrapper_h__
#define __CrossOriginWrapper_h__

#include "mozilla/Attributes.h"

#include "jswrapper.h"

namespace xpc {

class WaiveXrayWrapper : public js::CrossCompartmentWrapper {
  public:
    WaiveXrayWrapper(unsigned flags);

    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                          JS::Handle<jsid> id,
                                          JS::MutableHandle<JSPropertyDescriptor> desc) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JS::Handle<JSObject*> wrapper, JS::Handle<JSObject*> receiver,
                     JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp) MOZ_OVERRIDE;

    virtual bool call(JSContext *cx, JS::Handle<JSObject*> wrapper,
                      const JS::CallArgs &args) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JS::Handle<JSObject*> wrapper,
                           const JS::CallArgs &args) MOZ_OVERRIDE;

    virtual bool nativeCall(JSContext *cx, JS::IsAcceptableThis test,
                            JS::NativeImpl impl, JS::CallArgs args) MOZ_OVERRIDE;

    virtual bool getPrototypeOf(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                JS::MutableHandle<JSObject*> protop) MOZ_OVERRIDE;

    static WaiveXrayWrapper singleton;
};

}

#endif
