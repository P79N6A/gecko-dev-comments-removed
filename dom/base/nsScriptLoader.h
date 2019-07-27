








#ifndef __nsScriptLoader_h__
#define __nsScriptLoader_h__

#include "nsCOMPtr.h"
#include "nsIScriptElement.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsIDocument.h"
#include "nsIStreamLoader.h"
#include "mozilla/CORSMode.h"
#include "mozilla/LinkedList.h"
#include "mozilla/net/ReferrerPolicy.h"

class nsScriptLoadRequestList;
class nsIURI;

namespace JS {
  class SourceBufferHolder;
}

namespace mozilla {
namespace dom {
class AutoJSAPI;
}
}





class nsScriptLoadRequest final : public nsISupports,
                                  private mozilla::LinkedListElement<nsScriptLoadRequest>
{
  ~nsScriptLoadRequest()
  {
    js_free(mScriptTextBuf);
  }

  typedef LinkedListElement<nsScriptLoadRequest> super;

  
  friend class mozilla::LinkedListElement<nsScriptLoadRequest>;
  friend class nsScriptLoadRequestList;

public:
  nsScriptLoadRequest(nsIScriptElement* aElement,
                      uint32_t aVersion,
                      mozilla::CORSMode aCORSMode)
    : mElement(aElement),
      mLoading(true),
      mIsInline(true),
      mHasSourceMapURL(false),
      mIsDefer(false),
      mIsAsync(false),
      mIsNonAsyncScriptInserted(false),
      mIsXSLT(false),
      mScriptTextBuf(nullptr),
      mScriptTextLength(0),
      mJSVersion(aVersion),
      mLineNo(1),
      mCORSMode(aCORSMode),
      mReferrerPolicy(mozilla::net::RP_Default)
  {
  }

  NS_DECL_THREADSAFE_ISUPPORTS

  void FireScriptAvailable(nsresult aResult)
  {
    mElement->ScriptAvailable(aResult, mElement, mIsInline, mURI, mLineNo);
  }
  void FireScriptEvaluated(nsresult aResult)
  {
    mElement->ScriptEvaluated(aResult, mElement, mIsInline);
  }

  bool IsPreload()
  {
    return mElement == nullptr;
  }

  using super::getNext;
  using super::isInList;

  nsCOMPtr<nsIScriptElement> mElement;
  bool mLoading;          
  bool mIsInline;         
  bool mHasSourceMapURL;  
  bool mIsDefer;          
  bool mIsAsync;          
  bool mIsNonAsyncScriptInserted; 
  bool mIsXSLT;           
  nsString mSourceMapURL; 
  char16_t* mScriptTextBuf; 
  size_t mScriptTextLength; 
  uint32_t mJSVersion;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIPrincipal> mOriginPrincipal;
  nsAutoCString mURL;   
  int32_t mLineNo;
  const mozilla::CORSMode mCORSMode;
  mozilla::net::ReferrerPolicy mReferrerPolicy;
};

class nsScriptLoadRequestList : private mozilla::LinkedList<nsScriptLoadRequest>
{
  typedef mozilla::LinkedList<nsScriptLoadRequest> super;

public:
  ~nsScriptLoadRequestList();

  void Clear();

#ifdef DEBUG
  bool Contains(nsScriptLoadRequest* aElem);
#endif 

  using super::getFirst;
  using super::isEmpty;

  void AppendElement(nsScriptLoadRequest* aElem)
  {
    MOZ_ASSERT(!aElem->isInList());
    NS_ADDREF(aElem);
    insertBack(aElem);
  }

  MOZ_WARN_UNUSED_RESULT
  already_AddRefed<nsScriptLoadRequest> Steal(nsScriptLoadRequest* aElem)
  {
    aElem->removeFrom(*this);
    return dont_AddRef(aElem);
  }

  MOZ_WARN_UNUSED_RESULT
  already_AddRefed<nsScriptLoadRequest> StealFirst()
  {
    MOZ_ASSERT(!isEmpty());
    return Steal(getFirst());
  }
};





class nsScriptLoader final : public nsIStreamLoaderObserver
{
  class MOZ_STACK_CLASS AutoCurrentScriptUpdater
  {
  public:
    AutoCurrentScriptUpdater(nsScriptLoader* aScriptLoader,
                             nsIScriptElement* aCurrentScript)
      : mOldScript(aScriptLoader->mCurrentScript)
      , mScriptLoader(aScriptLoader)
    {
      mScriptLoader->mCurrentScript = aCurrentScript;
    }
    ~AutoCurrentScriptUpdater()
    {
      mScriptLoader->mCurrentScript.swap(mOldScript);
    }
  private:
    nsCOMPtr<nsIScriptElement> mOldScript;
    nsScriptLoader* mScriptLoader;
  };

  friend class nsScriptRequestProcessor;
  friend class AutoCurrentScriptUpdater;

public:
  explicit nsScriptLoader(nsIDocument* aDocument);

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
                                 nsIDocument* aDocument,
                                 char16_t*& aBufOut, size_t& aLengthOut);

  


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
                          bool aScriptFromHead,
                          const mozilla::net::ReferrerPolicy aReferrerPolicy);

  



  nsresult ProcessOffThreadRequest(nsScriptLoadRequest *aRequest,
                                   void **aOffThreadToken);

  bool AddPendingChildLoader(nsScriptLoader* aChild) {
    return mPendingChildLoaders.AppendElement(aChild) != nullptr;
  }

private:
  virtual ~nsScriptLoader();

  


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

  nsresult AttemptAsyncScriptParse(nsScriptLoadRequest* aRequest);
  nsresult ProcessRequest(nsScriptLoadRequest* aRequest,
                          void **aOffThreadToken = nullptr);
  void FireScriptAvailable(nsresult aResult,
                           nsScriptLoadRequest* aRequest);
  void FireScriptEvaluated(nsresult aResult,
                           nsScriptLoadRequest* aRequest);
  nsresult EvaluateScript(nsScriptLoadRequest* aRequest,
                          JS::SourceBufferHolder& aSrcBuf,
                          void **aOffThreadToken);

  already_AddRefed<nsIScriptGlobalObject> GetScriptGlobalObject();
  void FillCompileOptionsForRequest(const mozilla::dom::AutoJSAPI &jsapi,
                                    nsScriptLoadRequest *aRequest,
                                    JS::Handle<JSObject *> aScopeChain,
                                    JS::CompileOptions *aOptions);

  nsresult PrepareLoadedRequest(nsScriptLoadRequest* aRequest,
                                nsIStreamLoader* aLoader,
                                nsresult aStatus,
                                uint32_t aStringLen,
                                const uint8_t* aString);

  void AddDeferRequest(nsScriptLoadRequest* aRequest);
  bool MaybeRemovedDeferRequests();

  nsIDocument* mDocument;                   
  nsCOMArray<nsIScriptLoaderObserver> mObservers;
  nsScriptLoadRequestList mNonAsyncExternalScriptInsertedRequests;
  
  
  nsScriptLoadRequestList mLoadingAsyncRequests;
  nsScriptLoadRequestList mLoadedAsyncRequests;
  nsScriptLoadRequestList mDeferRequests;
  nsScriptLoadRequestList mXSLTRequests;
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
  bool mBlockingDOMContentLoaded;
};

class nsAutoScriptLoaderDisabler
{
public:
  explicit nsAutoScriptLoaderDisabler(nsIDocument* aDoc)
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
