




#ifndef MOZILLA_A11Y_HTMLFormControlAccessible_H_
#define MOZILLA_A11Y_HTMLFormControlAccessible_H_

#include "FormControlAccessible.h"
#include "HyperTextAccessibleWrap.h"

namespace mozilla {
namespace a11y {




typedef ProgressMeterAccessible<1> HTMLProgressMeterAccessible;




class HTMLCheckboxAccessible : public LeafAccessible
{

public:
  enum { eAction_Click = 0 };

  HTMLCheckboxAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t index);

  
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();

  
  virtual uint8_t ActionCount();

  
  virtual bool IsWidget() const;
};





class HTMLRadioButtonAccessible : public RadioButtonAccessible
{

public:
  HTMLRadioButtonAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual uint64_t NativeState();
  virtual void GetPositionAndSizeInternal(int32_t *aPosInSet,
                                          int32_t *aSetSize);
};






class HTMLButtonAccessible : public HyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  HTMLButtonAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t index);

  
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t State();
  virtual uint64_t NativeState();

  
  virtual uint8_t ActionCount();

  
  virtual bool IsWidget() const;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;
};





class HTMLTextFieldAccessible : public HyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  HTMLTextFieldAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t index);

  
  virtual already_AddRefed<nsIEditor> GetEditor() const;

  
  virtual void Value(nsString& aValue);
  virtual void ApplyARIAState(uint64_t* aState) const;
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t NativeState();

  
  virtual uint8_t ActionCount();

  
  virtual bool IsWidget() const;
  virtual Accessible* ContainerWidget() const;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;
};





class HTMLFileInputAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLFileInputAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual nsresult HandleAccEvent(AccEvent* aAccEvent);
};




class HTMLGroupboxAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLGroupboxAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual Relation RelationByType(uint32_t aType);

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;

  
  nsIContent* GetLegend();
};





class HTMLLegendAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLLegendAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual Relation RelationByType(uint32_t aType);
};




class HTMLFigureAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLFigureAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() MOZ_OVERRIDE;
  virtual mozilla::a11y::role NativeRole();
  virtual Relation RelationByType(uint32_t aType);

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;

  
  nsIContent* Caption() const;
};





class HTMLFigcaptionAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLFigcaptionAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual Relation RelationByType(uint32_t aType);
};

} 
} 

#endif
