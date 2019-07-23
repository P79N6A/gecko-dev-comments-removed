





































#ifndef _nsXULTabAccessible_H_
#define _nsXULTabAccessible_H_


#include "nsBaseWidgetAccessible.h"
#include "nsXULSelectAccessible.h"




class nsXULTabAccessible : public nsLeafAccessible
{
public:
  enum { eAction_Switch = 0 };

  nsXULTabAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};






class nsXULTabBoxAccessible : public nsAccessibleWrap
{
public:
  nsXULTabBoxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
};




class nsXULTabsAccessible : public nsXULSelectableAccessible
{
public:
  nsXULTabsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetValue(nsAString& _retval);

  
  virtual nsresult GetNameInternal(nsAString& aName);
};







class nsXULTabpanelAccessible : public nsAccessibleWrap
{
public:
  nsXULTabpanelAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);
};

#endif

