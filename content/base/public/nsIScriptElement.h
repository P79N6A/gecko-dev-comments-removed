





































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
{ 0x5bb3b905, 0x5988, 0x476f, \
  { 0x95, 0x4f, 0x99, 0x02, 0x59, 0x82, 0x24, 0x67 } }




class nsIScriptElement : public nsIScriptLoaderObserver {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTELEMENT_IID)

  nsIScriptElement(mozilla::dom::FromParser aFromParser)
    : mLineNumber(0),
      mAlreadyStarted(false),
      mMalformed(false),
      mDoneAddingChildren(aFromParser == mozilla::dom::NOT_FROM_PARSER ||
                          aFromParser == mozilla::dom::FROM_PARSER_FRAGMENT),
      mForceAsync(aFromParser == mozilla::dom::NOT_FROM_PARSER ||
                  aFromParser == mozilla::dom::FROM_PARSER_FRAGMENT),
      mFrozen(false),
      mDefer(false),
      mAsync(false),
      mExternal(false),
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

  


  bool GetScriptDeferred()
  {
    NS_PRECONDITION(mFrozen, "Not ready for this call yet!");
    return mDefer;
  }

  


  bool GetScriptAsync()
  {
    NS_PRECONDITION(mFrozen, "Not ready for this call yet!");
    return mAsync;  
  }

  


  bool GetScriptExternal()
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
    mMalformed = true;
  }
  bool IsMalformed()
  {
    return mMalformed;
  }

  void PreventExecution()
  {
    mAlreadyStarted = true;
  }

  void LoseParserInsertedness()
  {
    mFrozen = false;
    mUri = nsnull;
    mCreatorParser = nsnull;
    mParserCreated = mozilla::dom::NOT_FROM_PARSER;
    bool async = false;
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

  






  bool AttemptToExecute()
  {
    mDoneAddingChildren = true;
    bool block = MaybeProcessScript();
    if (!mAlreadyStarted) {
      
      
      LoseParserInsertedness();
    }
    return block;
  }

protected:
  















  virtual bool MaybeProcessScript() = 0;

  


  PRUint32 mLineNumber;
  
  


  bool mAlreadyStarted;
  
  


  bool mMalformed;
  
  


  bool mDoneAddingChildren;

  



  bool mForceAsync;

  


  bool mFrozen;
  
  


  bool mDefer;
  
  


  bool mAsync;
  
  



  bool mExternal;

  


  mozilla::dom::FromParser mParserCreated;

  


  nsCOMPtr<nsIURI> mUri;
  
  


  nsWeakPtr mCreatorParser;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptElement, NS_ISCRIPTELEMENT_IID)

#endif 
