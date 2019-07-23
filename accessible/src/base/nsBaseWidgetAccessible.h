





































#ifndef _nsBaseWidgetAccessible_H_
#define _nsBaseWidgetAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsHyperTextAccessibleWrap.h"
#include "nsIContent.h"

class nsIDOMNode;










class nsLeafAccessible : public nsAccessibleWrap
{
public:
  nsLeafAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_DECL_ISUPPORTS_INHERITED
  NS_IMETHOD GetFirstChild(nsIAccessible **_retval);
  NS_IMETHOD GetLastChild(nsIAccessible **_retval);
  NS_IMETHOD GetChildCount(PRInt32 *_retval);
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren);
  NS_IMETHOD GetChildAtPoint(PRInt32 aX, PRInt32 aY, nsIAccessible **aAccessible)
    { NS_ENSURE_ARG_POINTER(aAccessible); NS_ADDREF(*aAccessible = this); return NS_OK; } 
};






class nsLinkableAccessible : public nsHyperTextAccessibleWrap
{
public:
  enum { eAction_Jump = 0 };

  nsLinkableAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetValue(nsAString& _retval);
  NS_IMETHOD TakeFocus();
  NS_IMETHOD GetKeyboardShortcut(nsAString& _retval);

  
  NS_IMETHOD GetURI(PRInt32 i, nsIURI **aURI);

  
  virtual nsresult Init();
  virtual nsresult Shutdown();

  
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:
  


  already_AddRefed<nsIAccessible> GetActionAccessible();

  


  virtual void CacheActionContent();

  nsCOMPtr<nsIContent> mActionContent;
  PRPackedBool mIsLink;
  PRPackedBool mIsOnclick;
};



 
class nsEnumRoleAccessible : public nsAccessibleWrap
{
public:
  nsEnumRoleAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell, PRUint32 aRole);
  virtual ~nsEnumRoleAccessible() { }

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

protected:
  PRUint32 mRole;
};

#endif  
