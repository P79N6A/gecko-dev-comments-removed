









































#ifndef _nsAccessibleWrap_H_
#define _nsAccessibleWrap_H_

#include "nsCOMPtr.h"
#include "nsRect.h"

#include "nsTArray.h"
#include "nsAutoPtr.h"

#include "nsAccessible.h"

struct AccessibleWrapper;
struct objc_class;

class nsAccessibleWrap : public nsAccessible
{
  public: 
    nsAccessibleWrap(nsIDOMNode*, nsIWeakReference *aShell);
    virtual ~nsAccessibleWrap();
    
    
    NS_IMETHOD Init ();
    
    
    NS_IMETHOD GetNativeInterface (void **aOutAccessible);
    
    
    
    
    virtual objc_class* GetNativeType ();
    
    
    void GetNativeWindow (void **aOutNativeWindow);
    
    virtual nsresult Shutdown ();
    virtual nsresult InvalidateChildren ();

    NS_IMETHOD FireAccessibleEvent(nsIAccessibleEvent *aEvent);
    
    
    
    PRBool IsIgnored();
    
    PRInt32 GetUnignoredChildCount(PRBool aDeepCount);
    
    PRBool HasPopup () {
      PRUint32 state = 0;
      GetState(&state, nsnull);
      return (state & nsIAccessibleStates::STATE_HASPOPUP);
    }
    
    
    void GetUnignoredChildren(nsTArray<nsRefPtr<nsAccessibleWrap> > &aChildrenArray);
    virtual already_AddRefed<nsIAccessible> GetUnignoredParent();
    
  protected:
    
    PRBool AncestorIsFlat() {
      
      
      
      
      
      
      nsCOMPtr<nsIAccessible> curParent = GetParent();
      while (curParent) {
        if (MustPrune(curParent))
          return PR_TRUE;
        nsCOMPtr<nsIAccessible> newParent;
        curParent->GetParent(getter_AddRefs(newParent));
        curParent.swap(newParent);
      }
      
      return PR_FALSE;
    }

    
    AccessibleWrapper *mNativeWrapper;
};


typedef class nsHTMLTableCellAccessible    nsHTMLTableCellAccessibleWrap;
typedef class nsHTMLTableAccessible        nsHTMLTableAccessibleWrap;

#endif
