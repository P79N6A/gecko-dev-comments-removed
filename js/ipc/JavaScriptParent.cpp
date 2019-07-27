






#include "JavaScriptParent.h"
#include "mozilla/dom/ContentParent.h"
#include "nsJSUtils.h"
#include "jsfriendapi.h"
#include "jswrapper.h"
#include "js/Proxy.h"
#include "HeapAPI.h"
#include "xpcprivate.h"
#include "mozilla/Casting.h"

using namespace js;
using namespace JS;
using namespace mozilla;
using namespace mozilla::jsipc;
using namespace mozilla::dom;

static void
TraceParent(JSTracer *trc, void *data)
{
    static_cast<JavaScriptParent *>(data)->trace(trc);
}

JavaScriptParent::JavaScriptParent(JSRuntime *rt)
  : JavaScriptShared(rt),
    JavaScriptBase<PJavaScriptParent>(rt)
{
}

JavaScriptParent::~JavaScriptParent()
{
    JS_RemoveExtraGCRootsTracer(rt_, TraceParent, this);
}

bool
JavaScriptParent::init()
{
    if (!WrapperOwner::init())
        return false;

    JS_AddExtraGCRootsTracer(rt_, TraceParent, this);
    return true;
}

void
JavaScriptParent::trace(JSTracer *trc)
{
    objects_.trace(trc);
    unwaivedObjectIds_.trace(trc);
    waivedObjectIds_.trace(trc);
}

JSObject *
JavaScriptParent::scopeForTargetObjects()
{
    
    
    
    
    return xpc::UnprivilegedJunkScope();
}

mozilla::ipc::IProtocol*
JavaScriptParent::CloneProtocol(Channel* aChannel, ProtocolCloneContext* aCtx)
{
    ContentParent *contentParent = aCtx->GetContentParent();
    nsAutoPtr<PJavaScriptParent> actor(contentParent->AllocPJavaScriptParent());
    if (!actor || !contentParent->RecvPJavaScriptConstructor(actor)) {
        return nullptr;
    }
    return actor.forget();
}

PJavaScriptParent *
mozilla::jsipc::NewJavaScriptParent(JSRuntime *rt)
{
    JavaScriptParent *parent = new JavaScriptParent(rt);
    if (!parent->init()) {
        delete parent;
        return nullptr;
    }
    return parent;
}

void
mozilla::jsipc::ReleaseJavaScriptParent(PJavaScriptParent *parent)
{
    static_cast<JavaScriptParent *>(parent)->decref();
}
