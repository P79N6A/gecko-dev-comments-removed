




#ifndef mozilla_TextRage_h_
#define mozilla_TextRage_h_

#include <stdint.h>

#include "nsAutoPtr.h"
#include "nsColor.h"
#include "nsITextInputProcessor.h"
#include "nsStyleConsts.h"
#include "nsTArray.h"

namespace mozilla {





struct TextRangeStyle
{
  enum
  {
    LINESTYLE_NONE   = NS_STYLE_TEXT_DECORATION_STYLE_NONE,
    LINESTYLE_SOLID  = NS_STYLE_TEXT_DECORATION_STYLE_SOLID,
    LINESTYLE_DOTTED = NS_STYLE_TEXT_DECORATION_STYLE_DOTTED,
    LINESTYLE_DASHED = NS_STYLE_TEXT_DECORATION_STYLE_DASHED,
    LINESTYLE_DOUBLE = NS_STYLE_TEXT_DECORATION_STYLE_DOUBLE,
    LINESTYLE_WAVY   = NS_STYLE_TEXT_DECORATION_STYLE_WAVY
  };

  enum
  {
    DEFINED_NONE             = 0x00,
    DEFINED_LINESTYLE        = 0x01,
    DEFINED_FOREGROUND_COLOR = 0x02,
    DEFINED_BACKGROUND_COLOR = 0x04,
    DEFINED_UNDERLINE_COLOR  = 0x08
  };

  
  
  TextRangeStyle()
  {
    Clear();
  }

  void Clear()
  {
    mDefinedStyles = DEFINED_NONE;
    mLineStyle = LINESTYLE_NONE;
    mIsBoldLine = false;
    mForegroundColor = mBackgroundColor = mUnderlineColor = NS_RGBA(0, 0, 0, 0);
  }

  bool IsDefined() const { return mDefinedStyles != DEFINED_NONE; }

  bool IsLineStyleDefined() const
  {
    return (mDefinedStyles & DEFINED_LINESTYLE) != 0;
  }

  bool IsForegroundColorDefined() const
  {
    return (mDefinedStyles & DEFINED_FOREGROUND_COLOR) != 0;
  }

  bool IsBackgroundColorDefined() const
  {
    return (mDefinedStyles & DEFINED_BACKGROUND_COLOR) != 0;
  }

  bool IsUnderlineColorDefined() const
  {
    return (mDefinedStyles & DEFINED_UNDERLINE_COLOR) != 0;
  }

  bool IsNoChangeStyle() const
  {
    return !IsForegroundColorDefined() && !IsBackgroundColorDefined() &&
           IsLineStyleDefined() && mLineStyle == LINESTYLE_NONE;
  }

  bool Equals(const TextRangeStyle& aOther) const
  {
    if (mDefinedStyles != aOther.mDefinedStyles)
      return false;
    if (IsLineStyleDefined() && (mLineStyle != aOther.mLineStyle ||
                                 !mIsBoldLine != !aOther.mIsBoldLine))
      return false;
    if (IsForegroundColorDefined() &&
        (mForegroundColor != aOther.mForegroundColor))
      return false;
    if (IsBackgroundColorDefined() &&
        (mBackgroundColor != aOther.mBackgroundColor))
      return false;
    if (IsUnderlineColorDefined() &&
        (mUnderlineColor != aOther.mUnderlineColor))
      return false;
    return true;
  }

  bool operator !=(const TextRangeStyle &aOther) const
  {
    return !Equals(aOther);
  }

  bool operator ==(const TextRangeStyle &aOther) const
  {
    return Equals(aOther);
  }

  uint8_t mDefinedStyles;
  uint8_t mLineStyle;        

  bool mIsBoldLine;  

  nscolor mForegroundColor;  
  nscolor mBackgroundColor;  
  nscolor mUnderlineColor;   
};






enum
{
  NS_TEXTRANGE_UNDEFINED = 0x00,
  NS_TEXTRANGE_CARETPOSITION = 0x01,
  NS_TEXTRANGE_RAWINPUT =
    nsITextInputProcessor::ATTR_RAW_CLAUSE,
  NS_TEXTRANGE_SELECTEDRAWTEXT =
    nsITextInputProcessor::ATTR_SELECTED_RAW_CLAUSE,
  NS_TEXTRANGE_CONVERTEDTEXT =
    nsITextInputProcessor::ATTR_CONVERTED_CLAUSE,
  NS_TEXTRANGE_SELECTEDCONVERTEDTEXT =
    nsITextInputProcessor::ATTR_SELECTED_CLAUSE
};

struct TextRange
{
  TextRange() :
    mStartOffset(0), mEndOffset(0), mRangeType(NS_TEXTRANGE_UNDEFINED)
  {
  }

  uint32_t mStartOffset;
  
  
  uint32_t mEndOffset;
  uint32_t mRangeType;

  TextRangeStyle mRangeStyle;

  uint32_t Length() const { return mEndOffset - mStartOffset; }

  bool IsClause() const
  {
    MOZ_ASSERT(mRangeType >= NS_TEXTRANGE_CARETPOSITION &&
                 mRangeType <= NS_TEXTRANGE_SELECTEDCONVERTEDTEXT,
               "Invalid range type");
    return mRangeType != NS_TEXTRANGE_CARETPOSITION;
  }

  bool Equals(const TextRange& aOther) const
  {
    return mStartOffset == aOther.mStartOffset &&
           mEndOffset == aOther.mEndOffset &&
           mRangeType == aOther.mRangeType &&
           mRangeStyle == aOther.mRangeStyle;
  }
};




class TextRangeArray final : public nsAutoTArray<TextRange, 10>
{
  ~TextRangeArray() {}

  NS_INLINE_DECL_REFCOUNTING(TextRangeArray)

public:
  bool IsComposing() const
  {
    for (uint32_t i = 0; i < Length(); ++i) {
      if (ElementAt(i).IsClause()) {
        return true;
      }
    }
    return false;
  }

  
  
  uint32_t TargetClauseOffset() const
  {
    for (uint32_t i = 0; i < Length(); ++i) {
      const TextRange& range = ElementAt(i);
      if (range.mRangeType == NS_TEXTRANGE_SELECTEDRAWTEXT ||
          range.mRangeType == NS_TEXTRANGE_SELECTEDCONVERTEDTEXT) {
        return range.mStartOffset;
      }
    }
    return 0;
  }

  bool Equals(const TextRangeArray& aOther) const
  {
    size_t len = Length();
    if (len != aOther.Length()) {
      return false;
    }
    for (size_t i = 0; i < len; i++) {
      if (!ElementAt(i).Equals(aOther.ElementAt(i))) {
        return false;
      }
    }
    return true;
  }
};

} 

#endif 
