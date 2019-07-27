




#ifndef mozilla_a11y_XULElementAccessibles_h__
#define mozilla_a11y_XULElementAccessibles_h__

#include "HyperTextAccessibleWrap.h"
#include "TextLeafAccessibleWrap.h"

namespace mozilla {
namespace a11y {

class XULLabelTextLeafAccessible;




class XULLabelAccessible : public HyperTextAccessibleWrap
{
public:
  XULLabelAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Shutdown() override;
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
  virtual Relation RelationByType(RelationType aType) override;

  void UpdateLabelValue(const nsString& aValue);

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) override;
  virtual void CacheChildren() override;

private:
  nsRefPtr<XULLabelTextLeafAccessible> mValueTextLeaf;
};

inline XULLabelAccessible*
Accessible::AsXULLabel()
{
  return IsXULLabel() ? static_cast<XULLabelAccessible*>(this) : nullptr;
}






class XULLabelTextLeafAccessible final : public TextLeafAccessibleWrap
{
public:
  XULLabelTextLeafAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    TextLeafAccessibleWrap(aContent, aDoc)
  { mStateFlags |= eSharedNode; }

  virtual ~XULLabelTextLeafAccessible() { }

  
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
};





class XULTooltipAccessible : public LeafAccessible
{

public:
  XULTooltipAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
};

class XULLinkAccessible : public XULLabelAccessible
{

public:
  XULLinkAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Value(nsString& aValue) override;
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeLinkState() const override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t aIndex) override;

  
  virtual bool IsLink() override;
  virtual uint32_t StartOffset() override;
  virtual uint32_t EndOffset() override;
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex) override;

protected:
  virtual ~XULLinkAccessible();

  
  virtual ENameValueFlag NativeName(nsString& aName) override;

  enum { eAction_Jump = 0 };

};

} 
} 

#endif
