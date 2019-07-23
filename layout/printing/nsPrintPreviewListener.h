





































#ifndef nsPrintPreviewListener_h__
#define nsPrintPreviewListener_h__


#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"

#include "nsCOMPtr.h"









class nsPrintPreviewListener : public nsIDOMEventListener

{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
  
  nsPrintPreviewListener(nsIDOMEventTarget* aTarget);

  
  
  nsresult AddListeners();
  nsresult RemoveListeners();

private:

  nsCOMPtr<nsIDOMEventTarget> mEventTarget;

}; 



#endif 
