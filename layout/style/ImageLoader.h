






#ifndef mozilla_css_ImageLoader_h___
#define mozilla_css_ImageLoader_h___

#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "imgIRequest.h"
#include "imgIOnloadBlocker.h"
#include "imgINotificationObserver.h"
#include "mozilla/Attributes.h"

class imgIContainer;
class nsIFrame;
class nsIDocument;
class nsPresContext;
class nsIURI;
class nsIPrincipal;

namespace mozilla {
namespace css {

struct ImageValue;

class ImageLoader MOZ_FINAL : public imgINotificationObserver,
                              public imgIOnloadBlocker {
public:
  typedef mozilla::css::ImageValue Image;

  explicit ImageLoader(nsIDocument* aDocument)
  : mDocument(aDocument),
    mInClone(false)
  {
    MOZ_ASSERT(mDocument);
  }

  NS_DECL_ISUPPORTS
  NS_DECL_IMGIONLOADBLOCKER
  NS_DECL_IMGINOTIFICATIONOBSERVER

  void DropDocumentReference();

  void MaybeRegisterCSSImage(Image* aImage);
  void DeregisterCSSImage(Image* aImage);

  void AssociateRequestToFrame(imgIRequest* aRequest,
                               nsIFrame* aFrame);

  void DisassociateRequestFromFrame(imgIRequest* aRequest,
                                    nsIFrame* aFrame);

  void DropRequestsForFrame(nsIFrame* aFrame);

  void SetAnimationMode(uint16_t aMode);

  void ClearFrames();

  void LoadImage(nsIURI* aURI, nsIPrincipal* aPrincipal, nsIURI* aReferrer,
                 Image* aCSSValue);

  void DestroyRequest(imgIRequest* aRequest);

private:
  ~ImageLoader() {}

  
  
  
  

  typedef nsTArray<nsIFrame*> FrameSet;
  typedef nsTArray<nsCOMPtr<imgIRequest> > RequestSet;
  typedef nsTHashtable<nsPtrHashKey<Image> > ImageHashSet;
  typedef nsClassHashtable<nsISupportsHashKey,
                           FrameSet> RequestToFrameMap;
  typedef nsClassHashtable<nsPtrHashKey<nsIFrame>,
                           RequestSet> FrameToRequestMap;

  void AddImage(Image* aCSSImage);
  void RemoveImage(Image* aCSSImage);

  nsPresContext* GetPresContext();

  void DoRedraw(FrameSet* aFrameSet);

  static PLDHashOperator
  SetAnimationModeEnumerator(nsISupports* aKey, FrameSet* aValue,
                             void* aClosure);

  nsresult OnStartContainer(imgIRequest *aRequest, imgIContainer* aImage);
  nsresult OnStopFrame(imgIRequest *aRequest);
  nsresult OnImageIsAnimated(imgIRequest *aRequest);
  nsresult FrameChanged(imgIRequest* aRequest);
  
  
  

  
  RequestToFrameMap mRequestToFrameMap;

  
  FrameToRequestMap mFrameToRequestMap;

  
  nsIDocument* mDocument;

  
  
  
  ImageHashSet mImages;

  
  bool mInClone;
};

} 
} 

#endif 
