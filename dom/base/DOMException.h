




#ifndef mozilla_dom_DOMException_h__
#define mozilla_dom_DOMException_h__

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#include <stdint.h>
#include "jspubtd.h"
#include "js/GCAPI.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsID.h"
#include "nsIDOMDOMException.h"
#include "nsWrapperCache.h"
#include "xpcexception.h"

class nsIStackFrame;
class nsString;

nsresult
NS_GetNameAndMessageForDOMNSResult(nsresult aNSResult, const char** aName,
                                   const char** aMessage,
                                   uint16_t* aCode = nullptr);

namespace mozilla {
namespace dom {

#define MOZILLA_DOM_EXCEPTION_IID \
{ 0x55eda557, 0xeba0, 0x4fe3, \
  { 0xae, 0x2e, 0xf3, 0x94, 0x49, 0x23, 0x62, 0xd6 } }

class Exception : public nsIXPCException,
                  public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOM_EXCEPTION_IID)

  NS_DEFINE_STATIC_CID_ACCESSOR(NS_XPCEXCEPTION_CID)

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Exception)
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIEXCEPTION
  NS_DECL_NSIXPCEXCEPTION

  
  
  bool StealJSVal(JS::Value* aVp);
  void StowJSVal(JS::Value& aVp);

  
  virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> scope)
    MOZ_OVERRIDE;

  nsISupports* GetParentObject() const { return nullptr; }

  void GetMessageMoz(nsString& retval);

  uint32_t Result() const;

  void GetName(nsString& retval);

  void GetFilename(nsString& retval);

  uint32_t LineNumber() const;

  uint32_t ColumnNumber() const;

  already_AddRefed<nsIStackFrame> GetLocation() const;

  already_AddRefed<nsISupports> GetInner() const;

  already_AddRefed<nsISupports> GetData() const;

  void Stringify(nsString& retval);

  
  Exception();

  Exception(const char *aMessage,
            nsresult aResult,
            const char *aName,
            nsIStackFrame *aLocation,
            nsISupports *aData);

protected:
  virtual ~Exception();

  char*           mMessage;
  nsresult        mResult;
  char*           mName;
  nsCOMPtr<nsIStackFrame> mLocation;
  nsCOMPtr<nsISupports> mData;
  char*           mFilename;
  int             mLineNumber;
  nsCOMPtr<nsIException> mInner;
  bool            mInitialized;

  bool mHoldingJSVal;
  JS::Heap<JS::Value> mThrownJSVal;

private:
  static bool sEverMadeOneFromFactory;
};

NS_DEFINE_STATIC_IID_ACCESSOR(Exception, MOZILLA_DOM_EXCEPTION_IID)

class DOMException : public Exception,
                     public nsIDOMDOMException
{
public:
  DOMException(nsresult aRv, const char* aMessage,
               const char* aName, uint16_t aCode);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDOMEXCEPTION

  
  NS_IMETHOD ToString(char **aReturn) MOZ_OVERRIDE;

  
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
    MOZ_OVERRIDE;

  uint16_t Code() const {
    return mCode;
  }

  
  void GetMessageMoz(nsString& retval);
  void GetName(nsString& retval);

  static already_AddRefed<DOMException>
  Create(nsresult aRv);

protected:

  virtual ~DOMException() {}

  
  const char* mName;
  const char* mMessage;

  uint16_t mCode;
};

} 
} 

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif
