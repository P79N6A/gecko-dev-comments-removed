




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

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual void Value(nsString& aValue);
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeLinkState() const;
  virtual PRUint64 NativeInteractiveState() const;

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsLink();
  virtual already_AddRefed<nsIURI> AnchorURIAt(PRUint32 aAnchorIndex);

protected:
  enum { eAction_Jump = 0 };

  


  bool IsLinked();
};

} 
} 

#endif
