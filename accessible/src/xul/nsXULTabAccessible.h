





































#ifndef _nsXULTabAccessible_H_
#define _nsXULTabAccessible_H_


#include "nsBaseWidgetAccessible.h"
#include "nsXULMenuAccessible.h"




class nsXULTabAccessible : public nsLeafAccessible
{
public:
  enum { eAction_Switch = 0 };

  nsXULTabAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};






class nsXULTabBoxAccessible : public nsAccessibleWrap
{
public:
  nsXULTabBoxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};




class nsXULTabsAccessible : public nsXULSelectableAccessible
{
public:
  nsXULTabsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetValue(nsAString& _retval);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};







class nsXULTabpanelAccessible : public nsAccessibleWrap
{
public:
  nsXULTabpanelAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};

#endif

