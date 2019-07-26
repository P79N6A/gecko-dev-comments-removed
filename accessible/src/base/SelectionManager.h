




#ifndef mozilla_a11y_SelectionManager_h__
#define mozilla_a11y_SelectionManager_h__

#include "nsIFrame.h"
#include "nsISelectionListener.h"

class nsIPresShell;

namespace mozilla {

namespace dom {
class Element;
}

namespace a11y {

class AccEvent;


















struct SelData;

class SelectionManager : public nsISelectionListener
{
public:
  
  

  
  NS_DECL_NSISELECTIONLISTENER

  
  void Shutdown() { ClearControlSelectionListener(); }

  





  void SetControlSelectionListener(dom::Element* aFocusedElm);

  


  void ClearControlSelectionListener();

  


  void AddDocSelectionListener(nsIPresShell* aPresShell);

  


  void RemoveDocSelectionListener(nsIPresShell* aShell);

  



  void ProcessTextSelChangeEvent(AccEvent* aEvent);

protected:
  


  void ProcessSelectionChanged(SelData* aSelData);

private:
  
  nsWeakFrame mCurrCtrlFrame;
};

} 
} 

#endif
