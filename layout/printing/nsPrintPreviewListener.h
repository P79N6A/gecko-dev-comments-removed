





#ifndef nsPrintPreviewListener_h__
#define nsPrintPreviewListener_h__


#include "nsIDOMEventListener.h"

#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
class EventTarget;
}
}









class nsPrintPreviewListener MOZ_FINAL : public nsIDOMEventListener

{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  explicit nsPrintPreviewListener(mozilla::dom::EventTarget* aTarget);

  
  
  nsresult AddListeners();
  nsresult RemoveListeners();

private:
  ~nsPrintPreviewListener();

  nsCOMPtr<mozilla::dom::EventTarget> mEventTarget;

}; 



#endif 
