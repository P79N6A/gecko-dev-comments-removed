






































#ifndef __CrossOriginWrapper_h__
#define __CrossOriginWrapper_h__

#include "mozilla/Attributes.h"

#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class NoWaiverWrapper : public js::CrossCompartmentWrapper {
  public:
    NoWaiverWrapper(unsigned flags);
    virtual ~NoWaiverWrapper();

    virtual bool enter(JSContext *cx, JSObject *wrapper, jsid id, Action act, bool *bp) MOZ_OVERRIDE;
    virtual void leave(JSContext *cx, JSObject *wrapper) MOZ_OVERRIDE;

    static NoWaiverWrapper singleton;
};

class CrossOriginWrapper : public NoWaiverWrapper {
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
