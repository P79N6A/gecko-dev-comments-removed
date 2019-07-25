






































#include "jsapi.h"
#include "jswrapper.h"

class nsIPrincipal;

namespace xpc {

class AccessCheck {
  public:
    static bool isSameOrigin(JSCompartment *a, JSCompartment *b);
    static bool isChrome(JSCompartment *compartment);
    static nsIPrincipal *getPrincipal(JSCompartment *compartment);
    static bool isCrossOriginAccessPermitted(JSContext *cx, JSObject *obj, jsid id,
                                             JSWrapper::Action act);
    static bool isSystemOnlyAccessPermitted(JSContext *cx);

    static bool needsSystemOnlyWrapper(JSObject *obj);

    static void deny(JSContext *cx, jsid id);
};

struct Policy {
    typedef JSWrapper::Permission Permission;

    static const Permission PermitObjectAccess = JSWrapper::PermitObjectAccess;
    static const Permission PermitPropertyAccess = JSWrapper::PermitPropertyAccess;
    static const Permission DenyAccess = JSWrapper::DenyAccess;
};


struct Permissive : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, JSWrapper::Action act,
                      Permission &perm) {
        perm = PermitObjectAccess;
        return true;
    }
};



struct OnlyIfSubjectIsSystem : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, JSWrapper::Action act,
                      Permission &perm) {
        perm = DenyAccess;
        if (AccessCheck::isSystemOnlyAccessPermitted(cx))
            perm = PermitObjectAccess;
        return true;
    }
};



struct CrossOriginAccessiblePropertiesOnly : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, JSWrapper::Action act,
                      Permission &perm) {
        perm = DenyAccess;
        if (AccessCheck::isCrossOriginAccessPermitted(cx, wrapper, id, act))
            perm = PermitPropertyAccess;
        return true;
    }
};



struct ExposedPropertiesOnly : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, JSWrapper::Action act,
                      Permission &perm);
};

}
