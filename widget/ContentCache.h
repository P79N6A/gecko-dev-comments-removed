






#ifndef mozilla_ContentCache_h
#define mozilla_ContentCache_h

#include <stdint.h>

#include "nsString.h"

namespace mozilla {







class ContentCache final
{
public:
  void Clear();

  void SetText(const nsAString& aText);
  const nsString& Text() const { return mText; }
  uint32_t TextLength() const { return mText.Length(); }

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

private:
  
  nsString mText;

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
};

} 

#endif 
