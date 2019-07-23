




































#ifndef __nsCaretAccessible_h__
#define __nsCaretAccessible_h__

#include "nsIWeakReference.h"
#include "nsIAccessibleText.h"
#include "nsIDOMNode.h"
#include "nsISelectionListener.h"
#include "nsISelectionController.h"
#include "nsRect.h"

class nsRootAccessible;
class nsIView;
class nsIPresShell;
class nsIWidget;























class nsCaretAccessible : public nsISelectionListener
{
public:
  NS_DECL_ISUPPORTS

  nsCaretAccessible(nsRootAccessible *aRootAccessible);
  virtual ~nsCaretAccessible();
  void Shutdown();

  
  NS_DECL_NSISELECTIONLISTENER

  







  nsresult SetControlSelectionListener(nsIDOMNode *aCurrentNode);

  




  nsresult ClearControlSelectionListener();

  





  nsresult AddDocSelectionListener(nsIPresShell *aShell);

  







  nsresult RemoveDocSelectionListener(nsIPresShell *aShell);

  nsIntRect GetCaretRect(nsIWidget **aOutWidget);

protected:
  nsresult NormalSelectionChanged(nsIDOMDocument *aDoc, nsISelection *aSel);
  nsresult SpellcheckSelectionChanged(nsIDOMDocument *aDoc, nsISelection *aSel);

  already_AddRefed<nsISelectionController>
  GetSelectionControllerForNode(nsIDOMNode *aNode);

private:
  
  
  
  
  nsCOMPtr<nsIDOMNode> mCurrentControl;  

  
  
  
  nsCOMPtr<nsIWeakReference> mLastUsedSelection; 
  nsCOMPtr<nsIAccessibleText> mLastTextAccessible;
  PRInt32 mLastCaretOffset;

  nsRootAccessible *mRootAccessible;
};

#endif
