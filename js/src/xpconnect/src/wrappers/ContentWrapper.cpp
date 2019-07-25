






































#include "jsapi.h"
#include "jswrapper.h"

#include "XPCWrapper.h"

#include "nsJSPrincipals.h"

namespace xpc {

class ContentWrapper : public JSCrossCompartmentWrapper {
  public:
    ContentWrapper();
    virtual ~ContentWrapper();

    virtual bool enter(JSContext *cx, JSObject *proxy, jsid id);
    virtual void leave(JSContext *cx, JSObject *proxy);

    static ContentWrapper singleton;
};

ContentWrapper::ContentWrapper() : JSCrossCompartmentWrapper()
{
}

ContentWrapper::~ContentWrapper()
{
}

bool
ContentWrapper::enter(JSContext *cx, JSObject *wrapper, jsid id)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if(!ssm) {
        return true;
    }
    JSPrincipals *subjectPrincipals = wrapper->getCompartment(cx)->principals;
    JSPrincipals *objectPrincipals = wrappedObject(wrapper)->getCompartment(cx)->principals;
    if(!subjectPrincipals->subsume(subjectPrincipals, objectPrincipals)) {
        if(id == JSVAL_VOID) {
            JS_ReportError(cx, "Permission denied to access object");
        } else {
            JSString *str = JS_ValueToString(cx, id);
            JS_ReportError(cx, "Permission denied to access property '%hs'", str);
        }
        return false;
    }
    JSStackFrame *fp = nsnull;
    nsIPrincipal *principal = static_cast<nsJSPrincipals *>(objectPrincipals)->nsIPrincipalPtr;
    nsresult rv = ssm->PushContextPrincipal(cx, JS_FrameIterator(cx, &fp), principal);
    if (NS_FAILED(rv)) {
        NS_WARNING("Not allowing call because we're out of memory");
        JS_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

void
ContentWrapper::leave(JSContext *cx, JSObject *proxy)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (ssm) {
        ssm->PopContextPrincipal(cx);
    }
}

}
