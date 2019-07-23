






































#ifndef _nsHTMLTextAccessible_H_
#define _nsHTMLTextAccessible_H_

#include "nsTextAccessibleWrap.h"
#include "nsAutoPtr.h"
#include "nsBaseWidgetAccessible.h"

class nsIWeakReference;

class nsHTMLTextAccessible : public nsTextAccessibleWrap
{
public:
  nsHTMLTextAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  
  
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetRole(PRUint32 *aRole);

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
};

class nsHTMLHRAccessible : public nsLeafAccessible
{
public:
  nsHTMLHRAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *aRole); 
};

class nsHTMLBRAccessible : public nsLeafAccessible
{
public:
  nsHTMLBRAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRole(PRUint32 *aRole); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);

  
  virtual nsresult GetNameInternal(nsAString& aName);
};

class nsHTMLLabelAccessible : public nsTextAccessible 
{
public:
  nsHTMLLabelAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetFirstChild(nsIAccessible **aFirstChild);
  NS_IMETHOD GetLastChild(nsIAccessible **aLastChild);
  NS_IMETHOD GetChildCount(PRInt32 *aAccChildCount);

  
  virtual nsresult GetNameInternal(nsAString& aName);
};

class nsHTMLListBulletAccessible : public nsLeafAccessible
{
public:
  nsHTMLListBulletAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell,
                             const nsAString& aBulletText);

  
  NS_IMETHOD GetUniqueID(void **aUniqueID);

  
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);

  
  
  
  NS_IMETHOD SetParent(nsIAccessible *aParentAccessible);
  NS_IMETHOD GetParent(nsIAccessible **aParentAccessible);

  
  virtual nsresult Shutdown();

  
  NS_IMETHOD AppendTextTo(nsAString& aText, PRUint32 aStartOffset, PRUint32 aLength);

protected:
  
  
  
  
  
  
  nsIAccessible *mWeakParent;
  nsString mBulletText;
};

class nsHTMLListAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLListAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell):
    nsHyperTextAccessibleWrap(aDOMNode, aShell) { }
  NS_IMETHOD GetRole(PRUint32 *aRole) { *aRole = nsIAccessibleRole::ROLE_LIST; return NS_OK; }
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsHTMLLIAccessible : public nsLinkableAccessible
{
public:
  nsHTMLLIAccessible(nsIDOMNode *aDOMNode, nsIWeakReference* aShell, 
                     const nsAString& aBulletText);

  
  NS_IMETHOD GetRole(PRUint32 *aRole) { *aRole = nsIAccessibleRole::ROLE_LISTITEM; return NS_OK; }
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);

  
  virtual nsresult Shutdown();

protected:
  void CacheChildren();  

  nsRefPtr<nsHTMLListBulletAccessible> mBulletAccessible;
};

#endif  
