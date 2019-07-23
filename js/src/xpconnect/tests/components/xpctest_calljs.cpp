







































#include "xpctest_private.h"
#include "nsIXPCScriptable.h"
#include "xpctest_calljs.h"
#include "nsISupports.h"
#include "nsIClassInfoImpl.h"

class xpcTestCallJS : public nsIXPCTestCallJS, public nsIXPCScriptable {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTCALLJS
    NS_DECL_NSIXPCSCRIPTABLE
    xpcTestCallJS();
    virtual ~xpcTestCallJS();

private:
    nsIXPCTestCallJS* jsobject;
};


NS_IMPL_ISUPPORTS2_CI(xpcTestCallJS, nsIXPCTestCallJS, nsIXPCScriptable)


#define XPC_MAP_CLASSNAME           xpcTestCallJS
#define XPC_MAP_QUOTED_CLASSNAME   "xpcTestCallJS"
#define XPC_MAP_FLAGS               0
#include "xpc_map_end.h" 

xpcTestCallJS :: xpcTestCallJS() {
    NS_ADDREF_THIS();
}

xpcTestCallJS :: ~xpcTestCallJS() {

}

NS_IMETHODIMP xpcTestCallJS :: SetJSObject( nsIXPCTestCallJS* o ) {
    
    
    jsobject = o;
    if ( jsobject ) 
        NS_ADDREF( jsobject );
    return NS_OK;
}

NS_IMETHODIMP xpcTestCallJS :: CallMethodNoArgs(PRBool *_retval) {
    *_retval = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP xpcTestCallJS :: Evaluate ( const char *s ) {
    if (jsobject)
        return jsobject->Evaluate(s);
    return NS_OK;
}

NS_IMETHODIMP 
xpcTestCallJS :: EvaluateAndReturnError(nsresult in, nsresult *_retval){
    if (jsobject) {
        jsobject->EvaluateAndReturnError(in, _retval);
    } else {
        *_retval = in;
    }
    return *_retval;
}

NS_IMETHODIMP xpcTestCallJS :: EvaluateAndEatErrors(const char *s) {
    if ( jsobject ) 
        jsobject->Evaluate(s);
    return NS_OK;
}

NS_IMETHODIMP xpcTestCallJS :: UnscriptableMethod() {
    return NS_OK;
}

NS_IMETHODIMP
xpctest::ConstructXPCTestCallJS(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestCallJS *obj = new xpcTestCallJS();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }
    return rv;
}
