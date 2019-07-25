




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

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  NS_DECL_NSIACCESSIBLEVALUE

  
  virtual void Value(nsString& aValue);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeInteractiveState() const;
  virtual bool NativelyUnavailable() const;
  virtual bool CanHaveAnonChildren();

  
  virtual uint8_t ActionCount();

protected:
  


  nsIContent* GetSliderElement() const;

  nsresult GetSliderAttr(nsIAtom *aName, nsAString& aValue);
  nsresult SetSliderAttr(nsIAtom *aName, const nsAString& aValue);

  nsresult GetSliderAttr(nsIAtom *aName, double *aValue);
  nsresult SetSliderAttr(nsIAtom *aName, double aValue);

private:
  mutable nsCOMPtr<nsIContent> mSliderNode;
};





class XULThumbAccessible : public AccessibleWrap
{
public:
  XULThumbAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole();
};

} 
} 

#endif

