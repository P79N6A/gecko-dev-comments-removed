




#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsXBLProtoImplField.h"
#include "nsIScriptContext.h"
#include "nsIURI.h"
#include "nsXBLSerialize.h"
#include "nsXBLPrototypeBinding.h"

nsXBLProtoImplField::nsXBLProtoImplField(const PRUnichar* aName, const PRUnichar* aReadOnly)
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
    PRUnichar* temp = mFieldText;
    mFieldText = ToNewUnicode(newFieldText);
    mFieldTextLength = newFieldText.Length();
    nsMemory::Free(temp);
  }
  else {
    mFieldText = ToNewUnicode(aText);
    mFieldTextLength = aText.Length();
  }
}

nsresult
nsXBLProtoImplField::InstallField(nsIScriptContext* aContext,
                                  JSObject* aBoundNode,
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
  
  JSContext* cx = aContext->GetNativeContext();
  NS_ASSERTION(!::JS_IsExceptionPending(cx),
               "Shouldn't get here when an exception is pending!");
  
  
  nsCOMPtr<nsIScriptContext> context = aContext;

  JSAutoRequest ar(cx);
  jsval result = JSVAL_NULL;

  JS::CompileOptions options(cx);
  options.setFileAndLine(uriSpec.get(), mLineNumber)
         .setVersion(JSVERSION_LATEST)
         .setUserBit(true); 
  rv = context->EvaluateString(nsDependentString(mFieldText,
                                                 mFieldTextLength),
                               *aBoundNode, options,
                                false,
                               &result);
  if (NS_FAILED(rv)) {
    return rv;
  }


  
  nsDependentString name(mName);
  if (!::JS_DefineUCProperty(cx, aBoundNode,
                             reinterpret_cast<const jschar*>(mName), 
                             name.Length(), result, nullptr, nullptr,
                             mJSAttributes)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *aDidInstall = true;
  return NS_OK;
}

nsresult
nsXBLProtoImplField::Read(nsIScriptContext* aContext,
                          nsIObjectInputStream* aStream)
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
nsXBLProtoImplField::Write(nsIScriptContext* aContext,
                           nsIObjectOutputStream* aStream)
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

  return aStream->WriteWStringZ(mFieldText ? mFieldText : EmptyString().get());
}
