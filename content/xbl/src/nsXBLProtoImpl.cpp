




#include "mozilla/DebugOnly.h"

#include "nsXBLProtoImpl.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIXPConnect.h"
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsXBLPrototypeBinding.h"
#include "nsXBLProtoImplProperty.h"
#include "nsIURI.h"
#include "mozilla/dom/XULElementBinding.h"
#include "xpcpublic.h"

using namespace mozilla;

nsresult
nsXBLProtoImpl::InstallImplementation(nsXBLPrototypeBinding* aPrototypeBinding,
                                      nsXBLBinding* aBinding)
{
  
  
  
  
  if (!mMembers && !mFields)  
    return NS_OK; 

  
  
  nsIDocument* document = aBinding->GetBoundElement()->OwnerDoc();

  nsCOMPtr<nsIScriptGlobalObject> global =  do_QueryInterface(document->GetScopeObject());
  if (!global) return NS_OK;

  nsCOMPtr<nsIScriptContext> context = global->GetContext();
  if (!context) return NS_OK;
  JSContext* cx = context->GetNativeContext();
  AutoCxPusher pusher(cx);

  
  
  
  
  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  JS::Rooted<JSObject*> targetClassObject(cx, nullptr);
  bool targetObjectIsNew = false;
  nsresult rv = InitTargetObjects(aPrototypeBinding,
                                  aBinding->GetBoundElement(),
                                  getter_AddRefs(holder), &targetClassObject,
                                  &targetObjectIsNew);
  NS_ENSURE_SUCCESS(rv, rv); 
  MOZ_ASSERT(targetClassObject);

  
  aBinding->SetJSClass(nsXBLJSClass::fromJSClass(JS_GetClass(targetClassObject)));

  
  if (!targetObjectIsNew)
    return NS_OK;

  JS::Rooted<JSObject*> targetScriptObject(cx, holder->GetJSObject());

  JSAutoCompartment ac(cx, targetClassObject);

  
  for (nsXBLProtoImplMember* curr = mMembers;
       curr;
       curr = curr->GetNext())
    curr->InstallMember(cx, targetClassObject);

  
  
  
  JS::Rooted<JSObject*> globalObject(cx,
    JS_GetGlobalForObject(cx, targetClassObject));
  JS::Rooted<JSObject*> scopeObject(cx, xpc::GetXBLScope(cx, globalObject));
  NS_ENSURE_TRUE(scopeObject, NS_ERROR_OUT_OF_MEMORY);
  if (scopeObject != globalObject) {
    JSAutoCompartment ac2(cx, scopeObject);

    
    
    JS::Rooted<JSObject*> shadowProto(cx,
      JS_NewObjectWithGivenProto(cx, nullptr, nullptr, scopeObject));
    NS_ENSURE_TRUE(shadowProto, NS_ERROR_OUT_OF_MEMORY);

    
    
    bool ok = JS_DefineProperty(cx, scopeObject,
                                js::GetObjectClass(targetClassObject)->name,
                                JS::ObjectValue(*shadowProto), JS_PropertyStub,
                                JS_StrictPropertyStub,
                                JSPROP_PERMANENT | JSPROP_READONLY);
    NS_ENSURE_TRUE(ok, NS_ERROR_UNEXPECTED);

    
    
    
    
    ok = JS_CopyPropertiesFrom(cx, shadowProto, targetClassObject);
    NS_ENSURE_TRUE(ok, NS_ERROR_UNEXPECTED);

    
    
    ok = JS_FreezeObject(cx, shadowProto);
    NS_ENSURE_TRUE(ok, NS_ERROR_UNEXPECTED);
  }

  
  for (nsXBLProtoImplField* curr = mFields;
       curr;
       curr = curr->GetNext())
    curr->InstallAccessors(cx, targetClassObject);

  return NS_OK;
}

nsresult 
nsXBLProtoImpl::InitTargetObjects(nsXBLPrototypeBinding* aBinding,
                                  nsIContent* aBoundElement, 
                                  nsIXPConnectJSObjectHolder** aScriptObjectHolder, 
                                  JS::MutableHandle<JSObject*> aTargetClassObject,
                                  bool* aTargetIsNew)
{
  nsresult rv = NS_OK;
  *aScriptObjectHolder = nullptr;
  
  if (!mClassObject) {
    rv = CompilePrototypeMembers(aBinding); 
                                 
                                 
    if (NS_FAILED(rv))
      return rv;

    MOZ_ASSERT(mClassObject);
  }

  nsIDocument *ownerDoc = aBoundElement->OwnerDoc();
  nsIGlobalObject *sgo;

  if (!(sgo = ownerDoc->GetScopeObject())) {
    return NS_ERROR_UNEXPECTED;
  }

  
  
  AutoJSContext cx;
  JS::Rooted<JSObject*> global(cx, sgo->GetGlobalJSObject());
  nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
  JS::Rooted<JS::Value> v(cx);

  {
    JSAutoCompartment ac(cx, global);
    
    
    bool defineOnGlobal = dom::XULElementBinding::ConstructorEnabled(cx, global);
    dom::XULElementBinding::GetConstructorObject(cx, global, defineOnGlobal);
  }

  rv = nsContentUtils::WrapNative(cx, global, aBoundElement, &v,
                                  getter_AddRefs(wrapper));
  NS_ENSURE_SUCCESS(rv, rv);

  JS::Rooted<JSObject*> value(cx, &v.toObject());

  
  
  
  
  rv = aBinding->InitClass(mClassName, cx, global, value, aTargetClassObject, aTargetIsNew);
  if (NS_FAILED(rv)) {
    return rv;
  }

  aBoundElement->PreserveWrapper(aBoundElement);

  wrapper.swap(*aScriptObjectHolder);
  
  return rv;
}

nsresult
nsXBLProtoImpl::CompilePrototypeMembers(nsXBLPrototypeBinding* aBinding)
{
  
  
  
  AutoSafeJSContext cx;
  JS::Rooted<JSObject*> compilationGlobal(cx, aBinding->XBLDocumentInfo()->GetCompilationGlobal());
  NS_ENSURE_TRUE(compilationGlobal, NS_ERROR_UNEXPECTED);
  JSAutoCompartment ac(cx, compilationGlobal);

  JS::Rooted<JSObject*> classObject(cx);
  bool classObjectIsNew = false;
  nsresult rv = aBinding->InitClass(mClassName, cx, compilationGlobal, compilationGlobal,
                                    &classObject, &classObjectIsNew);
  if (NS_FAILED(rv))
    return rv;

  MOZ_ASSERT(classObjectIsNew);
  MOZ_ASSERT(classObject);
  mClassObject = classObject;

  
  
  for (nsXBLProtoImplMember* curr = mMembers;
       curr;
       curr = curr->GetNext()) {
    nsresult rv = curr->CompileMember(mClassName, classObject);
    if (NS_FAILED(rv)) {
      DestroyMembers();
      return rv;
    }
  }

  return NS_OK;
}

bool
nsXBLProtoImpl::LookupMember(JSContext* aCx, nsString& aName,
                             JS::HandleId aNameAsId,
                             JS::MutableHandle<JSPropertyDescriptor> aDesc,
                             JSObject* aClassObject)
{
  for (nsXBLProtoImplMember* m = mMembers; m; m = m->GetNext()) {
    if (aName.Equals(m->GetName())) {
      return JS_GetPropertyDescriptorById(aCx, aClassObject, aNameAsId, 0, aDesc);
    }
  }
  return true;
}

void
nsXBLProtoImpl::Trace(const TraceCallbacks& aCallbacks, void *aClosure)
{
  
  
  
  if (!mClassObject) {
    return;
  }

  nsXBLProtoImplMember *member;
  for (member = mMembers; member; member = member->GetNext()) {
    member->Trace(aCallbacks, aClosure);
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

  return nullptr;
}

bool
nsXBLProtoImpl::ResolveAllFields(JSContext *cx, JS::Handle<JSObject*> obj) const
{
  for (nsXBLProtoImplField* f = mFields; f; f = f->GetNext()) {
    
    
    
    nsDependentString name(f->GetName());
    JS::Rooted<JS::Value> dummy(cx);
    if (!::JS_LookupUCProperty(cx, obj,
                               reinterpret_cast<const jschar*>(name.get()),
                               name.Length(), &dummy)) {
      return false;
    }
  }

  return true;
}

void
nsXBLProtoImpl::UndefineFields(JSContext *cx, JS::Handle<JSObject*> obj) const
{
  JSAutoRequest ar(cx);
  for (nsXBLProtoImplField* f = mFields; f; f = f->GetNext()) {
    nsDependentString name(f->GetName());

    const jschar* s = reinterpret_cast<const jschar*>(name.get());
    bool hasProp;
    if (::JS_AlreadyHasOwnUCProperty(cx, obj, s, name.Length(), &hasProp) &&
        hasProp) {
      bool dummy;
      ::JS_DeleteUCProperty2(cx, obj, s, name.Length(), &dummy);
    }
  }
}

void
nsXBLProtoImpl::DestroyMembers()
{
  NS_ASSERTION(mClassObject, "This should never be called when there is no class object");

  delete mMembers;
  mMembers = nullptr;
  mConstructor = nullptr;
  mDestructor = nullptr;
}

nsresult
nsXBLProtoImpl::Read(nsIObjectInputStream* aStream,
                     nsXBLPrototypeBinding* aBinding)
{
  AssertInCompilationScope();
  AutoJSContext cx;
  
  JS::Rooted<JSObject*> global(cx, JS::CurrentGlobalOrNull(cx));

  JS::Rooted<JSObject*> classObject(cx);
  bool classObjectIsNew = false;
  nsresult rv = aBinding->InitClass(mClassName, cx, global, global, &classObject,
                                    &classObjectIsNew);
  NS_ENSURE_SUCCESS(rv, rv);
  MOZ_ASSERT(classObject);
  MOZ_ASSERT(classObjectIsNew);

  mClassObject = classObject;

  nsXBLProtoImplField* previousField = nullptr;
  nsXBLProtoImplMember* previousMember = nullptr;

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
        rv = field->Read(aStream);
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
        rv = prop->Read(aStream, type & XBLBinding_Serialize_Mask);
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
        rv = method->Read(aStream);
        if (NS_FAILED(rv)) {
          delete method;
          return rv;
        }

        previousMember = AddMember(method, previousMember);
        break;
      }
      case XBLBinding_Serialize_Constructor:
      {
        nsAutoString name;
        rv = aStream->ReadString(name);
        NS_ENSURE_SUCCESS(rv, rv);

        mConstructor = new nsXBLProtoImplAnonymousMethod(name.get());
        rv = mConstructor->Read(aStream);
        if (NS_FAILED(rv)) {
          delete mConstructor;
          mConstructor = nullptr;
          return rv;
        }

        previousMember = AddMember(mConstructor, previousMember);
        break;
      }
      case XBLBinding_Serialize_Destructor:
      {
        nsAutoString name;
        rv = aStream->ReadString(name);
        NS_ENSURE_SUCCESS(rv, rv);

        mDestructor = new nsXBLProtoImplAnonymousMethod(name.get());
        rv = mDestructor->Read(aStream);
        if (NS_FAILED(rv)) {
          delete mDestructor;
          mDestructor = nullptr;
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
nsXBLProtoImpl::Write(nsIObjectOutputStream* aStream,
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
    rv = curr->Write(aStream);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  for (nsXBLProtoImplMember* curr = mMembers; curr; curr = curr->GetNext()) {
    if (curr == mConstructor) {
      rv = mConstructor->Write(aStream, XBLBinding_Serialize_Constructor);
    }
    else if (curr == mDestructor) {
      rv = mDestructor->Write(aStream, XBLBinding_Serialize_Destructor);
    }
    else {
      rv = curr->Write(aStream);
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

