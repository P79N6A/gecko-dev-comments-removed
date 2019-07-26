








#ifndef __nsScriptLoader_h__
#define __nsScriptLoader_h__

#include "nsCOMPtr.h"
#include "nsIScriptElement.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsIDocument.h"
#include "nsIStreamLoader.h"

class nsScriptLoadRequest;
class nsIURI;





class nsScriptLoader : public nsIStreamLoaderObserver
{
  friend class nsScriptRequestProcessor;
public:
  nsScriptLoader(nsIDocument* aDocument);
  virtual ~nsScriptLoader();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER

  




  void DropDocumentReference()
  {
    mDocument = nullptr;
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
  
  















  bool ProcessScriptElement(nsIScriptElement* aElement);

  



  nsIScriptElement* GetCurrentScript()
  {
    return mCurrentScript;
  }

  nsIScriptElement* GetCurrentParserInsertedScript()
  {
    return mCurrentParserInsertedScript;
  }

  





  bool GetEnabled()
  {
    return mEnabled;
  }
  void SetEnabled(bool aEnabled)
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

  










  static nsresult ConvertToUTF16(nsIChannel* aChannel, const uint8_t* aData,
                                 uint32_t aLength,
                                 const nsAString& aHintCharset,
                                 nsIDocument* aDocument, nsString& aString);

  


  void ProcessPendingRequests();

  



  static nsresult ShouldLoadScript(nsIDocument* aDocument,
                                   nsISupports* aContext,
                                   nsIURI* aURI,
                                   const nsAString &aType);

  



  void BeginDeferringScripts()
  {
    mDeferEnabled = true;
    if (mDocument) {
      mDocument->BlockOnload();
    }
  }

  








  void ParsingComplete(bool aTerminated);

  


  uint32_t HasPendingOrCurrentScripts()
  {
    return mCurrentScript || mParserBlockingRequest;
  }

  









  virtual void PreloadURI(nsIURI *aURI, const nsAString &aCharset,
                          const nsAString &aType,
                          const nsAString &aCrossOrigin,
                          bool aScriptFromHead);

private:
  


  void UnblockParser(nsScriptLoadRequest* aParserBlockingRequest);

  


  void ContinueParserAsync(nsScriptLoadRequest* aParserBlockingRequest);


  


  static nsresult CheckContentPolicy(nsIDocument* aDocument,
                                     nsISupports *aContext,
                                     nsIURI *aURI,
                                     const nsAString &aType);

  


  nsresult StartLoad(nsScriptLoadRequest *aRequest, const nsAString &aType,
                     bool aScriptFromHead);

  






  virtual void ProcessPendingRequestsAsync();

  





  bool ReadyToExecuteScripts();

  


  bool SelfReadyToExecuteScripts()
  {
    return mEnabled && !mBlockerCount;
  }

  bool AddPendingChildLoader(nsScriptLoader* aChild) {
    return mPendingChildLoaders.AppendElement(aChild) != nullptr;
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
                                uint32_t aStringLen,
                                const uint8_t* aString);

  nsIDocument* mDocument;                   
  nsCOMArray<nsIScriptLoaderObserver> mObservers;
  nsTArray<nsRefPtr<nsScriptLoadRequest> > mNonAsyncExternalScriptInsertedRequests;
  nsTArray<nsRefPtr<nsScriptLoadRequest> > mAsyncRequests;
  nsTArray<nsRefPtr<nsScriptLoadRequest> > mDeferRequests;
  nsTArray<nsRefPtr<nsScriptLoadRequest> > mXSLTRequests;
  nsRefPtr<nsScriptLoadRequest> mParserBlockingRequest;

  
  struct PreloadInfo {
    nsRefPtr<nsScriptLoadRequest> mRequest;
    nsString mCharset;
  };

  struct PreloadRequestComparator {
    bool Equals(const PreloadInfo &aPi, nsScriptLoadRequest * const &aRequest)
        const
    {
      return aRequest == aPi.mRequest;
    }
  };
  struct PreloadURIComparator {
    bool Equals(const PreloadInfo &aPi, nsIURI * const &aURI) const;
  };
  nsTArray<PreloadInfo> mPreloads;

  nsCOMPtr<nsIScriptElement> mCurrentScript;
  nsCOMPtr<nsIScriptElement> mCurrentParserInsertedScript;
  
  nsTArray< nsRefPtr<nsScriptLoader> > mPendingChildLoaders;
  uint32_t mBlockerCount;
  bool mEnabled;
  bool mDeferEnabled;
  bool mDocumentParsingDone;
};

class nsAutoScriptLoaderDisabler
{
public:
  nsAutoScriptLoaderDisabler(nsIDocument* aDoc)
  {
    mLoader = aDoc->ScriptLoader();
    mWasEnabled = mLoader->GetEnabled();
    if (mWasEnabled) {
      mLoader->SetEnabled(false);
    }
  }
  
  ~nsAutoScriptLoaderDisabler()
  {
    if (mWasEnabled) {
      mLoader->SetEnabled(true);
    }
  }
  
  bool mWasEnabled;
  nsRefPtr<nsScriptLoader> mLoader;
};

#endif 
