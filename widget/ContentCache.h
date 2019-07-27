






#ifndef mozilla_ContentCache_h
#define mozilla_ContentCache_h

#include <stdint.h>

#include "mozilla/Assertions.h"
#include "mozilla/EventForwards.h"
#include "nsString.h"
#include "nsTArray.h"
#include "Units.h"

class nsIWidget;

namespace mozilla {







class ContentCache final
{
public:
  typedef InfallibleTArray<LayoutDeviceIntRect> RectArray;

  ContentCache();

  void Clear();

  void SetText(const nsAString& aText);
  const nsString& Text() const { return mText; }
  uint32_t TextLength() const { return mText.Length(); }

  



  bool OnCompositionEvent(const WidgetCompositionEvent& aCompositionEvent);
  













  uint32_t RequestToCommitComposition(nsIWidget* aWidget,
                                      bool aCancel,
                                      nsAString& aLastString);

  void SetSelection(uint32_t aCaretOffset)
  {
    SetSelection(aCaretOffset, aCaretOffset);
  }
  void SetSelection(uint32_t aAnchorOffset, uint32_t aFocusOffset);
  bool SelectionCollapsed() const { return mSelection.Collapsed(); }
  bool SelectionReversed() const { return mSelection.Reversed(); }
  bool SelectionEndIsGraterThanTextLength() const
  {
    return SelectionEnd() > TextLength();
  }
  uint32_t SelectionAnchor() const { return mSelection.mAnchor; }
  uint32_t SelectionFocus() const { return mSelection.mFocus; }
  uint32_t SelectionStart() const { return mSelection.StartOffset(); }
  uint32_t SelectionEnd() const { return mSelection.EndOffset(); }
  uint32_t SelectionLength() const { return mSelection.Length(); }

  bool UpdateTextRectArray(const RectArray& aTextRectArray)
  {
    return InitTextRectArray(mTextRectArray.mStart, aTextRectArray);
  }
  bool InitTextRectArray(uint32_t aOffset, const RectArray& aTextRectArray);
  bool GetTextRect(uint32_t aOffset,
                   LayoutDeviceIntRect& aTextRect) const;
  bool GetUnionTextRects(uint32_t aOffset,
                         uint32_t aLength,
                         LayoutDeviceIntRect& aUnionTextRect) const;

  bool UpdateCaretRect(const LayoutDeviceIntRect& aCaretRect)
  {
    return InitCaretRect(mCaret.mOffset, aCaretRect);
  }
  bool InitCaretRect(uint32_t aOffset, const LayoutDeviceIntRect& aCaretRect);
  uint32_t CaretOffset() const { return mCaret.mOffset; }
  bool GetCaretRect(uint32_t aOffset, LayoutDeviceIntRect& aCaretRect) const;

private:
  
  nsString mText;
  
  nsString mCommitStringByRequest;
  
  uint32_t mCompositionStart;
  
  
  uint32_t mCompositionEventsDuringRequest;

  struct Selection final
  {
    
    uint32_t mAnchor;
    uint32_t mFocus;

    Selection()
      : mAnchor(0)
      , mFocus(0)
    {
    }

    bool Collapsed() const { return mFocus == mAnchor; }
    bool Reversed() const { return mFocus < mAnchor; }
    uint32_t StartOffset() const { return Reversed() ? mFocus : mAnchor; }
    uint32_t EndOffset() const { return Reversed() ? mAnchor : mFocus; }
    uint32_t Length() const
    {
      return Reversed() ? mAnchor - mFocus : mFocus - mAnchor;
    }
  } mSelection;

  struct Caret final
  {
    uint32_t mOffset;
    LayoutDeviceIntRect mRect;

    Caret()
      : mOffset(UINT32_MAX)
    {
    }

    uint32_t Offset() const
    {
      NS_WARN_IF(mOffset == UINT32_MAX);
      return mOffset;
    }
  } mCaret;

  struct TextRectArray final
  {
    uint32_t mStart;
    RectArray mRects;

    TextRectArray()
      : mStart(UINT32_MAX)
    {
    }

    uint32_t StartOffset() const
    {
      NS_WARN_IF(mStart == UINT32_MAX);
      return mStart;
    }
    uint32_t EndOffset() const
    {
      if (NS_WARN_IF(mStart == UINT32_MAX) ||
          NS_WARN_IF(static_cast<uint64_t>(mStart) + mRects.Length() >
                       UINT32_MAX)) {
        return UINT32_MAX;
      }
      return mStart + mRects.Length();
    }
    bool InRange(uint32_t aOffset) const
    {
      return mStart != UINT32_MAX &&
             StartOffset() <= aOffset && aOffset < EndOffset();
    }
    bool InRange(uint32_t aOffset, uint32_t aLength) const
    {
      if (NS_WARN_IF(static_cast<uint64_t>(aOffset) + aLength > UINT32_MAX)) {
        return false;
      }
      return InRange(aOffset) && aOffset + aLength <= EndOffset();
    }
    LayoutDeviceIntRect GetRect(uint32_t aOffset) const;
    LayoutDeviceIntRect GetUnionRect(uint32_t aOffset, uint32_t aLength) const;
  } mTextRectArray;

  bool mIsComposing;
  bool mRequestedToCommitOrCancelComposition;
};

} 

#endif 
