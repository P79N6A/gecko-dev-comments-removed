





































#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsXBLProtoImplField.h"
#include "nsIScriptContext.h"
#include "nsContentUtils.h"
#include "nsIURI.h"

nsXBLProtoImplField::nsXBLProtoImplField(const PRUnichar* aName, const PRUnichar* aReadOnly)
  : mNext(nsnull),
    mFieldText(nsnull),
    mFieldTextLength(0),
    mLineNumber(0)
{
  MOZ_COUNT_CTOR(nsXBLProtoImplField);
  mName = NS_strdup(aName);  
  
  mJSAttributes = JSPROP_ENUMERATE;
  if (aReadOnly) {
    nsAutoString readOnly; readOnly.Assign(*aReadOnly);
    if (readOnly.LowerCaseEqualsLiteral("true"))
      mJSAttributes |= JSPROP_READONLY;
  }
}

nsXBLProtoImplField::~nsXBLProtoImplField()
{
  MOZ_COUNT_DTOR(nsXBLProtoImplField);
  if (mFieldText)
    nsMemory::Free(mFieldText);
  NS_Free(mName);
  delete mNext;
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
                                  nsIURI* aBindingDocURI) const
{
  NS_PRECONDITION(aBoundNode,
                  "uh-oh, bound node should NOT be null or bad things will "
                  "happen");

  jsval result = JSVAL_VOID;
  
  
  
  nsresult rv;
  nsAutoGCRoot root(&result, &rv);
  if (NS_FAILED(rv))
    return rv;

  if (mFieldTextLength != 0) {
    nsCAutoString uriSpec;
    aBindingDocURI->GetSpec(uriSpec);
  
    
    
    
    PRBool undefined;
    nsCOMPtr<nsIScriptContext> context = aContext;
    rv = context->EvaluateStringWithValue(nsDependentString(mFieldText,
                                                            mFieldTextLength), 
                                          aBoundNode,
                                          nsnull, uriSpec.get(),
                                          mLineNumber, nsnull,
                                          (void*) &result, &undefined);
    if (NS_FAILED(rv))
      return rv;

    if (undefined) {
      result = JSVAL_VOID;
    }
  }

  
  nsDependentString name(mName);
  JSContext* cx = (JSContext*) aContext->GetNativeContext();
  JSAutoRequest ar(cx);
  if (!::JS_DefineUCProperty(cx, aBoundNode,
                             reinterpret_cast<const jschar*>(mName), 
                             name.Length(), result, nsnull, nsnull,
                             mJSAttributes)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return NS_OK;
}
