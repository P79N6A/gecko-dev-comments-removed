









































#ifndef __NS_APPLICATION_ACCESSIBLE_H__
#define __NS_APPLICATION_ACCESSIBLE_H__

#include "nsAccessibleWrap.h"
#include "nsIAccessibleApplication.h"

#include "nsIMutableArray.h"
#include "nsIXULAppInfo.h"











class nsApplicationAccessible: public nsAccessibleWrap,
                               public nsIAccessibleApplication
{
public:
  nsApplicationAccessible();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSNODE

  
  NS_IMETHOD GetParent(nsIAccessible **aParent);
  NS_IMETHOD GetNextSibling(nsIAccessible **aNextSibling);
  NS_IMETHOD GetPreviousSibling(nsIAccessible **aPreviousSibling);
  NS_IMETHOD GetName(nsAString &aName);
  NS_IMETHOD GetValue(nsAString &aValue);
  NS_IMETHOD GetDescription(nsAString &aDescription);
  NS_IMETHOD GetKeyboardShortcut(nsAString &aKeyboardShortcut);
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState , PRUint32 *aExtraState );
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
  NS_IMETHOD GroupPosition(PRInt32 *aGroupLevel, PRInt32 *aSimilarItemsInGroup,
                           PRInt32 *aPositionInGroup);
  NS_IMETHOD GetChildAtPoint(PRInt32 aX, PRInt32 aY, nsIAccessible **aChild);
  NS_IMETHOD GetDeepestChildAtPoint(PRInt32 aX, PRInt32 aY, nsIAccessible **aChild);
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);
  NS_IMETHOD GetRelationsCount(PRUint32 *aRelationsCount);
  NS_IMETHOD GetRelation(PRUint32 aIndex, nsIAccessibleRelation **aRelation);
  NS_IMETHOD GetRelations(nsIArray **aRelations);
  NS_IMETHOD GetBounds(PRInt32 *aX, PRInt32 *aY,
                       PRInt32 *aWidth, PRInt32 *aHeight);
  NS_IMETHOD SetSelected(PRBool aIsSelected);
  NS_IMETHOD TakeSelection();
  NS_IMETHOD TakeFocus();
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString &aName);
  NS_IMETHOD GetActionDescription(PRUint8 aIndex, nsAString &aDescription);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_DECL_NSIACCESSIBLEAPPLICATION

  
  virtual PRBool IsDefunct();
  virtual PRBool Init();
  virtual void Shutdown();

  
  virtual nsresult GetARIAState(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  virtual void InvalidateChildren();

protected:

  
  virtual void CacheChildren();
  virtual nsAccessible* GetSiblingAtOffset(PRInt32 aOffset,
                                           nsresult *aError = nsnull);

private:
  nsCOMPtr<nsIXULAppInfo> mAppInfo;
};

#endif

