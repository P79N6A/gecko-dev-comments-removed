





































#ifndef MOZILLA_A11Y_OUTERDOCACCESSIBLE_H_
#define MOZILLA_A11Y_OUTERDOCACCESSIBLE_H_

#include "nsAccessibleWrap.h"

namespace mozilla {
namespace a11y {










class OuterDocAccessible : public nsAccessibleWrap
{
public:
  OuterDocAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~OuterDocAccessible();

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

} 
} 

#endif  
