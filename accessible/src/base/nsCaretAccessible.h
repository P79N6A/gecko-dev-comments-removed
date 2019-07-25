




#ifndef __nsCaretAccessible_h__
#define __nsCaretAccessible_h__

#include "HyperTextAccessible.h"

#include "nsISelectionListener.h"























class nsCaretAccessible : public nsISelectionListener
{
public:
  NS_DECL_ISUPPORTS

  nsCaretAccessible(mozilla::a11y::RootAccessible* aRootAccessible);
  virtual ~nsCaretAccessible();
  void Shutdown();

  
  NS_DECL_NSISELECTIONLISTENER

  







  nsresult SetControlSelectionListener(nsIContent *aCurrentNode);

  




  nsresult ClearControlSelectionListener();

  





  nsresult AddDocSelectionListener(nsIPresShell *aShell);

  







  nsresult RemoveDocSelectionListener(nsIPresShell *aShell);

  nsIntRect GetCaretRect(nsIWidget **aOutWidget);

protected:
  


  void ProcessSelectionChanged(nsISelection* aSelection);

  


  void NormalSelectionChanged(nsISelection* aSelection);

  



  void SpellcheckSelectionChanged(nsISelection* aSelection);

  


  already_AddRefed<nsISelectionController>
    GetSelectionControllerForNode(nsIContent *aNode);

private:
  
  
  
  

  
  nsCOMPtr<nsIContent> mCurrentControl;

  
  
  
  nsCOMPtr<nsIWeakReference> mLastUsedSelection; 
  nsRefPtr<HyperTextAccessible> mLastTextAccessible;
  int32_t mLastCaretOffset;

  mozilla::a11y::RootAccessible* mRootAccessible;
};

#endif
