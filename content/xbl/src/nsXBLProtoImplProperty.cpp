




#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsXBLProtoImplProperty.h"
#include "nsUnicharUtils.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "nsIScriptContext.h"
#include "nsJSUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsXBLPrototypeBinding.h"
#include "nsXBLSerialize.h"
#include "xpcpublic.h"

using namespace mozilla;

nsXBLProtoImplProperty::nsXBLProtoImplProperty(const PRUnichar* aName,
                                               const PRUnichar* aGetter, 
                                               const PRUnichar* aSetter,
                                               const PRUnichar* aReadOnly,
                                               uint32_t aLineNumber) :
  nsXBLProtoImplMember(aName), 
  mGetterText(nullptr),
  mSetterText(nullptr),
  mJSAttributes(JSPROP_ENUMERATE)
#ifdef DEBUG
  , mIsCompiled(false)
#endif
{
  MOZ_COUNT_CTOR(nsXBLProtoImplProperty);

  if (aReadOnly) {
    nsAutoString readOnly; readOnly.Assign(*aReadOnly);
    if (readOnly.LowerCaseEqualsLiteral("true"))
      mJSAttributes |= JSPROP_READONLY;
  }

  if (aGetter) {
    AppendGetterText(nsDependentString(aGetter));
    SetGetterLineNumber(aLineNumber);
  }
  if (aSetter) {
    AppendSetterText(nsDependentString(aSetter));
    SetSetterLineNumber(aLineNumber);
  }
}

nsXBLProtoImplProperty::nsXBLProtoImplProperty(const PRUnichar* aName,
                                               const bool aIsReadOnly)
  : nsXBLProtoImplMember(aName),
    mGetterText(nullptr),
    mSetterText(nullptr),
    mJSAttributes(JSPROP_ENUMERATE)
#ifdef DEBUG
  , mIsCompiled(false)
#endif
{
  MOZ_COUNT_CTOR(nsXBLProtoImplProperty);

  if (aIsReadOnly)
    mJSAttributes |= JSPROP_READONLY;
}

nsXBLProtoImplProperty::~nsXBLProtoImplProperty()
{
  MOZ_COUNT_DTOR(nsXBLProtoImplProperty);

  if (!(mJSAttributes & JSPROP_GETTER)) {
    delete mGetterText;
  }

  if (!(mJSAttributes & JSPROP_SETTER)) {
    delete mSetterText;
  }
}

void 
nsXBLProtoImplProperty::AppendGetterText(const nsAString& aText)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Must not be compiled when accessing getter text");
  if (!mGetterText) {
    mGetterText = new nsXBLTextWithLineNumber();
    if (!mGetterText)
      return;
  }

  mGetterText->AppendText(aText);
}

void 
nsXBLProtoImplProperty::AppendSetterText(const nsAString& aText)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Must not be compiled when accessing setter text");
  if (!mSetterText) {
    mSetterText = new nsXBLTextWithLineNumber();
    if (!mSetterText)
      return;
  }

  mSetterText->AppendText(aText);
}

void
nsXBLProtoImplProperty::SetGetterLineNumber(uint32_t aLineNumber)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Must not be compiled when accessing getter text");
  if (!mGetterText) {
    mGetterText = new nsXBLTextWithLineNumber();
    if (!mGetterText)
      return;
  }

  mGetterText->SetLineNumber(aLineNumber);
}

void
nsXBLProtoImplProperty::SetSetterLineNumber(uint32_t aLineNumber)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Must not be compiled when accessing setter text");
  if (!mSetterText) {
    mSetterText = new nsXBLTextWithLineNumber();
    if (!mSetterText)
      return;
  }

  mSetterText->SetLineNumber(aLineNumber);
}

const char* gPropertyArgs[] = { "val" };

nsresult
nsXBLProtoImplProperty::InstallMember(JSContext *aCx,
                                      JSObject* aTargetClassObject)
{
  NS_PRECONDITION(mIsCompiled,
                  "Should not be installing an uncompiled property");
  MOZ_ASSERT(js::IsObjectInContextCompartment(aTargetClassObject, aCx));
  JSObject * globalObject = JS_GetGlobalForObject(aCx, aTargetClassObject);
  JSObject * scopeObject = xpc::GetXBLScope(aCx, globalObject);

  
  if (mJSGetterObject || mJSSetterObject) {
    JSObject * getter = nullptr;

    
    JSAutoCompartment ac(aCx, scopeObject);
    if (mJSGetterObject)
      if (!(getter = ::JS_CloneFunctionObject(aCx, mJSGetterObject, scopeObject)))
        return NS_ERROR_OUT_OF_MEMORY;

    JSObject * setter = nullptr;
    if (mJSSetterObject)
      if (!(setter = ::JS_CloneFunctionObject(aCx, mJSSetterObject, scopeObject)))
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    JSAutoCompartment ac2(aCx, aTargetClassObject);
    nsDependentString name(mName);
    if (!JS_WrapObject(aCx, &getter) ||
        !JS_WrapObject(aCx, &setter) ||
        !::JS_DefineUCProperty(aCx, aTargetClassObject,
                               static_cast<const jschar*>(mName),
                               name.Length(), JSVAL_VOID,
                               JS_DATA_TO_FUNC_PTR(JSPropertyOp, getter),
                               JS_DATA_TO_FUNC_PTR(JSStrictPropertyOp, setter),
                               mJSAttributes))
      return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsresult 
nsXBLProtoImplProperty::CompileMember(nsIScriptContext* aContext, const nsCString& aClassStr,
                                      JSObject* aClassObject)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Trying to compile an already-compiled property");
  NS_PRECONDITION(aClassObject,
                  "Must have class object to compile");

  if (!mName)
    return NS_ERROR_FAILURE; 

  
  nsresult rv = NS_OK;

  nsAutoCString functionUri;
  if (mGetterText || mSetterText) {
    functionUri = aClassStr;
    int32_t hash = functionUri.RFindChar('#');
    if (hash != kNotFound) {
      functionUri.Truncate(hash);
    }
  }

  bool deletedGetter = false;
  if (mGetterText && mGetterText->GetText()) {
    nsDependentString getter(mGetterText->GetText());
    if (!getter.IsEmpty()) {
      
      JSObject* getterObject = nullptr;
      AutoPushJSContext cx(aContext->GetNativeContext());
      JSAutoRequest ar(cx);
      JSAutoCompartment ac(cx, aClassObject);
      JS::CompileOptions options(cx);
      options.setFileAndLine(functionUri.get(), mGetterText->GetLineNumber())
             .setVersion(JSVERSION_LATEST)
             .setUserBit(true); 
      nsCString name = NS_LITERAL_CSTRING("get_") + NS_ConvertUTF16toUTF8(mName);
      js::RootedObject rootedNull(cx, nullptr); 
      rv = nsJSUtils::CompileFunction(cx, rootedNull, options, name, 0, nullptr,
                                      getter, &getterObject);

      
      
      delete mGetterText;
      deletedGetter = true;
      mJSGetterObject = getterObject;
    
      if (mJSGetterObject && NS_SUCCEEDED(rv)) {
        mJSAttributes |= JSPROP_GETTER | JSPROP_SHARED;
      }
      if (NS_FAILED(rv)) {
        mJSGetterObject = nullptr;
        mJSAttributes &= ~JSPROP_GETTER;
        
      }
    }
  } 

  if (!deletedGetter) {  
    delete mGetterText;
    mJSGetterObject = nullptr;
  }
  
  if (NS_FAILED(rv)) {
    
    
    
    
    
    
    return rv;
  }

  bool deletedSetter = false;
  if (mSetterText && mSetterText->GetText()) {
    nsDependentString setter(mSetterText->GetText());
    if (!setter.IsEmpty()) {
      
      JSObject* setterObject = nullptr;
      AutoPushJSContext cx(aContext->GetNativeContext());
      JSAutoRequest ar(cx);
      JSAutoCompartment ac(cx, aClassObject);
      JS::CompileOptions options(cx);
      options.setFileAndLine(functionUri.get(), mSetterText->GetLineNumber())
             .setVersion(JSVERSION_LATEST)
             .setUserBit(true); 
      nsCString name = NS_LITERAL_CSTRING("set_") + NS_ConvertUTF16toUTF8(mName);
      js::RootedObject rootedNull(cx, nullptr); 
      rv = nsJSUtils::CompileFunction(cx, rootedNull, options, name, 1,
                                      gPropertyArgs, setter, &setterObject);

      
      
      delete mSetterText;
      deletedSetter = true;
      mJSSetterObject = setterObject;

      if (mJSSetterObject && NS_SUCCEEDED(rv)) {
        mJSAttributes |= JSPROP_SETTER | JSPROP_SHARED;
      }
      if (NS_FAILED(rv)) {
        mJSSetterObject = nullptr;
        mJSAttributes &= ~JSPROP_SETTER;
        
      }
    }
  } 

  if (!deletedSetter) {  
    delete mSetterText;
    mJSSetterObject = nullptr;
  }

#ifdef DEBUG
  mIsCompiled = NS_SUCCEEDED(rv);
#endif
  
  return rv;
}

void
nsXBLProtoImplProperty::Trace(TraceCallback aCallback, void *aClosure) const
{
  if (mJSAttributes & JSPROP_GETTER) {
    aCallback(mJSGetterObject, "mJSGetterObject", aClosure);
  }

  if (mJSAttributes & JSPROP_SETTER) {
    aCallback(mJSSetterObject, "mJSSetterObject", aClosure);
  }
}

nsresult
nsXBLProtoImplProperty::Read(nsIScriptContext* aContext,
                             nsIObjectInputStream* aStream,
                             XBLBindingSerializeDetails aType)
{
  if (aType == XBLBinding_Serialize_GetterProperty ||
      aType == XBLBinding_Serialize_GetterSetterProperty) {
    JSObject* getterObject;
    nsresult rv = XBL_DeserializeFunction(aContext, aStream, &getterObject);
    NS_ENSURE_SUCCESS(rv, rv);

    mJSGetterObject = getterObject;
    mJSAttributes |= JSPROP_GETTER | JSPROP_SHARED;
  }

  if (aType == XBLBinding_Serialize_SetterProperty ||
      aType == XBLBinding_Serialize_GetterSetterProperty) {
    JSObject* setterObject;
    nsresult rv = XBL_DeserializeFunction(aContext, aStream, &setterObject);
    NS_ENSURE_SUCCESS(rv, rv);

    mJSSetterObject = setterObject;
    mJSAttributes |= JSPROP_SETTER | JSPROP_SHARED;
  }

#ifdef DEBUG
  mIsCompiled = true;
#endif

  return NS_OK;
}

nsresult
nsXBLProtoImplProperty::Write(nsIScriptContext* aContext,
                              nsIObjectOutputStream* aStream)
{
  XBLBindingSerializeDetails type;

  if (mJSAttributes & JSPROP_GETTER) {
    type = mJSAttributes & JSPROP_SETTER ?
           XBLBinding_Serialize_GetterSetterProperty :
           XBLBinding_Serialize_GetterProperty;
  }
  else {
    type = XBLBinding_Serialize_SetterProperty;
  }

  if (mJSAttributes & JSPROP_READONLY) {
    type |= XBLBinding_Serialize_ReadOnly;
  }

  nsresult rv = aStream->Write8(type);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->WriteWStringZ(mName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mJSAttributes & JSPROP_GETTER) {
    rv = XBL_SerializeFunction(aContext, aStream, mJSGetterObject);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mJSAttributes & JSPROP_SETTER) {
    rv = XBL_SerializeFunction(aContext, aStream, mJSSetterObject);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
