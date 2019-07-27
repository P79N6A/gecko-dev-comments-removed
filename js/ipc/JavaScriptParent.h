






#ifndef mozilla_jsipc_JavaScriptParent__
#define mozilla_jsipc_JavaScriptParent__

#include "JavaScriptBase.h"
#include "mozilla/jsipc/PJavaScriptParent.h"

namespace mozilla {
namespace jsipc {

class JavaScriptParent : public JavaScriptBase<PJavaScriptParent>
{
  public:
    explicit JavaScriptParent(JSRuntime* rt);
    virtual ~JavaScriptParent();

    bool init();
    void trace(JSTracer* trc);

    void drop(JSObject* obj);

    mozilla::ipc::IProtocol*
    CloneProtocol(Channel* aChannel, ProtocolCloneContext* aCtx) override;

  protected:
    virtual bool isParent() override { return true; }
    virtual JSObject* scopeForTargetObjects() override;
};

} 
} 

#endif 

