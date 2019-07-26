




#ifndef mozilla_a11y_BaseAccessibles_h__
#define mozilla_a11y_BaseAccessibles_h__

#include "AccessibleWrap.h"
#include "HyperTextAccessibleWrap.h"

class nsIContent;







namespace mozilla {
namespace a11y {




class LeafAccessible : public AccessibleWrap
{
public:

  LeafAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild);
  virtual bool InsertChildAt(uint32_t aIndex, Accessible* aChild) MOZ_OVERRIDE MOZ_FINAL;
  virtual bool RemoveChild(Accessible* aChild) MOZ_OVERRIDE MOZ_FINAL;

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






class DummyAccessible : public AccessibleWrap
{
public:
  DummyAccessible() : AccessibleWrap(nullptr, nullptr) { }
  virtual ~DummyAccessible() { }

  virtual uint64_t NativeState() MOZ_OVERRIDE MOZ_FINAL;
  virtual uint64_t NativeInteractiveState() const MOZ_OVERRIDE MOZ_FINAL;
  virtual uint64_t NativeLinkState() const MOZ_OVERRIDE MOZ_FINAL;
  virtual bool NativelyUnavailable() const MOZ_OVERRIDE MOZ_FINAL;
  virtual void ApplyARIAState(uint64_t* aState) const MOZ_OVERRIDE MOZ_FINAL;
};

} 
} 

#endif
