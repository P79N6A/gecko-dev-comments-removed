




#ifndef __mozilla_widget_ActiveElementManager_h__
#define __mozilla_widget_ActiveElementManager_h__

#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"

class inIDOMUtils;
class nsIDOMEventTarget;
class nsIDOMElement;
class CancelableTask;

namespace mozilla {
namespace widget {





class ActiveElementManager {
public:
  NS_INLINE_DECL_REFCOUNTING(ActiveElementManager)

  ActiveElementManager();
  ~ActiveElementManager();

  






  void SetTargetElement(nsIDOMEventTarget* aTarget);
  



  void HandleTouchStart(bool aCanBePan);
  


  void HandlePanStart();
  



  void HandleTouchEnd(bool aWasClick);
private:
  nsCOMPtr<inIDOMUtils> mDomUtils;
  


  nsCOMPtr<nsIDOMElement> mTarget;
  


  bool mCanBePan;
  




  bool mCanBePanSet;
  


  CancelableTask* mSetActiveTask;

  
  void TriggerElementActivation();
  void SetActive(nsIDOMElement* aTarget);
  void ResetActive();
  void SetActiveTask(nsIDOMElement* aTarget);
  void CancelTask();
};

}
}

#endif 
