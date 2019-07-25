





































#ifndef nsXBLProtoImplProperty_h__
#define nsXBLProtoImplProperty_h__

#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsXBLSerialize.h"
#include "nsXBLProtoImplMember.h"

class nsXBLProtoImplProperty: public nsXBLProtoImplMember
{
public:
  nsXBLProtoImplProperty(const PRUnichar* aName,
                         const PRUnichar* aGetter, 
                         const PRUnichar* aSetter,
                         const PRUnichar* aReadOnly);

  nsXBLProtoImplProperty(const PRUnichar* aName, const bool aIsReadOnly);
 
  virtual ~nsXBLProtoImplProperty();

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

  virtual void Trace(TraceCallback aCallback, void *aClosure) const;

  nsresult Read(nsIScriptContext* aContext,
                nsIObjectInputStream* aStream,
                XBLBindingSerializeDetails aType);
  virtual nsresult Write(nsIScriptContext* aContext,
                         nsIObjectOutputStream* aStream);

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
  bool mIsCompiled;
#endif
};

#endif 
