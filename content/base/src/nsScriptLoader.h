









































#ifndef __nsScriptLoader_h__
#define __nsScriptLoader_h__

#include "nsCOMPtr.h"
#include "nsIScriptElement.h"
#include "nsIURI.h"
#include "nsCOMArray.h"
#include "nsIDocument.h"
#include "nsIStreamLoader.h"

class nsScriptLoadRequest;





class nsScriptLoader : public nsIStreamLoaderObserver
{
public:
  nsScriptLoader(nsIDocument* aDocument);
  virtual ~nsScriptLoader();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER

  




  void DropDocumentReference()
  {
    mDocument = nsnull;
  }

  




  nsresult AddObserver(nsIScriptLoaderObserver* aObserver)
  {
    return mObservers.AppendObject(aObserver) ? NS_OK :
      NS_ERROR_OUT_OF_MEMORY;
  }

  




  void RemoveObserver(nsIScriptLoaderObserver* aObserver)
  {
    mObservers.RemoveObject(aObserver);
  }
  
  















  nsresult ProcessScriptElement(nsIScriptElement* aElement);

  



  nsIScriptElement* GetCurrentScript()
  {
    return mCurrentScript;
  }

  






  PRBool GetEnabled()
  {
    return mEnabled;
  }
  void SetEnabled(PRBool aEnabled)
  {
    if (!mEnabled && aEnabled) {
      ProcessPendingRequestsAsync();
    }
    mEnabled = aEnabled;
  }

  






  void AddExecuteBlocker()
  {
    if (!mBlockerCount++) {
      mHadPendingScripts = mPendingRequests.Count() != 0;
    }
  }
  void RemoveExecuteBlocker()
  {
    if (!--mBlockerCount) {
      
      
      
      
      
      
      if (mHadPendingScripts) {
        ProcessPendingRequestsAsync();
      }
      else {
        ProcessPendingRequests();
      }
    }
  }

  










  static nsresult ConvertToUTF16(nsIChannel* aChannel, const PRUint8* aData,
                                 PRUint32 aLength,
                                 const nsString& aHintCharset,
                                 nsIDocument* aDocument, nsString& aString);

  


  void ProcessPendingRequests();

protected:
  




  virtual void ProcessPendingRequestsAsync();

  PRBool ReadyToExecuteScripts()
  {
    return mEnabled && !mBlockerCount;
  }

  nsresult ProcessRequest(nsScriptLoadRequest* aRequest);
  void FireScriptAvailable(nsresult aResult,
                           nsScriptLoadRequest* aRequest);
  void FireScriptEvaluated(nsresult aResult,
                           nsScriptLoadRequest* aRequest);
  nsresult EvaluateScript(nsScriptLoadRequest* aRequest,
                          const nsAFlatString& aScript);

  nsresult PrepareLoadedRequest(nsScriptLoadRequest* aRequest,
                                nsIStreamLoader* aLoader,
                                nsresult aStatus,
                                PRUint32 aStringLen,
                                const PRUint8* aString);

  nsIDocument* mDocument;                   
  nsCOMArray<nsIScriptLoaderObserver> mObservers;
  nsCOMArray<nsScriptLoadRequest> mPendingRequests;
  nsCOMPtr<nsIScriptElement> mCurrentScript;
  PRUint32 mBlockerCount;
  PRPackedBool mEnabled;
  PRPackedBool mHadPendingScripts;
};

#endif 
