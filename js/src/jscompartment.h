






































#ifndef jscompartment_h___
#define jscompartment_h___

#include "jscntxt.h"
#include "jsgc.h"
#include "jsmath.h"
#include "jsobj.h"
#include "jsfun.h"
#include "jsgcstats.h"
#include "jsclist.h"
#include "jsxml.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251) /* Silence warning about JS_FRIEND_API and data members. */
#endif

namespace js {
namespace mjit {
class JaegerCompartment;
}
}

struct JS_FRIEND_API(JSCompartment) {
    JSRuntime                    *rt;
    JSPrincipals                 *principals;
    js::gc::Chunk                *chunk;

    js::gc::ArenaList            arenas[js::gc::FINALIZE_LIMIT];
    js::gc::FreeLists            freeLists;

#ifdef JS_GCMETER
    js::gc::JSGCArenaStats       compartmentStats[js::gc::FINALIZE_LIMIT];
#endif

#ifdef JS_TYPE_INFERENCE
    
    js::types::TypeCompartment   types;
#endif

    void                         *data;
    bool                         marked;
    js::WrapperMap               crossCompartmentWrappers;

#ifdef JS_METHODJIT
    js::mjit::JaegerCompartment  *jaegerCompartment;
#endif

    bool                         debugMode;  
    JSCList                      scripts;    

    






    JSObject                     *anynameObject;
    JSObject                     *functionNamespaceObject;

    JSCompartment(JSRuntime *cx);
    ~JSCompartment();

    bool init();

    bool wrap(JSContext *cx, js::Value *vp);
    bool wrap(JSContext *cx, JSString **strp);
    bool wrap(JSContext *cx, JSObject **objp);
    bool wrapId(JSContext *cx, jsid *idp);
    bool wrap(JSContext *cx, js::PropertyOp *op);
    bool wrap(JSContext *cx, js::PropertyDescriptor *desc);
    bool wrap(JSContext *cx, js::AutoIdVector &props);
    bool wrapException(JSContext *cx);

    void sweep(JSContext *cx);
    void purge(JSContext *cx);
    void finishArenaLists();
    bool arenaListsAreEmpty();
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace js {

class PreserveCompartment {
  protected:
    JSContext *cx;
  private:
    JSCompartment *oldCompartment;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
  public:
     PreserveCompartment(JSContext *cx JS_GUARD_OBJECT_NOTIFIER_PARAM) : cx(cx) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        oldCompartment = cx->compartment;
    }

    ~PreserveCompartment() {
        cx->compartment = oldCompartment;
    }
};

class SwitchToCompartment : public PreserveCompartment {
  public:
    SwitchToCompartment(JSContext *cx, JSCompartment *newCompartment) : PreserveCompartment(cx) {
        cx->compartment = newCompartment;
    }

    SwitchToCompartment(JSContext *cx, JSObject *target) : PreserveCompartment(cx) {
        cx->compartment = target->getCompartment();
    }
};

}

inline js::types::TypeObject *
JSContext::getFixedTypeObject(js::types::FixedTypeObjectName which)
{
#ifdef JS_TYPE_INFERENCE
    JS_ASSERT(which < js::types::TYPE_OBJECT_FIXED_LIMIT);
    js::types::TypeObject *type = compartment->types.fixedTypeObjects[which];
    if (type)
        return type;
    type = compartment->types.makeFixedTypeObject(this, which);
    compartment->types.fixedTypeObjects[which] = type;
    return type;
#else
    return NULL;
#endif
}

#endif 
