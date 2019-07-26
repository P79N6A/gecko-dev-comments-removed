











#ifndef nsImageLoadingContent_h__
#define nsImageLoadingContent_h__

#include "imgIContainerObserver.h"
#include "imgIDecoderObserver.h"
#include "mozilla/CORSMode.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h" 
#include "nsEventStates.h"
#include "nsIImageLoadingContent.h"
#include "nsIRequest.h"

class nsIURI;
class nsIDocument;
class imgILoader;
class nsIIOService;

class nsImageLoadingContent : public nsIImageLoadingContent
{
  
public:
  nsImageLoadingContent();
  virtual ~nsImageLoadingContent();

  NS_DECL_IMGICONTAINEROBSERVER
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_NSIIMAGELOADINGCONTENT

protected:
  












  nsresult LoadImage(const nsAString& aNewURI, bool aForce,
                     bool aNotify);

  









  nsEventStates ImageState() const;

  














  nsresult LoadImage(nsIURI* aNewURI, bool aForce, bool aNotify,
                     nsIDocument* aDocument = nsnull,
                     nsLoadFlags aLoadFlags = nsIRequest::LOAD_NORMAL);

  






  nsIDocument* GetOurDocument();

  





  nsIFrame* GetOurPrimaryFrame();

  







  nsPresContext* GetFramePresContext();

  




  void CancelImageRequests(bool aNotify);

  





  nsresult UseAsPrimaryRequest(imgIRequest* aRequest, bool aNotify);

  







  void DestroyImageLoadingContent();

  void ClearBrokenState() { mBroken = false; }

  bool LoadingEnabled() { return mLoadingEnabled; }

  
  
  void SetBlockingOnload(bool aBlocking);

  



  virtual mozilla::CORSMode GetCORSMode();

private:
  


  struct ImageObserver {
    ImageObserver(imgIDecoderObserver* aObserver) :
      mObserver(aObserver),
      mNext(nsnull)
    {
      MOZ_COUNT_CTOR(ImageObserver);
    }
    ~ImageObserver()
    {
      MOZ_COUNT_DTOR(ImageObserver);
      NS_CONTENT_DELETE_LIST_MEMBER(ImageObserver, this, mNext);
    }

    nsCOMPtr<imgIDecoderObserver> mObserver;
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

  











  void CancelImageRequests(nsresult aReason, bool aEvenIfSizeAvailable,
                           PRInt16 aNewImageStatus);

  




  nsresult FireEvent(const nsAString& aEventType);
protected:
  








  nsresult StringToURI(const nsAString& aSpec, nsIDocument* aDocument,
                       nsIURI** aURI);

  void CreateStaticImageClone(nsImageLoadingContent* aDest) const;

  





   nsCOMPtr<imgIRequest>& PrepareNextRequest();

  



  void SetBlockedRequest(nsIURI* aURI, PRInt16 aContentDecision);

  






  nsCOMPtr<imgIRequest>& PrepareCurrentRequest();
  nsCOMPtr<imgIRequest>& PreparePendingRequest();

  



  void MakePendingRequestCurrent();

  


  void ClearCurrentRequest(nsresult aReason);
  void ClearPendingRequest(nsresult aReason);

  







  bool* GetRegisteredFlagForRequest(imgIRequest* aRequest);

  



  void ResetAnimationIfNeeded();

  



  static bool HaveSize(imgIRequest *aImage);

  




  nsresult TrackImage(imgIRequest* aImage);
  nsresult UntrackImage(imgIRequest* aImage);

  
  nsCOMPtr<imgIRequest> mCurrentRequest;
  nsCOMPtr<imgIRequest> mPendingRequest;

  
  
  
  
  nsCOMPtr<nsIURI>      mCurrentURI;

private:
  







  ImageObserver mObserverList;

  



  nsEventStates mForcedImageState;

  PRInt16 mImageBlockingStatus;
  bool mLoadingEnabled : 1;

  


  bool mIsImageStateForced : 1;

  



  bool mLoading : 1;
  bool mBroken : 1;
  bool mUserDisabled : 1;
  bool mSuppressed : 1;

  


  bool mBlockingOnload : 1;

protected:
  







  bool mNewRequestsWillNeedAnimationReset : 1;

private:
  bool mPendingRequestNeedsResetAnimation : 1;
  bool mCurrentRequestNeedsResetAnimation : 1;

  
  PRUint8 mStateChangerDepth;

  
  
  bool mCurrentRequestRegistered;
  bool mPendingRequestRegistered;
};

#endif 
