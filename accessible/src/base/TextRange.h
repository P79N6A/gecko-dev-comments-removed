





#ifndef mozilla_a11y_TextRange_h__
#define mozilla_a11y_TextRange_h__

#include "mozilla/Move.h"
#include "nsAutoPtr.h"
#include "nsCaseTreatment.h"
#include "nsRect.h"
#include "nsTArray.h"

 class nsIVariant;

namespace mozilla {
namespace a11y {

class Accessible;
class HyperTextAccessible;




struct TextPoint MOZ_FINAL
{
  TextPoint(HyperTextAccessible* aContainer, int32_t aOffset) :
    mContainer(aContainer), mOffset(aOffset) { }
  TextPoint(const TextPoint& aPoint) :
    mContainer(aPoint.mContainer), mOffset(aPoint.mOffset) { }

  HyperTextAccessible* mContainer;
  int32_t mOffset;

  bool operator ==(const TextPoint& aPoint) const
    { return mContainer == aPoint.mContainer && mOffset == aPoint.mOffset; }
  bool operator <(const TextPoint& aPoint) const;
};




class TextRange MOZ_FINAL
{
public:
  TextRange(HyperTextAccessible* aRoot,
            HyperTextAccessible* aStartContainer, int32_t aStartOffset,
            HyperTextAccessible* aEndContainer, int32_t aEndOffset);
  TextRange() {}
  TextRange(TextRange&& aRange) :
    mRoot(mozilla::Move(aRange.mRoot)),
    mStartContainer(mozilla::Move(aRange.mStartContainer)),
    mEndContainer(mozilla::Move(aRange.mEndContainer)),
    mStartOffset(aRange.mStartOffset), mEndOffset(aRange.mEndOffset) {}

  TextRange& operator= (TextRange&& aRange)
  {
    mRoot = mozilla::Move(aRange.mRoot);
    mStartContainer = mozilla::Move(aRange.mStartContainer);
    mEndContainer = mozilla::Move(aRange.mEndContainer);
    mStartOffset = aRange.mStartOffset;
    mEndOffset = aRange.mEndOffset;
    return *this;
  }

  HyperTextAccessible* StartContainer() const { return mStartContainer; }
  int32_t StartOffset() const { return mStartOffset; }
  HyperTextAccessible* EndContainer() const { return mEndContainer; }
  int32_t EndOffset() const { return mEndOffset; }

  bool operator ==(const TextRange& aRange) const
  {
    return mStartContainer == aRange.mStartContainer &&
      mStartOffset == aRange.mStartOffset &&
      mEndContainer == aRange.mEndContainer && mEndOffset == aRange.mEndOffset;
  }

  TextPoint StartPoint() const { return TextPoint(mStartContainer, mStartOffset); }
  TextPoint EndPoint() const { return TextPoint(mEndContainer, mEndOffset); }

  


  Accessible* Container() const;

  



  void EmbeddedChildren(nsTArray<Accessible*>* aChildren) const;

  


  void Text(nsAString& aText) const;

  


  void Bounds(nsTArray<nsIntRect> aRects) const;

  enum ETextUnit {
    eFormat,
    eWord,
    eLine,
    eParagraph,
    ePage,
    eDocument
  };

  


  void Move(ETextUnit aUnit, int32_t aCount)
  {
    MoveEnd(aUnit, aCount);
    MoveStart(aUnit, aCount);
  }
  void MoveStart(ETextUnit aUnit, int32_t aCount)
  {
    MoveInternal(aUnit, aCount, *mStartContainer, mStartOffset,
                 mEndContainer, mEndOffset);
  }
  void MoveEnd(ETextUnit aUnit, int32_t aCount)
    { MoveInternal(aUnit, aCount, *mEndContainer, mEndOffset); }

  


  void Normalize(ETextUnit aUnit);

  enum EDirection {
    eBackward,
    eForward
  };

  


  void FindText(const nsAString& aText, EDirection aDirection,
                nsCaseTreatment aCaseSensitive, TextRange* aFoundRange) const;

  enum EAttr {
    eAnimationStyleAttr,
    eAnnotationObjectsAttr,
    eAnnotationTypesAttr,
    eBackgroundColorAttr,
    eBulletStyleAttr,
    eCapStyleAttr,
    eCaretBidiModeAttr,
    eCaretPositionAttr,
    eCultureAttr,
    eFontNameAttr,
    eFontSizeAttr,
    eFontWeightAttr,
    eForegroundColorAttr,
    eHorizontalTextAlignmentAttr,
    eIndentationFirstLineAttr,
    eIndentationLeadingAttr,
    eIndentationTrailingAttr,
    eIsActiveAttr,
    eIsHiddenAttr,
    eIsItalicAttr,
    eIsReadOnlyAttr,
    eIsSubscriptAttr,
    eIsSuperscriptAttr,
    eLinkAttr,
    eMarginBottomAttr,
    eMarginLeadingAttr,
    eMarginTopAttr,
    eMarginTrailingAttr,
    eOutlineStylesAttr,
    eOverlineColorAttr,
    eOverlineStyleAttr,
    eSelectionActiveEndAttr,
    eStrikethroughColorAttr,
    eStrikethroughStyleAttr,
    eStyleIdAttr,
    eStyleNameAttr,
    eTabsAttr,
    eTextFlowDirectionsAttr,
    eUnderlineColorAttr,
    eUnderlineStyleAttr
  };

  


  void FindAttr(EAttr aAttr, nsIVariant* aValue, EDirection aDirection,
                TextRange* aFoundRange) const;

  


  void AddToSelection() const;
  void RemoveFromSelection() const;
  void Select() const;

  


  enum EHowToAlign {
    eAlignToTop,
    eAlignToBottom
  };
  void ScrollIntoView(EHowToAlign aHow) const;

  


  bool IsValid() const { return mRoot; }

  void SetStartPoint(HyperTextAccessible* aContainer, int32_t aOffset)
    { mStartContainer = aContainer; mStartOffset = aOffset; }
  void SetEndPoint(HyperTextAccessible* aContainer, int32_t aOffset)
    { mStartContainer = aContainer; mStartOffset = aOffset; }

private:
  TextRange(const TextRange& aRange) MOZ_DELETE;
  TextRange& operator=(const TextRange& aRange) MOZ_DELETE;

  friend class HyperTextAccessible;
  friend class xpcAccessibleTextRange;

  void Set(HyperTextAccessible* aRoot,
           HyperTextAccessible* aStartContainer, int32_t aStartOffset,
           HyperTextAccessible* aEndContainer, int32_t aEndOffset);

  






  bool TextInternal(nsAString& aText, Accessible* aCurrent,
                    uint32_t aStartIntlOffset) const;

  void MoveInternal(ETextUnit aUnit, int32_t aCount,
                    HyperTextAccessible& aContainer, int32_t aOffset,
                    HyperTextAccessible* aStopContainer = nullptr,
                    int32_t aStopOffset = 0);

  nsRefPtr<HyperTextAccessible> mRoot;
  nsRefPtr<HyperTextAccessible> mStartContainer;
  nsRefPtr<HyperTextAccessible> mEndContainer;
  int32_t mStartOffset;
  int32_t mEndOffset;
};


} 
} 

#endif
