





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

  Accessible* StartContainer() const { return mStartContainer; }
  int32_t StartOffset() const { return mStartOffset; }
  Accessible* EndContainer() const { return mEndContainer; }
  int32_t EndOffset() const { return mEndOffset; }

  


  void Text(nsAString& aText) const;

  


  bool IsValid() const { return mRoot; }

private:
  friend class HyperTextAccessible;

  TextRange(const TextRange&) MOZ_DELETE;
  TextRange& operator=(const TextRange&) MOZ_DELETE;

  const nsRefPtr<HyperTextAccessible> mRoot;
  nsRefPtr<Accessible> mStartContainer;
  nsRefPtr<Accessible> mEndContainer;
  int32_t mStartOffset;
  int32_t mEndOffset;
};


} 
} 

#endif
