






#ifndef nsElementFrameLoaderOwner_h
#define nsElementFrameLoaderOwner_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/Element.h"
#include "nsIFrameLoader.h"
#include "nsIDOMEventListener.h"
#include "mozilla/dom/FromParser.h"
#include "mozilla/ErrorResult.h"

#include "nsFrameLoader.h"

namespace mozilla {
namespace dom {
class Element;
} 
} 

class nsXULElement;




class nsElementFrameLoaderOwner : public nsIFrameLoaderOwner
{
public:
  explicit nsElementFrameLoaderOwner(mozilla::dom::FromParser aFromParser)
    : mNetworkCreated(aFromParser == mozilla::dom::FROM_PARSER_NETWORK)
    , mBrowserFrameListenersRegistered(false)
    , mFrameLoaderCreationDisallowed(false)
  {
  }

  virtual ~nsElementFrameLoaderOwner();

  NS_DECL_NSIFRAMELOADEROWNER

  
  void SwapFrameLoaders(nsXULElement& aOtherOwner, mozilla::ErrorResult& aError);

protected:
  
  
  void EnsureFrameLoader();
  nsresult LoadSrc();
  nsIDocument* GetContentDocument();
  nsresult GetContentDocument(nsIDOMDocument** aContentDocument);
  already_AddRefed<nsPIDOMWindow> GetContentWindow();
  nsresult GetContentWindow(nsIDOMWindow** aContentWindow);

  



  virtual mozilla::dom::Element* ThisFrameElement() = 0;

  nsRefPtr<nsFrameLoader> mFrameLoader;

  




  bool mNetworkCreated;

  bool mBrowserFrameListenersRegistered;
  bool mFrameLoaderCreationDisallowed;
};

#endif 
