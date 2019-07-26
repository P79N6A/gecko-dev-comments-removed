




#ifndef mozilla_a11y_SelectionManager_h__
#define mozilla_a11y_SelectionManager_h__

#include "nsAutoPtr.h"
#include "nsIFrame.h"
#include "nsISelectionListener.h"

class nsIContent;
class nsIntRect;
class nsIPresShell;
class nsIWeakReference;
class nsIWidget;

namespace mozilla {

namespace dom {
class Element;
}

namespace a11y {

class HyperTextAccessible;


















class SelectionManager : public nsISelectionListener
{
public:
  
  

  
  NS_DECL_NSISELECTIONLISTENER

  
  void Shutdown();

  





  void SetControlSelectionListener(dom::Element* aFocusedElm);

  


  void ClearControlSelectionListener();

  


  void AddDocSelectionListener(nsIPresShell* aPresShell);

  


  void RemoveDocSelectionListener(nsIPresShell* aShell);

  


  nsIntRect GetCaretRect(nsIWidget** aWidget);

protected:
  


  void ProcessSelectionChanged(nsISelection* aSelection);

  


  void NormalSelectionChanged(nsISelection* aSelection);

  



  void SpellcheckSelectionChanged(nsISelection* aSelection);

private:
  
  nsWeakFrame mCurrCtrlFrame;

  
  
  
  nsCOMPtr<nsIWeakReference> mLastUsedSelection; 
  nsRefPtr<HyperTextAccessible> mLastTextAccessible;
  int32_t mLastCaretOffset;
};

} 
} 

#endif
