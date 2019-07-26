





#ifndef nsPrintPreviewListener_h__
#define nsPrintPreviewListener_h__


#include "nsIDOMEventListener.h"
#include "mozilla/dom/EventTarget.h"

#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"









class nsPrintPreviewListener MOZ_FINAL : public nsIDOMEventListener

{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  nsPrintPreviewListener(mozilla::dom::EventTarget* aTarget);

  
  
  nsresult AddListeners();
  nsresult RemoveListeners();

private:

  nsCOMPtr<mozilla::dom::EventTarget> mEventTarget;

}; 



#endif 
