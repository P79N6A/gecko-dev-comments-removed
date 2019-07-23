











































#ifndef nsImageLoadingContent_h__
#define nsImageLoadingContent_h__

#include "nsIImageLoadingContent.h"
#include "imgIRequest.h"
#include "prtypes.h"
#include "nsCOMPtr.h"
#include "nsString.h"

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
  












  nsresult LoadImage(const nsAString& aNewURI, PRBool aForce,
                     PRBool aNotify);

  









  PRInt32 ImageState() const;

  














  nsresult LoadImage(nsIURI* aNewURI, PRBool aForce, PRBool aNotify,
                     nsIDocument* aDocument = nsnull,
                     nsLoadFlags aLoadFlags = nsIRequest::LOAD_NORMAL);

  






  nsIDocument* GetOurDocument();

  




  void CancelImageRequests(PRBool aNotify);

  





  nsresult UseAsPrimaryRequest(imgIRequest* aRequest, PRBool aNotify);

  







  void DestroyImageLoadingContent();

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
      delete mNext;
    }

    nsCOMPtr<imgIDecoderObserver> mObserver;
    ImageObserver* mNext;
  };

  


  struct AutoStateChanger {
    AutoStateChanger(nsImageLoadingContent* aImageContent,
                     PRBool aNotify) :
      mImageContent(aImageContent),
      mNotify(aNotify)
    {
      NS_ASSERTION(!mImageContent->mStartingLoad,
                   "Nested AutoStateChangers somehow?");
      mImageContent->mStartingLoad = PR_TRUE;
    }
    ~AutoStateChanger()
    {
      mImageContent->mStartingLoad = PR_FALSE;
      mImageContent->UpdateImageState(mNotify);
    }

    nsImageLoadingContent* mImageContent;
    PRBool mNotify;
  };

  friend struct AutoStateChanger;

  




  void UpdateImageState(PRBool aNotify);

  











  void CancelImageRequests(nsresult aReason, PRBool aEvenIfSizeAvailable,
                           PRInt16 aNewImageStatus);

  








  nsresult StringToURI(const nsAString& aSpec, nsIDocument* aDocument,
                       nsIURI** aURI);

  




  nsresult FireEvent(const nsAString& aEventType);
  class Event;
  friend class Event;

  
protected:
  nsCOMPtr<imgIRequest> mCurrentRequest;
  nsCOMPtr<imgIRequest> mPendingRequest;
  nsCOMPtr<nsIURI>      mCurrentURI;

private:
  







  ImageObserver mObserverList;

  PRInt16 mImageBlockingStatus;
  PRPackedBool mLoadingEnabled : 1;
  PRPackedBool mStartingLoad : 1;

  



  PRPackedBool mLoading : 1;
  PRPackedBool mBroken : 1;
  PRPackedBool mUserDisabled : 1;
  PRPackedBool mSuppressed : 1;
};

#endif 
