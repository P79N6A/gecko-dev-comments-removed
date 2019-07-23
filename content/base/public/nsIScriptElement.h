





































#ifndef nsIScriptElement_h___
#define nsIScriptElement_h___

#include "nsISupports.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsIScriptLoaderObserver.h"
#include "nsWeakPtr.h"
#include "nsIParser.h"

#define NS_ISCRIPTELEMENT_IID \
{ 0xa28c198e, 0x14f0, 0x42b1, \
{ 0x8f, 0x6b, 0x0e, 0x7f, 0xca, 0xb4, 0xf4, 0xe8 } }




class nsIScriptElement : public nsIScriptLoaderObserver {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTELEMENT_IID)

  nsIScriptElement()
    : mLineNumber(0),
      mIsEvaluated(PR_FALSE),
      mMalformed(PR_FALSE),
      mDoneAddingChildren(PR_TRUE),
      mCreatorParser(nsnull)
  {
  }

  



  virtual void GetScriptType(nsAString& type) = 0;
    
  



  virtual already_AddRefed<nsIURI> GetScriptURI() = 0;
  
  


  virtual void GetScriptText(nsAString& text) = 0;

  virtual void GetScriptCharset(nsAString& charset) = 0;

  


  virtual PRBool GetScriptDeferred() = 0;

  


  virtual PRBool GetScriptAsync() = 0;

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
  nsWeakPtr    mCreatorParser;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptElement, NS_ISCRIPTELEMENT_IID)

#endif 
