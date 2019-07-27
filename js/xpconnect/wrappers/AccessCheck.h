





#ifndef __AccessCheck_h__
#define __AccessCheck_h__

#include "jswrapper.h"
#include "js/Id.h"

class nsIPrincipal;

namespace xpc {

class AccessCheck {
  public:
    static bool subsumes(JSCompartment *a, JSCompartment *b);
    static bool subsumes(JSObject *a, JSObject *b);
    static bool wrapperSubsumes(JSObject *wrapper);
    static bool subsumesConsideringDomain(JSCompartment *a, JSCompartment *b);
    static bool isChrome(JSCompartment *compartment);
    static bool isChrome(JSObject *obj);
    static nsIPrincipal *getPrincipal(JSCompartment *compartment);
    static bool isCrossOriginAccessPermitted(JSContext *cx, JS::HandleObject obj,
                                             JS::HandleId id, js::Wrapper::Action act);
};

struct Policy {
};


struct Opaque : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act) {
        return false;
    }
    static bool deny(js::Wrapper::Action act, JS::HandleId id) {
        return false;
    }
    static bool allowNativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl) {
        return false;
    }
};


struct OpaqueWithCall : public Policy {
    static bool check(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act) {
        return act == js::Wrapper::CALL;
    }
    static bool deny(js::Wrapper::Action act, JS::HandleId id) {
        return false;
    }
    static bool allowNativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl) {
        return false;
    }
};



struct CrossOriginAccessiblePropertiesOnly : public Policy {
    static bool check(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id, js::Wrapper::Action act) {
        return AccessCheck::isCrossOriginAccessPermitted(cx, wrapper, id, act);
    }
    static bool deny(js::Wrapper::Action act, JS::HandleId id) {
        
        if (act == js::Wrapper::ENUMERATE)
            return true;
        return false;
    }
    static bool allowNativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl) {
        return false;
    }
};



struct ExposedPropertiesOnly : public Policy {
    static bool check(JSContext *cx, JS::HandleObject wrapper, JS::HandleId id, js::Wrapper::Action act);

    static bool deny(js::Wrapper::Action act, JS::HandleId id) {
        
        return act == js::Wrapper::GET || act == js::Wrapper::ENUMERATE ||
               act == js::Wrapper::GET_PROPERTY_DESCRIPTOR;
    }
    static bool allowNativeCall(JSContext *cx, JS::IsAcceptableThis test, JS::NativeImpl impl) {
        return false;
    }
};

}

#endif 
