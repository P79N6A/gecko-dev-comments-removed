




#ifndef MOZILLA_A11Y_OUTERDOCACCESSIBLE_H_
#define MOZILLA_A11Y_OUTERDOCACCESSIBLE_H_

#include "AccessibleWrap.h"

namespace mozilla {
namespace a11y {










class OuterDocAccessible : public AccessibleWrap
{
public:
  OuterDocAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~OuterDocAccessible();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD GetActionDescription(uint8_t aIndex, nsAString& aDescription);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual void Shutdown();

  
  virtual mozilla::a11y::role NativeRole();
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild);

  virtual void InvalidateChildren();
  virtual bool InsertChildAt(uint32_t aIdx, Accessible* aChild) MOZ_OVERRIDE;
  virtual bool RemoveChild(Accessible* aAccessible);

  
  virtual uint8_t ActionCount();

protected:
  
  virtual void CacheChildren();
};

} 
} 

#endif  
