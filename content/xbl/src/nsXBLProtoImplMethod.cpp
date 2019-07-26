




#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIScriptGlobalObject.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsXBLProtoImplMethod.h"
#include "nsIScriptContext.h"
#include "nsJSUtils.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "xpcpublic.h"
#include "nsXBLPrototypeBinding.h"

using namespace mozilla;

nsXBLProtoImplMethod::nsXBLProtoImplMethod(const PRUnichar* aName) :
  nsXBLProtoImplMember(aName), 
  mUncompiledMethod(BIT_UNCOMPILED)
{
  MOZ_COUNT_CTOR(nsXBLProtoImplMethod);
}

nsXBLProtoImplMethod::~nsXBLProtoImplMethod()
{
  MOZ_COUNT_DTOR(nsXBLProtoImplMethod);

  if (!IsCompiled()) {
    delete GetUncompiledMethod();
  }
}

void 
nsXBLProtoImplMethod::AppendBodyText(const nsAString& aText)
{
  NS_PRECONDITION(!IsCompiled(),
                  "Must not be compiled when accessing uncompiled method");

  nsXBLUncompiledMethod* uncompiledMethod = GetUncompiledMethod();
  if (!uncompiledMethod) {
    uncompiledMethod = new nsXBLUncompiledMethod();
    if (!uncompiledMethod)
      return;
    SetUncompiledMethod(uncompiledMethod);
  }

  uncompiledMethod->AppendBodyText(aText);
}

void 
nsXBLProtoImplMethod::AddParameter(const nsAString& aText)
{
  NS_PRECONDITION(!IsCompiled(),
                  "Must not be compiled when accessing uncompiled method");

  if (aText.IsEmpty()) {
    NS_WARNING("Empty name attribute in xbl:parameter!");
    return;
  }

  nsXBLUncompiledMethod* uncompiledMethod = GetUncompiledMethod();
  if (!uncompiledMethod) {
    uncompiledMethod = new nsXBLUncompiledMethod();
    if (!uncompiledMethod)
      return;
    SetUncompiledMethod(uncompiledMethod);
  }

  uncompiledMethod->AddParameter(aText);
}

void
nsXBLProtoImplMethod::SetLineNumber(uint32_t aLineNumber)
{
  NS_PRECONDITION(!IsCompiled(),
                  "Must not be compiled when accessing uncompiled method");

  nsXBLUncompiledMethod* uncompiledMethod = GetUncompiledMethod();
  if (!uncompiledMethod) {
    uncompiledMethod = new nsXBLUncompiledMethod();
    if (!uncompiledMethod)
      return;
    SetUncompiledMethod(uncompiledMethod);
  }

  uncompiledMethod->SetLineNumber(aLineNumber);
}

nsresult
nsXBLProtoImplMethod::InstallMember(JSContext* aCx,
                                    JSObject* aTargetClassObject)
{
  NS_PRECONDITION(IsCompiled(),
                  "Should not be installing an uncompiled method");
  MOZ_ASSERT(js::IsObjectInContextCompartment(aTargetClassObject, aCx));

  JSObject* globalObject = JS_GetGlobalForObject(aCx, aTargetClassObject);
  JSObject* scopeObject = xpc::GetXBLScope(aCx, globalObject);

  
  if (mJSMethodObject) {
    nsDependentString name(mName);

    
    JSAutoCompartment ac(aCx, scopeObject);
    JSObject * method = ::JS_CloneFunctionObject(aCx, mJSMethodObject, scopeObject);
    if (!method) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    JSAutoCompartment ac2(aCx, aTargetClassObject);
    if (!JS_WrapObject(aCx, &method) ||
        !::JS_DefineUCProperty(aCx, aTargetClassObject,
                               static_cast<const jschar*>(mName),
                               name.Length(), OBJECT_TO_JSVAL(method),
                               NULL, NULL, JSPROP_ENUMERATE)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return NS_OK;
}

nsresult 
nsXBLProtoImplMethod::CompileMember(nsIScriptContext* aContext, const nsCString& aClassStr,
                                    JSObject* aClassObject)
{
  NS_PRECONDITION(!IsCompiled(),
                  "Trying to compile an already-compiled method");
  NS_PRECONDITION(aClassObject,
                  "Must have class object to compile");

  nsXBLUncompiledMethod* uncompiledMethod = GetUncompiledMethod();

  
  if (!uncompiledMethod) {
    
    mJSMethodObject = nullptr;

    return NS_OK;
  }

  
  if (!mName) {
    delete uncompiledMethod;

    
    mJSMethodObject = nullptr;

    return NS_OK;
  }

  
  
  int32_t paramCount = uncompiledMethod->GetParameterCount();
  char** args = nullptr;
  if (paramCount > 0) {
    args = new char*[paramCount];
    if (!args)
      return NS_ERROR_OUT_OF_MEMORY;

    
    int32_t argPos = 0; 
    for (nsXBLParameter* curr = uncompiledMethod->mParameters; 
         curr; 
         curr = curr->mNext) {
      args[argPos] = curr->mName;
      argPos++;
    }
  }

  
  nsDependentString body;
  PRUnichar *bodyText = uncompiledMethod->mBodyText.GetText();
  if (bodyText)
    body.Rebind(bodyText);

  
  
  NS_ConvertUTF16toUTF8 cname(mName);
  nsAutoCString functionUri(aClassStr);
  int32_t hash = functionUri.RFindChar('#');
  if (hash != kNotFound) {
    functionUri.Truncate(hash);
  }

  JSObject* methodObject = nullptr;
  AutoPushJSContext cx(aContext->GetNativeContext());
  JSAutoRequest ar(cx);
  JSAutoCompartment ac(cx, aClassObject);
  JS::CompileOptions options(cx);
  options.setFileAndLine(functionUri.get(),
                         uncompiledMethod->mBodyText.GetLineNumber())
         .setVersion(JSVERSION_LATEST)
         .setUserBit(true); 
  js::RootedObject rootedNull(cx, nullptr); 
  nsresult rv = nsJSUtils::CompileFunction(cx, rootedNull, options, cname,
                                           paramCount,
                                           const_cast<const char**>(args),
                                           body, &methodObject);

  
  delete uncompiledMethod;
  delete [] args;
  if (NS_FAILED(rv)) {
    SetUncompiledMethod(nullptr);
    return rv;
  }

  mJSMethodObject = methodObject;

  return NS_OK;
}

void
nsXBLProtoImplMethod::Trace(TraceCallback aCallback, void *aClosure) const
{
  if (IsCompiled() && mJSMethodObject) {
    aCallback(mJSMethodObject, "mJSMethodObject", aClosure);
  }
}

nsresult
nsXBLProtoImplMethod::Read(nsIScriptContext* aContext,
                           nsIObjectInputStream* aStream)
{
  nsresult rv = XBL_DeserializeFunction(aContext, aStream, &mJSMethodObject);
  if (NS_FAILED(rv)) {
    SetUncompiledMethod(nullptr);
    return rv;
  }

#ifdef DEBUG
  mIsCompiled = true;
#endif

  return NS_OK;
}

nsresult
nsXBLProtoImplMethod::Write(nsIScriptContext* aContext,
                            nsIObjectOutputStream* aStream)
{
  if (mJSMethodObject) {
    nsresult rv = aStream->Write8(XBLBinding_Serialize_Method);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aStream->WriteWStringZ(mName);
    NS_ENSURE_SUCCESS(rv, rv);

    return XBL_SerializeFunction(aContext, aStream, mJSMethodObject);
  }

  return NS_OK;
}

nsresult
nsXBLProtoImplAnonymousMethod::Execute(nsIContent* aBoundElement)
{
  NS_PRECONDITION(IsCompiled(), "Can't execute uncompiled method");
  
  if (!mJSMethodObject) {
    
    return NS_OK;
  }

  
  
  nsIDocument* document = aBoundElement->OwnerDoc();

  nsIScriptGlobalObject* global = document->GetScriptGlobalObject();
  if (!global) {
    return NS_OK;
  }

  nsCOMPtr<nsIScriptContext> context = global->GetContext();
  if (!context) {
    return NS_OK;
  }

  nsAutoMicroTask mt;

  AutoPushJSContext cx(context->GetNativeContext());

  JSObject* globalObject = global->GetGlobalJSObject();

  nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
  jsval v;
  nsresult rv =
    nsContentUtils::WrapNative(cx, globalObject, aBoundElement, &v,
                               getter_AddRefs(wrapper));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  nsCxPusher pusher;
  NS_ENSURE_STATE(pusher.Push(aBoundElement));
  MOZ_ASSERT(cx == nsContentUtils::GetCurrentJSContext());

  JSObject* thisObject = JSVAL_TO_OBJECT(v);
  JSObject* scopeObject = xpc::GetXBLScope(cx, globalObject);

  JSAutoRequest ar(cx);
  JSAutoCompartment ac(cx, scopeObject);
  if (!JS_WrapObject(cx, &thisObject))
      return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  JSObject* method = ::JS_CloneFunctionObject(cx, mJSMethodObject, thisObject);
  if (!method)
    return NS_ERROR_OUT_OF_MEMORY;

  

  
  rv = nsContentUtils::GetSecurityManager()->CheckFunctionAccess(cx, method,
                                                                 thisObject);

  JSBool ok = JS_TRUE;
  if (NS_SUCCEEDED(rv)) {
    jsval retval;
    ok = ::JS_CallFunctionValue(cx, thisObject, OBJECT_TO_JSVAL(method),
                                0 , nullptr , &retval);
  }

  if (!ok) {
    
    
    
    
    JSBool saved = JS_SaveFrameChain(cx);
    JS_ReportPendingException(cx);
    if (saved)
        JS_RestoreFrameChain(cx);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsXBLProtoImplAnonymousMethod::Write(nsIScriptContext* aContext,
                                     nsIObjectOutputStream* aStream,
                                     XBLBindingSerializeDetails aType)
{
  if (mJSMethodObject) {
    nsresult rv = aStream->Write8(aType);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = XBL_SerializeFunction(aContext, aStream, mJSMethodObject);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
