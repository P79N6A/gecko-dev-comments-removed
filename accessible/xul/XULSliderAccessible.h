




#ifndef mozilla_a11y_XULSliderAccessible_h__
#define mozilla_a11y_XULSliderAccessible_h__

#include "AccessibleWrap.h"

#include "nsIDOMElement.h"

namespace mozilla {
namespace a11y {




class XULSliderAccessible : public AccessibleWrap
{
public:
  XULSliderAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Value(nsString& aValue);
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeInteractiveState() const MOZ_OVERRIDE;
  virtual bool NativelyUnavailable() const;
  virtual bool CanHaveAnonChildren();

  
  virtual double MaxValue() const MOZ_OVERRIDE;
  virtual double MinValue() const MOZ_OVERRIDE;
  virtual double CurValue() const MOZ_OVERRIDE;
  virtual double Step() const MOZ_OVERRIDE;
  virtual bool SetCurValue(double aValue) MOZ_OVERRIDE;

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t aIndex) MOZ_OVERRIDE;

protected:
  


  nsIContent* GetSliderElement() const;

  nsresult GetSliderAttr(nsIAtom *aName, nsAString& aValue) const;
  nsresult SetSliderAttr(nsIAtom *aName, const nsAString& aValue);

  double GetSliderAttr(nsIAtom *aName) const;
  bool SetSliderAttr(nsIAtom *aName, double aValue);

private:
  mutable nsCOMPtr<nsIContent> mSliderNode;
};





class XULThumbAccessible : public AccessibleWrap
{
public:
  XULThumbAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
};

} 
} 

#endif

