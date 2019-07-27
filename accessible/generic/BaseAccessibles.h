




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
                                   EWhichChildAtPoint aWhichChild) override;
  virtual bool InsertChildAt(uint32_t aIndex, Accessible* aChild) override final;
  virtual bool RemoveChild(Accessible* aChild) override final;

protected:
  virtual ~LeafAccessible() {}

  
  virtual void CacheChildren() override;
};







class LinkableAccessible : public AccessibleWrap
{
public:
  enum { eAction_Jump = 0 };

  LinkableAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Shutdown() override;
  virtual void Value(nsString& aValue) override;
  virtual uint64_t NativeLinkState() const override;
  virtual void TakeFocus() override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t index) override;
  virtual KeyBinding AccessKey() const override;

  
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex) override;

protected:
  virtual ~LinkableAccessible() {}

  
  virtual void BindToParent(Accessible* aParent, uint32_t aIndexInParent) override;
  virtual void UnbindFromParent() override;

  


  Accessible* mActionAcc;
  bool mIsLink;
  bool mIsOnclick;
};




template<a11y::role R>
class EnumRoleAccessible : public AccessibleWrap
{
public:
  EnumRoleAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    AccessibleWrap(aContent, aDoc) { }

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aPtr) override
    { return Accessible::QueryInterface(aIID, aPtr); }

  
  virtual a11y::role NativeRole() override { return R; }

protected:
  virtual ~EnumRoleAccessible() { }
};






class DummyAccessible : public AccessibleWrap
{
public:
  explicit DummyAccessible(DocAccessible* aDocument = nullptr) :
    AccessibleWrap(nullptr, aDocument) { }

  virtual uint64_t NativeState() override final;
  virtual uint64_t NativeInteractiveState() const override final;
  virtual uint64_t NativeLinkState() const override final;
  virtual bool NativelyUnavailable() const override final;
  virtual void ApplyARIAState(uint64_t* aState) const override final;

protected:
  virtual ~DummyAccessible() { }
};

} 
} 

#endif
