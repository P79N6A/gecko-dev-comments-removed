





































#include "nsXBLProtoImpl.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIScriptContext.h"
#include "nsIXPConnect.h"
#include "nsIServiceManager.h"
#include "nsIXBLDocumentInfo.h"
#include "nsIDOMNode.h"
#include "nsXBLPrototypeBinding.h"

nsresult
nsXBLProtoImpl::InstallImplementation(nsXBLPrototypeBinding* aBinding, nsIContent* aBoundElement)
{
  
  
  
  
  if (!mMembers && !mFields)  
    return NS_OK; 

  
  
  nsIDocument* document = aBoundElement->GetOwnerDoc();
  if (!document) return NS_OK;

  nsIScriptGlobalObject *global = document->GetScopeObject();
  if (!global) return NS_OK;

  nsCOMPtr<nsIScriptContext> context = global->GetContext();
  if (!context) return NS_OK;

  
  
  
  
  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  void * targetClassObject = nsnull;
  nsresult rv = InitTargetObjects(aBinding, context, aBoundElement,
                                  getter_AddRefs(holder), &targetClassObject);
  NS_ENSURE_SUCCESS(rv, rv); 

  JSObject * targetScriptObject;
  holder->GetJSObject(&targetScriptObject);

  JSContext *cx = (JSContext *)context->GetNativeContext();
  
  JSVersion oldVersion = ::JS_SetVersion(cx, JSVERSION_LATEST);
  
  
  for (nsXBLProtoImplMember* curr = mMembers;
       curr;
       curr = curr->GetNext())
    curr->InstallMember(context, aBoundElement, targetScriptObject,
                        targetClassObject, mClassName);

  ::JS_SetVersion(cx, oldVersion);
  return NS_OK;
}

nsresult 
nsXBLProtoImpl::InitTargetObjects(nsXBLPrototypeBinding* aBinding,
                                  nsIScriptContext* aContext, 
                                  nsIContent* aBoundElement, 
                                  nsIXPConnectJSObjectHolder** aScriptObjectHolder, 
                                  void** aTargetClassObject)
{
  nsresult rv = NS_OK;
  *aScriptObjectHolder = nsnull;
  
  if (!mClassObject) {
    rv = CompilePrototypeMembers(aBinding); 
                                 
                                 
    if (NS_FAILED(rv))
      return rv;

    if (!mClassObject)
      return NS_OK; 
  }

  nsIDocument *ownerDoc = aBoundElement->GetOwnerDoc();
  nsIScriptGlobalObject *sgo;

  if (!ownerDoc || !(sgo = ownerDoc->GetScopeObject())) {
    return NS_ERROR_UNEXPECTED;
  }

  
  
  JSContext* jscontext = (JSContext*)aContext->GetNativeContext();
  JSObject* global = sgo->GetGlobalJSObject();
  nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
  jsval v;
  rv = nsContentUtils::WrapNative(jscontext, global, aBoundElement, &v,
                                  getter_AddRefs(wrapper));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = aBinding->InitClass(mClassName, jscontext, global, JSVAL_TO_OBJECT(v),
                           aTargetClassObject);
  if (NS_FAILED(rv))
    return rv;

  nsContentUtils::PreserveWrapper(aBoundElement, aBoundElement);

  wrapper.swap(*aScriptObjectHolder);
  
  return rv;
}

nsresult
nsXBLProtoImpl::CompilePrototypeMembers(nsXBLPrototypeBinding* aBinding)
{
  
  
  
  nsCOMPtr<nsIScriptGlobalObjectOwner> globalOwner(
      do_QueryInterface(aBinding->XBLDocumentInfo()));
  nsIScriptGlobalObject* globalObject = globalOwner->GetScriptGlobalObject();
  NS_ENSURE_TRUE(globalObject, NS_ERROR_UNEXPECTED);

  nsIScriptContext *context = globalObject->GetContext();
  NS_ENSURE_TRUE(context, NS_ERROR_OUT_OF_MEMORY);

  JSContext *cx = (JSContext *)context->GetNativeContext();
  JSObject *global = globalObject->GetGlobalJSObject();
  

  void* classObject;
  nsresult rv = aBinding->InitClass(mClassName, cx, global, global,
                                    &classObject);
  if (NS_FAILED(rv))
    return rv;

  mClassObject = (JSObject*) classObject;
  if (!mClassObject)
    return NS_ERROR_FAILURE;

  
  JSVersion oldVersion = ::JS_SetVersion(cx, JSVERSION_LATEST);

  
  
  for (nsXBLProtoImplMember* curr = mMembers;
       curr;
       curr = curr->GetNext()) {
    nsresult rv = curr->CompileMember(context, mClassName, mClassObject);
    if (NS_FAILED(rv)) {
      DestroyMembers();
      ::JS_SetVersion(cx, oldVersion);
      return rv;
    }
  }

  ::JS_SetVersion(cx, oldVersion);
  return NS_OK;
}

void
nsXBLProtoImpl::Trace(TraceCallback aCallback, void *aClosure) const
{
  
  
  
  if (!mClassObject) {
    return;
  }

  nsXBLProtoImplMember *member;
  for (member = mMembers; member; member = member->GetNext()) {
    member->Trace(aCallback, aClosure);
  }
}

void
nsXBLProtoImpl::UnlinkJSObjects()
{
  if (mClassObject) {
    DestroyMembers();
  }
}

nsXBLProtoImplField*
nsXBLProtoImpl::FindField(const nsString& aFieldName) const
{
  for (nsXBLProtoImplField* f = mFields; f; f = f->GetNext()) {
    if (aFieldName.Equals(f->GetName())) {
      return f;
    }
  }

  return nsnull;
}

PRBool
nsXBLProtoImpl::ResolveAllFields(JSContext *cx, JSObject *obj) const
{
  
  JSVersion oldVersion = ::JS_SetVersion(cx, JSVERSION_LATEST);  
  for (nsXBLProtoImplField* f = mFields; f; f = f->GetNext()) {
    
    
    
    nsDependentString name(f->GetName());
    jsval dummy;
    if (!::JS_LookupUCProperty(cx, obj,
                               reinterpret_cast<const jschar*>(name.get()),
                               name.Length(), &dummy)) {
      ::JS_SetVersion(cx, oldVersion);
      return PR_FALSE;
    }
  }

  ::JS_SetVersion(cx, oldVersion);
  return PR_TRUE;
}

void
nsXBLProtoImpl::UndefineFields(JSContext *cx, JSObject *obj) const
{
  JSAutoRequest ar(cx);
  for (nsXBLProtoImplField* f = mFields; f; f = f->GetNext()) {
    nsDependentString name(f->GetName());

    const jschar* s = reinterpret_cast<const jschar*>(name.get());
    JSBool hasProp;
    if (::JS_AlreadyHasOwnUCProperty(cx, obj, s, name.Length(), &hasProp) &&
        hasProp) {
      jsval dummy;
      ::JS_DeleteUCProperty2(cx, obj, s, name.Length(), &dummy);
    }
  }
}

void
nsXBLProtoImpl::DestroyMembers()
{
  NS_ASSERTION(mClassObject, "This should never be called when there is no class object");

  delete mMembers;
  mMembers = nsnull;
  mConstructor = nsnull;
  mDestructor = nsnull;
}

nsresult
NS_NewXBLProtoImpl(nsXBLPrototypeBinding* aBinding, 
                   const PRUnichar* aClassName, 
                   nsXBLProtoImpl** aResult)
{
  nsXBLProtoImpl* impl = new nsXBLProtoImpl();
  if (!impl)
    return NS_ERROR_OUT_OF_MEMORY;
  if (aClassName)
    impl->mClassName.AssignWithConversion(aClassName);
  else
    aBinding->BindingURI()->GetSpec(impl->mClassName);
  aBinding->SetImplementation(impl);
  *aResult = impl;

  return NS_OK;
}

