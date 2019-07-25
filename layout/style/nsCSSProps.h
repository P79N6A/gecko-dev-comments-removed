











































#ifndef nsCSSProps_h___
#define nsCSSProps_h___

#include "nsString.h"
#include "nsChangeHint.h"
#include "nsCSSProperty.h"
#include "nsStyleStruct.h"
#include "nsCSSKeywords.h"





#define CSS_PROPERTY_DIRECTIONAL_SOURCE           (1<<0)

#define CSS_PROPERTY_VALUE_LIST_USES_COMMAS       (1<<1) /* otherwise spaces */

#define CSS_PROPERTY_APPLIES_TO_FIRST_LETTER      (1<<2)
#define CSS_PROPERTY_APPLIES_TO_FIRST_LINE        (1<<3)
#define CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE \
  (CSS_PROPERTY_APPLIES_TO_FIRST_LETTER | CSS_PROPERTY_APPLIES_TO_FIRST_LINE)



#define CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED (1<<4)






#define CSS_PROPERTY_START_IMAGE_LOADS            (1<<5)





#define CSS_PROPERTY_IMAGE_IS_IN_ARRAY_0          (1<<6)






#define CSS_PROPERTY_REPORT_OTHER_NAME            (1<<7)




enum nsStyleAnimType {
  
  
  eStyleAnimType_Custom,

  
  eStyleAnimType_Coord,

  
  
  eStyleAnimType_Sides_Top,
  eStyleAnimType_Sides_Right,
  eStyleAnimType_Sides_Bottom,
  eStyleAnimType_Sides_Left,

  
  
  eStyleAnimType_Corner_TopLeft,
  eStyleAnimType_Corner_TopRight,
  eStyleAnimType_Corner_BottomRight,
  eStyleAnimType_Corner_BottomLeft,

  
  eStyleAnimType_nscoord,

  
  
  
  
  eStyleAnimType_EnumU8,

  
  eStyleAnimType_float,

  
  eStyleAnimType_Color,

  
  eStyleAnimType_PaintServer,

  
  eStyleAnimType_Shadow,

  
  eStyleAnimType_None
};

class nsCSSProps {
public:
  static void AddRefTable(void);
  static void ReleaseTable(void);

  
  static nsCSSProperty LookupProperty(const nsAString& aProperty);
  static nsCSSProperty LookupProperty(const nsACString& aProperty);

  static inline PRBool IsShorthand(nsCSSProperty aProperty) {
    NS_ABORT_IF_FALSE(0 <= aProperty && aProperty < eCSSProperty_COUNT,
                 "out of range");
    return (aProperty >= eCSSProperty_COUNT_no_shorthands);
  }

  
  static nsCSSFontDesc LookupFontDesc(const nsAString& aProperty);
  static nsCSSFontDesc LookupFontDesc(const nsACString& aProperty);

  
  static const nsAFlatCString& GetStringValue(nsCSSProperty aProperty);
  static const nsAFlatCString& GetStringValue(nsCSSFontDesc aFontDesc);

  
  
  
  static nsCSSProperty OtherNameFor(nsCSSProperty aProperty);

  
  
  
  static const nsAFlatCString& LookupPropertyValue(nsCSSProperty aProperty, PRInt32 aValue);

  
  
  static PRBool GetColorName(PRInt32 aPropID, nsCString &aStr);

  
  
  static PRBool FindKeyword(nsCSSKeyword aKeyword, const PRInt32 aTable[], PRInt32& aValue);
  
  
  static nsCSSKeyword ValueToKeywordEnum(PRInt32 aValue, const PRInt32 aTable[]);
  
  static const nsAFlatCString& ValueToKeyword(PRInt32 aValue, const PRInt32 aTable[]);

  static const nsStyleStructID kSIDTable[eCSSProperty_COUNT_no_shorthands];
  static const PRInt32* const  kKeywordTableTable[eCSSProperty_COUNT_no_shorthands];
  static const nsStyleAnimType kAnimTypeTable[eCSSProperty_COUNT_no_shorthands];
  static const ptrdiff_t
    kStyleStructOffsetTable[eCSSProperty_COUNT_no_shorthands];

private:
  static const PRUint32        kFlagsTable[eCSSProperty_COUNT];

public:
  static inline PRBool PropHasFlags(nsCSSProperty aProperty, PRUint32 aFlags)
  {
    NS_ABORT_IF_FALSE(0 <= aProperty && aProperty < eCSSProperty_COUNT,
                      "out of range");
    return (nsCSSProps::kFlagsTable[aProperty] & aFlags) == aFlags;
  }

private:
  
  
  static const nsCSSProperty *const
    kSubpropertyTable[eCSSProperty_COUNT - eCSSProperty_COUNT_no_shorthands];

public:
  static inline
  const nsCSSProperty * SubpropertyEntryFor(nsCSSProperty aProperty) {
    NS_ABORT_IF_FALSE(eCSSProperty_COUNT_no_shorthands <= aProperty &&
                      aProperty < eCSSProperty_COUNT,
                      "out of range");
    return nsCSSProps::kSubpropertyTable[aProperty -
                                         eCSSProperty_COUNT_no_shorthands];
  }

  
  
  
  static const nsCSSProperty * ShorthandsContaining(nsCSSProperty aProperty) {
    NS_ABORT_IF_FALSE(gShorthandsContainingPool, "uninitialized");
    NS_ABORT_IF_FALSE(0 <= aProperty &&
                      aProperty < eCSSProperty_COUNT_no_shorthands,
                      "out of range");
    return gShorthandsContainingTable[aProperty];
  }
private:
  
  
  
  
  
  static nsCSSProperty *gShorthandsContainingTable[eCSSProperty_COUNT_no_shorthands];
  static nsCSSProperty* gShorthandsContainingPool;
  static PRBool BuildShorthandsContainingTable();

public:

#define CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(iter_, prop_)                    \
  for (const nsCSSProperty* iter_ = nsCSSProps::SubpropertyEntryFor(prop_);   \
       *iter_ != eCSSProperty_UNKNOWN; ++iter_)

  
  static const PRInt32 kAppearanceKTable[];
  static const PRInt32 kAzimuthKTable[];
  static const PRInt32 kBackgroundAttachmentKTable[];
  static const PRInt32 kBackgroundInlinePolicyKTable[];
  static const PRInt32 kBackgroundOriginKTable[];
  static const PRInt32 kBackgroundPositionKTable[];
  static const PRInt32 kBackgroundRepeatKTable[];
  static const PRInt32 kBackgroundSizeKTable[];
  static const PRInt32 kBorderCollapseKTable[];
  static const PRInt32 kBorderColorKTable[];
  static const PRInt32 kBorderImageKTable[];
  static const PRInt32 kBorderStyleKTable[];
  static const PRInt32 kBorderWidthKTable[];
  static const PRInt32 kBoxAlignKTable[];
  static const PRInt32 kBoxDirectionKTable[];
  static const PRInt32 kBoxOrientKTable[];
  static const PRInt32 kBoxPackKTable[];
  static const PRInt32 kDominantBaselineKTable[];
  static const PRInt32 kFillRuleKTable[];
  static const PRInt32 kImageRenderingKTable[];
  static const PRInt32 kShapeRenderingKTable[];
  static const PRInt32 kStrokeLinecapKTable[];
  static const PRInt32 kStrokeLinejoinKTable[];
  static const PRInt32 kTextAnchorKTable[];
  static const PRInt32 kTextRenderingKTable[];
  static const PRInt32 kColorInterpolationKTable[];
  static const PRInt32 kBoxPropSourceKTable[];
  static const PRInt32 kBoxShadowTypeKTable[];
  static const PRInt32 kBoxSizingKTable[];
  static const PRInt32 kCaptionSideKTable[];
  static const PRInt32 kClearKTable[];
  static const PRInt32 kColorKTable[];
  static const PRInt32 kContentKTable[];
  static const PRInt32 kCursorKTable[];
  static const PRInt32 kDirectionKTable[];
  static const PRInt32 kDisplayKTable[];
  static const PRInt32 kElevationKTable[];
  static const PRInt32 kEmptyCellsKTable[];
  static const PRInt32 kFloatKTable[];
  static const PRInt32 kFloatEdgeKTable[];
  static const PRInt32 kFontKTable[];
  static const PRInt32 kFontSizeKTable[];
  static const PRInt32 kFontStretchKTable[];
  static const PRInt32 kFontStyleKTable[];
  static const PRInt32 kFontVariantKTable[];
  static const PRInt32 kFontWeightKTable[];
  static const PRInt32 kIMEModeKTable[];
  static const PRInt32 kLineHeightKTable[];
  static const PRInt32 kListStylePositionKTable[];
  static const PRInt32 kListStyleKTable[];
  static const PRInt32 kOutlineStyleKTable[];
  static const PRInt32 kOutlineColorKTable[];
  static const PRInt32 kOverflowKTable[];
  static const PRInt32 kOverflowSubKTable[];
  static const PRInt32 kPageBreakKTable[];
  static const PRInt32 kPageBreakInsideKTable[];
  static const PRInt32 kPageMarksKTable[];
  static const PRInt32 kPageSizeKTable[];
  static const PRInt32 kPitchKTable[];
  static const PRInt32 kPointerEventsKTable[];
  static const PRInt32 kPositionKTable[];
  static const PRInt32 kRadialGradientShapeKTable[];
  static const PRInt32 kRadialGradientSizeKTable[];
  static const PRInt32 kResizeKTable[];
  static const PRInt32 kSpeakKTable[];
  static const PRInt32 kSpeakHeaderKTable[];
  static const PRInt32 kSpeakNumeralKTable[];
  static const PRInt32 kSpeakPunctuationKTable[];
  static const PRInt32 kSpeechRateKTable[];
  static const PRInt32 kStackSizingKTable[];
  static const PRInt32 kTableLayoutKTable[];
  static const PRInt32 kTextAlignKTable[];
  static const PRInt32 kTextDecorationKTable[];
  static const PRInt32 kTextTransformKTable[];
  static const PRInt32 kTransitionTimingFunctionKTable[];
  static const PRInt32 kUnicodeBidiKTable[];
  static const PRInt32 kUserFocusKTable[];
  static const PRInt32 kUserInputKTable[];
  static const PRInt32 kUserModifyKTable[];
  static const PRInt32 kUserSelectKTable[];
  static const PRInt32 kVerticalAlignKTable[];
  static const PRInt32 kVisibilityKTable[];
  static const PRInt32 kVolumeKTable[];
  static const PRInt32 kWhitespaceKTable[];
  static const PRInt32 kWidthKTable[]; 
  static const PRInt32 kWindowShadowKTable[];
  static const PRInt32 kWordwrapKTable[];
};

#endif 
