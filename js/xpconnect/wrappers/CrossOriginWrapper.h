






































#ifndef __CrossOriginWrapper_h__
#define __CrossOriginWrapper_h__

#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class NoWaiverWrapper : public js::CrossCompartmentWrapper {
  public:
    NoWaiverWrapper(uintN flags);
    virtual ~NoWaiverWrapper();

    virtual bool enter(JSContext *cx, JSObject *wrapper, jsid id, Action act, bool *bp);
    virtual void leave(JSContext *cx, JSObject *wrapper);

    static NoWaiverWrapper singleton;
};

class CrossOriginWrapper : public NoWaiverWrapper {
  public:
    CrossOriginWrapper(uintN flags);
    virtual ~CrossOriginWrapper();

    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                       bool set, js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          bool set, js::PropertyDescriptor *desc);
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     js::Value *vp);

    virtual bool call(JSContext *cx, JSObject *wrapper, uintN argc, js::Value *vp);
    virtual bool construct(JSContext *cx, JSObject *wrapper,
                           uintN argc, js::Value *argv, js::Value *rval);

    static CrossOriginWrapper singleton;
};

}

#endif
