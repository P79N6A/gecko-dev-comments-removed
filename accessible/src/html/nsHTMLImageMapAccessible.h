






































#ifndef _nsHTMLAreaAccessible_H_
#define _nsHTMLAreaAccessible_H_

#include "nsHTMLLinkAccessible.h"
#include "nsHTMLImageAccessibleWrap.h"

#include "nsIDOMHTMLMapElement.h"




class nsHTMLImageMapAccessible : public nsHTMLImageAccessibleWrap
{
public:
  nsHTMLImageMapAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell,
                           nsIDOMHTMLMapElement *aMapElm);

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLImageMapAccessible,
                                           nsAccessible)

  
  NS_IMETHOD GetAnchorCount(PRInt32 *aAnchorCount);
  NS_IMETHOD GetURI(PRInt32 aIndex, nsIURI **aURI);
  NS_IMETHOD GetAnchor(PRInt32 aIndex, nsIAccessible **aAccessible);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

protected:
  
  virtual nsresult Shutdown();

  
  virtual void CacheChildren();

  
  


  already_AddRefed<nsIDOMHTMLCollection> GetAreaCollection();

  


  already_AddRefed<nsAccessible>
    GetAreaAccessible(nsIDOMHTMLCollection* aAreaNodes, PRInt32 aAreaNum);

private:
  
  nsCOMPtr<nsIDOMHTMLMapElement> mMapElement;

  
  
  
  nsAccessibleHashtable mAreaAccCache;
};





class nsHTMLAreaAccessible : public nsHTMLLinkAccessible
{

public:
  nsHTMLAreaAccessible(nsIDOMNode *domNode, nsIAccessible *accParent,
                       nsIWeakReference* aShell);

  
  NS_IMETHOD GetDescription(nsAString& aDescription);

  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

protected:

  
  virtual void CacheChildren();
};

#endif  
