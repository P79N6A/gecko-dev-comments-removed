





































#ifndef _nsXULSliderAccessible_H_
#define _nsXULSliderAccessible_H_

#include "nsAccessibleWrap.h"

#include "nsIDOMElement.h"

class nsXULSliderAccessible : public nsAccessibleWrap
{
public:
  nsXULSliderAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetValue(nsAString& aValue);

  
  NS_DECL_NSIACCESSIBLEVALUE

  
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

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

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};

#endif

