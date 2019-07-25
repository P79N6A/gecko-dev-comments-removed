






































#include "jsapi.h"
#include "jswrapper.h"

class nsIPrincipal;

namespace xpc {

nsIPrincipal *
GetCompartmentPrincipal(JSCompartment *compartment);

class AccessCheck {
  public:
    static bool isSameOrigin(JSCompartment *a, JSCompartment *b);
    static bool isChrome(JSCompartment *compartment);
    static nsIPrincipal *getPrincipal(JSCompartment *compartment);
    static bool isCrossOriginAccessPermitted(JSContext *cx, JSObject *obj, jsid id,
                                             js::Wrapper::Action act);
    static bool isSystemOnlyAccessPermitted(JSContext *cx);
    static bool isLocationObjectSameOrigin(JSContext *cx, JSObject *wrapper);
    static bool documentDomainMakesSameOrigin(JSContext *cx, JSObject *obj);

    static bool needsSystemOnlyWrapper(JSObject *obj);

    static bool isScriptAccessOnly(JSContext *cx, JSObject *wrapper);

    static void deny(JSContext *cx, jsid id);
};

struct Policy {
    typedef js::Wrapper::Permission Permission;

    static const Permission PermitObjectAccess = js::Wrapper::PermitObjectAccess;
    static const Permission PermitPropertyAccess = js::Wrapper::PermitPropertyAccess;
    static const Permission DenyAccess = js::Wrapper::DenyAccess;
};


struct Permissive : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act,
                      Permission &perm) {
        perm = PermitObjectAccess;
        return true;
    }
};



struct OnlyIfSubjectIsSystem : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act,
                      Permission &perm) {
        if (AccessCheck::isSystemOnlyAccessPermitted(cx)) {
            perm = PermitObjectAccess;
            return true;
        }
        perm = DenyAccess;
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, wrapper))
            return false;
        AccessCheck::deny(cx, id);
        return false;
    }
};



struct CrossOriginAccessiblePropertiesOnly : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act,
                      Permission &perm) {
        if (AccessCheck::isCrossOriginAccessPermitted(cx, wrapper, id, act)) {
            perm = PermitPropertyAccess;
            return true;
        }
        perm = DenyAccess;
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, wrapper))
            return false;
        AccessCheck::deny(cx, id);
        return false;
    }
};



struct SameOriginOrCrossOriginAccessiblePropertiesOnly : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act,
                      Permission &perm) {
        if (AccessCheck::isCrossOriginAccessPermitted(cx, wrapper, id, act) ||
            AccessCheck::isLocationObjectSameOrigin(cx, wrapper)) {
            perm = PermitPropertyAccess;
            return true;
        }
        perm = DenyAccess;
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, wrapper))
            return false;
        AccessCheck::deny(cx, id);
        return false;
    }
};



struct ExposedPropertiesOnly : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act,
                      Permission &perm);
};

}
