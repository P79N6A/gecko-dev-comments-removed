









































#include "xpcprivate.h"




XPCContext*
XPCContext::newXPCContext(XPCJSRuntime* aRuntime,
                          JSContext* aJSContext)
{
    NS_PRECONDITION(aRuntime,"bad param");
    NS_PRECONDITION(aJSContext,"bad param");
    NS_ASSERTION(JS_GetRuntime(aJSContext) == aRuntime->GetJSRuntime(),
                 "XPConnect can not be used on multiple JSRuntimes!");

    return new XPCContext(aRuntime, aJSContext);
}

XPCContext::XPCContext(XPCJSRuntime* aRuntime,
                       JSContext* aJSContext)
    :   mRuntime(aRuntime),
        mJSContext(aJSContext),
        mLastResult(NS_OK),
        mPendingResult(NS_OK),
        mSecurityManager(nsnull),
        mException(nsnull),
        mCallingLangType(LANG_UNKNOWN),
        mSecurityManagerFlags(0),
        mMarked((JSPackedBool) JS_FALSE)
{
    MOZ_COUNT_CTOR(XPCContext);

    for(const char** p =  XPC_ARG_FORMATTER_FORMAT_STRINGS; *p; p++)
        JS_AddArgumentFormatter(mJSContext, *p, XPC_JSArgumentFormatter);
}

XPCContext::~XPCContext()
{
    MOZ_COUNT_DTOR(XPCContext);
    NS_IF_RELEASE(mException);
    NS_IF_RELEASE(mSecurityManager);
    
    
}

void
XPCContext::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth--;
    XPC_LOG_ALWAYS(("XPCContext @ %x", this));
        XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("mRuntime @ %x", mRuntime));
        XPC_LOG_ALWAYS(("mJSContext @ %x", mJSContext));
        XPC_LOG_ALWAYS(("mLastResult of %x", mLastResult));
        XPC_LOG_ALWAYS(("mPendingResult of %x", mPendingResult));
        XPC_LOG_ALWAYS(("mSecurityManager @ %x", mSecurityManager));
        XPC_LOG_ALWAYS(("mSecurityManagerFlags of %x", mSecurityManagerFlags));

        XPC_LOG_ALWAYS(("mException @ %x", mException));
        if(depth && mException)
        {
            
        }

        XPC_LOG_ALWAYS(("mCallingLangType of %s",
                         mCallingLangType == LANG_UNKNOWN ? "LANG_UNKNOWN" :
                         mCallingLangType == LANG_JS      ? "LANG_JS" :
                                                            "LANG_NATIVE"));
        XPC_LOG_OUTDENT();
#endif
}
