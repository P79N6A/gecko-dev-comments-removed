





































#ifndef _nsDocAccessible_H_
#define _nsDocAccessible_H_

#include "nsHyperTextAccessibleWrap.h"
#include "nsIAccessibleDocument.h"
#include "nsPIAccessibleDocument.h"
#include "nsIAccessibleEvent.h"
#include "nsIArray.h"
#include "nsIDocument.h"
#include "nsIDocumentObserver.h"
#include "nsIEditor.h"
#include "nsIObserver.h"
#include "nsIScrollPositionListener.h"
#include "nsITimer.h"
#include "nsIWeakReference.h"
#include "nsCOMArray.h"
#include "nsIDocShellTreeNode.h"

class nsIScrollableView;

const PRUint32 kDefaultCacheSize = 256;

class nsDocAccessible : public nsHyperTextAccessibleWrap,
                        public nsIAccessibleDocument,
                        public nsPIAccessibleDocument,
                        public nsIDocumentObserver,
                        public nsIObserver,
                        public nsIScrollPositionListener,
                        public nsSupportsWeakReference
{  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLEDOCUMENT
  NS_DECL_NSPIACCESSIBLEDOCUMENT
  NS_DECL_NSIOBSERVER

  public:
    nsDocAccessible(nsIDOMNode *aNode, nsIWeakReference* aShell);
    virtual ~nsDocAccessible();

    NS_IMETHOD GetRole(PRUint32 *aRole);
    NS_IMETHOD GetName(nsAString& aName);
    NS_IMETHOD GetValue(nsAString& aValue);
    NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
    NS_IMETHOD GetFocusedChild(nsIAccessible **aFocusedChild);
    NS_IMETHOD GetParent(nsIAccessible **aParent);
    NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
    NS_IMETHOD TakeFocus(void);

    
    NS_IMETHOD ScrollPositionWillChange(nsIScrollableView *aView, nscoord aX, nscoord aY);
    NS_IMETHOD ScrollPositionDidChange(nsIScrollableView *aView, nscoord aX, nscoord aY);

    
    NS_DECL_NSIDOCUMENTOBSERVER

    NS_IMETHOD FireToolkitEvent(PRUint32 aEvent, nsIAccessible* aAccessible,
                                void* aData);

    static void FlushEventsCallback(nsITimer *aTimer, void *aClosure);

    
    NS_IMETHOD Shutdown();
    NS_IMETHOD Init();

    
    NS_IMETHOD_(nsIFrame *) GetFrame(void);

    
    nsresult FireDelayedToolkitEvent(PRUint32 aEvent, nsIDOMNode *aDOMNode,
                                     void *aData, PRBool aAllowDupes = PR_FALSE);

    






    nsresult FireDelayedAccessibleEvent(nsIAccessibleEvent *aEvent,
                                        PRBool aAllowDupes = PR_FALSE);

    void ShutdownChildDocuments(nsIDocShellTreeItem *aStart);

  protected:
    virtual void GetBoundsRect(nsRect& aRect, nsIFrame** aRelativeFrame);
    virtual nsresult AddEventListeners();
    virtual nsresult RemoveEventListeners();
    void AddScrollListener();
    void RemoveScrollListener();
    void RefreshNodes(nsIDOMNode *aStartNode, PRUint32 aChangeEvent);
    static void ScrollTimerCallback(nsITimer *aTimer, void *aClosure);
    void CheckForEditor();
    virtual void SetEditor(nsIEditor *aEditor);
    virtual already_AddRefed<nsIEditor> GetEditor() { nsIEditor *editor = mEditor; NS_IF_ADDREF(editor); return editor; }

    





    void ARIAAttributeChanged(nsIContent* aContent, nsIAtom* aAttribute);

    nsInterfaceHashtable<nsVoidHashKey, nsIAccessNode> mAccessNodeCache;
    void *mWnd;
    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsITimer> mScrollWatchTimer;
    nsCOMPtr<nsITimer> mFireEventTimer;
    PRUint16 mScrollPositionChangedTicks; 
    PRPackedBool mIsContentLoaded;
    nsCOMArray<nsIAccessibleEvent> mEventsToFire;
    nsCOMPtr<nsIEditor> mEditor;

protected:
    PRBool mIsAnchor;
    PRBool mIsAnchorJumped;

private:
    static void DocLoadCallback(nsITimer *aTimer, void *aClosure);
    nsCOMPtr<nsITimer> mDocLoadTimer;
};

#endif  
