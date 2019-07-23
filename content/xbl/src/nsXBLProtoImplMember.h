





































#ifndef nsXBLProtoImplMember_h__
#define nsXBLProtoImplMember_h__

#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsIJSRuntimeService.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"

class nsIScriptContext;
struct JSRuntime;
class nsIJSRuntimeService;

struct nsXBLTextWithLineNumber
{
  PRUnichar* mText;
  PRUint32 mLineNumber;

  nsXBLTextWithLineNumber() :
    mText(nsnull),
    mLineNumber(0)
  {
    MOZ_COUNT_CTOR(nsXBLTextWithLineNumber);
  }

  ~nsXBLTextWithLineNumber() {
    MOZ_COUNT_DTOR(nsXBLTextWithLineNumber);
    if (mText) {
      nsMemory::Free(mText);
    }
  }

  void AppendText(const nsAString& aText) {
    if (mText) {
      PRUnichar* temp = mText;
      mText = ToNewUnicode(nsDependentString(temp) + aText);
      nsMemory::Free(temp);
    } else {
      mText = ToNewUnicode(aText);
    }
  }

  PRUnichar* GetText() {
    return mText;
  }

  void SetLineNumber(PRUint32 aLineNumber) {
    mLineNumber = aLineNumber;
  }

  PRUint32 GetLineNumber() {
    return mLineNumber;
  }
};

class nsXBLProtoImplMember
{
public:
  nsXBLProtoImplMember(const PRUnichar* aName) :mNext(nsnull) { mName = ToNewUnicode(nsDependentString(aName)); }
  virtual ~nsXBLProtoImplMember() { nsMemory::Free(mName); delete mNext; }
  virtual void Destroy(PRBool aIsCompiled)=0;

  nsXBLProtoImplMember* GetNext() { return mNext; }
  void SetNext(nsXBLProtoImplMember* aNext) { mNext = aNext; }

  virtual nsresult InstallMember(nsIScriptContext* aContext,
                                 nsIContent* aBoundElement, 
                                 void* aScriptObject,
                                 void* aTargetClassObject,
                                 const nsCString& aClassStr) = 0;
  virtual nsresult CompileMember(nsIScriptContext* aContext,
                                 const nsCString& aClassStr,
                                 void* aClassObject)=0;

protected:
  friend class nsAutoGCRoot;
  
  nsXBLProtoImplMember* mNext;  
  PRUnichar* mName;               
};

#endif 
