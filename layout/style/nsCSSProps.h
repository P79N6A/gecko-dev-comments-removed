









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





#define CSS_PROPERTY_STORES_CALC                  (1<<8)












#define CSS_PROPERTY_PARSE_PROPERTY_MASK          (7<<9)
#define CSS_PROPERTY_PARSE_INACCESSIBLE           (1<<9)
#define CSS_PROPERTY_PARSE_FUNCTION               (2<<9)
#define CSS_PROPERTY_PARSE_VALUE                  (3<<9)
#define CSS_PROPERTY_PARSE_VALUE_LIST             (4<<9)



#define CSS_PROPERTY_VALUE_PARSER_FUNCTION        (1<<12)
MOZ_STATIC_ASSERT((CSS_PROPERTY_PARSE_PROPERTY_MASK &
                   CSS_PROPERTY_VALUE_PARSER_FUNCTION) == 0,
                  "didn't leave enough room for the parse property constants");

#define CSS_PROPERTY_VALUE_RESTRICTION_MASK       (3<<13)


#define CSS_PROPERTY_VALUE_NONNEGATIVE            (1<<13)


#define CSS_PROPERTY_VALUE_AT_LEAST_ONE           (2<<13)


#define CSS_PROPERTY_HASHLESS_COLOR_QUIRK         (1<<15)


#define CSS_PROPERTY_UNITLESS_LENGTH_QUIRK        (1<<16)


#define CSS_PROPERTY_IS_ALIAS                     (1<<17)


#define CSS_PROPERTY_APPLIES_TO_PLACEHOLDER       (1<<18)


#define CSS_PROPERTY_APPLIES_TO_PAGE_RULE         (1<<19)




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

  
  enum EnabledState {
    eEnabled,
    eAny
  };
  static nsCSSProperty LookupProperty(const nsAString& aProperty,
                                      EnabledState aEnabled);
  static nsCSSProperty LookupProperty(const nsACString& aProperty,
                                      EnabledState aEnabled);

  static inline bool IsShorthand(nsCSSProperty aProperty) {
    NS_ABORT_IF_FALSE(0 <= aProperty && aProperty < eCSSProperty_COUNT,
                 "out of range");
    return (aProperty >= eCSSProperty_COUNT_no_shorthands);
  }

  
  static nsCSSFontDesc LookupFontDesc(const nsAString& aProperty);
  static nsCSSFontDesc LookupFontDesc(const nsACString& aProperty);

  
  static const nsAFlatCString& GetStringValue(nsCSSProperty aProperty);
  static const nsAFlatCString& GetStringValue(nsCSSFontDesc aFontDesc);

  
  
  
  static nsCSSProperty OtherNameFor(nsCSSProperty aProperty);

  
  
  
  static const nsAFlatCString& LookupPropertyValue(nsCSSProperty aProperty, int32_t aValue);

  
  
  static bool GetColorName(int32_t aPropID, nsCString &aStr);

  
  
  
  static int32_t FindIndexOfKeyword(nsCSSKeyword aKeyword, const int32_t aTable[]);

  
  
  static bool FindKeyword(nsCSSKeyword aKeyword, const int32_t aTable[], int32_t& aValue);
  
  
  static nsCSSKeyword ValueToKeywordEnum(int32_t aValue, const int32_t aTable[]);
  
  static const nsAFlatCString& ValueToKeyword(int32_t aValue, const int32_t aTable[]);

  static const nsStyleStructID kSIDTable[eCSSProperty_COUNT_no_shorthands];
  static const int32_t* const  kKeywordTableTable[eCSSProperty_COUNT_no_shorthands];
  static const nsStyleAnimType kAnimTypeTable[eCSSProperty_COUNT_no_shorthands];
  static const ptrdiff_t
    kStyleStructOffsetTable[eCSSProperty_COUNT_no_shorthands];

private:
  static const uint32_t        kFlagsTable[eCSSProperty_COUNT];

public:
  static inline bool PropHasFlags(nsCSSProperty aProperty, uint32_t aFlags)
  {
    NS_ABORT_IF_FALSE(0 <= aProperty && aProperty < eCSSProperty_COUNT,
                      "out of range");
    return (nsCSSProps::kFlagsTable[aProperty] & aFlags) == aFlags;
  }

  static inline uint32_t PropertyParseType(nsCSSProperty aProperty)
  {
    NS_ABORT_IF_FALSE(0 <= aProperty && aProperty < eCSSProperty_COUNT,
                      "out of range");
    return nsCSSProps::kFlagsTable[aProperty] &
           CSS_PROPERTY_PARSE_PROPERTY_MASK;
  }

  static inline uint32_t ValueRestrictions(nsCSSProperty aProperty)
  {
    NS_ABORT_IF_FALSE(0 <= aProperty && aProperty < eCSSProperty_COUNT,
                      "out of range");
    return nsCSSProps::kFlagsTable[aProperty] &
           CSS_PROPERTY_VALUE_RESTRICTION_MASK;
  }

private:
  
  static const uint32_t kParserVariantTable[eCSSProperty_COUNT_no_shorthands];

public:
  static inline uint32_t ParserVariant(nsCSSProperty aProperty) {
    NS_ABORT_IF_FALSE(0 <= aProperty &&
                      aProperty < eCSSProperty_COUNT_no_shorthands,
                      "out of range");
    return nsCSSProps::kParserVariantTable[aProperty];
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
  static bool BuildShorthandsContainingTable();

private:
  static const size_t gPropertyCountInStruct[nsStyleStructID_Length];
  static const size_t gPropertyIndexInStruct[eCSSProperty_COUNT_no_shorthands];
public:
  



  static size_t PropertyCountInStruct(nsStyleStructID aSID) {
    NS_ABORT_IF_FALSE(0 <= aSID && aSID < nsStyleStructID_Length,
                      "out of range");
    return gPropertyCountInStruct[aSID];
  }
  



  static size_t PropertyIndexInStruct(nsCSSProperty aProperty) {
    NS_ABORT_IF_FALSE(0 <= aProperty &&
                         aProperty < eCSSProperty_COUNT_no_shorthands,
                      "out of range");
    return gPropertyIndexInStruct[aProperty];
  }

private:
  static bool gPropertyEnabled[eCSSProperty_COUNT_with_aliases];

public:

  static bool IsEnabled(nsCSSProperty aProperty) {
    NS_ABORT_IF_FALSE(0 <= aProperty &&
                      aProperty < eCSSProperty_COUNT_with_aliases,
                      "out of range");
    return gPropertyEnabled[aProperty];
  }

public:

#define CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(iter_, prop_)                    \
  for (const nsCSSProperty* iter_ = nsCSSProps::SubpropertyEntryFor(prop_);   \
       *iter_ != eCSSProperty_UNKNOWN; ++iter_)

  
  static const int32_t kAnimationDirectionKTable[];
  static const int32_t kAnimationFillModeKTable[];
  static const int32_t kAnimationIterationCountKTable[];
  static const int32_t kAnimationPlayStateKTable[];
  static const int32_t kAnimationTimingFunctionKTable[];
  static const int32_t kAppearanceKTable[];
  static const int32_t kAzimuthKTable[];
  static const int32_t kBackfaceVisibilityKTable[];
  static const int32_t kTransformStyleKTable[];
  static const int32_t kBackgroundAttachmentKTable[];
  static const int32_t kBackgroundInlinePolicyKTable[];
  static const int32_t kBackgroundOriginKTable[];
  static const int32_t kBackgroundPositionKTable[];
  static const int32_t kBackgroundRepeatKTable[];
  static const int32_t kBackgroundRepeatPartKTable[];
  static const int32_t kBackgroundSizeKTable[];
  static const int32_t kBorderCollapseKTable[];
  static const int32_t kBorderColorKTable[];
  static const int32_t kBorderImageRepeatKTable[];
  static const int32_t kBorderImageSliceKTable[];
  static const int32_t kBorderStyleKTable[];
  static const int32_t kBorderWidthKTable[];
  static const int32_t kBoxAlignKTable[];
  static const int32_t kBoxDirectionKTable[];
  static const int32_t kBoxOrientKTable[];
  static const int32_t kBoxPackKTable[];
  static const int32_t kDominantBaselineKTable[];
  static const int32_t kFillRuleKTable[];
  static const int32_t kImageRenderingKTable[];
  static const int32_t kShapeRenderingKTable[];
  static const int32_t kStrokeLinecapKTable[];
  static const int32_t kStrokeLinejoinKTable[];
  static const int32_t kStrokeObjectValueKTable[];
  static const int32_t kVectorEffectKTable[];
  static const int32_t kTextAnchorKTable[];
  static const int32_t kTextRenderingKTable[];
  static const int32_t kColorInterpolationKTable[];
  static const int32_t kColumnFillKTable[];
  static const int32_t kBoxPropSourceKTable[];
  static const int32_t kBoxShadowTypeKTable[];
  static const int32_t kBoxSizingKTable[];
  static const int32_t kCaptionSideKTable[];
  static const int32_t kClearKTable[];
  static const int32_t kColorKTable[];
  static const int32_t kContentKTable[];
  static const int32_t kCursorKTable[];
  static const int32_t kDirectionKTable[];
  
  static int32_t kDisplayKTable[];
  static const int32_t kElevationKTable[];
  static const int32_t kEmptyCellsKTable[];
#ifdef MOZ_FLEXBOX
  static const int32_t kAlignItemsKTable[];
  static const int32_t kAlignSelfKTable[];
  static const int32_t kFlexDirectionKTable[];
  static const int32_t kJustifyContentKTable[];
#endif 
  static const int32_t kFloatKTable[];
  static const int32_t kFloatEdgeKTable[];
  static const int32_t kFontKTable[];
  static const int32_t kFontSizeKTable[];
  static const int32_t kFontStretchKTable[];
  static const int32_t kFontStyleKTable[];
  static const int32_t kFontVariantKTable[];
  static const int32_t kFontWeightKTable[];
  static const int32_t kIMEModeKTable[];
  static const int32_t kLineHeightKTable[];
  static const int32_t kListStylePositionKTable[];
  static const int32_t kListStyleKTable[];
  static const int32_t kMaskTypeKTable[];
  static const int32_t kObjectOpacityKTable[];
  static const int32_t kObjectPatternKTable[];
  static const int32_t kOrientKTable[];
  static const int32_t kOutlineStyleKTable[];
  static const int32_t kOutlineColorKTable[];
  static const int32_t kOverflowKTable[];
  static const int32_t kOverflowSubKTable[];
  static const int32_t kPageBreakKTable[];
  static const int32_t kPageBreakInsideKTable[];
  static const int32_t kPageMarksKTable[];
  static const int32_t kPageSizeKTable[];
  static const int32_t kPitchKTable[];
  static const int32_t kPointerEventsKTable[];
  static const int32_t kPositionKTable[];
  static const int32_t kRadialGradientShapeKTable[];
  static const int32_t kRadialGradientSizeKTable[];
  static const int32_t kRadialGradientLegacySizeKTable[];
  static const int32_t kResizeKTable[];
  static const int32_t kSpeakKTable[];
  static const int32_t kSpeakHeaderKTable[];
  static const int32_t kSpeakNumeralKTable[];
  static const int32_t kSpeakPunctuationKTable[];
  static const int32_t kSpeechRateKTable[];
  static const int32_t kStackSizingKTable[];
  static const int32_t kTableLayoutKTable[];
  static const int32_t kTextAlignKTable[];
  static const int32_t kTextAlignLastKTable[];
  static const int32_t kTextBlinkKTable[];
  static const int32_t kTextDecorationLineKTable[];
  static const int32_t kTextDecorationStyleKTable[];
  static const int32_t kTextOverflowKTable[];
  static const int32_t kTextTransformKTable[];
  static const int32_t kTransitionTimingFunctionKTable[];
  static const int32_t kUnicodeBidiKTable[];
  static const int32_t kUserFocusKTable[];
  static const int32_t kUserInputKTable[];
  static const int32_t kUserModifyKTable[];
  static const int32_t kUserSelectKTable[];
  static const int32_t kVerticalAlignKTable[];
  static const int32_t kVisibilityKTable[];
  static const int32_t kVolumeKTable[];
  static const int32_t kWhitespaceKTable[];
  static const int32_t kWidthKTable[]; 
  static const int32_t kWindowShadowKTable[];
  static const int32_t kWordBreakKTable[];
  static const int32_t kWordWrapKTable[];
  static const int32_t kHyphensKTable[];
};

#endif 
