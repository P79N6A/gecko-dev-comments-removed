







#include "mozilla/Util.h"
#include "AccessCheck.h"

#include "xpcprivate.h"

using namespace mozilla;
using namespace xpc;

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
        mCallerLanguage(callerLanguage),
        mFlattenedJSObject(nullptr),
        mWrapper(nullptr),
        mTearOff(nullptr)
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

#define IS_TEAROFF_CLASS(clazz) ((clazz) == &XPC_WN_Tearoff_JSClass)

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
        
        
        JSObject *unwrapped = js::UnwrapObjectChecked(obj,  false);
        if (!unwrapped) {
            mWrapper = UnwrapThisIfAllowed(obj, funobj, argc);
            if (!mWrapper) {
                JS_ReportError(mJSContext, "Permission denied to call method on |this|");
                mState = INIT_FAILED;
                return;
            }
        } else {
            js::Class *clasp = js::GetObjectClass(unwrapped);
            if (IS_WRAPPER_CLASS(clasp)) {
                if (IS_SLIM_WRAPPER_OBJECT(unwrapped))
                    mFlattenedJSObject = unwrapped;
                else
                    mWrapper = XPCWrappedNative::Get(unwrapped);
            } else if (IS_TEAROFF_CLASS(clasp)) {
                mTearOff = (XPCWrappedNativeTearOff*)js::GetObjectPrivate(unwrapped);
                mWrapper = XPCWrappedNative::Get(js::GetObjectParent(unwrapped));
            }
        }
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

    if (shouldReleaseXPC && mXPC)
        NS_RELEASE(mXPC);
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

XPCWrappedNative*
XPCCallContext::UnwrapThisIfAllowed(JSObject *object, JSObject *fun, unsigned argc)
{
    JS::Rooted<JSObject *> obj(mJSContext, object);
    
    MOZ_ASSERT(!js::UnwrapObjectChecked(obj));
    MOZ_ASSERT(js::IsObjectInContextCompartment(obj, mJSContext));

    
    if (!fun)
        return nullptr;

    
    
    
    
    
    
    
    

    
    
    MOZ_ASSERT(js::IsWrapper(obj));
    JSObject *unwrapped = js::UnwrapObject(obj,  false);
    MOZ_ASSERT(unwrapped == JS_ObjectToInnerObject(mJSContext, js::Wrapper::wrappedObject(obj)));

    
    MOZ_ASSERT(!IS_SLIM_WRAPPER(unwrapped), "security wrapping morphs slim wrappers");
    if (!IS_WRAPPER_CLASS(js::GetObjectClass(unwrapped)))
        return nullptr;
    XPCWrappedNative *wn = (XPCWrappedNative*)js::GetObjectPrivate(unwrapped);

    
    XPCNativeInterface *interface;
    XPCNativeMember *member;
    XPCNativeMember::GetCallInfo(fun, &interface, &member);

    
    
    
    if (!wn->HasInterfaceNoQI(*interface->GetIID()))
        return nullptr;

    
    
    
    
    bool set = argc && argc != NO_ARGS && member->IsWritableAttribute();
    js::Wrapper::Action act = set ? js::Wrapper::SET : js::Wrapper::GET;
    js::Wrapper *handler = js::Wrapper::wrapperHandler(obj);
    bool ignored;
    JS::Rooted<jsid> id(mJSContext, member->GetName());
    if (!handler->enter(mJSContext, obj, id, act, &ignored))
        return nullptr;

    
    return wn;
}

