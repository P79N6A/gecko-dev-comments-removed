






































#ifndef _nsHTMLTextAccessible_H_
#define _nsHTMLTextAccessible_H_

#include "nsTextAccessibleWrap.h"
#include "nsAutoPtr.h"

class nsIWeakReference;

class nsHTMLTextAccessible : public nsTextAccessibleWrap
{
public:
  nsHTMLTextAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell, nsIFrame *aFrame);
  
  
  NS_IMETHOD GetName(nsAString& _retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *aRole);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  NS_IMETHOD Shutdown() { mFrame = nsnull; return nsTextAccessibleWrap::Shutdown(); }
  
  
  NS_IMETHOD_(nsIFrame *) GetFrame(void);

  
  NS_IMETHOD FireToolkitEvent(PRUint32 aEvent, nsIAccessible *aTarget,
                              void *aData);

private:
  
  
  
  nsIFrame *mFrame; 
};

class nsHTMLHRAccessible : public nsLeafAccessible
{
public:
  nsHTMLHRAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *aRole); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsHTMLBRAccessible : public nsLeafAccessible
{
public:
  nsHTMLBRAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *aRole); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetName(nsAString& aName);
};

class nsHTMLLabelAccessible : public nsTextAccessible 
{
public:
  nsHTMLLabelAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetName(nsAString& _retval);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetFirstChild(nsIAccessible **aFirstChild);
  NS_IMETHOD GetLastChild(nsIAccessible **aLastChild);
  NS_IMETHOD GetChildCount(PRInt32 *aAccChildCount);
};

class nsHTMLListBulletAccessible : public nsHTMLTextAccessible
{
public:
  nsHTMLListBulletAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell,
                             nsIFrame *aFrame, const nsAString& aBulletText);
  NS_IMETHOD GetUniqueID(void **aUniqueID);
  NS_IMETHOD Shutdown();
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetRole(PRUint32 *aRole) { *aRole = nsIAccessibleRole::ROLE_STATICTEXT; return NS_OK; }
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  
  
  NS_IMETHOD SetParent(nsIAccessible *aParentAccessible) { mParent = nsnull; mWeakParent = aParentAccessible; return NS_OK; }
  NS_IMETHOD GetParent(nsIAccessible **aParentAccessible) { NS_IF_ADDREF(*aParentAccessible = mWeakParent); return NS_OK; }
protected:
  
  
  
  
  
  
  
  nsIAccessible *mWeakParent;
  nsString mBulletText;
};

class nsHTMLListAccessible : public nsHyperTextAccessible
{
public:
  nsHTMLListAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell):
    nsHyperTextAccessible(aDOMNode, aShell) { }
  NS_IMETHOD GetRole(PRUint32 *aRole) { *aRole = nsIAccessibleRole::ROLE_LIST; return NS_OK; }
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsHTMLLIAccessible : public nsHyperTextAccessible
{
public:
  nsHTMLLIAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell, 
                     nsIFrame *aBulletFrame, const nsAString& aBulletText);
  NS_IMETHOD Shutdown();
  NS_IMETHOD GetRole(PRUint32 *aRole) { *aRole = nsIAccessibleRole::ROLE_LISTITEM; return NS_OK; }
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetName(nsAString& aName) { aName.SetIsVoid(PR_TRUE); return mRoleMapEntry ? nsAccessible::GetName(aName) : NS_OK; }
  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);
  void CacheChildren();  
protected:
  nsRefPtr<nsHTMLListBulletAccessible> mBulletAccessible;
};

#endif  
