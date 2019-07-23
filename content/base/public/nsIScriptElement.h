





































#ifndef nsIScriptElement_h___
#define nsIScriptElement_h___

#include "nsISupports.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsIScriptLoaderObserver.h"

#define NS_ISCRIPTELEMENT_IID \
{ 0x4b916da5, 0x82c4, 0x45ab, \
  { 0x99, 0x15, 0xcc, 0xcd, 0x9e, 0x2c, 0xb1, 0xe6 } }




class nsIScriptElement : public nsIScriptLoaderObserver {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTELEMENT_IID)

  nsIScriptElement()
    : mLineNumber(0),
      mIsEvaluated(PR_FALSE),
      mMalformed(PR_FALSE),
      mDoneAddingChildren(PR_TRUE)
  {
  }

  



  virtual void GetScriptType(nsAString& type) = 0;
    
  



  virtual already_AddRefed<nsIURI> GetScriptURI() = 0;
  
  


  virtual void GetScriptText(nsAString& text) = 0;

  virtual void GetScriptCharset(nsAString& charset) = 0;

  


  virtual PRBool GetScriptDeferred() = 0;

  void SetScriptLineNumber(PRUint32 aLineNumber)
  {
    mLineNumber = aLineNumber;
  }
  PRUint32 GetScriptLineNumber()
  {
    return mLineNumber;
  }

  void SetIsMalformed()
  {
    mMalformed = PR_TRUE;
  }
  PRBool IsMalformed()
  {
    return mMalformed;
  }

  void PreventExecution()
  {
    mIsEvaluated = PR_TRUE;
  }

  void WillCallDoneAddingChildren()
  {
    NS_ASSERTION(mDoneAddingChildren, "unexpected, but not fatal");
    mDoneAddingChildren = PR_FALSE;
  }

protected:
  PRUint32 mLineNumber;
  PRPackedBool mIsEvaluated;
  PRPackedBool mMalformed;
  PRPackedBool mDoneAddingChildren;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptElement, NS_ISCRIPTELEMENT_IID)

#endif 
