






#ifndef __CrossOriginWrapper_h__
#define __CrossOriginWrapper_h__

#include "mozilla/Attributes.h"

#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class CrossOriginWrapper : public js::CrossCompartmentWrapper {
  public:
    CrossOriginWrapper(unsigned flags);
    virtual ~CrossOriginWrapper();

    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                       bool set, js::PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          bool set, js::PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     js::Value *vp) MOZ_OVERRIDE;

    virtual bool call(JSContext *cx, JSObject *wrapper, unsigned argc, js::Value *vp) MOZ_OVERRIDE;
    virtual bool construct(JSContext *cx, JSObject *wrapper,
                           unsigned argc, js::Value *argv, js::Value *rval) MOZ_OVERRIDE;

    static CrossOriginWrapper singleton;
};

}

#endif
