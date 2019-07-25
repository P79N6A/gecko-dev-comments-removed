






































#ifndef __CrossOriginWrapper_h__
#define __CrossOriginWrapper_h__

#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class CrossOriginWrapper : public JSCrossCompartmentWrapper {
  public:
    CrossOriginWrapper(uintN flags);
    virtual ~CrossOriginWrapper();

    virtual bool enter(JSContext *cx, JSObject *wrapper, jsid id, bool set);
    virtual void leave(JSContext *cx, JSObject *wrapper);

    static CrossOriginWrapper singleton;
};

}

#endif
