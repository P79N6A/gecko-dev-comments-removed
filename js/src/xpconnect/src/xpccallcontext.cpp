









































#include "xpcprivate.h"

XPCCallContext::XPCCallContext(XPCContext::LangType callerLanguage,
                               JSContext* cx    ,
                               JSObject* obj    ,
                               JSObject* funobj ,
                               jsval name       ,
                               uintN argc       ,
                               jsval *argv      ,
                               jsval *rval      )
    :   mState(INIT_FAILED),
        mXPC(nsXPConnect::GetXPConnect()),
        mThreadData(nsnull),
        mXPCContext(nsnull),
        mJSContext(cx),
        mContextPopRequired(JS_FALSE),
        mDestroyJSContextInDestructor(JS_FALSE),
        mCallerLanguage(callerLanguage),
        mCallee(nsnull)
{
    if(!mXPC)
        return;

    NS_ADDREF(mXPC);

    if(!(mThreadData = XPCPerThreadData::GetData()))
        return;

    XPCJSContextStack* stack = mThreadData->GetJSContextStack();
    JSContext* topJSContext;

    if(!stack || NS_FAILED(stack->Peek(&topJSContext)))
    {
        NS_ERROR("bad!");
        mJSContext = nsnull;
        return;
    }

    if(!mJSContext)
    {
        
        
        
        
        
        
        

        if(topJSContext)
            mJSContext = topJSContext;
        else if(NS_FAILED(stack->GetSafeJSContext(&mJSContext)) || !mJSContext)
            return;
    }

    
    

    if(mCallerLanguage == NATIVE_CALLER)
        JS_BeginRequest(mJSContext);

    if(topJSContext != mJSContext)
    {
        if(NS_FAILED(stack->Push(mJSContext)))
        {
            NS_ERROR("bad!");
            return;
        }
        mContextPopRequired = JS_TRUE;
    }

    
    
    
    mXPCContext = mThreadData->GetRecentXPCContext(mJSContext);

    if(!mXPCContext)
    {
        if(!(mXPCContext = nsXPConnect::GetContext(mJSContext, mXPC)))
            return;

        
        mThreadData->SetRecentContext(mJSContext, mXPCContext);
    }

    mPrevCallerLanguage = mXPCContext->SetCallingLangType(mCallerLanguage);

    
    mPrevCallContext = mThreadData->SetCallContext(this);

    mState = HAVE_CONTEXT;

    if(!obj)
        return;

    mMethodIndex = 0xDEAD;
    mOperandJSObject = obj;

    mState = HAVE_OBJECT;

    mTearOff = nsnull;
    mWrapper = XPCWrappedNative::GetWrappedNativeOfJSObject(mJSContext, obj,
                                                            funobj,
                                                            &mCurrentJSObject,
                                                            &mTearOff);
    if(!mWrapper)
        return;

    DEBUG_CheckWrapperThreadSafety(mWrapper);

    mFlattenedJSObject = mWrapper->GetFlatJSObject();

    if(mTearOff)
        mScriptableInfo = nsnull;
    else
        mScriptableInfo = mWrapper->GetScriptableInfo();

    if(name)
        SetName(name);

    if(argc != NO_ARGS)
        SetArgsAndResultPtr(argc, argv, rval);

    CHECK_STATE(HAVE_OBJECT);
}

void
XPCCallContext::SetName(jsval name)
{
    CHECK_STATE(HAVE_OBJECT);

    mName = name;

#ifdef XPC_IDISPATCH_SUPPORT
    mIDispatchMember = nsnull;
#endif
    if(mTearOff)
    {
        mSet = nsnull;
        mInterface = mTearOff->GetInterface();
        mMember = mInterface->FindMember(name);
        mStaticMemberIsLocal = JS_TRUE;
        if(mMember && !mMember->IsConstant())
            mMethodIndex = mMember->GetIndex();
    }
    else
    {
        mSet = mWrapper ? mWrapper->GetSet() : nsnull;

        if(mSet &&
           mSet->FindMember(name, &mMember, &mInterface,
                            mWrapper->HasProto() ?
                                mWrapper->GetProto()->GetSet() :
                                nsnull,
                            &mStaticMemberIsLocal))
        {
            if(mMember && !mMember->IsConstant())
                mMethodIndex = mMember->GetIndex();
        }
        else
        {
            mMember = nsnull;
            mInterface = nsnull;
            mStaticMemberIsLocal = JS_FALSE;
        }
    }

    mState = HAVE_NAME;
}

void
XPCCallContext::SetCallInfo(XPCNativeInterface* iface, XPCNativeMember* member,
                            JSBool isSetter)
{
    
    

    
    if(mTearOff && mTearOff->GetInterface() != iface)
        mTearOff = nsnull;

    mSet = nsnull;
    mInterface = iface;
    mMember = member;
    mMethodIndex = mMember->GetIndex() + (isSetter ? 1 : 0);
    mName = mMember->GetName();

    if(mState < HAVE_NAME)
        mState = HAVE_NAME;
#ifdef XPC_IDISPATCH_SUPPORT
    mIDispatchMember = nsnull;
#endif
}

void
XPCCallContext::SetArgsAndResultPtr(uintN argc,
                                    jsval *argv,
                                    jsval *rval)
{
    CHECK_STATE(HAVE_OBJECT);

    mArgc   = argc;
    mArgv   = argv;
    mRetVal = rval;

    mExceptionWasThrown = mReturnValueWasSet = JS_FALSE;
    mState = HAVE_ARGS;
}

nsresult
XPCCallContext::CanCallNow()
{
    nsresult rv;
    
    if(!HasInterfaceAndMember())
        return NS_ERROR_UNEXPECTED;
    if(mState < HAVE_ARGS)
        return NS_ERROR_UNEXPECTED;

    if(!mTearOff)
    {
        mTearOff = mWrapper->FindTearOff(*this, mInterface, JS_FALSE, &rv);
        if(!mTearOff || mTearOff->GetInterface() != mInterface)
        {
            mTearOff = nsnull;    
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
    mThreadData = nsnull;
    mXPCContext = nsnull;
    mState = SYSTEM_SHUTDOWN;
    if(mPrevCallContext)
        mPrevCallContext->SystemIsBeingShutDown();
}

XPCCallContext::~XPCCallContext()
{
    NS_ASSERTION(mRefCnt == 0, "Someone is holding a bad reference to a XPCCallContext");

    

    if(mXPCContext)
    {
        mXPCContext->SetCallingLangType(mPrevCallerLanguage);

#ifdef DEBUG
        XPCCallContext* old = mThreadData->SetCallContext(mPrevCallContext);
        NS_ASSERTION(old == this, "bad pop from per thread data");
#else
        (void) mThreadData->SetCallContext(mPrevCallContext);
#endif
    }

    if(mContextPopRequired)
    {
        XPCJSContextStack* stack = mThreadData->GetJSContextStack();
        NS_ASSERTION(stack, "bad!");
        if(stack)
        {
#ifdef DEBUG
            JSContext* poppedCX;
            nsresult rv = stack->Pop(&poppedCX);
            NS_ASSERTION(NS_SUCCEEDED(rv) && poppedCX == mJSContext, "bad pop");
#else
            (void) stack->Pop(nsnull);
#endif
        }
    }

    if(mJSContext)
    {
        if(mCallerLanguage == NATIVE_CALLER)
            JS_EndRequest(mJSContext);
        
        if(mDestroyJSContextInDestructor)
        {
#ifdef DEBUG_xpc_hacker
            printf("!xpc - doing deferred destruction of JSContext @ %0x\n", 
                   mJSContext);
#endif
            NS_ASSERTION(!mThreadData->GetJSContextStack() || 
                         !mThreadData->GetJSContextStack()->
                            DEBUG_StackHasJSContext(mJSContext),
                         "JSContext still in threadjscontextstack!");
        
            JS_DestroyContext(mJSContext);
            mXPC->SyncJSContexts();
        }
        else
        {
            
            
            
            if (!mJSContext->fp)
                JS_ClearNewbornRoots(mJSContext);
        }
    }

    NS_IF_RELEASE(mXPC);
}

NS_IMPL_QUERY_INTERFACE1(XPCCallContext, nsIXPCNativeCallContext)
NS_IMPL_ADDREF(XPCCallContext)

NS_IMETHODIMP_(nsrefcnt)
XPCCallContext::Release(void)
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  NS_ASSERT_OWNINGTHREAD(XPCCallContext);
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "XPCCallContext");
  
  return mRefCnt;
}


NS_IMETHODIMP
XPCCallContext::GetCallee(nsISupports * *aCallee)
{
    nsISupports* temp = mWrapper ? mWrapper->GetIdentityObject() : nsnull;
    NS_IF_ADDREF(temp);
    *aCallee = temp;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetCalleeMethodIndex(PRUint16 *aCalleeMethodIndex)
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
    nsIClassInfo* temp = mWrapper ? mWrapper->GetClassInfo() : nsnull;
    NS_IF_ADDREF(temp);
    *aCalleeClassInfo = temp;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetJSContext(JSContext * *aJSContext)
{
    *aJSContext = mJSContext;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetArgc(PRUint32 *aArgc)
{
    *aArgc = (PRUint32) mArgc;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetArgvPtr(jsval * *aArgvPtr)
{
    *aArgvPtr = mArgv;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetRetValPtr(jsval * *aRetValPtr)
{
    *aRetValPtr = mRetVal;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetExceptionWasThrown(PRBool *aExceptionWasThrown)
{
    *aExceptionWasThrown = mExceptionWasThrown;
    return NS_OK;
}
NS_IMETHODIMP
XPCCallContext::SetExceptionWasThrown(PRBool aExceptionWasThrown)
{
    mExceptionWasThrown = aExceptionWasThrown;
    return NS_OK;
}


NS_IMETHODIMP
XPCCallContext::GetReturnValueWasSet(PRBool *aReturnValueWasSet)
{
    *aReturnValueWasSet = mReturnValueWasSet;
    return NS_OK;
}
NS_IMETHODIMP
XPCCallContext::SetReturnValueWasSet(PRBool aReturnValueWasSet)
{
    mReturnValueWasSet = aReturnValueWasSet;
    return NS_OK;
}

#ifdef XPC_IDISPATCH_SUPPORT

void
XPCCallContext::SetIDispatchInfo(XPCNativeInterface* iface, 
                                 void * member)
{
    
    

    
    if(mTearOff && mTearOff->GetInterface() != iface)
        mTearOff = nsnull;

    mSet = nsnull;
    mInterface = iface;
    mMember = nsnull;
    mIDispatchMember = member;
    mName = NS_REINTERPRET_CAST(XPCDispInterface::Member*,member)->GetName();

    if(mState < HAVE_NAME)
        mState = HAVE_NAME;
}

#endif
