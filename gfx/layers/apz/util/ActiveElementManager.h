




#ifndef mozilla_layers_ActiveElementManager_h
#define mozilla_layers_ActiveElementManager_h

#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"

class inIDOMUtils;
class CancelableTask;

namespace mozilla {
namespace dom {
class Element;
class EventTarget;
}

namespace layers {





class ActiveElementManager {
  ~ActiveElementManager();
public:
  NS_INLINE_DECL_REFCOUNTING(ActiveElementManager)

  ActiveElementManager();

  






  void SetTargetElement(dom::EventTarget* aTarget);
  



  void HandleTouchStart(bool aCanBePan);
  


  void HandlePanStart();
  



  void HandleTouchEnd(bool aWasClick);
private:
  nsCOMPtr<inIDOMUtils> mDomUtils;
  


  nsCOMPtr<dom::Element> mTarget;
  


  bool mCanBePan;
  




  bool mCanBePanSet;
  


  CancelableTask* mSetActiveTask;

  
  void TriggerElementActivation();
  void SetActive(dom::Element* aTarget);
  void ResetActive();
  void ResetTouchBlockState();
  void SetActiveTask(dom::Element* aTarget);
  void CancelTask();
};

}
}

#endif 
