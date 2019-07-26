






#ifndef mozilla_jsipc_JavaScriptChild_h_
#define mozilla_jsipc_JavaScriptChild_h_

#include "JavaScriptBase.h"
#include "mozilla/jsipc/PJavaScriptChild.h"

namespace mozilla {
namespace jsipc {

class JavaScriptChild : public JavaScriptBase<PJavaScriptChild>
{
  public:
    JavaScriptChild(JSRuntime *rt);
    virtual ~JavaScriptChild();

    bool init();
    void finalize(JSFreeOp *fop);

    void drop(JSObject *obj);

  protected:
    virtual bool isParent() { return false; }

  private:
    bool fail(JSContext *cx, ReturnStatus *rs);
    bool ok(ReturnStatus *rs);
};

} 
} 

#endif
