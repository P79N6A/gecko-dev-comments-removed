





#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsprf.h"
#include "nsCOMPtr.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"
#include "xpcprivate.h"
#include "XPCInlines.h"
#include "XPCQuickStubs.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Exceptions.h"

using namespace mozilla;
using namespace JS;

nsresult
xpc_qsUnwrapArgImpl(JSContext *cx,
                    HandleObject src,
                    const nsIID &iid,
                    void **ppArg)
{
    nsISupports *iface = xpc::UnwrapReflectorToISupports(src);
    if (iface) {
        if (NS_FAILED(iface->QueryInterface(iid, ppArg))) {
            return NS_ERROR_XPC_BAD_CONVERT_JS;
        }

        return NS_OK;
    }

    
    XPCCallContext ccx(JS_CALLER, cx);
    if (!ccx.IsValid()) {
        return NS_ERROR_XPC_BAD_CONVERT_JS;
    }

    nsRefPtr<nsXPCWrappedJS> wrappedJS;
    nsresult rv = nsXPCWrappedJS::GetNewOrUsed(src, iid, getter_AddRefs(wrappedJS));
    if (NS_FAILED(rv) || !wrappedJS) {
        return rv;
    }

    
    
    
    
    return wrappedJS->QueryInterface(iid, ppArg);
}

namespace xpc {

bool
NonVoidStringToJsval(JSContext *cx, nsAString &str, MutableHandleValue rval)
{
    nsStringBuffer* sharedBuffer;
    if (!XPCStringConvert::ReadableToJSVal(cx, str, &sharedBuffer, rval))
      return false;

    if (sharedBuffer) {
        
        
        str.ForgetSharedBuffer();
    }
    return true;
}

} 

