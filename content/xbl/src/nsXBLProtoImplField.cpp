





































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

nsXBLProtoImplField::nsXBLProtoImplField(const PRUnichar* aName, const PRUnichar* aReadOnly)
  : nsXBLProtoImplMember(aName),
    mFieldText(nsnull),
    mFieldTextLength(0),
    mLineNumber(0)
{
  MOZ_COUNT_CTOR(nsXBLProtoImplField);
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
}

void
nsXBLProtoImplField::Destroy(PRBool aIsCompiled)
{
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
nsXBLProtoImplField::InstallMember(nsIScriptContext* aContext,
                                   nsIContent* aBoundElement, 
                                   void* aScriptObject,
                                   void* aTargetClassObject,
                                   const nsCString& aClassStr)
{
  if (mFieldTextLength == 0)
    return NS_OK; 

  JSContext* cx = (JSContext*) aContext->GetNativeContext();
  NS_ASSERTION(aScriptObject, "uh-oh, script Object should NOT be null or bad things will happen");
  if (!aScriptObject)
    return NS_ERROR_FAILURE;

  nsCAutoString bindingURI(aClassStr);
  PRInt32 hash = bindingURI.RFindChar('#');
  if (hash != kNotFound)
    bindingURI.Truncate(hash);
  
  
  jsval result = JSVAL_NULL;
  
  
  
  nsresult rv;
  nsAutoGCRoot root(&result, &rv);
  if (NS_FAILED(rv))
    return rv;
  PRBool undefined;
  
  nsCOMPtr<nsIScriptContext> context = aContext;
  rv = context->EvaluateStringWithValue(nsDependentString(mFieldText,
                                                          mFieldTextLength), 
                                        aScriptObject,
                                        nsnull, bindingURI.get(),
                                        mLineNumber, nsnull,
                                        (void*) &result, &undefined);
  if (NS_FAILED(rv))
    return rv;

  if (!undefined) {
    
    nsDependentString name(mName);
    JSAutoRequest ar(cx);
    if (!::JS_DefineUCProperty(cx, NS_STATIC_CAST(JSObject *, aScriptObject),
                               NS_REINTERPRET_CAST(const jschar*, mName), 
                               name.Length(), result, nsnull, nsnull, mJSAttributes))
      return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return NS_OK;
}

nsresult 
nsXBLProtoImplField::CompileMember(nsIScriptContext* aContext, const nsCString& aClassStr,
                                   void* aClassObject)
{
  return NS_OK;
}

void
nsXBLProtoImplField::Traverse(nsCycleCollectionTraversalCallback &cb) const
{
}
