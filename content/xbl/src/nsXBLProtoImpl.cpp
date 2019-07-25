





































#include "nsXBLProtoImpl.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIScriptContext.h"
#include "nsIXPConnect.h"
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsXBLPrototypeBinding.h"
#include "nsXBLProtoImplProperty.h"


class AutoVersionChecker
{
  JSContext * const cx;
  JSVersion versionBefore;

public:
  explicit AutoVersionChecker(JSContext *cx) : cx(cx) {
#ifdef DEBUG
    versionBefore = JS_GetVersion(cx);
#endif
  }

  ~AutoVersionChecker() {
#ifdef DEBUG
    JSVersion versionAfter = JS_GetVersion(cx);
    NS_ABORT_IF_FALSE(versionAfter == versionBefore, "version must not change");
#endif
  }
};

nsresult
nsXBLProtoImpl::InstallImplementation(nsXBLPrototypeBinding* aBinding, nsIContent* aBoundElement)
{
  
  
  
  
  if (!mMembers && !mFields)  
    return NS_OK; 

  
  
  nsIDocument* document = aBoundElement->OwnerDoc();
                                              
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

  JSContext *cx = context->GetNativeContext();

  AutoVersionChecker avc(cx);
  
  
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

  nsIDocument *ownerDoc = aBoundElement->OwnerDoc();
  nsIScriptGlobalObject *sgo;

  if (!(sgo = ownerDoc->GetScopeObject())) {
    return NS_ERROR_UNEXPECTED;
  }

  
  
  JSContext* jscontext = aContext->GetNativeContext();
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
      do_QueryObject(aBinding->XBLDocumentInfo()));
  nsIScriptGlobalObject* globalObject = globalOwner->GetScriptGlobalObject();
  NS_ENSURE_TRUE(globalObject, NS_ERROR_UNEXPECTED);

  nsIScriptContext *context = globalObject->GetContext();
  NS_ENSURE_TRUE(context, NS_ERROR_OUT_OF_MEMORY);

  JSContext *cx = context->GetNativeContext();
  JSObject *global = globalObject->GetGlobalJSObject();
  

  void* classObject;
  nsresult rv = aBinding->InitClass(mClassName, cx, global, global,
                                    &classObject);
  if (NS_FAILED(rv))
    return rv;

  mClassObject = (JSObject*) classObject;
  if (!mClassObject)
    return NS_ERROR_FAILURE;

  AutoVersionChecker avc(cx);

  
  
  for (nsXBLProtoImplMember* curr = mMembers;
       curr;
       curr = curr->GetNext()) {
    nsresult rv = curr->CompileMember(context, mClassName, mClassObject);
    if (NS_FAILED(rv)) {
      DestroyMembers();
      return rv;
    }
  }

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

bool
nsXBLProtoImpl::ResolveAllFields(JSContext *cx, JSObject *obj) const
{
  AutoVersionChecker avc(cx);
  for (nsXBLProtoImplField* f = mFields; f; f = f->GetNext()) {
    
    
    
    nsDependentString name(f->GetName());
    jsval dummy;
    if (!::JS_LookupUCProperty(cx, obj,
                               reinterpret_cast<const jschar*>(name.get()),
                               name.Length(), &dummy)) {
      return false;
    }
  }

  return true;
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
nsXBLProtoImpl::Read(nsIScriptContext* aContext,
                     nsIObjectInputStream* aStream,
                     nsXBLPrototypeBinding* aBinding,
                     nsIScriptGlobalObject* aGlobal)
{
  
  JSContext *cx = static_cast<JSContext *>(aContext->GetNativeContext());
  JSObject *global = aGlobal->GetGlobalJSObject();

  void* classObject;
  nsresult rv = aBinding->InitClass(mClassName, cx, global, global, &classObject);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(classObject, NS_ERROR_FAILURE);

  mClassObject = (JSObject*) classObject;

  nsXBLProtoImplField* previousField = nsnull;
  nsXBLProtoImplMember* previousMember = nsnull;

  do {
    XBLBindingSerializeDetails type;
    rv = aStream->Read8(&type);
    NS_ENSURE_SUCCESS(rv, rv);
    if (type == XBLBinding_Serialize_NoMoreItems)
      break;

    switch (type & XBLBinding_Serialize_Mask) {
      case XBLBinding_Serialize_Field:
      {
        nsXBLProtoImplField* field =
          new nsXBLProtoImplField(type & XBLBinding_Serialize_ReadOnly);
        rv = field->Read(aContext, aStream);
        if (NS_FAILED(rv)) {
          delete field;
          return rv;
        }

        if (previousField) {
          previousField->SetNext(field);
        }
        else {
          mFields = field;
        }
        previousField = field;

        break;
      }
      case XBLBinding_Serialize_GetterProperty:
      case XBLBinding_Serialize_SetterProperty:
      case XBLBinding_Serialize_GetterSetterProperty:
      {
        nsAutoString name;
        nsresult rv = aStream->ReadString(name);
        NS_ENSURE_SUCCESS(rv, rv);

        nsXBLProtoImplProperty* prop =
          new nsXBLProtoImplProperty(name.get(), type & XBLBinding_Serialize_ReadOnly);
        rv = prop->Read(aContext, aStream, type & XBLBinding_Serialize_Mask);
        if (NS_FAILED(rv)) {
          delete prop;
          return rv;
        }

        previousMember = AddMember(prop, previousMember);
        break;
      }
      case XBLBinding_Serialize_Method:
      {
        nsAutoString name;
        rv = aStream->ReadString(name);
        NS_ENSURE_SUCCESS(rv, rv);

        nsXBLProtoImplMethod* method = new nsXBLProtoImplMethod(name.get());
        rv = method->Read(aContext, aStream);
        if (NS_FAILED(rv)) {
          delete method;
          return rv;
        }

        previousMember = AddMember(method, previousMember);
        break;
      }
      case XBLBinding_Serialize_Constructor:
      {
        mConstructor = new nsXBLProtoImplAnonymousMethod();
        rv = mConstructor->Read(aContext, aStream);
        if (NS_FAILED(rv)) {
          delete mConstructor;
          mConstructor = nsnull;
          return rv;
        }

        previousMember = AddMember(mConstructor, previousMember);
        break;
      }
      case XBLBinding_Serialize_Destructor:
      {
        mDestructor = new nsXBLProtoImplAnonymousMethod();
        rv = mDestructor->Read(aContext, aStream);
        if (NS_FAILED(rv)) {
          delete mDestructor;
          mDestructor = nsnull;
          return rv;
        }

        previousMember = AddMember(mDestructor, previousMember);
        break;
      }
      default:
        NS_ERROR("Unexpected binding member type");
        break;
    }
  } while (1);

  return NS_OK;
}

nsresult
nsXBLProtoImpl::Write(nsIScriptContext* aContext,
                      nsIObjectOutputStream* aStream,
                      nsXBLPrototypeBinding* aBinding)
{
  nsresult rv;

  if (!mClassObject) {
    rv = CompilePrototypeMembers(aBinding);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aStream->WriteStringZ(mClassName.get());
  NS_ENSURE_SUCCESS(rv, rv);

  for (nsXBLProtoImplField* curr = mFields; curr; curr = curr->GetNext()) {
    rv = curr->Write(aContext, aStream);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  for (nsXBLProtoImplMember* curr = mMembers; curr; curr = curr->GetNext()) {
    if (curr == mConstructor) {
      rv = mConstructor->Write(aContext, aStream, XBLBinding_Serialize_Constructor);
    }
    else if (curr == mDestructor) {
      rv = mDestructor->Write(aContext, aStream, XBLBinding_Serialize_Destructor);
    }
    else {
      rv = curr->Write(aContext, aStream);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return aStream->Write8(XBLBinding_Serialize_NoMoreItems);
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

