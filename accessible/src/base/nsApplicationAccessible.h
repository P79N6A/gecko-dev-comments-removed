









































#ifndef __NS_APPLICATION_ACCESSIBLE_H__
#define __NS_APPLICATION_ACCESSIBLE_H__

#include "nsAccessibleWrap.h"
#include "nsIMutableArray.h"











class nsApplicationAccessible: public nsAccessibleWrap
{
public:
  nsApplicationAccessible();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetDescription(nsAString& aValue);
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);

  NS_IMETHOD GetParent(nsIAccessible **aAccessible);

  
  virtual PRBool IsDefunct();
  virtual nsresult Init();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsIAccessible* GetParent();

  virtual void InvalidateChildren();

  
  virtual nsresult AddRootAccessible(nsIAccessible *aRootAccWrap);
  virtual nsresult RemoveRootAccessible(nsIAccessible *aRootAccWrap);

protected:

  
  virtual void CacheChildren();
  virtual nsIAccessible* GetSiblingAtOffset(PRInt32 aOffset,
                                            nsresult *aError = nsnull);
};

#endif

