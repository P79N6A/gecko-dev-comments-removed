




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

  
  virtual void Shutdown();
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual Relation RelationByType(RelationType aType) MOZ_OVERRIDE;

  void UpdateLabelValue(const nsString& aValue);

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;
  virtual void CacheChildren() MOZ_OVERRIDE;

private:
  nsRefPtr<XULLabelTextLeafAccessible> mValueTextLeaf;
};

inline XULLabelAccessible*
Accessible::AsXULLabel()
{
  return IsXULLabel() ? static_cast<XULLabelAccessible*>(this) : nullptr;
}






class XULLabelTextLeafAccessible MOZ_FINAL : public TextLeafAccessibleWrap
{
public:
  XULLabelTextLeafAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    TextLeafAccessibleWrap(aContent, aDoc)
  { mStateFlags |= eSharedNode; }

  virtual ~XULLabelTextLeafAccessible() { }

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
};





class XULTooltipAccessible : public LeafAccessible
{

public:
  XULTooltipAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
};

class XULLinkAccessible : public XULLabelAccessible
{

public:
  XULLinkAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Value(nsString& aValue);
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeLinkState() const MOZ_OVERRIDE;

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t aIndex) MOZ_OVERRIDE;

  
  virtual bool IsLink();
  virtual uint32_t StartOffset();
  virtual uint32_t EndOffset();
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex);

protected:
  virtual ~XULLinkAccessible();

  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;

  enum { eAction_Jump = 0 };

};

} 
} 

#endif
