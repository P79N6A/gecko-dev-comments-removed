






#ifndef __CrossOriginWrapper_h__
#define __CrossOriginWrapper_h__

#include "mozilla/Attributes.h"

#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class WaiveXrayWrapper : public js::CrossCompartmentWrapper {
  public:
    WaiveXrayWrapper(unsigned flags);
    virtual ~WaiveXrayWrapper();

    virtual bool getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject *> wrapper, JS::Handle<jsid> id, js::PropertyDescriptor *desc, unsigned flags) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject *> wrapper, JS::Handle<jsid> id,
                                          js::PropertyDescriptor *desc, unsigned flags) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     js::Value *vp) MOZ_OVERRIDE;

    virtual bool call(JSContext *cx, JSObject *wrapper, unsigned argc, js::Value *vp) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JSObject *wrapper,
                           unsigned argc, js::Value *argv, js::Value *rval) MOZ_OVERRIDE;

    static WaiveXrayWrapper singleton;
};

}

#endif
