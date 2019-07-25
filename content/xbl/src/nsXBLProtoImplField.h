





































#ifndef nsXBLProtoImplField_h__
#define nsXBLProtoImplField_h__

#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsXBLProtoImplMember.h"

class nsIURI;

class nsXBLProtoImplField
{
public:
  nsXBLProtoImplField(const PRUnichar* aName, const PRUnichar* aReadOnly);
  ~nsXBLProtoImplField();

  void AppendFieldText(const nsAString& aText);
  void SetLineNumber(PRUint32 aLineNumber) {
    mLineNumber = aLineNumber;
  }
  
  nsXBLProtoImplField* GetNext() const { return mNext; }
  void SetNext(nsXBLProtoImplField* aNext) { mNext = aNext; }

  nsresult InstallField(nsIScriptContext* aContext,
                        JSObject* aBoundNode,
                        nsIPrincipal* aPrincipal,
                        nsIURI* aBindingDocURI,
                        bool* aDidInstall) const;

  const PRUnichar* GetName() const { return mName; }

protected:
  nsXBLProtoImplField* mNext;
  PRUnichar* mName;
  PRUnichar* mFieldText;
  PRUint32 mFieldTextLength;
  PRUint32 mLineNumber;
  uintN mJSAttributes;
};

#endif 
