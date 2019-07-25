





































#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "mozilla/FunctionTimer.h"
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
    nsAutoString readOnly; readOnly.Assign(aReadOnly);
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
                                  nsIPrincipal* aPrincipal,
                                  nsIURI* aBindingDocURI,
                                  PRBool* aDidInstall) const
{
  NS_TIME_FUNCTION_MIN(5);
  NS_PRECONDITION(aBoundNode,
                  "uh-oh, bound node should NOT be null or bad things will "
                  "happen");

  *aDidInstall = PR_FALSE;

  if (mFieldTextLength == 0) {
    return NS_OK;
  }

  
  
  nsresult rv;

  nsCAutoString uriSpec;
  aBindingDocURI->GetSpec(uriSpec);
  
  JSContext* cx = aContext->GetNativeContext();
  NS_ASSERTION(!::JS_IsExceptionPending(cx),
               "Shouldn't get here when an exception is pending!");
  
  
  PRBool undefined;
  nsCOMPtr<nsIScriptContext> context = aContext;

  JSAutoRequest ar(cx);
  jsval result = JSVAL_NULL;
  rv = context->EvaluateStringWithValue(nsDependentString(mFieldText,
                                                          mFieldTextLength), 
                                        aBoundNode,
                                        aPrincipal, uriSpec.get(),
                                        mLineNumber, JSVERSION_LATEST,
                                        (void*) &result, &undefined);
  if (NS_FAILED(rv))
    return rv;

  if (undefined) {
    result = JSVAL_VOID;
  }

  
  nsDependentString name(mName);
  if (!::JS_DefineUCProperty(cx, aBoundNode,
                             reinterpret_cast<const jschar*>(mName), 
                             name.Length(), result, nsnull, nsnull,
                             mJSAttributes)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *aDidInstall = PR_TRUE;
  return NS_OK;
}
