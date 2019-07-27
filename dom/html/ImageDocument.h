



#ifndef mozilla_dom_ImageDocument_h
#define mozilla_dom_ImageDocument_h

#include "mozilla/Attributes.h"
#include "imgINotificationObserver.h"
#include "MediaDocument.h"
#include "nsIDOMEventListener.h"
#include "nsIImageDocument.h"

namespace mozilla {
namespace dom {

class ImageDocument final : public MediaDocument,
                                public nsIImageDocument,
                                public imgINotificationObserver,
                                public nsIDOMEventListener
{
public:
  ImageDocument();

  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult Init() override;

  virtual nsresult StartDocumentLoad(const char*         aCommand,
                                     nsIChannel*         aChannel,
                                     nsILoadGroup*       aLoadGroup,
                                     nsISupports*        aContainer,
                                     nsIStreamListener** aDocListener,
                                     bool                aReset = true,
                                     nsIContentSink*     aSink = nullptr) override;

  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject) override;
  virtual void Destroy() override;
  virtual void OnPageShow(bool aPersisted,
                          EventTarget* aDispatchStartTarget) override;

  NS_DECL_NSIIMAGEDOCUMENT
  NS_DECL_IMGINOTIFICATIONOBSERVER

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) override;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ImageDocument, MediaDocument)

  friend class ImageListener;

  void DefaultCheckOverflowing() { CheckOverflowing(mResizeImageByDefault); }

  
  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
    override;

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

  virtual nsresult CreateSyntheticDocument() override;

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

  nsresult OnSizeAvailable(imgIRequest* aRequest, imgIContainer* aImage);
  nsresult OnLoadComplete(imgIRequest* aRequest, nsresult aStatus);
  void OnHasTransparency();

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
