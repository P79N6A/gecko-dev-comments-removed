




#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIGlobalObject.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsXBLProtoImplMethod.h"
#include "nsJSUtils.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "xpcpublic.h"
#include "nsXBLPrototypeBinding.h"
#include "mozilla/dom/ScriptSettings.h"

using namespace mozilla;

nsXBLProtoImplMethod::nsXBLProtoImplMethod(const char16_t* aName) :
  nsXBLProtoImplMember(aName),
  mMethod()
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
                                    JS::Handle<JSObject*> aTargetClassObject)
{
  NS_PRECONDITION(IsCompiled(),
                  "Should not be installing an uncompiled method");
  MOZ_ASSERT(js::IsObjectInContextCompartment(aTargetClassObject, aCx));

  JS::Rooted<JSObject*> globalObject(aCx, JS_GetGlobalForObject(aCx, aTargetClassObject));
  MOZ_ASSERT(xpc::IsInContentXBLScope(globalObject) ||
             xpc::IsInAddonScope(globalObject) ||
             globalObject == xpc::GetXBLScope(aCx, globalObject));

  JS::Rooted<JSObject*> jsMethodObject(aCx, GetCompiledMethod());
  if (jsMethodObject) {
    nsDependentString name(mName);

    JS::Rooted<JSObject*> method(aCx, JS_CloneFunctionObject(aCx, jsMethodObject, globalObject));
    NS_ENSURE_TRUE(method, NS_ERROR_OUT_OF_MEMORY);

    if (!::JS_DefineUCProperty(aCx, aTargetClassObject,
                               static_cast<const char16_t*>(mName),
                               name.Length(), method,
                               JSPROP_ENUMERATE)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return NS_OK;
}

nsresult
nsXBLProtoImplMethod::CompileMember(const nsCString& aClassStr,
                                    JS::Handle<JSObject*> aClassObject)
{
  AssertInCompilationScope();
  NS_PRECONDITION(!IsCompiled(),
                  "Trying to compile an already-compiled method");
  NS_PRECONDITION(aClassObject,
                  "Must have class object to compile");

  nsXBLUncompiledMethod* uncompiledMethod = GetUncompiledMethod();

  
  if (!uncompiledMethod) {
    
    SetCompiledMethod(nullptr);

    return NS_OK;
  }

  
  if (!mName) {
    delete uncompiledMethod;

    
    SetCompiledMethod(nullptr);

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
  char16_t *bodyText = uncompiledMethod->mBodyText.GetText();
  if (bodyText)
    body.Rebind(bodyText);

  
  
  NS_ConvertUTF16toUTF8 cname(mName);
  nsAutoCString functionUri(aClassStr);
  int32_t hash = functionUri.RFindChar('#');
  if (hash != kNotFound) {
    functionUri.Truncate(hash);
  }

  AutoJSContext cx;
  JSAutoCompartment ac(cx, aClassObject);
  JS::CompileOptions options(cx);
  options.setFileAndLine(functionUri.get(),
                         uncompiledMethod->mBodyText.GetLineNumber())
         .setVersion(JSVERSION_LATEST);
  JS::Rooted<JSObject*> methodObject(cx);
  nsresult rv = nsJSUtils::CompileFunction(cx, JS::NullPtr(), options, cname,
                                           paramCount,
                                           const_cast<const char**>(args),
                                           body, methodObject.address());

  
  delete uncompiledMethod;
  delete [] args;
  if (NS_FAILED(rv)) {
    SetUncompiledMethod(nullptr);
    return rv;
  }

  SetCompiledMethod(methodObject);

  return NS_OK;
}

void
nsXBLProtoImplMethod::Trace(const TraceCallbacks& aCallbacks, void *aClosure)
{
  if (IsCompiled() && GetCompiledMethodPreserveColor()) {
    aCallbacks.Trace(&mMethod.AsHeapObject(), "mMethod", aClosure);
  }
}

nsresult
nsXBLProtoImplMethod::Read(nsIObjectInputStream* aStream)
{
  AssertInCompilationScope();
  MOZ_ASSERT(!IsCompiled() && !GetUncompiledMethod());

  AutoJSContext cx;
  JS::Rooted<JSObject*> methodObject(cx);
  nsresult rv = XBL_DeserializeFunction(aStream, &methodObject);
  if (NS_FAILED(rv)) {
    SetUncompiledMethod(nullptr);
    return rv;
  }

  SetCompiledMethod(methodObject);

  return NS_OK;
}

nsresult
nsXBLProtoImplMethod::Write(nsIObjectOutputStream* aStream)
{
  AssertInCompilationScope();
  MOZ_ASSERT(IsCompiled());
  if (GetCompiledMethodPreserveColor()) {
    nsresult rv = aStream->Write8(XBLBinding_Serialize_Method);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aStream->WriteWStringZ(mName);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    JS::Handle<JSObject*> method =
      JS::Handle<JSObject*>::fromMarkedLocation(mMethod.AsHeapObject().address());
    return XBL_SerializeFunction(aStream, method);
  }

  return NS_OK;
}

nsresult
nsXBLProtoImplAnonymousMethod::Execute(nsIContent* aBoundElement, JSAddonId* aAddonId)
{
  NS_PRECONDITION(IsCompiled(), "Can't execute uncompiled method");

  if (!GetCompiledMethod()) {
    
    return NS_OK;
  }

  
  
  nsIDocument* document = aBoundElement->OwnerDoc();

  nsCOMPtr<nsIGlobalObject> global =
    do_QueryInterface(document->GetInnerWindow());
  if (!global) {
    return NS_OK;
  }

  nsAutoMicroTask mt;

  
  
  dom::AutoEntryScript aes(global);
  JSContext* cx = aes.cx();

  JS::Rooted<JSObject*> globalObject(cx, global->GetGlobalJSObject());

  JS::Rooted<JS::Value> v(cx);
  nsresult rv = nsContentUtils::WrapNative(cx, aBoundElement, &v);
  NS_ENSURE_SUCCESS(rv, rv);

  JS::Rooted<JSObject*> thisObject(cx, &v.toObject());
  JS::Rooted<JSObject*> scopeObject(cx, xpc::GetScopeForXBLExecution(cx, globalObject, aAddonId));
  NS_ENSURE_TRUE(scopeObject, NS_ERROR_OUT_OF_MEMORY);

  JSAutoCompartment ac(cx, scopeObject);
  if (!JS_WrapObject(cx, &thisObject))
      return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  JS::Rooted<JSObject*> jsMethodObject(cx, GetCompiledMethod());
  JS::Rooted<JSObject*> method(cx, ::JS_CloneFunctionObject(cx, jsMethodObject, thisObject));
  if (!method)
    return NS_ERROR_OUT_OF_MEMORY;

  

  
  bool scriptAllowed = nsContentUtils::GetSecurityManager()->
                         ScriptAllowed(js::GetGlobalForObjectCrossCompartment(method));

  bool ok = true;
  if (scriptAllowed) {
    JS::Rooted<JS::Value> retval(cx);
    JS::Rooted<JS::Value> methodVal(cx, JS::ObjectValue(*method));
    ok = ::JS::Call(cx, thisObject, methodVal, JS::HandleValueArray::empty(), &retval);
  }

  if (!ok) {
    
    
    
    
    nsJSUtils::ReportPendingException(cx);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsXBLProtoImplAnonymousMethod::Write(nsIObjectOutputStream* aStream,
                                     XBLBindingSerializeDetails aType)
{
  AssertInCompilationScope();
  MOZ_ASSERT(IsCompiled());
  if (GetCompiledMethodPreserveColor()) {
    nsresult rv = aStream->Write8(aType);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aStream->WriteWStringZ(mName);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    JS::Handle<JSObject*> method =
      JS::Handle<JSObject*>::fromMarkedLocation(mMethod.AsHeapObject().address());
    rv = XBL_SerializeFunction(aStream, method);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
