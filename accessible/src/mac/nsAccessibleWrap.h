









































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
    
    
    
    PRBool IsFlat() {
      PRUint32 role = Role(this);
      return (role == nsIAccessibleRole::ROLE_CHECKBUTTON ||
              role == nsIAccessibleRole::ROLE_PUSHBUTTON ||
              role == nsIAccessibleRole::ROLE_TOGGLE_BUTTON ||
              role == nsIAccessibleRole::ROLE_SPLITBUTTON ||
              role == nsIAccessibleRole::ROLE_ENTRY);
    }
    
    
    
    PRBool IsIgnored();
    
    PRInt32 GetUnignoredChildCount(PRBool aDeepCount);
    
    PRBool HasPopup () {
      PRUint32 state = 0;
      GetState(&state);
      return (state & nsIAccessibleStates::STATE_HASPOPUP);
    }
    
    
    void GetUnignoredChildren(nsTArray<nsRefPtr<nsAccessibleWrap> > &aChildrenArray);
    virtual already_AddRefed<nsIAccessible> GetUnignoredParent();
    
  protected:
    
    PRBool AncestorIsFlat() {
      
      
      
      
      
      
      nsCOMPtr<nsIAccessible> curParent = GetParent();
      while (curParent) {
        nsAccessibleWrap *ancestorWrap = NS_STATIC_CAST(nsAccessibleWrap*, (nsIAccessible*)curParent.get());
        if (ancestorWrap->IsFlat())
          return PR_TRUE;
        curParent = NS_STATIC_CAST(nsAccessibleWrap*, (nsIAccessible*)curParent.get())->GetParent();
      }
      
      return PR_FALSE;
    }

    
    AccessibleWrapper *mNativeWrapper;
};


typedef class nsHTMLTableCellAccessible    nsHTMLTableCellAccessibleWrap;
typedef class nsHTMLTableAccessible        nsHTMLTableAccessibleWrap;

#endif
