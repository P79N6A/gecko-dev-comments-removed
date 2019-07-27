





#ifndef builtin_AtomicsObject_h
#define builtin_AtomicsObject_h

#include "jsobj.h"

namespace js {

class AtomicsObject : public JSObject
{
  public:
    static const Class class_;
    static JSObject* initClass(JSContext *cx, Handle<GlobalObject *> global);
    static bool toString(JSContext *cx, unsigned int argc, jsval *vp);

    static const int FutexOK = 0;

    
    
    static const int FutexNotequal = -1;
    static const int FutexTimedout = -2;

    
    static const int FutexInterrupted = -1000;
};

void atomics_fullMemoryBarrier();

bool atomics_compareExchange(JSContext *cx, unsigned argc, Value *vp);
bool atomics_load(JSContext *cx, unsigned argc, Value *vp);
bool atomics_store(JSContext *cx, unsigned argc, Value *vp);
bool atomics_fence(JSContext *cx, unsigned argc, Value *vp);
bool atomics_add(JSContext *cx, unsigned argc, Value *vp);
bool atomics_sub(JSContext *cx, unsigned argc, Value *vp);
bool atomics_and(JSContext *cx, unsigned argc, Value *vp);
bool atomics_or(JSContext *cx, unsigned argc, Value *vp);
bool atomics_xor(JSContext *cx, unsigned argc, Value *vp);
bool atomics_futexWait(JSContext *cx, unsigned argc, Value *vp);
bool atomics_futexWake(JSContext *cx, unsigned argc, Value *vp);
bool atomics_futexWakeOrRequeue(JSContext *cx, unsigned argc, Value *vp);

}  

JSObject *
js_InitAtomicsClass(JSContext *cx, js::HandleObject obj);

#endif 
