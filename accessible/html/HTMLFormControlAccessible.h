




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

  HTMLCheckboxAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    LeafAccessible(aContent, aDoc)
  {
    
    
    mStateFlags |= eIgnoreDOMUIEvent;
  }

  
  virtual mozilla::a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t aIndex) override;

  
  virtual bool IsWidget() const override;
};





class HTMLRadioButtonAccessible : public RadioButtonAccessible
{

public:
  HTMLRadioButtonAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    RadioButtonAccessible(aContent, aDoc)
  {
    
    
    mStateFlags |= eIgnoreDOMUIEvent;
  }

  
  virtual uint64_t NativeState() override;
  virtual void GetPositionAndSizeInternal(int32_t *aPosInSet,
                                          int32_t *aSetSize) override;
};






class HTMLButtonAccessible : public HyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  HTMLButtonAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole() override;
  virtual uint64_t State() override;
  virtual uint64_t NativeState() override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t aIndex) override;

  
  virtual bool IsWidget() const override;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) override;
};






class HTMLTextFieldAccessible final : public HyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  HTMLTextFieldAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual already_AddRefed<nsIEditor> GetEditor() const override;

  
  virtual void Value(nsString& aValue) override;
  virtual void ApplyARIAState(uint64_t* aState) const override;
  virtual mozilla::a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
  virtual already_AddRefed<nsIPersistentProperties> NativeAttributes() override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t aIndex) override;

  
  virtual bool IsWidget() const override;
  virtual Accessible* ContainerWidget() const override;

protected:
  virtual ~HTMLTextFieldAccessible() {}

  
  virtual ENameValueFlag NativeName(nsString& aName) override;

  


  nsIContent* XULWidgetElm() const { return mContent->GetBindingParent(); }
};





class HTMLFileInputAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLFileInputAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole() override;
  virtual nsresult HandleAccEvent(AccEvent* aAccEvent) override;
};





class HTMLSpinnerAccessible : public AccessibleWrap
{
public:
  HTMLSpinnerAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    AccessibleWrap(aContent, aDoc)
  {
    mStateFlags |= eHasNumericValue;
}

  
  virtual mozilla::a11y::role NativeRole() override;
  virtual void Value(nsString& aValue) override;

  virtual double MaxValue() const override;
  virtual double MinValue() const override;
  virtual double CurValue() const override;
  virtual double Step() const override;
  virtual bool SetCurValue(double aValue) override;
};





class HTMLRangeAccessible : public LeafAccessible
{
public:
  HTMLRangeAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    LeafAccessible(aContent, aDoc)
  {
    mStateFlags |= eHasNumericValue;
  }

  
  virtual void Value(nsString& aValue) override;
  virtual mozilla::a11y::role NativeRole() override;

  
  virtual double MaxValue() const override;
  virtual double MinValue() const override;
  virtual double CurValue() const override;
  virtual double Step() const override;
  virtual bool SetCurValue(double aValue) override;

  
  virtual bool IsWidget() const override;
};





class HTMLGroupboxAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLGroupboxAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole() override;
  virtual Relation RelationByType(RelationType aType) override;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) override;

  
  nsIContent* GetLegend() const;
};





class HTMLLegendAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLLegendAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual Relation RelationByType(RelationType aType) override;
};




class HTMLFigureAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLFigureAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual Relation RelationByType(RelationType aType) override;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) override;

  
  nsIContent* Caption() const;
};





class HTMLFigcaptionAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLFigcaptionAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual Relation RelationByType(RelationType aType) override;
};

} 
} 

#endif
