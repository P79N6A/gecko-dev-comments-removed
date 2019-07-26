





#ifndef mozilla_a11y_xpcAccessibleTextRange_h_
#define mozilla_a11y_xpcAccessibleTextRange_h_

#include "nsIAccessibleTextRange.h"
#include "TextRange.h"

#include "mozilla/Move.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace a11y {

class TextRange;

class xpcAccessibleTextRange MOZ_FINAL : public nsIAccessibleTextRange
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(xpcAccessibleTextRange)

  NS_IMETHOD GetStartContainer(nsIAccessible** aAnchor) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetStartOffset(int32_t* aOffset) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetEndContainer(nsIAccessible** aAnchor) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetEndOffset(int32_t* aOffset) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetText(nsAString& aText) MOZ_FINAL MOZ_OVERRIDE;

private:
  xpcAccessibleTextRange(TextRange&& aRange) :
    mRange(Forward<TextRange>(aRange)) {}
  xpcAccessibleTextRange() {}
  friend class xpcAccessibleHyperText;

  xpcAccessibleTextRange(const xpcAccessibleTextRange&) MOZ_DELETE;
  xpcAccessibleTextRange& operator =(const xpcAccessibleTextRange&) MOZ_DELETE;

  TextRange mRange;
};

} 
} 

#endif
