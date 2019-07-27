



#ifndef mozilla_dom_ImageDocument_h
#define mozilla_dom_ImageDocument_h

#include "mozilla/Attributes.h"
#include "imgINotificationObserver.h"
#include "MediaDocument.h"
#include "nsIDOMEventListener.h"
#include "nsIImageDocument.h"

namespace mozilla {
namespace dom {

class ImageDocument MOZ_FINAL : public MediaDocument,
                                public nsIImageDocument,
                                public imgINotificationObserver,
                                public nsIDOMEventListener
{
public:
  ImageDocument();

  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult Init() MOZ_OVERRIDE;

  virtual nsresult StartDocumentLoad(const char*         aCommand,
                                     nsIChannel*         aChannel,
                                     nsILoadGroup*       aLoadGroup,
                                     nsISupports*        aContainer,
                                     nsIStreamListener** aDocListener,
                                     bool                aReset = true,
                                     nsIContentSink*     aSink = nullptr) MOZ_OVERRIDE;

  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject) MOZ_OVERRIDE;
  virtual void Destroy() MOZ_OVERRIDE;
  virtual void OnPageShow(bool aPersisted,
                          EventTarget* aDispatchStartTarget) MOZ_OVERRIDE;

  NS_DECL_NSIIMAGEDOCUMENT
  NS_DECL_IMGINOTIFICATIONOBSERVER

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ImageDocument, MediaDocument)

  friend class ImageListener;

  void DefaultCheckOverflowing() { CheckOverflowing(mResizeImageByDefault); }

  void AddDecodedClass();
  void RemoveDecodedClass();

  
  virtual JSObject* WrapNode(JSContext* aCx)
    MOZ_OVERRIDE;

  bool ImageResizingEnabled() const
  {
    return true;
  }
  bool ImageIsOverflowing() const
  {
    return mImageIsOverflowing;
  }
  bool ImageIsResized() const
  {
    return mImageIsResized;
  }
  already_AddRefed<imgIRequest> GetImageRequest(ErrorResult& aRv);
  void ShrinkToFit();
  void RestoreImage();
  void RestoreImageTo(int32_t aX, int32_t aY)
  {
    ScrollImageTo(aX, aY, true);
  }
  void ToggleImageSize();

protected:
  virtual ~ImageDocument();

  virtual nsresult CreateSyntheticDocument();

  nsresult CheckOverflowing(bool changeState);

  void UpdateTitleAndCharset();

  void ScrollImageTo(int32_t aX, int32_t aY, bool restoreImage);

  float GetRatio() {
    return std::min(mVisibleWidth / mImageWidth,
                    mVisibleHeight / mImageHeight);
  }

  void ResetZoomLevel();
  float GetZoomLevel();

  void UpdateSizeFromLayout();

  enum eModeClasses {
    eNone,
    eShrinkToFit,
    eOverflowing
  };
  void SetModeClass(eModeClasses mode);

  nsresult OnStartContainer(imgIRequest* aRequest, imgIContainer* aImage);
  nsresult OnStopRequest(imgIRequest *aRequest, nsresult aStatus);

  nsCOMPtr<nsIContent>          mImageContent;

  float                         mVisibleWidth;
  float                         mVisibleHeight;
  int32_t                       mImageWidth;
  int32_t                       mImageHeight;

  bool                          mResizeImageByDefault;
  bool                          mClickResizingEnabled;
  bool                          mImageIsOverflowing;
  
  bool                          mImageIsResized;
  
  
  
  bool                          mShouldResize;
  bool                          mFirstResize;
  
  bool                          mObservingImageLoader;

  float                         mOriginalZoomLevel;
};

} 
} 

#endif 
