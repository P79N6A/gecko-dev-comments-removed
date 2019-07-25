




#ifndef _nsBaseWidgetAccessible_H_
#define _nsBaseWidgetAccessible_H_

#include "AccessibleWrap.h"
#include "nsHyperTextAccessibleWrap.h"
#include "nsIContent.h"










class nsLeafAccessible : public AccessibleWrap
{
public:

  nsLeafAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual Accessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   EWhichChildAtPoint aWhichChild);

protected:

  
  virtual void CacheChildren();
};







class nsLinkableAccessible : public AccessibleWrap
{
public:
  enum { eAction_Jump = 0 };

  nsLinkableAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD TakeFocus();

  
  virtual void Shutdown();

  
  virtual void Value(nsString& aValue);
  virtual PRUint64 NativeLinkState() const;

  
  virtual PRUint8 ActionCount();
  virtual KeyBinding AccessKey() const;

  
  virtual already_AddRefed<nsIURI> AnchorURIAt(PRUint32 aAnchorIndex);

protected:
  
  virtual void BindToParent(Accessible* aParent, PRUint32 aIndexInParent);
  virtual void UnbindFromParent();

  


  Accessible* mActionAcc;
  bool mIsLink;
  bool mIsOnclick;
};



 
class nsEnumRoleAccessible : public AccessibleWrap
{
public:
  nsEnumRoleAccessible(nsIContent* aContent, DocAccessible* aDoc,
                       mozilla::a11y::role aRole);
  virtual ~nsEnumRoleAccessible() { }

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual mozilla::a11y::role NativeRole();

protected:
  mozilla::a11y::role mRole;
};

#endif  
