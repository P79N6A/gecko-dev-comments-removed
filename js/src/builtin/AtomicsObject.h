





#ifndef builtin_AtomicsObject_h
#define builtin_AtomicsObject_h

#include "jslock.h"
#include "jsobj.h"

namespace js {

class AtomicsObject : public JSObject
{
  public:
    static const Class class_;
    static JSObject* initClass(JSContext *cx, Handle<GlobalObject *> global);
    static bool toString(JSContext *cx, unsigned int argc, jsval *vp);

    
    
    
    enum FutexWaitResult : int32_t {
        FutexOK = 0,
        FutexNotequal = -1,
        FutexTimedout = -2
    };
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

class FutexRuntime
{
public:
    static bool initialize();
    static void destroy();

    static void lock();
    static void unlock();

    FutexRuntime();
    bool initInstance();
    void destroyInstance();

    
    enum WakeReason {
        WakeExplicit,           
        WakeForJSInterrupt      
    };

    
    
    
    
    
    
    
    
    
    
    bool wait(JSContext *cx, double timeout, AtomicsObject::FutexWaitResult *result);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void wake(WakeReason reason);

    bool isWaiting();

  private:
    enum FutexState {
        Idle,                   
        Waiting,                
        WaitingInterrupted,     
        Woken,                  
        WokenForJSInterrupt     
    };

    
    PRCondVar *cond_;

    
    
    
    FutexState state_;

    
    
    
    static mozilla::Atomic<PRLock*> lock_;

#ifdef DEBUG
    
    static mozilla::Atomic<PRThread*> lockHolder_;
#endif
};

}  

JSObject *
js_InitAtomicsClass(JSContext *cx, js::HandleObject obj);

#endif
