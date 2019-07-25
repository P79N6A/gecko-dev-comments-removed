









































#ifndef _nsAccessibleWrap_H_
#define _nsAccessibleWrap_H_

#include "nsAccessible.h"
#include "nsAccUtils.h"

#include "nsCOMPtr.h"
#include "nsRect.h"

#include "nsTArray.h"
#include "nsAutoPtr.h"

struct AccessibleWrapper;
struct objc_class;

class nsAccessibleWrap : public nsAccessible
{
  public: 
    nsAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell);
    virtual ~nsAccessibleWrap();
    
    
    virtual PRBool Init ();
    
    
    NS_IMETHOD GetNativeInterface (void **aOutAccessible);
    
    
    
    
    virtual objc_class* GetNativeType ();
    
    
    void GetNativeWindow (void **aOutNativeWindow);
    
    virtual void Shutdown ();
    virtual void InvalidateChildren();

    virtual nsresult HandleAccEvent(AccEvent* aEvent);

    
    
    PRBool IsIgnored();
    
    PRInt32 GetUnignoredChildCount(PRBool aDeepCount);
    
    PRBool HasPopup () {
      PRUint32 state = 0;
      GetStateInternal(&state, nsnull);
      return (state & nsIAccessibleStates::STATE_HASPOPUP);
    }
    
    
    void GetUnignoredChildren(nsTArray<nsRefPtr<nsAccessibleWrap> > &aChildrenArray);
    virtual already_AddRefed<nsIAccessible> GetUnignoredParent();
    
  protected:

    virtual nsresult FirePlatformEvent(AccEvent* aEvent);

  


  PRBool AncestorIsFlat();

    
    AccessibleWrapper *mNativeWrapper;
};


typedef class nsHTMLTableCellAccessible    nsHTMLTableCellAccessibleWrap;
typedef class nsHTMLTableAccessible        nsHTMLTableAccessibleWrap;

#endif
