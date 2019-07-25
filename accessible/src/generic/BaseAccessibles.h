




#ifndef mozilla_a11y_BaseAccessibles_h__
#define mozilla_a11y_BaseAccessibles_h__

#include "AccessibleWrap.h"
#include "HyperTextAccessibleWrap.h"
#include "nsIContent.h"







namespace mozilla {
namespace a11y {




class LeafAccessible : public AccessibleWrap
{
public:

  LeafAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual Accessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   EWhichChildAtPoint aWhichChild);

protected:

  
  virtual void CacheChildren();
};







class LinkableAccessible : public AccessibleWrap
{
public:
  enum { eAction_Jump = 0 };

  LinkableAccessible(nsIContent* aContent, DocAccessible* aDoc);

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




class EnumRoleAccessible : public AccessibleWrap
{
public:
  EnumRoleAccessible(nsIContent* aContent, DocAccessible* aDoc, 
                     a11y::role aRole);
  virtual ~EnumRoleAccessible() { }

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole();

protected:
  a11y::role mRole;
};

} 
} 

#endif
