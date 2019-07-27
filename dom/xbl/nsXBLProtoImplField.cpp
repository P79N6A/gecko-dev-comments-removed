




#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsJSUtils.h"
#include "jsapi.h"
#include "js/CharacterEncoding.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsXBLProtoImplField.h"
#include "nsIScriptContext.h"
#include "nsIURI.h"
#include "nsXBLSerialize.h"
#include "nsXBLPrototypeBinding.h"
#include "mozilla/AddonPathService.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsGlobalWindow.h"
#include "xpcpublic.h"
#include "WrapperFactory.h"

using namespace mozilla;
using namespace mozilla::dom;

nsXBLProtoImplField::nsXBLProtoImplField(const char16_t* aName, const char16_t* aReadOnly)
  : mNext(nullptr),
    mFieldText(nullptr),
    mFieldTextLength(0),
    mLineNumber(0)
{
  MOZ_COUNT_CTOR(nsXBLProtoImplField);
  mName = NS_strdup(aName);  
  
  mJSAttributes = JSPROP_ENUMERATE;
  if (aReadOnly) {
    nsAutoString readOnly; readOnly.Assign(aReadOnly);
    if (readOnly.LowerCaseEqualsLiteral("true"))
      mJSAttributes |= JSPROP_READONLY;
  }
}


nsXBLProtoImplField::nsXBLProtoImplField(const bool aIsReadOnly)
  : mNext(nullptr),
    mFieldText(nullptr),
    mFieldTextLength(0),
    mLineNumber(0)
{
  MOZ_COUNT_CTOR(nsXBLProtoImplField);

  mJSAttributes = JSPROP_ENUMERATE;
  if (aIsReadOnly)
    mJSAttributes |= JSPROP_READONLY;
}

nsXBLProtoImplField::~nsXBLProtoImplField()
{
  MOZ_COUNT_DTOR(nsXBLProtoImplField);
  if (mFieldText)
    nsMemory::Free(mFieldText);
  NS_Free(mName);
  NS_CONTENT_DELETE_LIST_MEMBER(nsXBLProtoImplField, this, mNext);
}

void 
nsXBLProtoImplField::AppendFieldText(const nsAString& aText)
{
  if (mFieldText) {
    nsDependentString fieldTextStr(mFieldText, mFieldTextLength);
    nsAutoString newFieldText = fieldTextStr + aText;
    char16_t* temp = mFieldText;
    mFieldText = ToNewUnicode(newFieldText);
    mFieldTextLength = newFieldText.Length();
    nsMemory::Free(temp);
  }
  else {
    mFieldText = ToNewUnicode(aText);
    mFieldTextLength = aText.Length();
  }
}





















static const uint32_t XBLPROTO_SLOT = 0;
static const uint32_t FIELD_SLOT = 1;

bool
ValueHasISupportsPrivate(JS::Handle<JS::Value> v)
{
  if (!v.isObject()) {
    return false;
  }

  const DOMJSClass* domClass = GetDOMClass(&v.toObject());
  if (domClass) {
    return domClass->mDOMObjectIsISupports;
  }

  const JSClass* clasp = ::JS_GetClass(&v.toObject());
  const uint32_t HAS_PRIVATE_NSISUPPORTS =
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS;
  return (clasp->flags & HAS_PRIVATE_NSISUPPORTS) == HAS_PRIVATE_NSISUPPORTS;
}

#ifdef DEBUG
static bool
ValueHasISupportsPrivate(JSContext* cx, const JS::Value& aVal)
{
    JS::Rooted<JS::Value> v(cx, aVal);
    return ValueHasISupportsPrivate(v);
}
#endif





static bool
InstallXBLField(JSContext* cx,
                JS::Handle<JSObject*> callee, JS::Handle<JSObject*> thisObj,
                JS::MutableHandle<jsid> idp, bool* installed)
{
  *installed = false;

  
  
  
  
  MOZ_ASSERT(ValueHasISupportsPrivate(cx, JS::ObjectValue(*thisObj)));

  
  
  
  nsISupports* native =
    nsContentUtils::XPConnect()->GetNativeOfWrapper(cx, thisObj);
  if (!native) {
    
    
    
    
    
    
    return true;
  }

  nsCOMPtr<nsIContent> xblNode = do_QueryInterface(native);
  if (!xblNode) {
    xpc::Throw(cx, NS_ERROR_UNEXPECTED);
    return false;
  }

  

  
  
  
  
  
  
  
  nsXBLPrototypeBinding* protoBinding;
  nsAutoJSString fieldName;
  {
    JSAutoCompartment ac(cx, callee);

    JS::Rooted<JSObject*> xblProto(cx);
    xblProto = &js::GetFunctionNativeReserved(callee, XBLPROTO_SLOT).toObject();

    JS::Rooted<JS::Value> name(cx, js::GetFunctionNativeReserved(callee, FIELD_SLOT));
    if (!fieldName.init(cx, name.toString())) {
      return false;
    }

    MOZ_ALWAYS_TRUE(JS_ValueToId(cx, name, idp));

    
    
    xblProto = js::UncheckedUnwrap(xblProto);
    JSAutoCompartment ac2(cx, xblProto);
    JS::Value slotVal = ::JS_GetReservedSlot(xblProto, 0);
    protoBinding = static_cast<nsXBLPrototypeBinding*>(slotVal.toPrivate());
    MOZ_ASSERT(protoBinding);
  }

  nsXBLProtoImplField* field = protoBinding->FindField(fieldName);
  MOZ_ASSERT(field);

  nsresult rv = field->InstallField(thisObj, protoBinding->DocURI(), installed);
  if (NS_SUCCEEDED(rv)) {
    return true;
  }

  if (!::JS_IsExceptionPending(cx)) {
    xpc::Throw(cx, rv);
  }
  return false;
}

bool
FieldGetterImpl(JSContext *cx, JS::CallArgs args)
{
  JS::Handle<JS::Value> thisv = args.thisv();
  MOZ_ASSERT(ValueHasISupportsPrivate(thisv));

  JS::Rooted<JSObject*> thisObj(cx, &thisv.toObject());

  
  
  
  
  
  bool installed = false;
  JS::Rooted<JSObject*> callee(cx, js::UncheckedUnwrap(&args.calleev().toObject()));
  JS::Rooted<jsid> id(cx);
  if (!InstallXBLField(cx, callee, thisObj, &id, &installed)) {
    return false;
  }

  if (!installed) {
    args.rval().setUndefined();
    return true;
  }

  JS::Rooted<JS::Value> v(cx);
  if (!JS_GetPropertyById(cx, thisObj, id, &v)) {
    return false;
  }
  args.rval().set(v);
  return true;
}

static bool
FieldGetter(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  return JS::CallNonGenericMethod<ValueHasISupportsPrivate, FieldGetterImpl>
                                 (cx, args);
}

bool
FieldSetterImpl(JSContext *cx, JS::CallArgs args)
{
  JS::Handle<JS::Value> thisv = args.thisv();
  MOZ_ASSERT(ValueHasISupportsPrivate(thisv));

  JS::Rooted<JSObject*> thisObj(cx, &thisv.toObject());

  
  
  
  
  
  bool installed = false;
  JS::Rooted<JSObject*> callee(cx, js::UncheckedUnwrap(&args.calleev().toObject()));
  JS::Rooted<jsid> id(cx);
  if (!InstallXBLField(cx, callee, thisObj, &id, &installed)) {
    return false;
  }

  if (installed) {
    if (!::JS_SetPropertyById(cx, thisObj, id, args.get(0))) {
      return false;
    }
  }
  args.rval().setUndefined();
  return true;
}

static bool
FieldSetter(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  return JS::CallNonGenericMethod<ValueHasISupportsPrivate, FieldSetterImpl>
                                 (cx, args);
}

nsresult
nsXBLProtoImplField::InstallAccessors(JSContext* aCx,
                                      JS::Handle<JSObject*> aTargetClassObject)
{
  MOZ_ASSERT(js::IsObjectInContextCompartment(aTargetClassObject, aCx));
  JS::Rooted<JSObject*> globalObject(aCx, JS_GetGlobalForObject(aCx, aTargetClassObject));
  JS::Rooted<JSObject*> scopeObject(aCx, xpc::GetXBLScopeOrGlobal(aCx, globalObject));
  NS_ENSURE_TRUE(scopeObject, NS_ERROR_OUT_OF_MEMORY);

  
  
  if (IsEmpty()) {
    return NS_OK;
  }

  
  

  
  JS::Rooted<jsid> id(aCx);
  JS::TwoByteChars chars(mName, NS_strlen(mName));
  if (!JS_CharsToId(aCx, chars, &id))
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  bool found = false;
  if (!JS_AlreadyHasOwnPropertyById(aCx, aTargetClassObject, id, &found))
    return NS_ERROR_FAILURE;
  if (found)
    return NS_OK;

  
  

  
  JSAutoCompartment ac(aCx, scopeObject);
  JS::Rooted<JS::Value> wrappedClassObj(aCx, JS::ObjectValue(*aTargetClassObject));
  if (!JS_WrapValue(aCx, &wrappedClassObj))
    return NS_ERROR_OUT_OF_MEMORY;

  JS::Rooted<JSObject*> get(aCx,
    JS_GetFunctionObject(js::NewFunctionByIdWithReserved(aCx, FieldGetter,
                                                         0, 0, scopeObject, id)));
  if (!get) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  js::SetFunctionNativeReserved(get, XBLPROTO_SLOT, wrappedClassObj);
  js::SetFunctionNativeReserved(get, FIELD_SLOT,
                                JS::StringValue(JSID_TO_STRING(id)));

  JS::Rooted<JSObject*> set(aCx,
    JS_GetFunctionObject(js::NewFunctionByIdWithReserved(aCx, FieldSetter,
                                                          1, 0, scopeObject, id)));
  if (!set) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  js::SetFunctionNativeReserved(set, XBLPROTO_SLOT, wrappedClassObj);
  js::SetFunctionNativeReserved(set, FIELD_SLOT,
                                JS::StringValue(JSID_TO_STRING(id)));

  
  
  JSAutoCompartment ac2(aCx, aTargetClassObject);
  if (!JS_WrapObject(aCx, &get) || !JS_WrapObject(aCx, &set)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!::JS_DefinePropertyById(aCx, aTargetClassObject, id, JS::UndefinedHandleValue,
                               AccessorAttributes(),
                               JS_DATA_TO_FUNC_PTR(JSPropertyOp, get.get()),
                               JS_DATA_TO_FUNC_PTR(JSStrictPropertyOp, set.get()))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

nsresult
nsXBLProtoImplField::InstallField(JS::Handle<JSObject*> aBoundNode,
                                  nsIURI* aBindingDocURI,
                                  bool* aDidInstall) const
{
  NS_PRECONDITION(aBoundNode,
                  "uh-oh, bound node should NOT be null or bad things will "
                  "happen");

  *aDidInstall = false;

  
  if (IsEmpty()) {
    return NS_OK;
  }

  nsAutoMicroTask mt;

  
  
  nsresult rv;

  nsAutoCString uriSpec;
  aBindingDocURI->GetSpec(uriSpec);

  nsIGlobalObject* globalObject = xpc::WindowGlobalOrNull(aBoundNode);
  if (!globalObject) {
    return NS_OK;
  }

  
  
  AutoEntryScript entryScript(globalObject, true);
  JSContext* cx = entryScript.cx();

  NS_ASSERTION(!::JS_IsExceptionPending(cx),
               "Shouldn't get here when an exception is pending!");

  JSAddonId* addonId = MapURIToAddonID(aBindingDocURI);

  
  
  JS::Rooted<JSObject*> scopeObject(cx, xpc::GetScopeForXBLExecution(cx, aBoundNode, addonId));
  NS_ENSURE_TRUE(scopeObject, NS_ERROR_OUT_OF_MEMORY);
  JSAutoCompartment ac(cx, scopeObject);

  JS::Rooted<JSObject*> wrappedNode(cx, aBoundNode);
  if (!JS_WrapObject(cx, &wrappedNode))
      return NS_ERROR_OUT_OF_MEMORY;

  JS::Rooted<JS::Value> result(cx);
  JS::CompileOptions options(cx);
  options.setFileAndLine(uriSpec.get(), mLineNumber)
         .setVersion(JSVERSION_LATEST);
  nsJSUtils::EvaluateOptions evalOptions;
  rv = nsJSUtils::EvaluateString(cx, nsDependentString(mFieldText,
                                                       mFieldTextLength),
                                 wrappedNode, options, evalOptions,
                                 &result);
  if (NS_FAILED(rv)) {
    return rv;
  }


  
  
  JSAutoCompartment ac2(cx, aBoundNode);
  nsDependentString name(mName);
  if (!JS_WrapValue(cx, &result) ||
      !::JS_DefineUCProperty(cx, aBoundNode,
                             reinterpret_cast<const char16_t*>(mName),
                             name.Length(), result, mJSAttributes)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *aDidInstall = true;
  return NS_OK;
}

nsresult
nsXBLProtoImplField::Read(nsIObjectInputStream* aStream)
{
  nsAutoString name;
  nsresult rv = aStream->ReadString(name);
  NS_ENSURE_SUCCESS(rv, rv);
  mName = ToNewUnicode(name);

  rv = aStream->Read32(&mLineNumber);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString fieldText;
  rv = aStream->ReadString(fieldText);
  NS_ENSURE_SUCCESS(rv, rv);
  mFieldTextLength = fieldText.Length();
  if (mFieldTextLength)
    mFieldText = ToNewUnicode(fieldText);

  return NS_OK;
}

nsresult
nsXBLProtoImplField::Write(nsIObjectOutputStream* aStream)
{
  XBLBindingSerializeDetails type = XBLBinding_Serialize_Field;

  if (mJSAttributes & JSPROP_READONLY) {
    type |= XBLBinding_Serialize_ReadOnly;
  }

  nsresult rv = aStream->Write8(type);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->WriteWStringZ(mName);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->Write32(mLineNumber);
  NS_ENSURE_SUCCESS(rv, rv);

  return aStream->WriteWStringZ(mFieldText ? mFieldText : MOZ_UTF16(""));
}
