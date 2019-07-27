




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
  virtual ~LeafAccessible() {}

  
  virtual void CacheChildren();
};







class LinkableAccessible : public AccessibleWrap
{
public:
  enum { eAction_Jump = 0 };

  LinkableAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Shutdown() MOZ_OVERRIDE;
  virtual void Value(nsString& aValue) MOZ_OVERRIDE;
  virtual uint64_t NativeLinkState() const MOZ_OVERRIDE;
  virtual void TakeFocus() MOZ_OVERRIDE;

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t index) MOZ_OVERRIDE;
  virtual KeyBinding AccessKey() const;

  
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex);

protected:
  virtual ~LinkableAccessible() {}

  
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

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;

protected:
  virtual ~EnumRoleAccessible() { }

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
