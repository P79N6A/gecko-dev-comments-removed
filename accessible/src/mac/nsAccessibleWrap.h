









































#ifndef _nsAccessibleWrap_H_
#define _nsAccessibleWrap_H_

#include <objc/objc.h>

#include "nsAccessible.h"
#include "nsAccUtils.h"
#include "States.h"

#include "nsCOMPtr.h"
#include "nsRect.h"

#include "nsTArray.h"
#include "nsAutoPtr.h"

struct AccessibleWrapper;

class nsAccessibleWrap : public nsAccessible
{
  public: 
    nsAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell);
    virtual ~nsAccessibleWrap();
    
    
    virtual bool Init ();
    
    
    NS_IMETHOD GetNativeInterface (void **aOutAccessible);
    
    
    
    
    virtual Class GetNativeType ();

    virtual void Shutdown ();
    virtual void InvalidateChildren();

    virtual nsresult HandleAccEvent(AccEvent* aEvent);

    
    
    bool IsIgnored();
    
    PRInt32 GetUnignoredChildCount(bool aDeepCount);
    
    bool HasPopup () {
      return (NativeState() & mozilla::a11y::states::HASPOPUP);
    }
    
    
    void GetUnignoredChildren(nsTArray<nsRefPtr<nsAccessibleWrap> > &aChildrenArray);
    virtual already_AddRefed<nsIAccessible> GetUnignoredParent();
    
  protected:

    virtual nsresult FirePlatformEvent(AccEvent* aEvent);

  


  bool AncestorIsFlat();

    
    AccessibleWrapper *mNativeWrapper;
};


typedef class nsHTMLTableCellAccessible    nsHTMLTableCellAccessibleWrap;
typedef class nsHTMLTableAccessible        nsHTMLTableAccessibleWrap;

#endif
