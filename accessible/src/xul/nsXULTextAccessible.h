






































#ifndef _nsXULTextAccessible_H_
#define _nsXULTextAccessible_H_

#include "nsBaseWidgetAccessible.h"
#include "nsHyperTextAccessibleWrap.h"




class nsXULTextAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsXULTextAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual Relation RelationByType(PRUint32 aRelationType);
};




class nsXULTooltipAccessible : public nsLeafAccessible
{

public:
  nsXULTooltipAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
};

class nsXULLinkAccessible : public nsHyperTextAccessibleWrap
{

public:
  nsXULLinkAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetValue(nsAString& aValue);

  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsLink();
  virtual PRUint32 StartOffset();
  virtual PRUint32 EndOffset();
  virtual already_AddRefed<nsIURI> AnchorURIAt(PRUint32 aAnchorIndex);

protected:
  enum { eAction_Jump = 0 };

};

#endif  
