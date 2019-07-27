






#ifndef mozilla_jsipc_WrapperOwner_h__
#define mozilla_jsipc_WrapperOwner_h__

#include "JavaScriptShared.h"
#include "mozilla/ipc/ProtocolUtils.h"
#include "js/Class.h"

#ifdef XP_WIN
#undef GetClassName
#undef GetClassInfo
#endif

namespace mozilla {
namespace jsipc {

class WrapperOwner : public virtual JavaScriptShared
{
  public:
    typedef mozilla::ipc::IProtocolManager<
                       mozilla::ipc::IProtocol>::ActorDestroyReason
           ActorDestroyReason;

    explicit WrapperOwner(JSRuntime *rt);
    bool init();

    
    
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
    bool callOrConstruct(JSContext *cx, JS::HandleObject proxy, const JS::CallArgs &args,
                         bool construct);
    bool hasInstance(JSContext *cx, JS::HandleObject proxy, JS::MutableHandleValue v, bool *bp);
    bool objectClassIs(JSContext *cx, JS::HandleObject obj, js::ESClassValue classValue);
    const char* className(JSContext *cx, JS::HandleObject proxy);
    bool isCallable(JSObject *obj);
    

    nsresult instanceOf(JSObject *obj, const nsID *id, bool *bp);

    bool toString(JSContext *cx, JS::HandleObject callee, JS::CallArgs &args);

    



    bool domInstanceOf(JSContext *cx, JSObject *obj, int prototypeID, int depth, bool *bp);

    bool active() { return !inactive_; }

    void drop(JSObject *obj);
    void updatePointer(JSObject *obj, const JSObject *old);

    virtual void ActorDestroy(ActorDestroyReason why);

    virtual bool toObjectVariant(JSContext *cx, JSObject *obj, ObjectVariant *objVarp);
    virtual JSObject *fromObjectVariant(JSContext *cx, ObjectVariant objVar);
    JSObject *fromRemoteObjectVariant(JSContext *cx, RemoteObject objVar);
    JSObject *fromLocalObjectVariant(JSContext *cx, LocalObject objVar);

  protected:
    ObjectId idOf(JSObject *obj);

  private:
    ObjectId idOfUnchecked(JSObject *obj);

    bool getPropertyNames(JSContext *cx, JS::HandleObject proxy, uint32_t flags,
                          JS::AutoIdVector &props);

    
    bool ipcfail(JSContext *cx);

    
    bool ok(JSContext *cx, const ReturnStatus &status);

    bool inactive_;

    
  public:
    virtual bool SendDropObject(const ObjectId &objId) = 0;
    virtual bool CallPreventExtensions(const ObjectId &objId, ReturnStatus *rs) = 0;
    virtual bool CallGetPropertyDescriptor(const ObjectId &objId, const nsString &id,
                                           ReturnStatus *rs,
                                           PPropertyDescriptor *out) = 0;
    virtual bool CallGetOwnPropertyDescriptor(const ObjectId &objId,
                                              const nsString &id,
                                              ReturnStatus *rs,
                                              PPropertyDescriptor *out) = 0;
    virtual bool CallDefineProperty(const ObjectId &objId, const nsString &id,
                                    const PPropertyDescriptor &flags,
                                    ReturnStatus *rs) = 0;
    virtual bool CallDelete(const ObjectId &objId, const nsString &id,
                            ReturnStatus *rs, bool *success) = 0;

    virtual bool CallHas(const ObjectId &objId, const nsString &id,
                         ReturnStatus *rs, bool *bp) = 0;
    virtual bool CallHasOwn(const ObjectId &objId, const nsString &id,
                            ReturnStatus *rs, bool *bp) = 0;
    virtual bool CallGet(const ObjectId &objId, const ObjectVariant &receiverVar,
                         const nsString &id,
                         ReturnStatus *rs, JSVariant *result) = 0;
    virtual bool CallSet(const ObjectId &objId, const ObjectVariant &receiverVar,
                         const nsString &id, const bool &strict,
                         const JSVariant &value, ReturnStatus *rs, JSVariant *result) = 0;

    virtual bool CallIsExtensible(const ObjectId &objId, ReturnStatus *rs,
                                  bool *result) = 0;
    virtual bool CallCallOrConstruct(const ObjectId &objId, const nsTArray<JSParam> &argv,
                                     const bool &construct, ReturnStatus *rs, JSVariant *result,
                                     nsTArray<JSParam> *outparams) = 0;
    virtual bool CallHasInstance(const ObjectId &objId, const JSVariant &v,
                                 ReturnStatus *rs, bool *bp) = 0;
    virtual bool CallObjectClassIs(const ObjectId &objId, const uint32_t &classValue,
                                   bool *result) = 0;
    virtual bool CallClassName(const ObjectId &objId, nsString *result) = 0;

    virtual bool CallGetPropertyNames(const ObjectId &objId, const uint32_t &flags,
                                      ReturnStatus *rs, nsTArray<nsString> *names) = 0;
    virtual bool CallInstanceOf(const ObjectId &objId, const JSIID &iid,
                                ReturnStatus *rs, bool *instanceof) = 0;
    virtual bool CallDOMInstanceOf(const ObjectId &objId, const int &prototypeID, const int &depth,
                                   ReturnStatus *rs, bool *instanceof) = 0;
};

bool
IsCPOW(JSObject *obj);

bool
IsWrappedCPOW(JSObject *obj);

nsresult
InstanceOf(JSObject *obj, const nsID *id, bool *bp);

bool
DOMInstanceOf(JSContext *cx, JSObject *obj, int prototypeID, int depth, bool *bp);

} 
} 

#endif 
