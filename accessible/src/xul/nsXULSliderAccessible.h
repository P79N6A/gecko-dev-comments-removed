




#ifndef _nsXULSliderAccessible_H_
#define _nsXULSliderAccessible_H_

#include "AccessibleWrap.h"

#include "nsIDOMElement.h"




class nsXULSliderAccessible : public AccessibleWrap
{
public:
  nsXULSliderAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_DECL_NSIACCESSIBLEVALUE

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeInteractiveState() const;
  virtual bool NativelyUnavailable() const;
  virtual bool CanHaveAnonChildren();

  
  virtual PRUint8 ActionCount();

protected:
  


  nsIContent* GetSliderElement() const;

  nsresult GetSliderAttr(nsIAtom *aName, nsAString& aValue);
  nsresult SetSliderAttr(nsIAtom *aName, const nsAString& aValue);

  nsresult GetSliderAttr(nsIAtom *aName, double *aValue);
  nsresult SetSliderAttr(nsIAtom *aName, double aValue);

private:
  mutable nsCOMPtr<nsIContent> mSliderNode;
};





class nsXULThumbAccessible : public AccessibleWrap
{
public:
  nsXULThumbAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
};

#endif

