





































#ifndef _nsBaseWidgetAccessible_H_
#define _nsBaseWidgetAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsHyperTextAccessible.h"
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
};






class nsLinkableAccessible : public nsHyperTextAccessible
{
public:
  enum { eAction_Jump = 0 };

  nsLinkableAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_DECL_ISUPPORTS_INHERITED
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetState(PRUint32 *_retval);
  NS_IMETHOD GetValue(nsAString& _retval);
  NS_IMETHOD TakeFocus();
  NS_IMETHOD GetKeyboardShortcut(nsAString& _retval);
  NS_IMETHOD GetURI(PRInt32 i, nsIURI **aURI);
  NS_IMETHOD Init();
  NS_IMETHOD Shutdown();

protected:
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
  NS_IMETHODIMP GetRole(PRUint32 *aRole) { *aRole = mRole; return NS_OK; }

protected:
  PRUint32 mRole;
};

#endif  
