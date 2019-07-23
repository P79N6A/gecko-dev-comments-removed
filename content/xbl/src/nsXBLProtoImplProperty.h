





































#ifndef nsXBLProtoImplProperty_h__
#define nsXBLProtoImplProperty_h__

#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsXBLProtoImplMember.h"

class nsXBLProtoImplProperty: public nsXBLProtoImplMember
{
public:
  nsXBLProtoImplProperty(const PRUnichar* aName,
                         const PRUnichar* aGetter, 
                         const PRUnichar* aSetter,
                         const PRUnichar* aReadOnly);
 
  virtual ~nsXBLProtoImplProperty();
  virtual void Destroy(PRBool aIsCompiled);

  void AppendGetterText(const nsAString& aGetter);
  void AppendSetterText(const nsAString& aSetter);

  void SetGetterLineNumber(PRUint32 aLineNumber);
  void SetSetterLineNumber(PRUint32 aLineNumber);

  virtual nsresult InstallMember(nsIScriptContext* aContext,
                                 nsIContent* aBoundElement, 
                                 void* aScriptObject,
                                 void* aTargetClassObject,
                                 const nsCString& aClassStr);
  virtual nsresult CompileMember(nsIScriptContext* aContext,
                                 const nsCString& aClassStr,
                                 void* aClassObject);

protected:
  union {
    
    nsXBLTextWithLineNumber* mGetterText;
    
    JSObject *               mJSGetterObject;
  };

  union {
    
    nsXBLTextWithLineNumber* mSetterText;
    
    JSObject *               mJSSetterObject;
  };
  
  uintN mJSAttributes;          

#ifdef DEBUG
  PRBool mIsCompiled;
#endif
};

#endif 
