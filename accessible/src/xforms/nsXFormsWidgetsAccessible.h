





































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

  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState);

  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);
};





class nsXFormsCalendarWidgetAccessible : public nsAccessibleWrap
{
public:
  nsXFormsCalendarWidgetAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  NS_IMETHOD GetRole(PRUint32 *aRole);
};






class nsXFormsComboboxPopupWidgetAccessible : public nsXFormsAccessible
{
public:
  nsXFormsComboboxPopupWidgetAccessible(nsIDOMNode *aNode,
                                        nsIWeakReference *aShell);

  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState);
  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetDescription(nsAString& aDescription);

  void CacheChildren();
};

#endif
