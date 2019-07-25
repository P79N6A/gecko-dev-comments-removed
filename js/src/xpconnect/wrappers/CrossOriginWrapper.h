






































#ifndef __CrossOriginWrapper_h__
#define __CrossOriginWrapper_h__

#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class CrossOriginWrapper : public JSCrossCompartmentWrapper {
  public:
    CrossOriginWrapper(uintN flags);
    virtual ~CrossOriginWrapper();

    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                       bool set, js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          bool set, js::PropertyDescriptor *desc);
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                     js::Value *vp);

    virtual bool enter(JSContext *cx, JSObject *wrapper, jsid id, Action act);
    virtual void leave(JSContext *cx, JSObject *wrapper);

    static CrossOriginWrapper singleton;
};

}

#endif
