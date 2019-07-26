






#ifndef mozilla_jsipc_JavaScriptParent__
#define mozilla_jsipc_JavaScriptParent__

#include "JavaScriptBase.h"
#include "mozilla/jsipc/PJavaScriptParent.h"

namespace mozilla {
namespace jsipc {

class JavaScriptParent : public JavaScriptBase<PJavaScriptParent>
{
  public:
    JavaScriptParent();

    bool init();

    void decref();
    void incref();

    void drop(JSObject *obj);

    mozilla::ipc::IProtocol*
    CloneProtocol(Channel* aChannel, ProtocolCloneContext* aCtx) MOZ_OVERRIDE;

  private:
    JSObject *fromId(JSContext *cx, ObjectId objId);
    bool toId(JSContext *cx, JSObject *obj, ObjectId *idp);

  private:
    uintptr_t refcount_;
};

} 
} 

#endif 

