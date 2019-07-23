





































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
  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_DECL_NSIACCESSIBLEVALUE

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  
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

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};

#endif

