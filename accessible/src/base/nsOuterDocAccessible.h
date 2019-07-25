





































#ifndef _nsOuterDocAccessible_H_
#define _nsOuterDocAccessible_H_

#include "nsAccessibleWrap.h"










class nsOuterDocAccessible : public nsAccessibleWrap
{
public:
  nsOuterDocAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetActionDescription(PRUint8 aIndex, nsAString& aDescription);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual void Shutdown();

  
  virtual PRUint32 NativeRole();
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   PRBool aDeepestChild,
                                   nsIAccessible **aChild);

  virtual void InvalidateChildren();
  virtual PRBool AppendChild(nsAccessible *aAccessible);
  virtual PRBool RemoveChild(nsAccessible *aAccessible);

protected:
  
  virtual void CacheChildren();
};

#endif  
