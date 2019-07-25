




#ifndef _nsXULSliderAccessible_H_
#define _nsXULSliderAccessible_H_

#include "nsAccessibleWrap.h"

#include "nsIDOMElement.h"




class nsXULSliderAccessible : public nsAccessibleWrap
{
public:
  nsXULSliderAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_DECL_NSIACCESSIBLEVALUE

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual bool CanHaveAnonChildren();

  
  virtual PRUint8 ActionCount();

protected:
  already_AddRefed<nsIContent> GetSliderNode();

  nsresult GetSliderAttr(nsIAtom *aName, nsAString& aValue);
  nsresult SetSliderAttr(nsIAtom *aName, const nsAString& aValue);

  nsresult GetSliderAttr(nsIAtom *aName, double *aValue);
  nsresult SetSliderAttr(nsIAtom *aName, double aValue);

private:
  nsCOMPtr<nsIDOMElement> mSliderNode;
};





class nsXULThumbAccessible : public nsAccessibleWrap
{
public:
  nsXULThumbAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
};

#endif

