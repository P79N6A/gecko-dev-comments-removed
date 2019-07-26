




#ifndef nsXBLProtoImplMethod_h__
#define nsXBLProtoImplMethod_h__

#include "mozilla/Attributes.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsString.h"
#include "nsXBLMaybeCompiled.h"
#include "nsXBLProtoImplMember.h"
#include "nsXBLSerialize.h"

class nsIContent;

struct nsXBLParameter {
  nsXBLParameter* mNext;
  char* mName;

  nsXBLParameter(const nsAString& aName) {
    MOZ_COUNT_CTOR(nsXBLParameter);
    mName = ToNewCString(aName);
    mNext = nullptr;
  }

  ~nsXBLParameter() {
    MOZ_COUNT_DTOR(nsXBLParameter);
    nsMemory::Free(mName);
    NS_CONTENT_DELETE_LIST_MEMBER(nsXBLParameter, this, mNext);
  }
};

struct nsXBLUncompiledMethod {
  nsXBLParameter* mParameters;
  nsXBLParameter* mLastParameter;
  nsXBLTextWithLineNumber mBodyText;

  nsXBLUncompiledMethod() :
    mParameters(nullptr),
    mLastParameter(nullptr),
    mBodyText()
  {
    MOZ_COUNT_CTOR(nsXBLUncompiledMethod);
  }

  ~nsXBLUncompiledMethod() {
    MOZ_COUNT_DTOR(nsXBLUncompiledMethod);
    delete mParameters;
  }

  int32_t GetParameterCount() {
    int32_t result = 0;
    for (nsXBLParameter* curr = mParameters; curr; curr=curr->mNext)
      result++;
    return result;
  }

  void AppendBodyText(const nsAString& aText) {
    mBodyText.AppendText(aText);
  }

  void AddParameter(const nsAString& aText) {
    nsXBLParameter* param = new nsXBLParameter(aText);
    if (!param)
      return;
    if (!mParameters)
      mParameters = param;
    else
      mLastParameter->mNext = param;
    mLastParameter = param;
  }

  void SetLineNumber(uint32_t aLineNumber) {
    mBodyText.SetLineNumber(aLineNumber);
  }
};

class nsXBLProtoImplMethod: public nsXBLProtoImplMember
{
public:
  nsXBLProtoImplMethod(const char16_t* aName);
  virtual ~nsXBLProtoImplMethod();

  void AppendBodyText(const nsAString& aBody);
  void AddParameter(const nsAString& aName);

  void SetLineNumber(uint32_t aLineNumber);
  
  virtual nsresult InstallMember(JSContext* aCx,
                                 JS::Handle<JSObject*> aTargetClassObject) MOZ_OVERRIDE;
  virtual nsresult CompileMember(const nsCString& aClassStr,
                                 JS::Handle<JSObject*> aClassObject) MOZ_OVERRIDE;

  virtual void Trace(const TraceCallbacks& aCallbacks, void *aClosure) MOZ_OVERRIDE;

  nsresult Read(nsIObjectInputStream* aStream);
  virtual nsresult Write(nsIObjectOutputStream* aStream) MOZ_OVERRIDE;

  bool IsCompiled() const
  {
    return mMethod.IsCompiled();
  }

  void SetUncompiledMethod(nsXBLUncompiledMethod* aUncompiledMethod)
  {
    mMethod.SetUncompiled(aUncompiledMethod);
  }

  nsXBLUncompiledMethod* GetUncompiledMethod() const
  {
    return mMethod.GetUncompiled();
  }

protected:
  void SetCompiledMethod(JSObject* aCompiledMethod)
  {
    mMethod.SetJSFunction(aCompiledMethod);
  }

  JSObject* GetCompiledMethod() const
  {
    return mMethod.GetJSFunction();
  }

  JSObject* GetCompiledMethodPreserveColor() const
  {
    return mMethod.GetJSFunctionPreserveColor();
  }

  JS::Heap<nsXBLMaybeCompiled<nsXBLUncompiledMethod> > mMethod;
};

class nsXBLProtoImplAnonymousMethod : public nsXBLProtoImplMethod {
public:
  nsXBLProtoImplAnonymousMethod(const char16_t* aName) :
    nsXBLProtoImplMethod(aName)
  {}
  
  nsresult Execute(nsIContent* aBoundElement, JSAddonId* aAddonId);

  
  
  
  virtual nsresult InstallMember(JSContext* aCx,
                                 JS::Handle<JSObject*> aTargetClassObject) MOZ_OVERRIDE {
    return NS_OK;
  }

  using nsXBLProtoImplMethod::Write;
  nsresult Write(nsIObjectOutputStream* aStream,
                 XBLBindingSerializeDetails aType);
};

#endif 
