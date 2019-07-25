






































#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class AccessCheck {
  public:
    static bool subsumes(JSCompartment *subject, JSCompartment *object, bool *yesno);
    static bool isPrivileged(JSCompartment *compartment);

    static void deny(JSContext *cx, jsid id);

    static bool enter(JSContext *cx, JSObject *wrapper, JSObject *wrappedObject, jsid id,
                      JSCrossCompartmentWrapper::Mode mode);
    static void leave(JSContext *cx, JSObject *wrapper, JSObject *wrappedObject);
};

}
