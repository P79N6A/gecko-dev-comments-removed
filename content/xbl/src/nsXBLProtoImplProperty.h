




#ifndef nsXBLProtoImplProperty_h__
#define nsXBLProtoImplProperty_h__

#include "mozilla/Attributes.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsString.h"
#include "nsXBLSerialize.h"
#include "nsXBLMaybeCompiled.h"
#include "nsXBLProtoImplMember.h"

class nsXBLProtoImplProperty: public nsXBLProtoImplMember
{
public:
  nsXBLProtoImplProperty(const PRUnichar* aName,
                         const PRUnichar* aGetter, 
                         const PRUnichar* aSetter,
                         const PRUnichar* aReadOnly,
                         uint32_t aLineNumber);

  nsXBLProtoImplProperty(const PRUnichar* aName, const bool aIsReadOnly);
 
  virtual ~nsXBLProtoImplProperty();

  void AppendGetterText(const nsAString& aGetter);
  void AppendSetterText(const nsAString& aSetter);

  void SetGetterLineNumber(uint32_t aLineNumber);
  void SetSetterLineNumber(uint32_t aLineNumber);

  virtual nsresult InstallMember(JSContext* aCx,
                                 JS::Handle<JSObject*> aTargetClassObject) MOZ_OVERRIDE;
  virtual nsresult CompileMember(const nsCString& aClassStr,
                                 JS::Handle<JSObject*> aClassObject) MOZ_OVERRIDE;

  virtual void Trace(const TraceCallbacks& aCallback, void *aClosure) MOZ_OVERRIDE;

  nsresult Read(nsIObjectInputStream* aStream,
                XBLBindingSerializeDetails aType);
  virtual nsresult Write(nsIObjectOutputStream* aStream) MOZ_OVERRIDE;

protected:
  typedef JS::Heap<nsXBLMaybeCompiled<nsXBLTextWithLineNumber> > PropertyOp;

  void EnsureUncompiledText(PropertyOp& aPropertyOp);

  
  PropertyOp mGetter;

  
  PropertyOp mSetter;
  
  unsigned mJSAttributes;  

#ifdef DEBUG
  bool mIsCompiled;
#endif
};

#endif 
