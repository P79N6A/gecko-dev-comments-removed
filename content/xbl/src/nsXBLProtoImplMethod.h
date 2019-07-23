





































#ifndef nsXBLProtoImplMethod_h__
#define nsXBLProtoImplMethod_h__

#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsXBLProtoImplMember.h"

struct nsXBLParameter {
  nsXBLParameter* mNext;
  char* mName;

  nsXBLParameter(const nsAString& aName) {
    MOZ_COUNT_CTOR(nsXBLParameter);
    mName = ToNewCString(aName);
    mNext = nsnull;
  }

  ~nsXBLParameter() {
    MOZ_COUNT_DTOR(nsXBLParameter);
    nsMemory::Free(mName);
    delete mNext;
  }
};

struct nsXBLUncompiledMethod {
  nsXBLParameter* mParameters;
  nsXBLParameter* mLastParameter;
  nsXBLTextWithLineNumber mBodyText;

  nsXBLUncompiledMethod() :
    mParameters(nsnull),
    mLastParameter(nsnull),
    mBodyText()
  {
    MOZ_COUNT_CTOR(nsXBLUncompiledMethod);
  }

  ~nsXBLUncompiledMethod() {
    MOZ_COUNT_DTOR(nsXBLUncompiledMethod);
    delete mParameters;
  }

  PRInt32 GetParameterCount() {
    PRInt32 result = 0;
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

  void SetLineNumber(PRUint32 aLineNumber) {
    mBodyText.SetLineNumber(aLineNumber);
  }
};

class nsXBLProtoImplMethod: public nsXBLProtoImplMember
{
public:
  nsXBLProtoImplMethod(const PRUnichar* aName);
  virtual ~nsXBLProtoImplMethod();
  virtual void Destroy(PRBool aIsCompiled);

  void AppendBodyText(const nsAString& aBody);
  void AddParameter(const nsAString& aName);

  void SetLineNumber(PRUint32 aLineNumber);
  
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
    nsXBLUncompiledMethod* mUncompiledMethod; 
    JSObject * mJSMethodObject;               
  };

#ifdef DEBUG
  PRBool mIsCompiled;
#endif
};

class nsXBLProtoImplAnonymousMethod : public nsXBLProtoImplMethod {
public:
  nsXBLProtoImplAnonymousMethod() :
    nsXBLProtoImplMethod(EmptyString().get())
  {}
  
  nsresult Execute(nsIContent* aBoundElement);

  
  
  
  virtual nsresult InstallMember(nsIScriptContext* aContext,
                                 nsIContent* aBoundElement, 
                                 void* aScriptObject,
                                 void* aTargetClassObject,
                                 const nsCString& aClassStr) {
    return NS_OK;
  }
};

#endif 
