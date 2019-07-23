





































#ifndef nsXBLProtoImplField_h__
#define nsXBLProtoImplField_h__

#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsXBLProtoImplMember.h"

class nsXBLProtoImplField: public nsXBLProtoImplMember
{
public:
  nsXBLProtoImplField(const PRUnichar* aName, const PRUnichar* aReadOnly);
  virtual ~nsXBLProtoImplField();
  virtual void Destroy(PRBool aIsCompiled);

  void AppendFieldText(const nsAString& aText);
  void SetLineNumber(PRUint32 aLineNumber) {
    mLineNumber = aLineNumber;
  }
  
  virtual nsresult InstallMember(nsIScriptContext* aContext,
                                 nsIContent* aBoundElement, 
                                 void* aScriptObject,
                                 void* aTargetClassObject,
                                 const nsCString& aClassStr);
  virtual nsresult CompileMember(nsIScriptContext* aContext,
                                 const nsCString& aClassStr,
                                 void* aClassObject);

protected:
  PRUnichar* mFieldText;
  PRUint32 mFieldTextLength;
  PRUint32 mLineNumber;
  uintN mJSAttributes;
};

#endif 
