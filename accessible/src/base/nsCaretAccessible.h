




































#ifndef __nsCaretAccessible_h__
#define __nsCaretAccessible_h__

#include "nsIWeakReference.h"
#include "nsIAccessibleText.h"
#include "nsICaret.h"
#include "nsIDOMNode.h"
#include "nsISelectionListener.h"
#include "nsRect.h"

class nsRootAccessible;
class nsIView;























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

  





  nsresult AddDocSelectionListener(nsIDOMDocument *aDoc);

  





  nsresult RemoveDocSelectionListener(nsIDOMDocument *aDoc);

  nsRect GetCaretRect(nsIWidget **aOutWidget);

private:
  
  
  
  
  nsCOMPtr<nsIDOMNode> mCurrentControl;  
  nsCOMPtr<nsIWeakReference> mCurrentControlSelection;

  
  
  
  nsCOMPtr<nsIWeakReference> mLastUsedSelection; 
  nsCOMPtr<nsIAccessibleText> mLastTextAccessible;
  PRInt32 mLastCaretOffset;

  nsRootAccessible *mRootAccessible;
};

#endif
