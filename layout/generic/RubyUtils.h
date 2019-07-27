





#ifndef mozilla_RubyUtils_h_
#define mozilla_RubyUtils_h_

#include "nsTArray.h"
#include "nsGkAtoms.h"

#define RTC_ARRAY_SIZE 1

class nsRubyFrame;
class nsRubyBaseFrame;
class nsRubyTextFrame;
class nsRubyContentFrame;
class nsRubyBaseContainerFrame;
class nsRubyTextContainerFrame;

namespace mozilla {






























class RubyUtils
{
public:
  static inline bool IsRubyContentBox(nsIAtom* aFrameType)
  {
    return aFrameType == nsGkAtoms::rubyBaseFrame ||
           aFrameType == nsGkAtoms::rubyTextFrame;
  }

  static inline bool IsRubyContainerBox(nsIAtom* aFrameType)
  {
    return aFrameType == nsGkAtoms::rubyBaseContainerFrame ||
           aFrameType == nsGkAtoms::rubyTextContainerFrame;
  }

  static inline bool IsRubyBox(nsIAtom* aFrameType)
  {
    return aFrameType == nsGkAtoms::rubyFrame ||
      IsRubyContentBox(aFrameType) || IsRubyContainerBox(aFrameType);
  }

  static inline bool IsExpandableRubyBox(nsIFrame* aFrame)
  {
    nsIAtom* type = aFrame->GetType();
    return IsRubyContentBox(type) || IsRubyContainerBox(type);
  }

  static void SetReservedISize(nsIFrame* aFrame, nscoord aISize);
  static void ClearReservedISize(nsIFrame* aFrame);
  static nscoord GetReservedISize(nsIFrame* aFrame);
};





class MOZ_STACK_CLASS AutoRubyTextContainerArray final
  : public nsAutoTArray<nsRubyTextContainerFrame*, RTC_ARRAY_SIZE>
{
public:
  explicit AutoRubyTextContainerArray(nsRubyBaseContainerFrame* aBaseContainer);
};




class MOZ_STACK_CLASS RubySegmentEnumerator
{
public:
  explicit RubySegmentEnumerator(nsRubyFrame* aRubyFrame);

  void Next();
  bool AtEnd() const { return !mBaseContainer; }

  nsRubyBaseContainerFrame* GetBaseContainer() const
  {
    return mBaseContainer;
  }

private:
  nsRubyBaseContainerFrame* mBaseContainer;
};






struct MOZ_STACK_CLASS RubyColumn
{
  nsRubyBaseFrame* mBaseFrame;
  nsAutoTArray<nsRubyTextFrame*, RTC_ARRAY_SIZE> mTextFrames;
  bool mIsIntraLevelWhitespace;
  RubyColumn() : mBaseFrame(nullptr), mIsIntraLevelWhitespace(false) { }
};




class MOZ_STACK_CLASS RubyColumnEnumerator
{
public:
  RubyColumnEnumerator(nsRubyBaseContainerFrame* aRBCFrame,
                       const AutoRubyTextContainerArray& aRTCFrames);

  void Next();
  bool AtEnd() const;

  uint32_t GetLevelCount() const { return mFrames.Length(); }
  nsRubyContentFrame* GetFrameAtLevel(uint32_t aIndex) const;
  void GetColumn(RubyColumn& aColumn) const;

private:
  
  
  
  nsAutoTArray<nsRubyContentFrame*, RTC_ARRAY_SIZE + 1> mFrames;
  
  bool mAtIntraLevelWhitespace;
};

}

#endif 
