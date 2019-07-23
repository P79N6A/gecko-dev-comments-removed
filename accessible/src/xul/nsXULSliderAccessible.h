





































#ifndef _nsXULSliderAccessible_H_
#define _nsXULSliderAccessible_H_

#include "nsAccessibleWrap.h"

class nsXULSliderAccessible : public nsAccessibleWrap
{
public:
  nsXULSliderAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetValue(nsAString& aValue);

  
  NS_IMETHOD GetMaximumValue(double *aMaximumValue);
  NS_IMETHOD GetMinimumValue(double *aMinimumValue);
  NS_IMETHOD GetMinimumIncrement(double *aMinIncrement);
  NS_IMETHOD GetCurrentValue(double *aValue);
  NS_IMETHOD SetCurrentValue(double aValue);

  
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren);

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
  nsXULThumbAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRole(PRUint32 *aRole);
};

#endif
