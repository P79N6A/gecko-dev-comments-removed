






#ifndef mozilla_jsipc_JavaScriptParent__
#define mozilla_jsipc_JavaScriptParent__

#include "JavaScriptShared.h"
#include "mozilla/jsipc/PJavaScriptParent.h"
#include "js/Class.h"

#ifdef XP_WIN
#undef GetClassName
#undef GetClassInfo
#endif

namespace mozilla {
namespace jsipc {

class JavaScriptParent
  : public PJavaScriptParent,
    public JavaScriptShared
{
  public:
    JavaScriptParent();

    bool init();

  public:
    
    
    bool preventExtensions(JSContext *cx, JS::HandleObject proxy);
    bool getPropertyDescriptor(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
                               JS::MutableHandle<JSPropertyDescriptor> desc);
    bool getOwnPropertyDescriptor(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
                                  JS::MutableHandle<JSPropertyDescriptor> desc);
    bool defineProperty(JSContext *cx, JS::HandleObject proxy, JS::HandleId id,
                        JS::MutableHandle<JSPropertyDescriptor> desc);
    bool getOwnPropertyNames(JSContext *cx, JS::HandleObject proxy, JS::AutoIdVector &props);
    bool delete_(JSContext *cx, JS::HandleObject proxy, JS::HandleId id, bool *bp);
    bool enumerate(JSContext *cx, JS::HandleObject proxy, JS::AutoIdVector &props);

    
    bool has(JSContext *cx, JS::HandleObject proxy, JS::HandleId id, bool *bp);
    bool hasOwn(JSContext *cx, JS::HandleObject proxy, JS::HandleId id, bool *bp);
    bool get(JSContext *cx, JS::HandleObject proxy, JS::HandleObject receiver,
             JS::HandleId id, JS::MutableHandleValue vp);
    bool set(JSContext *cx, JS::HandleObject proxy, JS::HandleObject receiver,
             JS::HandleId id, bool strict, JS::MutableHandleValue vp);
    bool keys(JSContext *cx, JS::HandleObject proxy, JS::AutoIdVector &props);
    

    
    bool isExtensible(JSContext *cx, JS::HandleObject proxy, bool *extensible);
    bool call(JSContext *cx, JS::HandleObject proxy, const JS::CallArgs &args);
    bool objectClassIs(JSContext *cx, JS::HandleObject obj, js::ESClassValue classValue);
    const char* className(JSContext *cx, JS::HandleObject proxy);

    virtual void ActorDestroy(ActorDestroyReason why);

    void decref();
    void incref();

    bool active() { return !inactive_; }

    void drop(JSObject *obj);

    nsresult instanceOf(JSObject *obj, const nsID *id, bool *bp);

    



    bool domInstanceOf(JSContext *cx, JSObject *obj, int prototypeID, int depth, bool *bp);

    mozilla::ipc::IProtocol*
    CloneProtocol(Channel* aChannel, ProtocolCloneContext* aCtx) MOZ_OVERRIDE;

  private:
    JSObject *fromId(JSContext *cx, ObjectId objId);
    bool toId(JSContext *cx, JSObject *obj, ObjectId *idp);

    bool getPropertyNames(JSContext *cx, JS::HandleObject proxy, uint32_t flags,
                          JS::AutoIdVector &props);
    ObjectId idOf(JSObject *obj);

    
    bool ipcfail(JSContext *cx);

    
    bool ok(JSContext *cx, const ReturnStatus &status);

  private:
    uintptr_t refcount_;
    bool inactive_;
};

bool
IsCPOW(JSObject *obj);

nsresult
InstanceOf(JSObject *obj, const nsID *id, bool *bp);

bool
DOMInstanceOf(JSContext *cx, JSObject *obj, int prototypeID, int depth, bool *bp);

} 
} 

#endif 

