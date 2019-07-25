






#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsInterfaceHashtable.h"
#include "nsCSSValue.h"
#include "imgIRequest.h"
#include "imgIOnloadBlocker.h"
#include "nsStubImageDecoderObserver.h"

class nsIFrame;
class nsIDocument;
class nsPresContext;
class nsIURI;
class nsIPrincipal;

namespace mozilla {
namespace css {

class ImageLoader : public nsStubImageDecoderObserver,
                    public imgIOnloadBlocker {
public:
  ImageLoader(nsIDocument* aDocument)
  : mDocument(aDocument),
    mInClone(false)
  {
    MOZ_ASSERT(mDocument);

    mRequestToFrameMap.Init();
    mFrameToRequestMap.Init();
    mImages.Init();
  }

  NS_DECL_ISUPPORTS
  NS_DECL_IMGIONLOADBLOCKER

  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnStopFrame(imgIRequest *aRequest, uint32_t aFrame);
  NS_IMETHOD OnImageIsAnimated(imgIRequest *aRequest);
  
  
  

  
  NS_IMETHOD FrameChanged(imgIRequest* aRequest,
                          imgIContainer *aContainer,
                          const nsIntRect *aDirtyRect);

  void DropDocumentReference();

  void MaybeRegisterCSSImage(nsCSSValue::Image* aImage);
  void DeregisterCSSImage(nsCSSValue::Image* aImage);

  void AssociateRequestToFrame(imgIRequest* aRequest,
                               nsIFrame* aFrame);

  void DisassociateRequestFromFrame(imgIRequest* aRequest,
                                    nsIFrame* aFrame);

  void DropRequestsForFrame(nsIFrame* aFrame);

  void SetAnimationMode(uint16_t aMode);

  void ClearAll();

  void LoadImage(nsIURI* aURI, nsIPrincipal* aPrincipal, nsIURI* aReferrer,
                 nsCSSValue::Image* aCSSValue);

  void DestroyRequest(imgIRequest* aRequest);

private:
  
  
  
  

  typedef nsTArray<nsIFrame*> FrameSet;
  typedef nsTArray<nsCOMPtr<imgIRequest> > RequestSet;
  typedef nsTHashtable<nsPtrHashKey<nsCSSValue::Image> > ImageHashSet;
  typedef nsClassHashtable<nsISupportsHashKey,
                           FrameSet> RequestToFrameMap;
  typedef nsClassHashtable<nsPtrHashKey<nsIFrame>,
                           RequestSet> FrameToRequestMap;

  void AddImage(nsCSSValue::Image* aCSSImage);
  void RemoveImage(nsCSSValue::Image* aCSSImage);

  nsPresContext* GetPresContext();

  void DoRedraw(FrameSet* aFrameSet);

  static PLDHashOperator
  SetAnimationModeEnumerator(nsISupports* aKey, FrameSet* aValue,
                             void* aClosure);

  
  RequestToFrameMap mRequestToFrameMap;

  
  FrameToRequestMap mFrameToRequestMap;

  
  nsIDocument* mDocument;

  
  
  
  ImageHashSet mImages;

  
  bool mInClone;
};

} 
} 
