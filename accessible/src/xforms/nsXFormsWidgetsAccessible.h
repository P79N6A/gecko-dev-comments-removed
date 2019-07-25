





































#ifndef _nsXFormsWidgetsAccessible_H_
#define _nsXFormsWidgetsAccessible_H_

#include "nsXFormsAccessible.h"
#include "nsBaseWidgetAccessible.h"






class nsXFormsDropmarkerWidgetAccessible : public nsLeafAccessible,
                                           public nsXFormsAccessibleBase
{
public:
  nsXFormsDropmarkerWidgetAccessible(nsIContent *aContent,
                                     nsIWeakReference *aShell);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();
};





class nsXFormsCalendarWidgetAccessible : public nsAccessibleWrap
{
public:
  nsXFormsCalendarWidgetAccessible(nsIContent *aContent,
                                   nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
};






class nsXFormsComboboxPopupWidgetAccessible : public nsXFormsAccessible
{
public:
  nsXFormsComboboxPopupWidgetAccessible(nsIContent *aContent,
                                        nsIWeakReference *aShell);

  
  NS_IMETHOD GetValue(nsAString& aValue);

  
  virtual void Description(nsString& aDescription);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

protected:
  
  virtual void CacheChildren();
};

#endif
