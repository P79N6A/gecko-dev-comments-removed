





































#ifndef nsIScriptElement_h___
#define nsIScriptElement_h___

#include "nsISupports.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsIScriptLoaderObserver.h"
#include "nsWeakPtr.h"
#include "nsIParser.h"

#define NS_ISCRIPTELEMENT_IID \
{ 0xa9d5732a, 0x8c1f, 0x439d, \
{ 0x83, 0x75, 0x3d, 0xf6, 0xa9, 0xba, 0xa3, 0x7d } }




class nsIScriptElement : public nsIScriptLoaderObserver {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTELEMENT_IID)

  nsIScriptElement()
    : mLineNumber(0),
      mIsEvaluated(PR_FALSE),
      mMalformed(PR_FALSE),
      mDoneAddingChildren(PR_TRUE),
      mFrozen(PR_FALSE),
      mDefer(PR_FALSE),
      mAsync(PR_FALSE),
      mCreatorParser(nsnull)
  {
  }

  



  virtual void GetScriptType(nsAString& type) = 0;
    
  



  nsIURI* GetScriptURI()
  {
    NS_PRECONDITION(mFrozen, "Not ready for this call yet!");
    return mUri;
  }
  
  


  virtual void GetScriptText(nsAString& text) = 0;

  virtual void GetScriptCharset(nsAString& charset) = 0;

  




  virtual void FreezeUriAsyncDefer() = 0;

  


  PRBool GetScriptDeferred()
  {
    NS_PRECONDITION(mFrozen, "Not ready for this call yet!");
    return mDefer;
  }

  


  PRBool GetScriptAsync()
  {
    NS_PRECONDITION(mFrozen, "Not ready for this call yet!");
    return mAsync;  
  }

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

  void SetCreatorParser(nsIParser* aParser)
  {
    mCreatorParser = getter_AddRefs(NS_GetWeakReference(aParser));
  }

  


  void BeginEvaluating()
  {
    nsCOMPtr<nsIParser> parser = do_QueryReferent(mCreatorParser);
    if (parser) {
      parser->BeginEvaluatingParserInsertedScript();
    }
  }

  


  void EndEvaluating()
  {
    nsCOMPtr<nsIParser> parser = do_QueryReferent(mCreatorParser);
    if (parser) {
      parser->EndEvaluatingParserInsertedScript();
    }
  }
  
  


  already_AddRefed<nsIParser> GetCreatorParser()
  {
    nsCOMPtr<nsIParser> parser = do_QueryReferent(mCreatorParser);
    return parser.forget();
  }

protected:
  


  PRUint32 mLineNumber;
  
  


  PRPackedBool mIsEvaluated;
  
  


  PRPackedBool mMalformed;
  
  


  PRPackedBool mDoneAddingChildren;

  


  PRPackedBool mFrozen;
  
  


  PRPackedBool mDefer;
  
  


  PRPackedBool mAsync;
  
  


  nsCOMPtr<nsIURI> mUri;
  
  


  nsWeakPtr mCreatorParser;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptElement, NS_ISCRIPTELEMENT_IID)

#endif 
