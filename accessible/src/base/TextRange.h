





#ifndef mozilla_a11y_TextRange_h__
#define mozilla_a11y_TextRange_h__

#include "mozilla/Move.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace a11y {

class Accessible;
class HyperTextAccessible;




class TextRange MOZ_FINAL
{
public:
  TextRange(HyperTextAccessible* aRoot,
            Accessible* aStartContainer, int32_t aStartOffset,
            Accessible* aEndContainer, int32_t aEndOffset);
  TextRange() {}
  TextRange(TextRange&& aRange) :
    mRoot(Move(aRange.mRoot)), mStartContainer(Move(aRange.mStartContainer)),
    mEndContainer(Move(aRange.mEndContainer)),
    mStartOffset(aRange.mStartOffset), mEndOffset(aRange.mEndOffset) {}

  TextRange& operator= (TextRange&& aRange)
  {
    mRoot = Move(aRange.mRoot);
    mStartContainer = Move(aRange.mStartContainer);
    mEndContainer = Move(aRange.mEndContainer);
    mStartOffset = aRange.mStartOffset;
    mEndOffset = aRange.mEndOffset;
    return *this;
  }

  Accessible* StartContainer() const { return mStartContainer; }
  int32_t StartOffset() const { return mStartOffset; }
  Accessible* EndContainer() const { return mEndContainer; }
  int32_t EndOffset() const { return mEndOffset; }

  


  void Text(nsAString& aText) const;

  


  bool IsValid() const { return mRoot; }

private:
  TextRange(const TextRange& aRange) MOZ_DELETE;
  TextRange& operator=(const TextRange& aRange) MOZ_DELETE;

  friend class HyperTextAccessible;
  friend class xpcAccessibleTextRange;

  void Set(HyperTextAccessible* aRoot,
           Accessible* aStartContainer, int32_t aStartOffset,
           Accessible* aEndContainer, int32_t aEndOffset);

  nsRefPtr<HyperTextAccessible> mRoot;
  nsRefPtr<Accessible> mStartContainer;
  nsRefPtr<Accessible> mEndContainer;
  int32_t mStartOffset;
  int32_t mEndOffset;
};


} 
} 

#endif
