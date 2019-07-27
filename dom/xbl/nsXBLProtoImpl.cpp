




#include "mozilla/DebugOnly.h"

#include "nsXBLProtoImpl.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsXBLPrototypeBinding.h"
#include "nsXBLProtoImplProperty.h"
#include "nsIURI.h"
#include "mozilla/AddonPathService.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/XULElementBinding.h"
#include "xpcpublic.h"
#include "js/CharacterEncoding.h"

using namespace mozilla;
using js::GetGlobalForObjectCrossCompartment;
using js::AssertSameCompartment;

nsresult
nsXBLProtoImpl::InstallImplementation(nsXBLPrototypeBinding* aPrototypeBinding,
                                      nsXBLBinding* aBinding)
{
  
  
  
  
  if (!mMembers && !mFields)  
    return NS_OK; 

  
  
  nsIDocument* document = aBinding->GetBoundElement()->OwnerDoc();

  
  
  if (NS_WARN_IF(!document->GetWindow())) {
    return NS_OK;
  }

  
  
  
  dom::AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.Init(document->GetScopeObject()))) {
    return NS_OK;
  }
  JSContext* cx = jsapi.cx();

  
  
  
  
  JS::Rooted<JSObject*> targetClassObject(cx, nullptr);
  bool targetObjectIsNew = false;
  nsresult rv = InitTargetObjects(aPrototypeBinding,
                                  aBinding->GetBoundElement(),
                                  &targetClassObject,
                                  &targetObjectIsNew);
  NS_ENSURE_SUCCESS(rv, rv); 
  MOZ_ASSERT(targetClassObject);

  
  if (!targetObjectIsNew)
    return NS_OK;

  
  
  
  
  
  

  
  
  JSAddonId* addonId = MapURIToAddonID(aPrototypeBinding->BindingURI());
  JS::Rooted<JSObject*> globalObject(cx,
    GetGlobalForObjectCrossCompartment(targetClassObject));
  JS::Rooted<JSObject*> scopeObject(cx, xpc::GetScopeForXBLExecution(cx, globalObject, addonId));
  NS_ENSURE_TRUE(scopeObject, NS_ERROR_OUT_OF_MEMORY);
  JSAutoCompartment ac(cx, scopeObject);

  
  
  
  
  
  
  
  
  
  const char* className = aPrototypeBinding->ClassName().get();
  JS::Rooted<JSObject*> propertyHolder(cx);
  JS::Rooted<JSPropertyDescriptor> existingHolder(cx);
  if (scopeObject != globalObject &&
      !JS_GetOwnPropertyDescriptor(cx, scopeObject, className, &existingHolder)) {
    return NS_ERROR_FAILURE;
  }
  bool propertyHolderIsNew = !existingHolder.object() || !existingHolder.value().isObject();

  if (!propertyHolderIsNew) {
    propertyHolder = &existingHolder.value().toObject();
  } else if (scopeObject != globalObject) {

    
    propertyHolder = JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr(), scopeObject);
    NS_ENSURE_TRUE(propertyHolder, NS_ERROR_OUT_OF_MEMORY);

    
    
    bool ok = JS_DefineProperty(cx, scopeObject, className, propertyHolder,
                                JSPROP_PERMANENT | JSPROP_READONLY,
                                JS_PropertyStub, JS_StrictPropertyStub);
    NS_ENSURE_TRUE(ok, NS_ERROR_UNEXPECTED);
  } else {
    propertyHolder = targetClassObject;
  }

  
  if (propertyHolderIsNew) {
    for (nsXBLProtoImplMember* curr = mMembers;
         curr;
         curr = curr->GetNext())
      curr->InstallMember(cx, propertyHolder);
  }

  
  
  
  
  if (propertyHolder != targetClassObject) {
    AssertSameCompartment(propertyHolder, scopeObject);
    AssertSameCompartment(targetClassObject, globalObject);
    bool inContentXBLScope = xpc::IsInContentXBLScope(scopeObject);
    for (nsXBLProtoImplMember* curr = mMembers; curr; curr = curr->GetNext()) {
      if (!inContentXBLScope || curr->ShouldExposeToUntrustedContent()) {
        JS::Rooted<jsid> id(cx);
        JS::TwoByteChars chars(curr->GetName(), NS_strlen(curr->GetName()));
        bool ok = JS_CharsToId(cx, chars, &id);
        NS_ENSURE_TRUE(ok, NS_ERROR_UNEXPECTED);

        bool found;
        ok = JS_HasPropertyById(cx, propertyHolder, id, &found);
        NS_ENSURE_TRUE(ok, NS_ERROR_UNEXPECTED);
        if (!found) {
          
          
          
          continue;
        }

        ok = JS_CopyPropertyFrom(cx, id, targetClassObject, propertyHolder);
        NS_ENSURE_TRUE(ok, NS_ERROR_UNEXPECTED);
      }
    }
  }

  
  JSAutoCompartment ac2(cx, targetClassObject);

  
  for (nsXBLProtoImplField* curr = mFields;
       curr;
       curr = curr->GetNext())
    curr->InstallAccessors(cx, targetClassObject);

  return NS_OK;
}

nsresult 
nsXBLProtoImpl::InitTargetObjects(nsXBLPrototypeBinding* aBinding,
                                  nsIContent* aBoundElement, 
                                  JS::MutableHandle<JSObject*> aTargetClassObject,
                                  bool* aTargetIsNew)
{
  nsresult rv = NS_OK;

  if (!mPrecompiledMemberHolder) {
    rv = CompilePrototypeMembers(aBinding); 
                                 
                                 
    if (NS_FAILED(rv))
      return rv;

    MOZ_ASSERT(mPrecompiledMemberHolder);
  }

  nsIDocument *ownerDoc = aBoundElement->OwnerDoc();
  nsIGlobalObject *sgo;

  if (!(sgo = ownerDoc->GetScopeObject())) {
    return NS_ERROR_UNEXPECTED;
  }

  
  
  AutoJSContext cx;
  JS::Rooted<JSObject*> global(cx, sgo->GetGlobalJSObject());
  JS::Rooted<JS::Value> v(cx);

  JSAutoCompartment ac(cx, global);
  
  
  bool defineOnGlobal = dom::XULElementBinding::ConstructorEnabled(cx, global);
  dom::XULElementBinding::GetConstructorObject(cx, global, defineOnGlobal);

  rv = nsContentUtils::WrapNative(cx, aBoundElement, &v,
                                   false);
  NS_ENSURE_SUCCESS(rv, rv);

  JS::Rooted<JSObject*> value(cx, &v.toObject());
  JSAutoCompartment ac2(cx, value);

  
  
  
  
  rv = aBinding->InitClass(mClassName, cx, value, aTargetClassObject, aTargetIsNew);
  if (NS_FAILED(rv)) {
    return rv;
  }

  aBoundElement->PreserveWrapper(aBoundElement);

  return rv;
}

nsresult
nsXBLProtoImpl::CompilePrototypeMembers(nsXBLPrototypeBinding* aBinding)
{
  
  
  
  AutoSafeJSContext cx;
  JS::Rooted<JSObject*> compilationGlobal(cx, xpc::CompilationScope());
  JSAutoCompartment ac(cx, compilationGlobal);

  mPrecompiledMemberHolder = JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr(), compilationGlobal);
  if (!mPrecompiledMemberHolder)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  JS::Rooted<JSObject*> rootedHolder(cx, mPrecompiledMemberHolder);
  for (nsXBLProtoImplMember* curr = mMembers;
       curr;
       curr = curr->GetNext()) {
    nsresult rv = curr->CompileMember(mClassName, rootedHolder);
    if (NS_FAILED(rv)) {
      DestroyMembers();
      return rv;
    }
  }

  return NS_OK;
}

bool
nsXBLProtoImpl::LookupMember(JSContext* aCx, nsString& aName,
                             JS::Handle<jsid> aNameAsId,
                             JS::MutableHandle<JSPropertyDescriptor> aDesc,
                             JS::Handle<JSObject*> aClassObject)
{
  for (nsXBLProtoImplMember* m = mMembers; m; m = m->GetNext()) {
    if (aName.Equals(m->GetName())) {
      return JS_GetPropertyDescriptorById(aCx, aClassObject, aNameAsId, aDesc);
    }
  }
  return true;
}

void
nsXBLProtoImpl::Trace(const TraceCallbacks& aCallbacks, void *aClosure)
{
  
  
  
  if (!mPrecompiledMemberHolder) {
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
  if (mPrecompiledMemberHolder) {
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
    if (!::JS_LookupUCProperty(cx, obj, name.get(), name.Length(), &dummy)) {
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

    const char16_t* s = name.get();
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
  MOZ_ASSERT(mPrecompiledMemberHolder);

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
  mPrecompiledMemberHolder = JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr(), global);
  if (!mPrecompiledMemberHolder)
    return NS_ERROR_OUT_OF_MEMORY;

  nsXBLProtoImplField* previousField = nullptr;
  nsXBLProtoImplMember* previousMember = nullptr;

  do {
    XBLBindingSerializeDetails type;
    nsresult rv = aStream->Read8(&type);
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

  if (!mPrecompiledMemberHolder) {
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
                   const char16_t* aClassName, 
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

