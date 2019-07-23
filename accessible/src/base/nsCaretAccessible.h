




































#ifndef __nsCaretAccessible_h__
#define __nsCaretAccessible_h__

#include "nsBaseWidgetAccessible.h"
#include "nsIWeakReference.h"
#include "nsIDOMNode.h"
#include "nsIAccessibleCaret.h"
#include "nsISelectionListener.h"
#include "nsRect.h"

class nsRootAccessible;










class nsCaretAccessible : public nsLeafAccessible, public nsIAccessibleCaret, public nsISelectionListener
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  nsCaretAccessible(nsIDOMNode* aDocumentNode, nsIWeakReference* aShell, nsRootAccessible *aRootAccessible);

  
  NS_IMETHOD GetParent(nsIAccessible **_retval);
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);
  NS_IMETHOD GetNextSibling(nsIAccessible **_retval);
  NS_IMETHOD GetPreviousSibling(nsIAccessible **_retval);

  
  NS_IMETHOD AttachNewSelectionListener(nsIDOMNode *aFocusedNode);
  NS_IMETHOD RemoveSelectionListener();

  
  NS_DECL_NSISELECTIONLISTENER

  
  NS_IMETHOD Init()
  {
#ifdef DEBUG_A11Y
    mIsInitialized = PR_TRUE;
#endif
    return NS_OK;
  }
  NS_IMETHOD Shutdown();

private:
  nsRect mCaretRect;
  PRBool mVisible;
  PRInt32 mLastCaretOffset;
  nsCOMPtr<nsIDOMNode> mLastNodeWithCaret;
  nsCOMPtr<nsIDOMNode> mSelectionControllerNode;
  
  
  nsRootAccessible *mRootAccessible;
  nsCOMPtr<nsIWeakReference> mDomSelectionWeak;
};

#endif
