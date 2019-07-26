




#ifndef nsXBLProtoImplMember_h__
#define nsXBLProtoImplMember_h__

#include "nsIAtom.h"
#include "nsString.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsContentUtils.h" 
#include "nsCycleCollectionParticipant.h"

class nsIContent;
class nsIObjectOutputStream;

struct nsXBLTextWithLineNumber
{
  PRUnichar* mText;
  uint32_t mLineNumber;

  nsXBLTextWithLineNumber() :
    mText(nullptr),
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

  void SetLineNumber(uint32_t aLineNumber) {
    mLineNumber = aLineNumber;
  }

  uint32_t GetLineNumber() {
    return mLineNumber;
  }
};

class nsXBLProtoImplMember
{
public:
  nsXBLProtoImplMember(const PRUnichar* aName) :mNext(nullptr) { mName = ToNewUnicode(nsDependentString(aName)); }
  virtual ~nsXBLProtoImplMember() {
    nsMemory::Free(mName);
    NS_CONTENT_DELETE_LIST_MEMBER(nsXBLProtoImplMember, this, mNext);
  }

  nsXBLProtoImplMember* GetNext() { return mNext; }
  void SetNext(nsXBLProtoImplMember* aNext) { mNext = aNext; }
  const PRUnichar* GetName() { return mName; }

  virtual nsresult InstallMember(JSContext* aCx,
                                 JS::Handle<JSObject*> aTargetClassObject) = 0;
  virtual nsresult CompileMember(const nsCString& aClassStr,
                                 JS::Handle<JSObject*> aClassObject) = 0;

  virtual void Trace(const TraceCallbacks& aCallbacks, void *aClosure) = 0;

  virtual nsresult Write(nsIObjectOutputStream* aStream)
  {
    return NS_OK;
  }

protected:
  nsXBLProtoImplMember* mNext;  
  PRUnichar* mName;               
};

#endif 
