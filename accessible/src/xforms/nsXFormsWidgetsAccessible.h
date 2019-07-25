




#ifndef _nsXFormsWidgetsAccessible_H_
#define _nsXFormsWidgetsAccessible_H_

#include "BaseAccessibles.h"
#include "nsXFormsAccessible.h"






class nsXFormsDropmarkerWidgetAccessible : public mozilla::a11y::LeafAccessible,
                                           public nsXFormsAccessibleBase
{
public:
  nsXFormsDropmarkerWidgetAccessible(nsIContent* aContent,
                                     DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();
};





class nsXFormsCalendarWidgetAccessible : public AccessibleWrap
{
public:
  nsXFormsCalendarWidgetAccessible(nsIContent* aContent,
                                   DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
};






class nsXFormsComboboxPopupWidgetAccessible : public nsXFormsAccessible
{
public:
  nsXFormsComboboxPopupWidgetAccessible(nsIContent* aContent,
                                        DocAccessible* aDoc);

  
  virtual void Description(nsString& aDescription);
  virtual void Value(nsString& aValue);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeInteractiveState() const;

protected:
  
  virtual void CacheChildren();
};

#endif
