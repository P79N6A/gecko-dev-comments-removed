




#ifndef MOZILLA_A11Y_OUTERDOCACCESSIBLE_H_
#define MOZILLA_A11Y_OUTERDOCACCESSIBLE_H_

#include "AccessibleWrap.h"

namespace mozilla {
namespace a11y {










class OuterDocAccessible MOZ_FINAL : public AccessibleWrap
{
public:
  OuterDocAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Shutdown() MOZ_OVERRIDE;
  virtual mozilla::a11y::role NativeRole() MOZ_OVERRIDE;
  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild) MOZ_OVERRIDE;

  virtual void InvalidateChildren() MOZ_OVERRIDE;
  virtual bool InsertChildAt(uint32_t aIdx, Accessible* aChild) MOZ_OVERRIDE;
  virtual bool RemoveChild(Accessible* aAccessible) MOZ_OVERRIDE;

protected:
  virtual ~OuterDocAccessible() MOZ_OVERRIDE;

  
  virtual void CacheChildren() MOZ_OVERRIDE;
};

} 
} 

#endif  
