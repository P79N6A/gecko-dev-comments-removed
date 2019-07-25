




#ifndef _nsXULTextAccessible_H_
#define _nsXULTextAccessible_H_

#include "BaseAccessibles.h"
#include "HyperTextAccessibleWrap.h"




class nsXULTextAccessible : public HyperTextAccessibleWrap
{
public:
  nsXULTextAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual Relation RelationByType(PRUint32 aRelationType);
};




class nsXULTooltipAccessible : public mozilla::a11y::LeafAccessible
{

public:
  nsXULTooltipAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
};

class nsXULLinkAccessible : public HyperTextAccessibleWrap
{

public:
  nsXULLinkAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual void Value(nsString& aValue);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeLinkState() const;

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsLink();
  virtual PRUint32 StartOffset();
  virtual PRUint32 EndOffset();
  virtual already_AddRefed<nsIURI> AnchorURIAt(PRUint32 aAnchorIndex);

protected:
  enum { eAction_Jump = 0 };

};

#endif  
