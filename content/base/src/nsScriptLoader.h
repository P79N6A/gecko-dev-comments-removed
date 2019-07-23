









































#ifndef __nsScriptLoader_h__
#define __nsScriptLoader_h__

#include "nsCOMPtr.h"
#include "nsIScriptElement.h"
#include "nsIURI.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
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
    ++mBlockerCount;
  }
  void RemoveExecuteBlocker()
  {
    if (!--mBlockerCount) {
      ProcessPendingRequestsAsync();
    }
  }

  










  static nsresult ConvertToUTF16(nsIChannel* aChannel, const PRUint8* aData,
                                 PRUint32 aLength,
                                 const nsString& aHintCharset,
                                 nsIDocument* aDocument, nsString& aString);

  


  void ProcessPendingRequests();

  



  static nsresult ShouldLoadScript(nsIDocument* aDocument,
                                   nsISupports* aContext,
                                   nsIURI* aURI,
                                   const nsAString &aType);

  



  static PRBool ShouldExecuteScript(nsIDocument* aDocument,
                                    nsIChannel* aChannel);

  



  void BeginDeferringScripts()
  {
    mDeferEnabled = PR_TRUE;
    if (mDocument) {
      mDocument->BlockOnload();
    }
  }

  








  void ParsingComplete(PRBool aTerminated);

  


  PRUint32 HasPendingOrCurrentScripts()
  {
    return mCurrentScript || GetFirstPendingRequest();
  }

  






  virtual void PreloadURI(nsIURI *aURI, const nsAString &aCharset,
                          const nsAString &aType);

protected:
  


  static nsresult CheckContentPolicy(nsIDocument* aDocument,
                                     nsISupports *aContext,
                                     nsIURI *aURI,
                                     const nsAString &aType);

  


  nsresult StartLoad(nsScriptLoadRequest *aRequest, const nsAString &aType);

  






  virtual void ProcessPendingRequestsAsync();

  





  PRBool ReadyToExecuteScripts();

  


  PRBool SelfReadyToExecuteScripts()
  {
    return mEnabled && !mBlockerCount;
  }

  PRBool AddPendingChildLoader(nsScriptLoader* aChild) {
    return mPendingChildLoaders.AppendElement(aChild) != nsnull;
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

  
  nsScriptLoadRequest* GetFirstPendingRequest();

  nsIDocument* mDocument;                   
  nsCOMArray<nsIScriptLoaderObserver> mObservers;
  nsCOMArray<nsScriptLoadRequest> mRequests;
  nsCOMArray<nsScriptLoadRequest> mAsyncRequests;

  
  struct PreloadInfo {
    nsRefPtr<nsScriptLoadRequest> mRequest;
    nsString mCharset;
  };

  struct PreloadRequestComparator {
    PRBool Equals(const PreloadInfo &aPi, nsScriptLoadRequest * const &aRequest)
        const
    {
      return aRequest == aPi.mRequest;
    }
  };
  struct PreloadURIComparator {
    PRBool Equals(const PreloadInfo &aPi, nsIURI * const &aURI) const;
  };
  nsTArray<PreloadInfo> mPreloads;

  nsCOMPtr<nsIScriptElement> mCurrentScript;
  
  nsTArray< nsRefPtr<nsScriptLoader> > mPendingChildLoaders;
  PRUint32 mBlockerCount;
  PRPackedBool mEnabled;
  PRPackedBool mDeferEnabled;
  PRPackedBool mUnblockOnloadWhenDoneProcessing;
};

#endif 
