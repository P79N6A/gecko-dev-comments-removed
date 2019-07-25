




#ifndef mozilla_a11y_XULElementAccessibles_h__
#define mozilla_a11y_XULElementAccessibles_h__

#include "BaseAccessibles.h"
#include "HyperTextAccessibleWrap.h"

namespace mozilla {
namespace a11y {




class XULLabelAccessible : public HyperTextAccessibleWrap
{
public:
  XULLabelAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual Relation RelationByType(uint32_t aRelationType);
};




class XULTooltipAccessible : public LeafAccessible
{

public:
  XULTooltipAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
};

class XULLinkAccessible : public HyperTextAccessibleWrap
{

public:
  XULLinkAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual void Value(nsString& aValue);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeLinkState() const;

  
  virtual uint8_t ActionCount();

  
  virtual bool IsLink();
  virtual uint32_t StartOffset();
  virtual uint32_t EndOffset();
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex);

protected:
  enum { eAction_Jump = 0 };

};

} 
} 

#endif
