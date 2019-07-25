




































#ifndef _nsRootAccessible_H_
#define _nsRootAccessible_H_

#include "nsCaretAccessible.h"
#include "nsDocAccessibleWrap.h"

#include "nsIAccessibleDocument.h"
#ifdef MOZ_XUL
#include "nsXULTreeAccessible.h"
#endif

#include "nsHashtable.h"
#include "nsCaretAccessible.h"
#include "nsIDocument.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMFormListener.h"

#define NS_ROOTACCESSIBLE_IMPL_CID                      \
{  /* eaba2cf0-21b1-4e2b-b711-d3a89dcd5e1a */           \
  0xeaba2cf0,                                           \
  0x21b1,                                               \
  0x4e2b,                                               \
  { 0xb7, 0x11, 0xd3, 0xa8, 0x9d, 0xcd, 0x5e, 0x1a }    \
}

const PRInt32 SCROLL_HASH_START_SIZE = 6;

class nsRootAccessible : public nsDocAccessibleWrap,
                         public nsIDOMEventListener
{
  NS_DECL_ISUPPORTS_INHERITED

public:
  nsRootAccessible(nsIDocument *aDocument, nsIContent *aRootContent,
                   nsIWeakReference *aShell);
  virtual ~nsRootAccessible();

  
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  
  virtual void Shutdown();

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ROOTACCESSIBLE_IMPL_CID)

  




















  void FireAccessibleFocusEvent(nsAccessible* aFocusAccessible,
                                nsIContent* aRealFocusContent,
                                PRBool aForceEvent = PR_FALSE,
                                EIsFromUserInput aIsFromUserInput = eAutoDetect);

    



    void FireCurrentFocusEvent();

    nsCaretAccessible *GetCaretAccessible();

  


  virtual void DocumentActivated(nsDocAccessible* aDocument);

protected:
  NS_DECL_RUNNABLEMETHOD(nsRootAccessible, FireCurrentFocusEvent)

  


  virtual nsresult AddEventListeners();
  virtual nsresult RemoveEventListeners();

  


  void ProcessDOMEvent(nsIDOMEvent* aEvent);

  


  void HandlePopupShownEvent(nsAccessible* aAccessible);

  


  void HandlePopupHidingEvent(nsINode* aNode, nsAccessible* aAccessible);

#ifdef MOZ_XUL
    void HandleTreeRowCountChangedEvent(nsIDOMEvent* aEvent,
                                        nsXULTreeAccessible* aAccessible);
    void HandleTreeInvalidatedEvent(nsIDOMEvent* aEvent,
                                    nsXULTreeAccessible* aAccessible);

    PRUint32 GetChromeFlags();
#endif
    already_AddRefed<nsIDocShellTreeItem>
           GetContentDocShell(nsIDocShellTreeItem *aStart);
    nsRefPtr<nsCaretAccessible> mCaretAccessible;
  nsCOMPtr<nsINode> mCurrentARIAMenubar;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsRootAccessible, NS_ROOTACCESSIBLE_IMPL_CID)

inline nsRootAccessible*
nsAccessible::AsRoot()
{
  return mFlags & eRootAccessible ?
    static_cast<nsRootAccessible*>(this) : nsnull;
}

#endif
