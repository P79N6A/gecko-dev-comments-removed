











#ifndef nsImageLoadingContent_h__
#define nsImageLoadingContent_h__

#include "imgINotificationObserver.h"
#include "imgIOnloadBlocker.h"
#include "mozilla/CORSMode.h"
#include "mozilla/EventStates.h"
#include "nsCOMPtr.h"
#include "nsIImageLoadingContent.h"
#include "nsIRequest.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"
#include "nsIContentPolicy.h"
#include "mozilla/dom/BindingDeclarations.h"

class nsIURI;
class nsIDocument;
class nsPresContext;
class nsIContent;
class imgRequestProxy;

#ifdef LoadImage

#undef LoadImage
#endif

class nsImageLoadingContent : public nsIImageLoadingContent,
                              public imgIOnloadBlocker
{
  
public:
  nsImageLoadingContent();
  virtual ~nsImageLoadingContent();

  NS_DECL_IMGINOTIFICATIONOBSERVER
  NS_DECL_NSIIMAGELOADINGCONTENT
  NS_DECL_IMGIONLOADBLOCKER

  
  
  
  

  bool LoadingEnabled() const { return mLoadingEnabled; }
  int16_t ImageBlockingStatus() const
  {
    return mImageBlockingStatus;
  }
  already_AddRefed<imgIRequest>
    GetRequest(int32_t aRequestType, mozilla::ErrorResult& aError);
  int32_t
    GetRequestType(imgIRequest* aRequest, mozilla::ErrorResult& aError);
  already_AddRefed<nsIURI> GetCurrentURI(mozilla::ErrorResult& aError);
  void ForceReload(const mozilla::dom::Optional<bool>& aNotify,
                   mozilla::ErrorResult& aError);

  
  nsresult ForceReload(bool aNotify = true) {
    return ForceReload(aNotify, 1);
  }

  



  already_AddRefed<nsIStreamListener>
    LoadImageWithChannel(nsIChannel* aChannel, mozilla::ErrorResult& aError);

protected:
  enum ImageLoadType {
    
    eImageLoadType_Normal,
    
    
    eImageLoadType_Imageset
  };

  













  nsresult LoadImage(const nsAString& aNewURI, bool aForce,
                     bool aNotify, ImageLoadType aImageLoadType);

  









  mozilla::EventStates ImageState() const;

  















  nsresult LoadImage(nsIURI* aNewURI, bool aForce, bool aNotify,
                     ImageLoadType aImageLoadType, nsIDocument* aDocument = nullptr,
                     nsLoadFlags aLoadFlags = nsIRequest::LOAD_NORMAL);

  






  nsIDocument* GetOurOwnerDoc();
  nsIDocument* GetOurCurrentDoc();

  





  nsIFrame* GetOurPrimaryFrame();

  







  nsPresContext* GetFramePresContext();

  




  void CancelImageRequests(bool aNotify);

  





  nsresult UseAsPrimaryRequest(imgRequestProxy* aRequest, bool aNotify,
                               ImageLoadType aImageLoadType);

  







  void DestroyImageLoadingContent();

  void ClearBrokenState() { mBroken = false; }

  
  
  void SetBlockingOnload(bool aBlocking);

  



  virtual mozilla::CORSMode GetCORSMode();

  
  void BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                  nsIContent* aBindingParent, bool aCompileEventHandlers);
  void UnbindFromTree(bool aDeep, bool aNullParent);

  nsresult OnLoadComplete(imgIRequest* aRequest, nsresult aStatus);
  void OnUnlockedDraw();
  nsresult OnImageIsAnimated(imgIRequest *aRequest);

  
  static nsContentPolicyType PolicyTypeForLoad(ImageLoadType aImageLoadType);

private:
  


  struct ImageObserver {
    explicit ImageObserver(imgINotificationObserver* aObserver);
    ~ImageObserver();

    nsCOMPtr<imgINotificationObserver> mObserver;
    ImageObserver* mNext;
  };

  


  struct AutoStateChanger {
    AutoStateChanger(nsImageLoadingContent* aImageContent,
                     bool aNotify) :
      mImageContent(aImageContent),
      mNotify(aNotify)
    {
      mImageContent->mStateChangerDepth++;
    }
    ~AutoStateChanger()
    {
      mImageContent->mStateChangerDepth--;
      mImageContent->UpdateImageState(mNotify);
    }

    nsImageLoadingContent* mImageContent;
    bool mNotify;
  };

  friend struct AutoStateChanger;

  




  void UpdateImageState(bool aNotify);

  




  nsresult FireEvent(const nsAString& aEventType);

protected:
  








  nsresult StringToURI(const nsAString& aSpec, nsIDocument* aDocument,
                       nsIURI** aURI);

  void CreateStaticImageClone(nsImageLoadingContent* aDest) const;

  







   nsRefPtr<imgRequestProxy>& PrepareNextRequest(ImageLoadType aImageLoadType);

  



  void SetBlockedRequest(nsIURI* aURI, int16_t aContentDecision);

  








  nsRefPtr<imgRequestProxy>& PrepareCurrentRequest(ImageLoadType aImageLoadType);
  nsRefPtr<imgRequestProxy>& PreparePendingRequest(ImageLoadType aImageLoadType);

  



  void MakePendingRequestCurrent();

  





  void ClearCurrentRequest(nsresult aReason, uint32_t aNonvisibleAction);
  void ClearPendingRequest(nsresult aReason, uint32_t aNonvisibleAction);

  







  bool* GetRegisteredFlagForRequest(imgIRequest* aRequest);

  



  void ResetAnimationIfNeeded();

  



  static bool HaveSize(imgIRequest *aImage);

  









  void TrackImage(imgIRequest* aImage);
  void UntrackImage(imgIRequest* aImage,
                    uint32_t aNonvisibleAction = ON_NONVISIBLE_NO_ACTION);

  
  nsRefPtr<imgRequestProxy> mCurrentRequest;
  nsRefPtr<imgRequestProxy> mPendingRequest;
  uint32_t mCurrentRequestFlags;
  uint32_t mPendingRequestFlags;

  enum {
    
    REQUEST_NEEDS_ANIMATION_RESET = 0x00000001U,
    
    REQUEST_BLOCKS_ONLOAD = 0x00000002U,
    
    REQUEST_IS_TRACKED = 0x00000004U,
    
    
    REQUEST_IS_IMAGESET = 0x00000008U
  };

  
  
  
  
  nsCOMPtr<nsIURI>      mCurrentURI;

private:
  







  ImageObserver mObserverList;

  



  mozilla::EventStates mForcedImageState;

  int16_t mImageBlockingStatus;
  bool mLoadingEnabled : 1;

  


  bool mIsImageStateForced : 1;

  



  bool mLoading : 1;
  bool mBroken : 1;
  bool mUserDisabled : 1;
  bool mSuppressed : 1;
  bool mFireEventsOnDecode : 1;

protected:
  







  bool mNewRequestsWillNeedAnimationReset : 1;

private:
  
  uint8_t mStateChangerDepth;

  
  
  bool mCurrentRequestRegistered;
  bool mPendingRequestRegistered;

  
  bool mFrameCreateCalled;

  uint32_t mVisibleCount;
};

#endif 
