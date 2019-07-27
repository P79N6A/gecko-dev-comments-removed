




#ifndef MOZILLA_A11Y_OUTERDOCACCESSIBLE_H_
#define MOZILLA_A11Y_OUTERDOCACCESSIBLE_H_

#include "AccessibleWrap.h"

namespace mozilla {
namespace a11y {










class OuterDocAccessible final : public AccessibleWrap
{
public:
  OuterDocAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Shutdown() override;
  virtual mozilla::a11y::role NativeRole() override;
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild) override;

  virtual void InvalidateChildren() override;
  virtual bool InsertChildAt(uint32_t aIdx, Accessible* aChild) override;
  virtual bool RemoveChild(Accessible* aAccessible) override;

protected:
  virtual ~OuterDocAccessible() override;

  
  virtual void CacheChildren() override;
};

inline OuterDocAccessible*
Accessible::AsOuterDoc()
{
  return IsOuterDoc() ? static_cast<OuterDocAccessible*>(this) : nullptr;
}
} 
} 

#endif  
