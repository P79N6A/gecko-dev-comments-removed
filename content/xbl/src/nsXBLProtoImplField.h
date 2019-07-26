




#ifndef nsXBLProtoImplField_h__
#define nsXBLProtoImplField_h__

#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsString.h"
#include "nsXBLProtoImplMember.h"

class nsIObjectInputStream;
class nsIObjectOutputStream;
class nsIURI;

class nsXBLProtoImplField
{
public:
  nsXBLProtoImplField(const PRUnichar* aName, const PRUnichar* aReadOnly);
  nsXBLProtoImplField(const bool aIsReadOnly);
  ~nsXBLProtoImplField();

  void AppendFieldText(const nsAString& aText);
  void SetLineNumber(uint32_t aLineNumber) {
    mLineNumber = aLineNumber;
  }
  
  nsXBLProtoImplField* GetNext() const { return mNext; }
  void SetNext(nsXBLProtoImplField* aNext) { mNext = aNext; }

  nsresult InstallField(nsIScriptContext* aContext,
                        JSObject* aBoundNode,
                        nsIURI* aBindingDocURI,
                        bool* aDidInstall) const;

  nsresult InstallAccessors(JSContext* aCx,
                            JSObject* aTargetClassObject);

  nsresult Read(nsIScriptContext* aContext, nsIObjectInputStream* aStream);
  nsresult Write(nsIScriptContext* aContext, nsIObjectOutputStream* aStream);

  const PRUnichar* GetName() const { return mName; }

  unsigned AccessorAttributes() const {
    return JSPROP_SHARED | JSPROP_GETTER | JSPROP_SETTER |
           (mJSAttributes & (JSPROP_ENUMERATE | JSPROP_PERMANENT));
  }

  bool IsEmpty() const { return mFieldTextLength == 0; }

protected:
  nsXBLProtoImplField* mNext;
  PRUnichar* mName;
  PRUnichar* mFieldText;
  uint32_t mFieldTextLength;
  uint32_t mLineNumber;
  unsigned mJSAttributes;
};

#endif 
