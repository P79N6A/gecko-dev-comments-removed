




#ifndef mozilla_TextEvents_h__
#define mozilla_TextEvents_h__

#include <stdint.h>

#include "mozilla/Assertions.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventForwards.h" 
#include "nsColor.h"
#include "nsCOMPtr.h"
#include "nsIDOMKeyEvent.h"
#include "nsITransferable.h"
#include "nsRect.h"
#include "nsStringGlue.h"
#include "nsStyleConsts.h"
#include "nsTArray.h"





#define NS_DEFINE_VK(aDOMKeyName, aDOMKeyCode) NS_##aDOMKeyName = aDOMKeyCode

enum
{
#include "nsVKList.h"
};

#undef NS_DEFINE_VK

namespace mozilla {

namespace dom {
  class PBrowserParent;
  class PBrowserChild;
} 
namespace plugins {
  class PPluginInstanceChild;
} 








struct AlternativeCharCode
{
  AlternativeCharCode(uint32_t aUnshiftedCharCode, uint32_t aShiftedCharCode) :
    mUnshiftedCharCode(aUnshiftedCharCode), mShiftedCharCode(aShiftedCharCode)
  {
  }
  uint32_t mUnshiftedCharCode;
  uint32_t mShiftedCharCode;
};





class WidgetKeyboardEvent : public WidgetInputEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;

public:
  WidgetKeyboardEvent()
  {
  }

  WidgetKeyboardEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetInputEvent(aIsTrusted, aMessage, aWidget, NS_KEY_EVENT),
    keyCode(0), charCode(0),
    location(nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD), isChar(0),
    mKeyNameIndex(mozilla::KEY_NAME_INDEX_Unidentified),
    mNativeKeyEvent(nullptr),
    mUniqueId(0)
  {
  }

  
  
  uint32_t keyCode;
  
  
  
  
  uint32_t charCode;
  
  uint32_t location;
  
  
  nsTArray<AlternativeCharCode> alternativeCharCodes;
  
  bool isChar;
  
  KeyNameIndex mKeyNameIndex;
  
  void* mNativeKeyEvent;
  
  
  
  
  uint32_t mUniqueId;

  void GetDOMKeyName(nsAString& aKeyName)
  {
    GetDOMKeyName(mKeyNameIndex, aKeyName);
  }

  static void GetDOMKeyName(mozilla::KeyNameIndex aKeyNameIndex,
                            nsAString& aKeyName)
  {
#define NS_DEFINE_KEYNAME(aCPPName, aDOMKeyName) \
      case KEY_NAME_INDEX_##aCPPName: \
        aKeyName.Assign(NS_LITERAL_STRING(aDOMKeyName)); return;
    switch (aKeyNameIndex) {
#include "nsDOMKeyNameList.h"
      default:
        aKeyName.Truncate();
        return;
    }
#undef NS_DEFINE_KEYNAME
  }

  void AssignKeyEventData(const WidgetKeyboardEvent& aEvent, bool aCopyTargets)
  {
    AssignInputEventData(aEvent, aCopyTargets);

    keyCode = aEvent.keyCode;
    charCode = aEvent.charCode;
    location = aEvent.location;
    alternativeCharCodes = aEvent.alternativeCharCodes;
    isChar = aEvent.isChar;
    mKeyNameIndex = aEvent.mKeyNameIndex;
    
    
    mNativeKeyEvent = nullptr;
    mUniqueId = aEvent.mUniqueId;
  }
};





struct TextRangeStyle
{
  enum {
    LINESTYLE_NONE   = NS_STYLE_TEXT_DECORATION_STYLE_NONE,
    LINESTYLE_SOLID  = NS_STYLE_TEXT_DECORATION_STYLE_SOLID,
    LINESTYLE_DOTTED = NS_STYLE_TEXT_DECORATION_STYLE_DOTTED,
    LINESTYLE_DASHED = NS_STYLE_TEXT_DECORATION_STYLE_DASHED,
    LINESTYLE_DOUBLE = NS_STYLE_TEXT_DECORATION_STYLE_DOUBLE,
    LINESTYLE_WAVY   = NS_STYLE_TEXT_DECORATION_STYLE_WAVY
  };

  enum {
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

  bool Equals(const TextRangeStyle& aOther)
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

  bool operator !=(const TextRangeStyle &aOther)
  {
    return !Equals(aOther);
  }

  bool operator ==(const TextRangeStyle &aOther)
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






#define NS_TEXTRANGE_CARETPOSITION         0x01
#define NS_TEXTRANGE_RAWINPUT              0x02
#define NS_TEXTRANGE_SELECTEDRAWTEXT       0x03
#define NS_TEXTRANGE_CONVERTEDTEXT         0x04
#define NS_TEXTRANGE_SELECTEDCONVERTEDTEXT 0x05

struct TextRange
{
  TextRange() :
    mStartOffset(0), mEndOffset(0), mRangeType(0)
  {
  }

  uint32_t mStartOffset;
  
  
  uint32_t mEndOffset;
  uint32_t mRangeType;

  TextRangeStyle mRangeStyle;

  uint32_t Length() const { return mEndOffset - mStartOffset; }
};







typedef TextRange* TextRangeArray;











class WidgetTextEvent : public WidgetGUIEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;
  friend class plugins::PPluginInstanceChild;

  WidgetTextEvent()
  {
  }

public:
  uint32_t seqno;

public:
  WidgetTextEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_TEXT_EVENT),
    rangeCount(0), rangeArray(nullptr), isChar(false)
  {
  }

  
  nsString theText;
  
  uint32_t rangeCount;
  
  
  
  
  TextRangeArray rangeArray;
  
  
  
  bool isChar;

  void AssignTextEventData(const WidgetTextEvent& aEvent, bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    isChar = aEvent.isChar;

    
    
  }
};





class WidgetCompositionEvent : public WidgetGUIEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  WidgetCompositionEvent()
  {
  }

public:
  uint32_t seqno;

public:
  WidgetCompositionEvent(bool aIsTrusted, uint32_t aMessage,
                         nsIWidget* aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_COMPOSITION_EVENT)
  {
    
    
    
    mFlags.mCancelable = false;
  }

  
  
  
  nsString data;

  void AssignCompositionEventData(const WidgetCompositionEvent& aEvent,
                                  bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    data = aEvent.data;
  }
};





class WidgetQueryContentEvent : public WidgetGUIEvent
{
private:
  friend class dom::PBrowserParent;
  friend class dom::PBrowserChild;

  WidgetQueryContentEvent()
  {
    MOZ_CRASH("WidgetQueryContentEvent is created without proper arguments");
  }

public:
  WidgetQueryContentEvent(bool aIsTrusted, uint32_t aMessage,
                          nsIWidget* aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_QUERY_CONTENT_EVENT),
    mSucceeded(false), mWasAsync(false)
  {
  }

  void InitForQueryTextContent(uint32_t aOffset, uint32_t aLength)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_CONTENT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
  }

  void InitForQueryCaretRect(uint32_t aOffset)
  {
    NS_ASSERTION(message == NS_QUERY_CARET_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
  }

  void InitForQueryTextRect(uint32_t aOffset, uint32_t aLength)
  {
    NS_ASSERTION(message == NS_QUERY_TEXT_RECT,
                 "wrong initializer is called");
    mInput.mOffset = aOffset;
    mInput.mLength = aLength;
  }

  void InitForQueryDOMWidgetHittest(const mozilla::LayoutDeviceIntPoint& aPoint)
  {
    NS_ASSERTION(message == NS_QUERY_DOM_WIDGET_HITTEST,
                 "wrong initializer is called");
    refPoint = aPoint;
  }

  uint32_t GetSelectionStart(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT,
                 "not querying selection");
    return mReply.mOffset + (mReply.mReversed ? mReply.mString.Length() : 0);
  }

  uint32_t GetSelectionEnd(void) const
  {
    NS_ASSERTION(message == NS_QUERY_SELECTED_TEXT,
                 "not querying selection");
    return mReply.mOffset + (mReply.mReversed ? 0 : mReply.mString.Length());
  }

  bool mSucceeded;
  bool mWasAsync;
  struct
  {
    uint32_t mOffset;
    uint32_t mLength;
  } mInput;
  struct
  {
    void* mContentsRoot;
    uint32_t mOffset;
    nsString mString;
    
    nsIntRect mRect;
    
    nsIWidget* mFocusedWidget;
    
    bool mReversed;
    
    bool mHasSelection;
    
    bool mWidgetIsHit;
    
    nsCOMPtr<nsITransferable> mTransferable;
  } mReply;

  enum
  {
    NOT_FOUND = UINT32_MAX
  };

  
  enum
  {
    SCROLL_ACTION_NONE,
    SCROLL_ACTION_LINE,
    SCROLL_ACTION_PAGE
  };
};





class WidgetSelectionEvent : public WidgetGUIEvent
{
private:
  friend class mozilla::dom::PBrowserParent;
  friend class mozilla::dom::PBrowserChild;

  WidgetSelectionEvent()
  {
    MOZ_CRASH("WidgetSelectionEvent is created without proper arguments");
  }

public:
  uint32_t seqno;

public:
  WidgetSelectionEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_SELECTION_EVENT),
    mExpandToClusterBoundary(true), mSucceeded(false)
  {
  }

  
  uint32_t mOffset;
  
  uint32_t mLength;
  
  bool mReversed;
  
  bool mExpandToClusterBoundary;
  
  bool mSucceeded;
};

} 


typedef mozilla::TextRange               nsTextRange;
typedef mozilla::WidgetTextEvent         nsTextEvent;
typedef mozilla::WidgetCompositionEvent  nsCompositionEvent;
typedef mozilla::WidgetQueryContentEvent nsQueryContentEvent;
typedef mozilla::WidgetSelectionEvent    nsSelectionEvent;

#endif 
