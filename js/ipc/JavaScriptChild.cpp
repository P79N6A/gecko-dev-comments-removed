






#include "JavaScriptChild.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/ipc/MessageChannel.h"
#include "nsContentUtils.h"
#include "xpcprivate.h"
#include "jsfriendapi.h"
#include "AccessCheck.h"

using namespace JS;
using namespace mozilla;
using namespace mozilla::jsipc;

using mozilla::AutoSafeJSContext;

static void
FinalizeChild(JSFreeOp *fop, JSFinalizeStatus status, bool isCompartment, void *data)
{
    if (status == JSFINALIZE_GROUP_START) {
        static_cast<JavaScriptChild *>(data)->finalize(fop);
    }
}

JavaScriptChild::JavaScriptChild(JSRuntime *rt)
  : JavaScriptShared(rt),
    JavaScriptBase<PJavaScriptChild>(rt)
{
}

JavaScriptChild::~JavaScriptChild()
{
    JS_RemoveFinalizeCallback(rt_, FinalizeChild);
}

bool
JavaScriptChild::init()
{
    if (!WrapperOwner::init())
        return false;
    if (!WrapperAnswer::init())
        return false;

    JS_AddFinalizeCallback(rt_, FinalizeChild, this);
    return true;
}

void
JavaScriptChild::finalize(JSFreeOp *fop)
{
    objects_.finalize(fop);
    objectIds_.finalize(fop);
}
