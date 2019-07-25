





































#ifndef _nsOuterDocAccessible_H_
#define _nsOuterDocAccessible_H_

#include "nsAccessibleWrap.h"










class nsOuterDocAccessible : public nsAccessibleWrap
{
public:
  nsOuterDocAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetActionDescription(PRUint8 aIndex, nsAString& aDescription);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual void Shutdown();

  
  virtual mozilla::a11y::role NativeRole();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsAccessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                     EWhichChildAtPoint aWhichChild);

  virtual void InvalidateChildren();
  virtual bool AppendChild(nsAccessible *aAccessible);
  virtual bool RemoveChild(nsAccessible *aAccessible);

  
  virtual PRUint8 ActionCount();

protected:
  
  virtual void CacheChildren();
};

#endif  
