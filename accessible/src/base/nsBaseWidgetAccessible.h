





































#ifndef _nsBaseWidgetAccessible_H_
#define _nsBaseWidgetAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsHyperTextAccessibleWrap.h"
#include "nsIContent.h"










class nsLeafAccessible : public nsAccessibleWrap
{
public:
  using nsAccessible::GetChildAtPoint;

  nsLeafAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

protected:

  
  virtual void CacheChildren();
};







class nsLinkableAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Jump = 0 };

  nsLinkableAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetValue(nsAString& _retval);
  NS_IMETHOD TakeFocus();
  NS_IMETHOD GetKeyboardShortcut(nsAString& _retval);

  
  virtual PRBool Init();
  virtual void Shutdown();

  
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  
  virtual already_AddRefed<nsIURI> GetAnchorURI(PRUint32 aAnchorIndex);

protected:
  


  nsAccessible *GetActionAccessible() const;

  


  virtual void CacheActionContent();

  nsCOMPtr<nsIContent> mActionContent;
  PRPackedBool mIsLink;
  PRPackedBool mIsOnclick;
};



 
class nsEnumRoleAccessible : public nsAccessibleWrap
{
public:
  nsEnumRoleAccessible(nsIContent *aContent, nsIWeakReference *aShell,
                       PRUint32 aRole);
  virtual ~nsEnumRoleAccessible() { }

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

protected:
  PRUint32 mRole;
};

#endif  
