








#include "mozilla/Util.h"
#include "AccessCheck.h"

#include "xpcprivate.h"

using namespace mozilla;
using namespace xpc;
using namespace JS;

#define IS_TEAROFF_CLASS(clazz) ((clazz) == &XPC_WN_Tearoff_JSClass)

XPCCallContext::XPCCallContext(XPCContext::LangType callerLanguage,
                               JSContext* cx       ,
                               HandleObject obj    ,
                               HandleObject funobj ,
                               HandleId name       ,
                               unsigned argc       ,
                               jsval *argv         ,
                               jsval *rval         )
    :   mPusher(cx),
        mState(INIT_FAILED),
        mXPC(nsXPConnect::XPConnect()),
        mXPCContext(nullptr),
        mJSContext(cx),
        mCallerLanguage(callerLanguage),
        mFlattenedJSObject(cx),
        mWrapper(nullptr),
        mTearOff(nullptr),
        mName(cx)
{
    MOZ_ASSERT(cx);

    NS_ASSERTION(mJSContext, "No JSContext supplied to XPCCallContext");
    if (!mXPC)
        return;

    mXPCContext = XPCContext::GetXPCContext(mJSContext);
    mPrevCallerLanguage = mXPCContext->SetCallingLangType(mCallerLanguage);

    
    mPrevCallContext = XPCJSRuntime::Get()->SetCallContext(this);

    
    
    if (!mPrevCallContext)
        NS_ADDREF(mXPC);

    mState = HAVE_CONTEXT;

    if (!obj)
        return;

    mMethodIndex = 0xDEAD;

    mState = HAVE_OBJECT;

    mTearOff = nullptr;

    
    
    JSObject *unwrapped = js::CheckedUnwrap(obj,  false);
    if (!unwrapped) {
        mWrapper = UnwrapThisIfAllowed(obj, funobj, argc);
        if (!mWrapper) {
            JS_ReportError(mJSContext, "Permission denied to call method on |this|");
            mState = INIT_FAILED;
            return;
        }
    } else {
        js::Class *clasp = js::GetObjectClass(unwrapped);
        if (IS_WN_CLASS(clasp)) {
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
        NS_ABORT_IF_FALSE(!mFlattenedJSObject, "What object do we have?");
    }

    if (!JSID_IS_VOID(name))
        SetName(name);

    if (argc != NO_ARGS)
        SetArgsAndResultPtr(argc, argv, rval);

    CHECK_STATE(HAVE_OBJECT);
}


JSContext *
XPCCallContext::GetDefaultJSContext()
{
    
    
    
    
    
    
    

    XPCJSContextStack* stack = XPCJSRuntime::Get()->GetJSContextStack();
    JSContext *topJSContext = stack->Peek();

    return topJSContext ? topJSContext : stack->GetSafeJSContext();
}

void
XPCCallContext::SetName(jsid name)
{
    CHECK_STATE(HAVE_OBJECT);

    mName = name;

    if (mTearOff) {
        mSet = nullptr;
        mInterface = mTearOff->GetInterface();
        mMember = mInterface->FindMember(mName);
        mStaticMemberIsLocal = true;
        if (mMember && !mMember->IsConstant())
            mMethodIndex = mMember->GetIndex();
    } else {
        mSet = mWrapper ? mWrapper->GetSet() : nullptr;

        if (mSet &&
            mSet->FindMember(mName, &mMember, &mInterface,
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
        mTearOff = mWrapper->FindTearOff(mInterface, false, &rv);
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

XPCWrappedNative*
XPCCallContext::UnwrapThisIfAllowed(HandleObject obj, HandleObject fun, unsigned argc)
{
    
    MOZ_ASSERT(!js::CheckedUnwrap(obj));
    MOZ_ASSERT(js::IsObjectInContextCompartment(obj, mJSContext));

    
    if (!fun)
        return nullptr;

    
    
    
    
    
    
    
    

    
    
    MOZ_ASSERT(js::IsWrapper(obj));
    RootedObject unwrapped(mJSContext, js::UncheckedUnwrap(obj,  false));
    MOZ_ASSERT(unwrapped == JS_ObjectToInnerObject(mJSContext, js::Wrapper::wrappedObject(obj)));

    
    if (!IS_WN_REFLECTOR(unwrapped))
        return nullptr;
    XPCWrappedNative *wn = XPCWrappedNative::Get(unwrapped);

    
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

