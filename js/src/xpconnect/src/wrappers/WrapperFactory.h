






































#include "jsapi.h"
#include "jswrapper.h"

namespace xpc {

class WrapperFactory {
    
    static JSCrossCompartmentWrapper *select(JSContext *cx,
                                             JSCompartment *subject,
                                             JSCompartment *object);
};

}
