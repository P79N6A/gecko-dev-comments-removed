




#ifndef _nsXFormsFormControlsAccessible_H_
#define _nsXFormsFormControlsAccessible_H_

#include "nsXFormsAccessible.h"





class nsXFormsLabelAccessible : public nsXFormsAccessible
{
public:
  nsXFormsLabelAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Description(nsString& aDescription);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
};





class nsXFormsOutputAccessible : public nsXFormsAccessible
{
public:
  nsXFormsOutputAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
};





class nsXFormsTriggerAccessible : public nsXFormsAccessible
{
public:
  nsXFormsTriggerAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();

  
  virtual uint8_t ActionCount();
};





class nsXFormsInputAccessible : public nsXFormsEditableAccessible
{
public:
  nsXFormsInputAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual mozilla::a11y::role NativeRole();

  
  virtual uint8_t ActionCount();
};





class nsXFormsInputBooleanAccessible : public nsXFormsAccessible
{
public:
  nsXFormsInputBooleanAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();

  
  virtual uint8_t ActionCount();
};





class nsXFormsInputDateAccessible : public nsXFormsContainerAccessible
{
public:
  nsXFormsInputDateAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
};





class nsXFormsSecretAccessible : public nsXFormsInputAccessible
{
public:
  nsXFormsSecretAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();
};






class nsXFormsRangeAccessible : public nsXFormsAccessible
{
public:
  nsXFormsRangeAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetMaximumValue(double *aMaximumValue);
  NS_IMETHOD GetMinimumValue(double *aMinimumValue);
  NS_IMETHOD GetMinimumIncrement(double *aMinimumIncrement);
  NS_IMETHOD GetCurrentValue(double *aCurrentValue);

  
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();
};







class nsXFormsSelectAccessible : public nsXFormsContainerAccessible
{
public:
  nsXFormsSelectAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual uint64_t NativeState();
};






class nsXFormsChoicesAccessible : public nsXFormsAccessible
{
public:
  nsXFormsChoicesAccessible(nsIContent* aContent, DocAccessible* aDoc);

  

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();

protected:
  
  virtual void CacheChildren();
};







class nsXFormsSelectFullAccessible : public nsXFormsSelectableAccessible
{
public:
  nsXFormsSelectFullAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();

protected:
  
  virtual void CacheChildren();
};








class nsXFormsItemCheckgroupAccessible : public nsXFormsSelectableItemAccessible
{
public:
  nsXFormsItemCheckgroupAccessible(nsIContent* aContent,
                                   DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);

  
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();
};








class nsXFormsItemRadiogroupAccessible : public nsXFormsSelectableItemAccessible
{
public:
  nsXFormsItemRadiogroupAccessible(nsIContent* aContent,
                                   DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);

  
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();
};







class nsXFormsSelectComboboxAccessible : public nsXFormsSelectableAccessible
{
public:
  nsXFormsSelectComboboxAccessible(nsIContent* aContent,
                                   DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual uint64_t NativeInteractiveState() const;
  virtual bool CanHaveAnonChildren();
};








class nsXFormsItemComboboxAccessible : public nsXFormsSelectableItemAccessible
{
public:
  nsXFormsItemComboboxAccessible(nsIContent* aContent,
                                 DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);

  
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual uint64_t NativeInteractiveState() const;
};

#endif

