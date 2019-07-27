




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
  nsXBLProtoImplProperty(const char16_t* aName,
                         const char16_t* aGetter, 
                         const char16_t* aSetter,
                         const char16_t* aReadOnly,
                         uint32_t aLineNumber);

  nsXBLProtoImplProperty(const char16_t* aName, const bool aIsReadOnly);
 
  virtual ~nsXBLProtoImplProperty();

  void AppendGetterText(const nsAString& aGetter);
  void AppendSetterText(const nsAString& aSetter);

  void SetGetterLineNumber(uint32_t aLineNumber);
  void SetSetterLineNumber(uint32_t aLineNumber);

  virtual nsresult InstallMember(JSContext* aCx,
                                 JS::Handle<JSObject*> aTargetClassObject) override;
  virtual nsresult CompileMember(mozilla::dom::AutoJSAPI& jsapi, const nsString& aClassStr,
                                 JS::Handle<JSObject*> aClassObject) override;

  virtual void Trace(const TraceCallbacks& aCallback, void *aClosure) override;

  nsresult Read(nsIObjectInputStream* aStream,
                XBLBindingSerializeDetails aType);
  virtual nsresult Write(nsIObjectOutputStream* aStream) override;

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
