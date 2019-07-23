





































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
  NS_IMETHOD GetState(PRUint32 *_retval);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
};






class nsXULTabBoxAccessible : public nsAccessibleWrap
{
public:
  nsXULTabBoxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval); 
  
};





class nsXULTabPanelsAccessible : public nsAccessibleWrap
{
public:
  nsXULTabPanelsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetState(PRUint32 *_retval);

protected:
  nsresult GetAccPluginChild(nsIAccessible **_retval);

  
  nsCOMPtr<nsIDOMNode> mGParentDOMNode;
  nsCOMPtr<nsIDOMNode> mParentDOMNode;
};


class nsXULTabsAccessible : public nsXULSelectableAccessible
{
public:
  nsXULTabsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetState(PRUint32 *_retval);
  NS_IMETHOD GetValue(nsAString& _retval);
  NS_IMETHOD GetName(nsAString& _retval);
};

#endif  
