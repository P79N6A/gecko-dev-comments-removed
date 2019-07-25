







#include "mozilla/Util.h"

#include "xpcprivate.h"

using namespace mozilla;

XPCCallContext::XPCCallContext(XPCContext::LangType callerLanguage,
                               JSContext* cx    ,
                               JSObject* obj    ,
                               JSObject* funobj ,
                               jsid name        ,
                               unsigned argc       ,
                               jsval *argv      ,
                               jsval *rval      )
    :   mState(INIT_FAILED),
        mXPC(nsXPConnect::GetXPConnect()),
        mXPCContext(nullptr),
        mJSContext(cx),
        mContextPopRequired(false),
        mDestroyJSContextInDestructor(false),
        mCallerLanguage(callerLanguage)
{
    Init(callerLanguage, callerLanguage == NATIVE_CALLER, obj, funobj,
         INIT_SHOULD_LOOKUP_WRAPPER, name, argc, argv, rval);
}

XPCCallContext::XPCCallContext(XPCContext::LangType callerLanguage,
                               JSContext* cx,
                               JSBool callBeginRequest,
                               JSObject* obj,
                               JSObject* flattenedJSObject,
                               XPCWrappedNative* wrapper,
                               XPCWrappedNativeTearOff* tearOff)
    :   mState(INIT_FAILED),
        mXPC(nsXPConnect::GetXPConnect()),
        mXPCContext(nullptr),
        mJSContext(cx),
        mContextPopRequired(false),
        mDestroyJSContextInDestructor(false),
        mCallerLanguage(callerLanguage),
        mFlattenedJSObject(flattenedJSObject),
        mWrapper(wrapper),
        mTearOff(tearOff)
{
    Init(callerLanguage, callBeginRequest, obj, nullptr,
         WRAPPER_PASSED_TO_CONSTRUCTOR, JSID_VOID, NO_ARGS,
         nullptr, nullptr);
}

void
XPCCallContext::Init(XPCContext::LangType callerLanguage,
                     JSBool callBeginRequest,
                     JSObject* obj,
                     JSObject* funobj,
                     WrapperInitOptions wrapperInitOptions,
                     jsid name,
                     unsigned argc,
                     jsval *argv,
                     jsval *rval)
{
    if (!mXPC)
        return;

    XPCJSContextStack* stack = XPCJSRuntime::Get()->GetJSContextStack();

    if (!stack) {
        
        mJSContext = nullptr;
        return;
    }

    JSContext *topJSContext = stack->Peek();

    if (!mJSContext) {
        
        
        
        
        
        
        

        if (topJSContext) {
            mJSContext = topJSContext;
        } else {
            mJSContext = stack->GetSafeJSContext();
            if (!mJSContext)
                return;
        }
    }

    if (topJSContext != mJSContext) {
        if (!stack->Push(mJSContext)) {
            NS_ERROR("bad!");
            return;
        }
        mContextPopRequired = true;
    }

    
    

    NS_ASSERTION(!callBeginRequest || mCallerLanguage == NATIVE_CALLER,
                 "Don't call JS_BeginRequest unless the caller is native.");
    if (callBeginRequest)
        JS_BeginRequest(mJSContext);

    mXPCContext = XPCContext::GetXPCContext(mJSContext);
    mPrevCallerLanguage = mXPCContext->SetCallingLangType(mCallerLanguage);

    
    mPrevCallContext = XPCJSRuntime::Get()->SetCallContext(this);

    
    
    if (!mPrevCallContext)
        NS_ADDREF(mXPC);

    mState = HAVE_CONTEXT;

    if (!obj)
        return;

    mScopeForNewJSObjects = obj;

    mState = HAVE_SCOPE;

    mMethodIndex = 0xDEAD;

    mState = HAVE_OBJECT;

    mTearOff = nullptr;
    if (wrapperInitOptions == INIT_SHOULD_LOOKUP_WRAPPER) {
        mWrapper = XPCWrappedNative::GetWrappedNativeOfJSObject(mJSContext, obj,
                                                                funobj,
                                                                &mFlattenedJSObject,
                                                                &mTearOff);
        if (mWrapper) {
            mFlattenedJSObject = mWrapper->GetFlatJSObject();

            if (mTearOff)
                mScriptableInfo = nullptr;
            else
                mScriptableInfo = mWrapper->GetScriptableInfo();
        } else {
            NS_ABORT_IF_FALSE(!mFlattenedJSObject || IS_SLIM_WRAPPER(mFlattenedJSObject),
                              "should have a slim wrapper");
        }
    }

    if (!JSID_IS_VOID(name))
        SetName(name);

    if (argc != NO_ARGS)
        SetArgsAndResultPtr(argc, argv, rval);

    CHECK_STATE(HAVE_OBJECT);
}

void
XPCCallContext::SetName(jsid name)
{
    CHECK_STATE(HAVE_OBJECT);

    mName = name;

    if (mTearOff) {
        mSet = nullptr;
        mInterface = mTearOff->GetInterface();
        mMember = mInterface->FindMember(name);
        mStaticMemberIsLocal = true;
        if (mMember && !mMember->IsConstant())
            mMethodIndex = mMember->GetIndex();
    } else {
        mSet = mWrapper ? mWrapper->GetSet() : nullptr;

        if (mSet &&
            mSet->FindMember(name, &mMember, &mInterface,
                             mWrapper->HasProto() ?
                             mWrapper->GetProto()->GetSet() :
                             nullptr,
                             &mStaticMemberIsLocal)) {
            if (mMember && !mMember->IsConstant())
                mMethodIndex = mMember->GetIndex();
        } else {
            mMember = nullptr;
            mInterface = nullptr;
            mStaticMemberIsLocal = false;
        }
    }

    mState = HAVE_NAME;
}

void
XPCCallContext::SetCallInfo(XPCNativeInterface* iface, XPCNativeMember* member,
                            JSBool isSetter)
{
    CHECK_STATE(HAVE_CONTEXT);

    
    

    
    if (mTearOff && mTearOff->GetInterface() != iface)
        mTearOff = nullptr;

    mSet = nullptr;
    mInterface = iface;
    mMember = member;
    mMethodIndex = mMember->GetIndex() + (isSetter ? 1 : 0);
    mName = mMember->GetName();

    if (mState < HAVE_NAME)
        mState = HAVE_NAME;
}

void
XPCCallContext::SetArgsAndResultPtr(unsigned argc,
                                    jsval *argv,
                                    jsval *rval)
{
    CHECK_STATE(HAVE_OBJECT);

    if (mState < HAVE_NAME) {
        mSet = nullptr;
        mInterface = nullptr;
        mMember = nullptr;
        mStaticMemberIsLocal = false;
    }

    mArgc   = argc;
    mArgv   = argv;
    mRetVal = rval;

    mState = HAVE_ARGS;
}

nsresult
XPCCallContext::CanCallNow()
{
    nsresult rv;

    if (!HasInterfaceAndMember())
        return NS_ERROR_UNEXPECTED;
    if (mState < HAVE_ARGS)
        return NS_ERROR_UNEXPECTED;

    if (!mTearOff) {
        mTearOff = mWrapper->FindTearOff(*this, mInterface, false, &rv);
        if (!mTearOff || mTearOff->GetInterface() != mInterface) {
            mTearOff = nullptr;
            return NS_FAILED(rv) ? rv : NS_ERROR_UNEXPECTED;
        }
    }

    
    mSet = mWrapper->GetSet();

    mState = READY_TO_CALL;
    return NS_OK;
}

void
XPCCallContext::SystemIsBeingShutDown()
{
    
    
    
    NS_WARNING("Shutting Down XPConnect even through there is a live XPCCallContext");
    mXPCContext = nullptr;
    mState = SYSTEM_SHUTDOWN;
    if (mPrevCallContext)
        mPrevCallContext->SystemIsBeingShutDown();
}

XPCCallContext::~XPCCallContext()
{
    

    bool shouldReleaseXPC = false;

    if (mXPCContext) {
        mXPCContext->SetCallingLangType(mPrevCallerLanguage);

        DebugOnly<XPCCallContext*> old = XPCJSRuntime::Get()->SetCallContext(mPrevCallContext);
        NS_ASSERTION(old == this, "bad pop from per thread data");

        shouldReleaseXPC = mPrevCallContext == nullptr;
    }

    
    if (mJSContext && mCallerLanguage == NATIVE_CALLER)
        JS_EndRequest(mJSContext);

    if (mContextPopRequired) {
        XPCJSContextStack* stack = XPCJSRuntime::Get()->GetJSContextStack();
        NS_ASSERTION(stack, "bad!");
        if (stack) {
            DebugOnly<JSContext*> poppedCX = stack->Pop();
            NS_ASSERTION(poppedCX == mJSContext, "bad pop");
        }
    }

    if (mJSContext) {
        if (mDestroyJSContextInDestructor) {
#ifdef DEBUG_xpc_hacker
            printf("!xpc - doing deferred destruction of JSContext @ %p\n",
                   mJSContext);
#endif
            NS_ASSERTION(!XPCJSRuntime::Get()->GetJSContextStack()->
                         DEBUG_StackHasJSContext(mJSContext),
                         "JSContext still in threadjscontextstack!");

            JS_DestroyContext(mJSContext);
        }
    }

#ifdef DEBUG
    for (uint32_t i = 0; i < XPCCCX_STRING_CACHE_SIZE; ++i) {
        NS_ASSERTION(!mScratchStrings[i].mInUse, "Uh, string wrapper still in use!");
    }
#endif

    if (shouldReleaseXPC && mXPC)
        NS_RELEASE(mXPC);
}

XPCReadableJSStringWrapper *
XPCCallContext::NewStringWrapper(const PRUnichar *str, uint32_t len)
{
    for (uint32_t i = 0; i < XPCCCX_STRING_CACHE_SIZE; ++i) {
        StringWrapperEntry& ent = mScratchStrings[i];

        if (!ent.mInUse) {
            ent.mInUse = true;

            

            return new (ent.mString.addr()) XPCReadableJSStringWrapper(str, len);
        }
    }

    

    return new XPCReadableJSStringWrapper(str, len);
}

void
XPCCallContext::DeleteString(nsAString *string)
{
    for (uint32_t i = 0; i < XPCCCX_STRING_CACHE_SIZE; ++i) {
        StringWrapperEntry& ent = mScratchStrings[i];
        if (string == ent.mString.addr()) {
            
            

            ent.mInUse = false;
            ent.mString.addr()->~XPCReadableJSStringWrapper();

            return;
        }
    }

    
    
    delete string;
}


NS_IMETHODIMP
XPCCallContext::GetCallee(nsISupports * *aCallee)
{
    nsISupports* temp = mWrapper ? mWrapper->GetIdentityObject() : nullptr;
    NS_IF_ADDREF(temp);
    *aCallee = temp;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetCalleeMethodIndex(uint16_t *aCalleeMethodIndex)
{
    *aCalleeMethodIndex = mMethodIndex;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetCalleeWrapper(nsIXPConnectWrappedNative * *aCalleeWrapper)
{
    nsIXPConnectWrappedNative* temp = mWrapper;
    NS_IF_ADDREF(temp);
    *aCalleeWrapper = temp;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetCalleeInterface(nsIInterfaceInfo * *aCalleeInterface)
{
    nsIInterfaceInfo* temp = mInterface->GetInterfaceInfo();
    NS_IF_ADDREF(temp);
    *aCalleeInterface = temp;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetCalleeClassInfo(nsIClassInfo * *aCalleeClassInfo)
{
    nsIClassInfo* temp = mWrapper ? mWrapper->GetClassInfo() : nullptr;
    NS_IF_ADDREF(temp);
    *aCalleeClassInfo = temp;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetJSContext(JSContext * *aJSContext)
{
    JS_AbortIfWrongThread(JS_GetRuntime(mJSContext));
    *aJSContext = mJSContext;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetArgc(uint32_t *aArgc)
{
    *aArgc = (uint32_t) mArgc;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetArgvPtr(jsval * *aArgvPtr)
{
    *aArgvPtr = mArgv;
    return NS_OK;
}

NS_IMETHODIMP
XPCCallContext::GetPreviousCallContext(nsAXPCNativeCallContext **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = GetPrevCallContext();
  return NS_OK;
}

NS_IMETHODIMP
XPCCallContext::GetLanguage(uint16_t *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = GetCallerLanguage();
  return NS_OK;
}

#ifdef DEBUG

void
XPCLazyCallContext::AssertContextIsTopOfStack(JSContext* cx)
{
    XPCJSContextStack* stack = XPCJSRuntime::Get()->GetJSContextStack();

    JSContext *topJSContext = stack->Peek();
    NS_ASSERTION(cx == topJSContext, "wrong context on XPCJSContextStack!");
}
#endif
