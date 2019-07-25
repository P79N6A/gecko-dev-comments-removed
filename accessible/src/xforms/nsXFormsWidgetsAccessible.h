




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

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();

  
  virtual uint8_t ActionCount();
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
  virtual uint64_t NativeState();
  virtual uint64_t NativeInteractiveState() const;

protected:
  
  virtual void CacheChildren();
};

#endif
