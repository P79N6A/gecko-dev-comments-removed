









































#include "xpcprivate.h"



XPCContext::XPCContext(XPCJSRuntime* aRuntime,
                       JSContext* aJSContext)
    :   mRuntime(aRuntime),
        mJSContext(aJSContext),
        mLastResult(NS_OK),
        mPendingResult(NS_OK),
        mSecurityManager(nsnull),
        mException(nsnull),
        mCallingLangType(LANG_UNKNOWN),
        mSecurityManagerFlags(0)
{
    MOZ_COUNT_CTOR(XPCContext);

    PR_INIT_CLIST(&mScopes);

    NS_ASSERTION(!mJSContext->data2, "Must be null");
    mJSContext->data2 = this;
}

XPCContext::~XPCContext()
{
    MOZ_COUNT_DTOR(XPCContext);
    NS_ASSERTION(mJSContext->data2 == this, "Must match this");
    mJSContext->data2 = nsnull;
    NS_IF_RELEASE(mException);
    NS_IF_RELEASE(mSecurityManager);

    
    for (PRCList *scopeptr = PR_NEXT_LINK(&mScopes);
         scopeptr != &mScopes;
         scopeptr = PR_NEXT_LINK(scopeptr))
    {
        XPCWrappedNativeScope *scope = (XPCWrappedNativeScope *)scopeptr;
        scope->SetContext(nsnull);
    }

    
    
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
        if (depth && mException)
        {
            
        }

        XPC_LOG_ALWAYS(("mCallingLangType of %s",
                        mCallingLangType == LANG_UNKNOWN ? "LANG_UNKNOWN" :
                        mCallingLangType == LANG_JS      ? "LANG_JS" :
                        "LANG_NATIVE"));
        XPC_LOG_OUTDENT();
#endif
}
