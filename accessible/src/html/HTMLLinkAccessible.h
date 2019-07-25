




#ifndef mozilla_a11y_HTMLLinkAccessible_h__
#define mozilla_a11y_HTMLLinkAccessible_h__

#include "HyperTextAccessibleWrap.h"

namespace mozilla {
namespace a11y {

class HTMLLinkAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLLinkAccessible(nsIContent* aContent, DocAccessible* aDoc);
 
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual void Value(nsString& aValue);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual uint64_t NativeLinkState() const;
  virtual uint64_t NativeInteractiveState() const;

  
  virtual uint8_t ActionCount();

  
  virtual bool IsLink();
  virtual already_AddRefed<nsIURI> AnchorURIAt(uint32_t aAnchorIndex);

protected:
  enum { eAction_Jump = 0 };

  


  bool IsLinked();
};

} 
} 

#endif
