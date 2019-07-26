




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
class HyperTextAccessible;


















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

  




  inline HyperTextAccessible* AccessibleWithCaret(int32_t* aCaret)
  {
    if (aCaret)
      *aCaret = mCaretOffset;

    return mAccWithCaret;
  }

  


  inline void UpdateCaretOffset(HyperTextAccessible* aItem, int32_t aOffset)
  {
    mAccWithCaret = aItem;
    mCaretOffset = aOffset;
  }

  inline void ResetCaretOffset()
  {
    mCaretOffset = -1;
    mAccWithCaret = nullptr;
  }

protected:

  SelectionManager();

  


  void ProcessSelectionChanged(SelData* aSelData);

private:
  
  nsWeakFrame mCurrCtrlFrame;
  int32_t mCaretOffset;
  HyperTextAccessible* mAccWithCaret;
};

} 
} 

#endif
