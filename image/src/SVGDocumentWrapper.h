






#ifndef mozilla_image_src_SVGDocumentWrapper_h
#define mozilla_image_src_SVGDocumentWrapper_h

#include "mozilla/Attributes.h"

#include "nsCOMPtr.h"
#include "nsIStreamListener.h"
#include "nsIObserver.h"
#include "nsIContentViewer.h"
#include "nsWeakReference.h"
#include "nsSize.h"

class nsIAtom;
class nsIPresShell;
class nsIRequest;
class nsILoadGroup;
class nsIFrame;

#define OBSERVER_SVC_CID "@mozilla.org/observer-service;1"


#undef GetCurrentTime

namespace mozilla {
namespace dom {
class SVGSVGElement;
}

namespace image {

class SVGDocumentWrapper final : public nsIStreamListener,
                                 public nsIObserver,
                                 nsSupportsWeakReference
{
public:
  SVGDocumentWrapper();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIOBSERVER

  enum Dimension {
    eWidth,
    eHeight
  };

  


  nsIDocument* GetDocument();

  



  mozilla::dom::SVGSVGElement* GetRootSVGElem();

  




  nsIFrame* GetRootLayoutFrame();

  







  inline nsresult  GetPresShell(nsIPresShell** aPresShell)
    { return mViewer->GetPresShell(aPresShell); }

  






  void UpdateViewportBounds(const nsIntSize& aViewportSize);

  






  void FlushImageTransformInvalidation();

  




  bool      IsAnimated();

  





  bool ShouldIgnoreInvalidation() { return mIgnoreInvalidation; }

  


  void StartAnimation();
  void StopAnimation();
  void ResetAnimation();
  float GetCurrentTime();
  void SetCurrentTime(float aTime);
  void TickRefreshDriver();

  


  void FlushLayout();

private:
  ~SVGDocumentWrapper();

  nsresult SetupViewer(nsIRequest* aRequest,
                       nsIContentViewer** aViewer,
                       nsILoadGroup** aLoadGroup);
  void     DestroyViewer();
  void     RegisterForXPCOMShutdown();
  void     UnregisterForXPCOMShutdown();

  nsCOMPtr<nsIContentViewer>  mViewer;
  nsCOMPtr<nsILoadGroup>      mLoadGroup;
  nsCOMPtr<nsIStreamListener> mListener;
  bool                        mIgnoreInvalidation;
  bool                        mRegisteredForXPCOMShutdown;
};

} 
} 

#endif 
