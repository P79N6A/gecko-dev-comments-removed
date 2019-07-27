






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
UpdateChildWeakPointersAfterGC(JSRuntime *rt, void *data)
{
    static_cast<JavaScriptChild *>(data)->updateWeakPointers();
}

JavaScriptChild::JavaScriptChild(JSRuntime *rt)
  : JavaScriptShared(rt),
    JavaScriptBase<PJavaScriptChild>(rt)
{
}

JavaScriptChild::~JavaScriptChild()
{
    JS_RemoveWeakPointerCallback(rt_, UpdateChildWeakPointersAfterGC);
}

bool
JavaScriptChild::init()
{
    if (!WrapperOwner::init())
        return false;
    if (!WrapperAnswer::init())
        return false;

    JS_AddWeakPointerCallback(rt_, UpdateChildWeakPointersAfterGC, this);
    return true;
}

void
JavaScriptChild::updateWeakPointers()
{
    objects_.sweep();
    objectIds_.sweep();
}

JSObject *
JavaScriptChild::scopeForTargetObjects()
{
    
    
    return xpc::PrivilegedJunkScope();
}
