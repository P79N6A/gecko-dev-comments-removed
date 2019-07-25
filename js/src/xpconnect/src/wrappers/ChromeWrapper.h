






































#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class ChromeWrapper : public JSCrossCompartmentWrapper {
  public:
    ChromeWrapper();
    virtual ~ChromeWrapper();

    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper, js::AutoValueVector &props);
    virtual bool enumerate(JSContext *cx, JSObject *wrapper, js::AutoValueVector &props);
    virtual bool enumerateOwn(JSContext *cx, JSObject *wrapper, js::AutoValueVector &props);
    virtual bool iterate(JSContext *cx, JSObject *proxy, uintN flags, jsval *vp);

    virtual bool enter(JSContext *cx, JSObject *wrapper, jsid id, Mode mode);

    virtual JSString *fun_toString(JSContext *cx, JSObject *wrapper, uintN indent);

    static ChromeWrapper singleton;
};

}
