




































#ifndef __nsCaretAccessible_h__
#define __nsCaretAccessible_h__

#include "nsHyperTextAccessible.h"

#include "nsISelectionListener.h"

class nsRootAccessible;























class nsCaretAccessible : public nsISelectionListener
{
public:
  NS_DECL_ISUPPORTS

  nsCaretAccessible(nsRootAccessible *aRootAccessible);
  virtual ~nsCaretAccessible();
  void Shutdown();

  
  NS_DECL_NSISELECTIONLISTENER

  







  nsresult SetControlSelectionListener(nsIContent *aCurrentNode);

  




  nsresult ClearControlSelectionListener();

  





  nsresult AddDocSelectionListener(nsIPresShell *aShell);

  







  nsresult RemoveDocSelectionListener(nsIPresShell *aShell);

  nsIntRect GetCaretRect(nsIWidget **aOutWidget);

protected:
  nsresult NormalSelectionChanged(nsIDOMDocument *aDoc, nsISelection *aSel);
  nsresult SpellcheckSelectionChanged(nsIDOMDocument *aDoc, nsISelection *aSel);

  


  already_AddRefed<nsISelectionController>
    GetSelectionControllerForNode(nsIContent *aNode);

private:
  
  
  
  

  
  nsCOMPtr<nsIContent> mCurrentControl;

  
  
  
  nsCOMPtr<nsIWeakReference> mLastUsedSelection; 
  nsRefPtr<nsHyperTextAccessible> mLastTextAccessible;
  PRInt32 mLastCaretOffset;

  nsRootAccessible *mRootAccessible;
};

#endif
