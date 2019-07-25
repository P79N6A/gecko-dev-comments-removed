




#ifndef _nsHTMLLinkAccessible_H_
#define _nsHTMLLinkAccessible_H_

#include "HyperTextAccessibleWrap.h"

class nsHTMLLinkAccessible : public HyperTextAccessibleWrap
{
public:
  nsHTMLLinkAccessible(nsIContent* aContent, DocAccessible* aDoc);
 
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeLinkState() const;

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsLink();
  virtual already_AddRefed<nsIURI> AnchorURIAt(PRUint32 aAnchorIndex);

protected:
  enum { eAction_Jump = 0 };

  


  bool IsLinked();
};

#endif  
