






#ifndef mozilla_ContentCache_h
#define mozilla_ContentCache_h

#include <stdint.h>

#include "mozilla/Assertions.h"
#include "mozilla/EventForwards.h"
#include "mozilla/WritingModes.h"
#include "nsString.h"
#include "nsTArray.h"
#include "Units.h"

class nsIWidget;

namespace mozilla {

namespace widget {
struct IMENotification;
}











class ContentCache final
{
public:
  typedef InfallibleTArray<LayoutDeviceIntRect> RectArray;
  typedef widget::IMENotification IMENotification;

  ContentCache();

  




  void Clear();

  





  void AssignContent(const ContentCache& aOther,
                     const IMENotification* aNotification = nullptr);

  





















  bool HandleQueryContentEvent(WidgetQueryContentEvent& aEvent,
                               nsIWidget* aWidget) const;

  





  bool CacheEditorRect(nsIWidget* aWidget,
                       const IMENotification* aNotification = nullptr);
  bool CacheSelection(nsIWidget* aWidget,
                      const IMENotification* aNotification = nullptr);
  bool CacheText(nsIWidget* aWidget,
                 const IMENotification* aNotification = nullptr);

  bool CacheAll(nsIWidget* aWidget,
                const IMENotification* aNotification = nullptr);

  




  bool OnCompositionEvent(const WidgetCompositionEvent& aCompositionEvent);
  














  uint32_t RequestToCommitComposition(nsIWidget* aWidget,
                                      bool aCancel,
                                      nsAString& aLastString);

  




  void InitNotification(IMENotification& aNotification) const;

  




  void SetSelection(nsIWidget* aWidget,
                    uint32_t aStartOffset,
                    uint32_t aLength,
                    bool aReversed,
                    const WritingMode& aWritingMode);

private:
  
  nsString mText;
  
  
  nsString mCommitStringByRequest;
  
  
  uint32_t mCompositionStart;
  
  
  
  uint32_t mCompositionEventsDuringRequest;

  struct Selection final
  {
    
    uint32_t mAnchor;
    uint32_t mFocus;

    WritingMode mWritingMode;

    
    LayoutDeviceIntRect mAnchorCharRect;
    LayoutDeviceIntRect mFocusCharRect;

    Selection()
      : mAnchor(0)
      , mFocus(0)
    {
    }

    void Clear()
    {
      mAnchor = mFocus = 0;
      mWritingMode = WritingMode();
      mAnchorCharRect.SetEmpty();
      mFocusCharRect.SetEmpty();
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

    void Clear()
    {
      mOffset = UINT32_MAX;
      mRect.SetEmpty();
    }

    bool IsValid() const { return mOffset != UINT32_MAX; }

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

    void Clear()
    {
      mStart = UINT32_MAX;
      mRects.Clear();
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

  LayoutDeviceIntRect mEditorRect;

  
  bool mIsComposing;
  
  bool mRequestedToCommitOrCancelComposition;
  bool mIsChrome;

  bool QueryCharRect(nsIWidget* aWidget,
                     uint32_t aOffset,
                     LayoutDeviceIntRect& aCharRect) const;
  bool CacheCaret(nsIWidget* aWidget,
                  const IMENotification* aNotification = nullptr);
  bool CacheTextRects(nsIWidget* aWidget,
                      const IMENotification* aNotification = nullptr);

  bool GetCaretRect(uint32_t aOffset, LayoutDeviceIntRect& aCaretRect) const;
  bool GetTextRect(uint32_t aOffset,
                   LayoutDeviceIntRect& aTextRect) const;
  bool GetUnionTextRects(uint32_t aOffset,
                         uint32_t aLength,
                         LayoutDeviceIntRect& aUnionTextRect) const;

  friend struct IPC::ParamTraits<ContentCache>;
};

} 

#endif 
