







































#ifndef mozilla_imagelib_SVGDocumentWrapper_h_
#define mozilla_imagelib_SVGDocumentWrapper_h_

#include "nsCOMPtr.h"
#include "nsIStreamListener.h"
#include "nsIObserver.h"
#include "nsIContentViewer.h"
#include "nsWeakReference.h"

class nsIAtom;
class nsIPresShell;
class nsIRequest;
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

  












  bool      GetWidthOrHeight(Dimension aDimension, PRInt32& aResult);

  



  nsSVGSVGElement* GetRootSVGElem();

  




  nsIFrame* GetRootLayoutFrame();

  







  inline nsresult  GetPresShell(nsIPresShell** aPresShell)
    { return mViewer->GetPresShell(aPresShell); }

  






  inline bool      ParsedSuccessfully()  { return !!GetRootSVGElem(); }

  






  void UpdateViewportBounds(const nsIntSize& aViewportSize);

  






  void FlushImageTransformInvalidation();

  




  bool      IsAnimated();

  





  bool ShouldIgnoreInvalidation() { return mIgnoreInvalidation; }

  


  void StartAnimation();
  void StopAnimation();
  void ResetAnimation();

private:
  nsresult SetupViewer(nsIRequest *aRequest,
                       nsIContentViewer** aViewer,
                       nsILoadGroup** aLoadGroup);
  void     DestroyViewer();
  void     RegisterForXPCOMShutdown();
  void     UnregisterForXPCOMShutdown();

  void     FlushLayout();

  nsCOMPtr<nsIContentViewer>  mViewer;
  nsCOMPtr<nsILoadGroup>      mLoadGroup;
  nsCOMPtr<nsIStreamListener> mListener;
  bool                        mIgnoreInvalidation;
  bool                        mRegisteredForXPCOMShutdown;

  
  
  static nsIAtom* kSVGAtom;
};

} 
} 

#endif 
