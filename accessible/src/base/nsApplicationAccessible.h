









































#ifndef __NS_APPLICATION_ACCESSIBLE_H__
#define __NS_APPLICATION_ACCESSIBLE_H__

#include "nsAccessibleWrap.h"
#include "nsIAccessibleApplication.h"

#include "nsIMutableArray.h"
#include "nsIXULAppInfo.h"











class nsApplicationAccessible: public nsAccessibleWrap,
                               public nsIAccessibleApplication
{
public:
  nsApplicationAccessible();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetRootDocument(nsIAccessibleDocument **aRootDocument);

  
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetDescription(nsAString& aValue);
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);

  NS_IMETHOD GetParent(nsIAccessible **aAccessible);

  
  NS_DECL_NSIACCESSIBLEAPPLICATION

  
  virtual PRBool IsDefunct();
  virtual nsresult Init();
  virtual nsresult Shutdown();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsAccessible* GetParent();

  virtual void InvalidateChildren();

  
  virtual nsresult AddRootAccessible(nsIAccessible *aRootAccWrap);
  virtual nsresult RemoveRootAccessible(nsIAccessible *aRootAccWrap);

protected:

  
  virtual void CacheChildren();
  virtual nsAccessible* GetSiblingAtOffset(PRInt32 aOffset,
                                           nsresult *aError = nsnull);

private:
  nsCOMPtr<nsIXULAppInfo> mAppInfo;
};

#endif

