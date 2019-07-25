





































#ifndef nsIScriptElement_h___
#define nsIScriptElement_h___

#include "nsISupports.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsIScriptLoaderObserver.h"
#include "nsWeakPtr.h"
#include "nsIParser.h"
#include "nsContentCreatorFunctions.h"
#include "nsIDOMHTMLScriptElement.h"

#define NS_ISCRIPTELEMENT_IID \
{ 0x6d625b30, 0xfac4, 0x11de, \
{ 0x8a, 0x39, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } }




class nsIScriptElement : public nsIScriptLoaderObserver {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTELEMENT_IID)

  nsIScriptElement(mozilla::dom::FromParser aFromParser)
    : mLineNumber(0),
      mAlreadyStarted(PR_FALSE),
      mMalformed(PR_FALSE),
      mDoneAddingChildren(aFromParser == mozilla::dom::NOT_FROM_PARSER ||
                          aFromParser == mozilla::dom::FROM_PARSER_FRAGMENT),
      mForceAsync(aFromParser == mozilla::dom::NOT_FROM_PARSER ||
                  aFromParser == mozilla::dom::FROM_PARSER_FRAGMENT),
      mFrozen(PR_FALSE),
      mDefer(PR_FALSE),
      mAsync(PR_FALSE),
      mExternal(PR_FALSE),
      mParserCreated(aFromParser == mozilla::dom::FROM_PARSER_FRAGMENT ?
                     mozilla::dom::NOT_FROM_PARSER : aFromParser),
                     
                     
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

  


  PRBool GetScriptExternal()
  {
    NS_PRECONDITION(mFrozen, "Not ready for this call yet!");
    return mExternal;
  }

  


  mozilla::dom::FromParser GetParserCreated()
  {
    return mParserCreated;
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
    mAlreadyStarted = PR_TRUE;
  }

  void LoseParserInsertedness()
  {
    mFrozen = PR_FALSE;
    mUri = nsnull;
    mCreatorParser = nsnull;
    mParserCreated = mozilla::dom::NOT_FROM_PARSER;
    PRBool async = PR_FALSE;
    nsCOMPtr<nsIDOMHTMLScriptElement> htmlScript = do_QueryInterface(this);
    if (htmlScript) {
      htmlScript->GetAsync(&async);
    }
    mForceAsync = !async;
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
  
  


  PRPackedBool mAlreadyStarted;
  
  


  PRPackedBool mMalformed;
  
  


  PRPackedBool mDoneAddingChildren;

  



  PRPackedBool mForceAsync;

  


  PRPackedBool mFrozen;
  
  


  PRPackedBool mDefer;
  
  


  PRPackedBool mAsync;
  
  



  PRPackedBool mExternal;

  


  mozilla::dom::FromParser mParserCreated;

  


  nsCOMPtr<nsIURI> mUri;
  
  


  nsWeakPtr mCreatorParser;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptElement, NS_ISCRIPTELEMENT_IID)

#endif 
