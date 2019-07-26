






#ifndef __AccessCheck_h__
#define __AccessCheck_h__

#include "jsapi.h"
#include "jswrapper.h"
#include "WrapperFactory.h"

class nsIPrincipal;

namespace xpc {

class AccessCheck {
  public:
    static bool subsumes(JSCompartment *a, JSCompartment *b);
    static bool subsumes(JSObject *a, JSObject *b);
    static bool wrapperSubsumes(JSObject *wrapper);
    static bool subsumesIgnoringDomain(JSCompartment *a, JSCompartment *b);
    static bool isChrome(JSCompartment *compartment);
    static bool isChrome(JSObject *obj);
    static bool callerIsChrome();
    static nsIPrincipal *getPrincipal(JSCompartment *compartment);
    static bool isCrossOriginAccessPermitted(JSContext *cx, JSObject *obj, jsid id,
                                             js::Wrapper::Action act);
    static bool isSystemOnlyAccessPermitted(JSContext *cx);

    static bool needsSystemOnlyWrapper(JSObject *obj);

    static bool isScriptAccessOnly(JSContext *cx, JSObject *wrapper);
};

struct Policy {
};


struct Opaque : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act) {
        return act == js::Wrapper::CALL;
    }
    static bool deny(js::Wrapper::Action act) {
        return false;
    }
    static bool allowNativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl)
    {
        return false;
    }
};



struct OnlyIfSubjectIsSystem : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act) {
        return AccessCheck::isSystemOnlyAccessPermitted(cx);
    }

    static bool deny(js::Wrapper::Action act) {
        return false;
    }

    static bool allowNativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl)
    {
        return AccessCheck::isSystemOnlyAccessPermitted(cx);
    }
};



struct CrossOriginAccessiblePropertiesOnly : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act) {
        return AccessCheck::isCrossOriginAccessPermitted(cx, wrapper, id, act);
    }
    static bool deny(js::Wrapper::Action act) {
        return false;
    }
    static bool allowNativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl)
    {
        return false;
    }
};



struct ExposedPropertiesOnly : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act);

    static bool deny(js::Wrapper::Action act) {
        
        return act == js::Wrapper::GET;
    }
    static bool allowNativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl);
};


struct ComponentsObjectPolicy : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act);

    static bool deny(js::Wrapper::Action act) {
        return false;
    }
    static bool allowNativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl) {
        return false;
    }
};

}

#endif 
