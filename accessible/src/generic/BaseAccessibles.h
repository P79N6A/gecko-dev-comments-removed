




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

  
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
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

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t index);
  NS_IMETHOD TakeFocus();

  
  virtual void Shutdown();

  
  virtual void Value(nsString& aValue);
  virtual uint64_t NativeLinkState() const;

  
  virtual uint8_t ActionCount();
  virtual KeyBinding AccessKey() const;

  
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex);

protected:
  
  virtual void BindToParent(Accessible* aParent, uint32_t aIndexInParent);
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
