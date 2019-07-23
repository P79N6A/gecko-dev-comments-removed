









































#ifndef __NS_APPLICATION_ACCESSIBLE_H__
#define __NS_APPLICATION_ACCESSIBLE_H__

#include "nsAccessibleWrap.h"
#include "nsIMutableArray.h"











class nsApplicationAccessible: public nsAccessibleWrap
{
public:
  nsApplicationAccessible();

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsApplicationAccessible,
                                           nsAccessible)

  
  virtual nsresult Init();

  
  NS_IMETHOD GetName(nsAString & aName);
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetParent(nsIAccessible * *aParent);
  NS_IMETHOD GetNextSibling(nsIAccessible * *aNextSibling);
  NS_IMETHOD GetPreviousSibling(nsIAccessible **aPreviousSibling);
  NS_IMETHOD GetIndexInParent(PRInt32 *aIndexInParent);
  NS_IMETHOD GetChildAt(PRInt32 aChildNum, nsIAccessible **aChild);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  
  virtual nsresult AddRootAccessible(nsIAccessible *aRootAccWrap);
  virtual nsresult RemoveRootAccessible(nsIAccessible *aRootAccWrap);

protected:
  
  virtual void CacheChildren();

  nsCOMPtr<nsIMutableArray> mChildren;
};

#endif

