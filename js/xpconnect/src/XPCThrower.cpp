







#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "jsprf.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Exceptions.h"

using namespace mozilla;
using namespace mozilla::dom;

bool XPCThrower::sVerbose = true;


void
XPCThrower::Throw(nsresult rv, JSContext* cx)
{
    const char* format;
    if (JS_IsExceptionPending(cx))
        return;
    if (!nsXPCException::NameAndFormatForNSResult(rv, nullptr, &format))
        format = "";
    dom::Throw(cx, rv, format);
}

namespace xpc {

bool
Throw(JSContext *cx, nsresult rv)
{
    XPCThrower::Throw(rv, cx);
    return false;
}

} 







bool
XPCThrower::CheckForPendingException(nsresult result, JSContext *cx)
{
    nsCOMPtr<nsIException> e = XPCJSRuntime::Get()->GetPendingException();
    if (!e)
        return false;
    XPCJSRuntime::Get()->SetPendingException(nullptr);

    nsresult e_result;
    if (NS_FAILED(e->GetResult(&e_result)) || e_result != result)
        return false;

    if (!ThrowExceptionObject(cx, e))
        JS_ReportOutOfMemory(cx);
    return true;
}


void
XPCThrower::Throw(nsresult rv, XPCCallContext& ccx)
{
    char* sz;
    const char* format;

    if (CheckForPendingException(rv, ccx))
        return;

    if (!nsXPCException::NameAndFormatForNSResult(rv, nullptr, &format))
        format = "";

    sz = (char*) format;

    if (sz && sVerbose)
        Verbosify(ccx, &sz, false);

    dom::Throw(ccx, rv, sz);

    if (sz && sz != format)
        JS_smprintf_free(sz);
}



void
XPCThrower::ThrowBadResult(nsresult rv, nsresult result, XPCCallContext& ccx)
{
    char* sz;
    const char* format;
    const char* name;

    









    if (CheckForPendingException(result, ccx))
        return;

    

    if (!nsXPCException::NameAndFormatForNSResult(rv, nullptr, &format) || !format)
        format = "";

    if (nsXPCException::NameAndFormatForNSResult(result, &name, nullptr) && name)
        sz = JS_smprintf("%s 0x%x (%s)", format, result, name);
    else
        sz = JS_smprintf("%s 0x%x", format, result);

    if (sz && sVerbose)
        Verbosify(ccx, &sz, true);

    dom::Throw(ccx, result, sz);

    if (sz)
        JS_smprintf_free(sz);
}


void
XPCThrower::ThrowBadParam(nsresult rv, unsigned paramNum, XPCCallContext& ccx)
{
    char* sz;
    const char* format;

    if (!nsXPCException::NameAndFormatForNSResult(rv, nullptr, &format))
        format = "";

    sz = JS_smprintf("%s arg %d", format, paramNum);

    if (sz && sVerbose)
        Verbosify(ccx, &sz, true);

    dom::Throw(ccx, rv, sz);

    if (sz)
        JS_smprintf_free(sz);
}



void
XPCThrower::Verbosify(XPCCallContext& ccx,
                      char** psz, bool own)
{
    char* sz = nullptr;

    if (ccx.HasInterfaceAndMember()) {
        XPCNativeInterface* iface = ccx.GetInterface();
        jsid id = ccx.GetMember()->GetName();
        JSAutoByteString bytes;
        const char *name = JSID_IS_VOID(id) ? "Unknown" : bytes.encodeLatin1(ccx, JSID_TO_STRING(id));
        if (!name) {
            name = "";
        }
        sz = JS_smprintf("%s [%s.%s]", *psz, iface->GetNameString(), name);
    }

    if (sz) {
        if (own)
            JS_smprintf_free(*psz);
        *psz = sz;
    }
}
