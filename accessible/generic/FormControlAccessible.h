




#ifndef MOZILLA_A11Y_FormControlAccessible_H_
#define MOZILLA_A11Y_FormControlAccessible_H_

#include "BaseAccessibles.h"

namespace mozilla {
namespace a11y {




template<int Max>
class ProgressMeterAccessible : public LeafAccessible
{
public:
  ProgressMeterAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    LeafAccessible(aContent, aDoc)
  {
    
    
    mStateFlags |= eHasNumericValue | eIgnoreDOMUIEvent;
    mType = eProgressType;
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;

  
  virtual double MaxValue() const MOZ_OVERRIDE;
  virtual double MinValue() const MOZ_OVERRIDE;
  virtual double CurValue() const MOZ_OVERRIDE;
  virtual double Step() const MOZ_OVERRIDE;
  virtual bool SetCurValue(double aValue) MOZ_OVERRIDE;

  
  virtual bool IsWidget() const;

protected:
  virtual ~ProgressMeterAccessible() {}
};




class RadioButtonAccessible : public LeafAccessible
{

public:
  RadioButtonAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t aIndex) MOZ_OVERRIDE;

  enum { eAction_Click = 0 };

  
  virtual bool IsWidget() const;
};

} 
} 

#endif

