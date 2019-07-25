






































#include "jsapi.h"
#include "jswrapper.h"









namespace xpc {

class ContentWrapper : public JSCrossCompartmentWrapper {
  public:
    ContentWrapper();
    virtual ~ContentWrapper();

    virtual bool enter(JSContext *cx, JSObject *wrapper, jsid id, Mode mode);
    virtual void leave(JSContext *cx, JSObject *wrapper);

    static ContentWrapper singleton;
};

}
