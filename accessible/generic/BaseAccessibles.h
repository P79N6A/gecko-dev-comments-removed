




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
                                   EWhichChildAtPoint aWhichChild) MOZ_OVERRIDE;
  virtual bool InsertChildAt(uint32_t aIndex, Accessible* aChild) MOZ_OVERRIDE MOZ_FINAL;
  virtual bool RemoveChild(Accessible* aChild) MOZ_OVERRIDE MOZ_FINAL;

protected:
  virtual ~LeafAccessible() {}

  
  virtual void CacheChildren() MOZ_OVERRIDE;
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
  virtual KeyBinding AccessKey() const MOZ_OVERRIDE;

  
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex) MOZ_OVERRIDE;

protected:
  virtual ~LinkableAccessible() {}

  
  virtual void BindToParent(Accessible* aParent, uint32_t aIndexInParent) MOZ_OVERRIDE;
  virtual void UnbindFromParent() MOZ_OVERRIDE;

  


  Accessible* mActionAcc;
  bool mIsLink;
  bool mIsOnclick;
};




template<a11y::role R>
class RoleTAccessible : public AccessibleWrap
{
public:
  RoleTAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    AccessibleWrap(aContent, aDoc) { }

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aPtr) MOZ_OVERRIDE
    { return Accessible::QueryInterface(aIID, aPtr); }

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE { return R; }

protected:
  virtual ~RoleTAccessible() { }
};






class DummyAccessible : public AccessibleWrap
{
public:
  explicit DummyAccessible(DocAccessible* aDocument = nullptr) :
    AccessibleWrap(nullptr, aDocument) { }

  virtual uint64_t NativeState() MOZ_OVERRIDE MOZ_FINAL;
  virtual uint64_t NativeInteractiveState() const MOZ_OVERRIDE MOZ_FINAL;
  virtual uint64_t NativeLinkState() const MOZ_OVERRIDE MOZ_FINAL;
  virtual bool NativelyUnavailable() const MOZ_OVERRIDE MOZ_FINAL;
  virtual void ApplyARIAState(uint64_t* aState) const MOZ_OVERRIDE MOZ_FINAL;

protected:
  virtual ~DummyAccessible() { }
};

} 
} 

#endif
