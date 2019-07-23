





































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

nsresult
nsXBLProtoImpl::InstallImplementation(nsXBLPrototypeBinding* aBinding, nsIContent* aBoundElement)
{
  
  
  
  
  if (!mMembers)  
    return NS_OK; 

  
  
  nsIDocument* document = aBoundElement->GetOwnerDoc();
  if (!document) return NS_OK;

  nsIScriptGlobalObject *global = document->GetScriptGlobalObject();
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

  
  for (nsXBLProtoImplMember* curr = mMembers;
       curr;
       curr = curr->GetNext())
    curr->InstallMember(context, aBoundElement, targetScriptObject,
                        targetClassObject, mClassName);
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

  if (!ownerDoc || !(sgo = ownerDoc->GetScriptGlobalObject())) {
    NS_ERROR("Can't find global object for bound content!");

    return NS_ERROR_UNEXPECTED;
  }

  
  
  JSContext* jscontext = (JSContext*)aContext->GetNativeContext();
  JSObject* global = sgo->GetGlobalJSObject();
  nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
  rv = nsContentUtils::XPConnect()->WrapNative(jscontext, global,
                                               aBoundElement,
                                               NS_GET_IID(nsISupports),
                                               getter_AddRefs(wrapper));
  NS_ENSURE_SUCCESS(rv, rv);
  JSObject * object = nsnull;
  rv = wrapper->GetJSObject(&object);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = aBinding->InitClass(mClassName, jscontext, global, object,
                           aTargetClassObject);
  if (NS_FAILED(rv))
    return rv;

  
  nsIDocument* doc = aBoundElement->GetOwnerDoc();
  if (doc) {
    nsCOMPtr<nsIXPConnectWrappedNative> nativeWrapper(do_QueryInterface(wrapper));
    if (nativeWrapper)
      doc->AddReference(aBoundElement, nativeWrapper);
  }

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
  JSObject *global = globalObject->GetGlobalJSObject();

  void* classObject;
  nsresult rv = aBinding->InitClass(mClassName,
                                    (JSContext *)context->GetNativeContext(),
                                    global, global,
                                    &classObject);
  if (NS_FAILED(rv))
    return rv;

  mClassObject = (JSObject*) classObject;
  if (!mClassObject)
    return NS_ERROR_FAILURE;

  
  
  for (nsXBLProtoImplMember* curr = mMembers;
       curr;
       curr = curr->GetNext()) {
    nsresult rv = curr->CompileMember(context, mClassName, mClassObject);
    if (NS_FAILED(rv)) {
      DestroyMembers(curr);
      return rv;
    }
  }
  return NS_OK;
}

void
nsXBLProtoImpl::Traverse(nsCycleCollectionTraversalCallback &cb) const
{
  
  
  
  if (!mClassObject) {
    return;
  }

  nsXBLProtoImplMember *member;
  for (member = mMembers; member; member = member->GetNext()) {
    member->Traverse(cb);
  }
}

void
nsXBLProtoImpl::DestroyMembers(nsXBLProtoImplMember* aBrokenMember)
{
  NS_ASSERTION(mClassObject, "This should never be called when there is no class object");
  PRBool compiled = PR_TRUE;
  for (nsXBLProtoImplMember* curr = mMembers; curr; curr = curr->GetNext()) {
    if (curr == aBrokenMember) {
      compiled = PR_FALSE;
    }
    curr->Destroy(compiled);
  }

  
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

