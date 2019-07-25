





































#ifndef _nsBaseWidgetAccessible_H_
#define _nsBaseWidgetAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsHyperTextAccessibleWrap.h"
#include "nsIContent.h"










class nsLeafAccessible : public nsAccessibleWrap
{
public:

  nsLeafAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsAccessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                     EWhichChildAtPoint aWhichChild);

protected:

  
  virtual void CacheChildren();
};







class nsLinkableAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Jump = 0 };

  nsLinkableAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetValue(nsAString& _retval);
  NS_IMETHOD TakeFocus();

  
  virtual void Shutdown();

  
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();
  virtual KeyBinding AccessKey() const;

  
  virtual already_AddRefed<nsIURI> AnchorURIAt(PRUint32 aAnchorIndex);

protected:
  
  virtual void BindToParent(nsAccessible* aParent, PRUint32 aIndexInParent);

  


  nsAccessible* mActionAcc;
  bool mIsLink;
  bool mIsOnclick;
};



 
class nsEnumRoleAccessible : public nsAccessibleWrap
{
public:
  nsEnumRoleAccessible(nsIContent* aContent, nsDocAccessible* aDoc,
                       mozilla::a11y::role aRole);
  virtual ~nsEnumRoleAccessible() { }

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual mozilla::a11y::role NativeRole();

protected:
  mozilla::a11y::role mRole;
};

#endif  
