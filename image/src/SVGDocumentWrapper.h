






#ifndef mozilla_imagelib_SVGDocumentWrapper_h_
#define mozilla_imagelib_SVGDocumentWrapper_h_

#include "mozilla/Attributes.h"

#include "nsCOMPtr.h"
#include "nsIStreamListener.h"
#include "nsIObserver.h"
#include "nsIContentViewer.h"
#include "nsWeakReference.h"
#include "nsMimeTypes.h"

class nsIAtom;
class nsIPresShell;
class nsIRequest;
class nsILoadGroup;
class nsIFrame;
struct nsIntSize;

#define OBSERVER_SVC_CID "@mozilla.org/observer-service;1"


namespace mozilla {
namespace dom {
class SVGSVGElement;
}

namespace image {

class SVGDocumentWrapper MOZ_FINAL : public nsIStreamListener,
                                     public nsIObserver,
                                     nsSupportsWeakReference
{
public:
  SVGDocumentWrapper();
  ~SVGDocumentWrapper();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIOBSERVER

  enum Dimension {
    eWidth,
    eHeight
  };

  












  bool      GetWidthOrHeight(Dimension aDimension, int32_t& aResult);

  


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

  


  void FlushLayout();

private:
  nsresult SetupViewer(nsIRequest *aRequest,
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
