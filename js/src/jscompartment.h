






































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

struct JSCompartment {
    JSRuntime       *rt;
    JSPrincipals    *principals;
    js::gc::Chunk   *chunk;

    js::gc::ArenaList<JSObject>      objArena;
    js::gc::ArenaList<JSFunction>    funArena;
    js::gc::ArenaList<JSShortString> shortStringArena;
    js::gc::ArenaList<JSString>      stringArena;
    js::gc::ArenaList<JSString>      externalStringArenas[js::gc::JS_EXTERNAL_STRING_LIMIT];
#if JS_HAS_XML_SUPPORT
    js::gc::ArenaList<JSXML>         xmlArena;
#endif

    js::gc::FreeLists                freeLists;
    
#ifdef JS_GCMETER
    js::gc::JSGCArenaStats compartmentStats[js::gc::FINALIZE_LIMIT];
#endif

    void *data;
    bool marked;
    js::WrapperMap crossCompartmentWrappers;
    bool debugMode;

    
    JSCList scripts;

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

#endif 
