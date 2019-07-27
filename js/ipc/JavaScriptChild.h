






#ifndef mozilla_jsipc_JavaScriptChild_h_
#define mozilla_jsipc_JavaScriptChild_h_

#include "JavaScriptBase.h"
#include "mozilla/jsipc/PJavaScriptChild.h"

namespace mozilla {
namespace jsipc {

class JavaScriptChild : public JavaScriptBase<PJavaScriptChild>
{
  public:
    explicit JavaScriptChild(JSRuntime *rt);
    virtual ~JavaScriptChild();

    bool init();
    void updateWeakPointers();

    void drop(JSObject *obj);

  protected:
    virtual bool isParent() { return false; }
    virtual JSObject *scopeForTargetObjects() MOZ_OVERRIDE;

  private:
    bool fail(JSContext *cx, ReturnStatus *rs);
    bool ok(ReturnStatus *rs);
};

} 
} 

#endif
