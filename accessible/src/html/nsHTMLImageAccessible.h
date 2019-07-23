





































#ifndef _nsHTMLImageAccessible_H_
#define _nsHTMLImageAccessible_H_

#include "nsBaseWidgetAccessible.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIAccessibleImage.h"






class nsHTMLImageAccessible : public nsLinkableAccessible,
                              public nsIAccessibleImage
{

  NS_DECL_ISUPPORTS_INHERITED

public:
  nsHTMLImageAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  NS_IMETHOD GetAnchorCount(PRInt32 *aAnchorCount);
  NS_IMETHOD GetURI(PRInt32 aIndex, nsIURI **aURI);
  NS_IMETHOD GetAnchor(PRInt32 aIndex, nsIAccessible **aAccessible);

  
  NS_DECL_NSIACCESSIBLEIMAGE

  
  virtual nsresult Shutdown();

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

protected:
  
  virtual void CacheChildren();

  already_AddRefed<nsIDOMHTMLCollection> GetAreaCollection();
  already_AddRefed<nsIAccessible>
    GetAreaAccessible(nsIDOMHTMLCollection* aAreaNodes, PRInt32 aAreaNum);

  
  nsCOMPtr<nsIDOMHTMLMapElement> mMapElement;

  
  
  
  nsAccessNodeHashtable *mAccessNodeCache;

private:
  




  PRBool HasLongDesc();
  
  








  PRBool IsValidLongDescIndex(PRUint8 aIndex);
};

#endif

