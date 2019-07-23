





































#ifndef _nsXFormsWidgetsAccessible_H_
#define _nsXFormsWidgetsAccessible_H_

#include "nsXFormsAccessible.h"
#include "nsBaseWidgetAccessible.h"






class nsXFormsDropmarkerWidgetAccessible : public nsLeafAccessible,
                                           public nsXFormsAccessibleBase
{
public:
  nsXFormsDropmarkerWidgetAccessible(nsIDOMNode *aNode,
                                     nsIWeakReference *aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};





class nsXFormsCalendarWidgetAccessible : public nsAccessibleWrap
{
public:
  nsXFormsCalendarWidgetAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};






class nsXFormsComboboxPopupWidgetAccessible : public nsXFormsAccessible
{
public:
  nsXFormsComboboxPopupWidgetAccessible(nsIDOMNode *aNode,
                                        nsIWeakReference *aShell);

  
  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD GetDescription(nsAString& aDescription);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:
  void CacheChildren();
};

#endif
