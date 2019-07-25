







































#ifndef mozilla_imagelib_SVGDocumentWrapper_h_
#define mozilla_imagelib_SVGDocumentWrapper_h_

#include "nsCOMPtr.h"
#include "nsIStreamListener.h"
#include "nsIObserver.h"
#include "nsIDocumentViewer.h"
#include "nsWeakReference.h"

class nsIAtom;
class nsIPresShell;
class nsIRequest;
class nsIDocumentViewer;
class nsILoadGroup;
class nsIFrame;
struct nsIntSize;
class nsSVGSVGElement;

#define SVG_MIMETYPE     "image/svg+xml"
#define OBSERVER_SVC_CID "@mozilla.org/observer-service;1"


namespace mozilla {
namespace imagelib {

class SVGDocumentWrapper : public nsIStreamListener,
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

  












  PRBool    GetWidthOrHeight(Dimension aDimension, PRInt32& aResult);

  



  nsSVGSVGElement* GetRootSVGElem();

  




  nsIFrame* GetRootLayoutFrame();

  







  inline nsresult  GetPresShell(nsIPresShell** aPresShell)
    { return mViewer->GetPresShell(aPresShell); }

  






  inline PRBool    ParsedSuccessfully()  { return !!GetRootSVGElem(); }

  






  void UpdateViewportBounds(const nsIntSize& aViewportSize);

  






  void FlushPreserveAspectRatioOverride();

  




  PRBool    IsAnimated();

  





  PRBool ShouldIgnoreInvalidation() { return mIgnoreInvalidation; }

  


  void StartAnimation();
  void StopAnimation();
  void ResetAnimation();

private:
  nsresult SetupViewer(nsIRequest *aRequest,
                       nsIDocumentViewer** aViewer,
                       nsILoadGroup** aLoadGroup);
  void     DestroyViewer();
  void     RegisterForXPCOMShutdown();
  void     UnregisterForXPCOMShutdown();

  void     FlushLayout();

  nsCOMPtr<nsIDocumentViewer> mViewer;
  nsCOMPtr<nsILoadGroup>      mLoadGroup;
  nsCOMPtr<nsIStreamListener> mListener;
  PRPackedBool                mIgnoreInvalidation;
  PRPackedBool                mRegisteredForXPCOMShutdown;

  
  
  static nsIAtom* kSVGAtom;
};

} 
} 

#endif 
