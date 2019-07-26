






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
    ~JavaScriptChild();

    bool init();
    void trace(JSTracer *trc);

    bool RecvDropObject(const ObjectId &objId) MOZ_OVERRIDE;

    virtual void drop(JSObject *obj) { MOZ_CRASH(); }

  private:
    JSObject *fromId(JSContext *cx, ObjectId id);
    bool toId(JSContext *cx, JSObject *obj, ObjectId *idp);

    bool fail(JSContext *cx, ReturnStatus *rs);
    bool ok(ReturnStatus *rs);

  private:
    ObjectId lastId_;
    JSRuntime *rt_;
    ObjectToIdMap ids_;
};

} 
} 

#endif
