









#ifndef nsCSSProps_h___
#define nsCSSProps_h___

#include "nsString.h"
#include "nsCSSProperty.h"
#include "nsStyleStructFwd.h"
#include "nsCSSKeywords.h"



#define CSS_CUSTOM_NAME_PREFIX_LENGTH 2


#define VARIANT_KEYWORD         0x000001  // K
#define VARIANT_LENGTH          0x000002  // L
#define VARIANT_PERCENT         0x000004  // P
#define VARIANT_COLOR           0x000008  // C eCSSUnit_*Color, eCSSUnit_Ident (e.g.  "red")
#define VARIANT_URL             0x000010  // U
#define VARIANT_NUMBER          0x000020  // N
#define VARIANT_INTEGER         0x000040  // I
#define VARIANT_ANGLE           0x000080  // G
#define VARIANT_FREQUENCY       0x000100  // F
#define VARIANT_TIME            0x000200  // T
#define VARIANT_STRING          0x000400  // S
#define VARIANT_COUNTER         0x000800  //
#define VARIANT_ATTR            0x001000  //
#define VARIANT_IDENTIFIER      0x002000  // D
#define VARIANT_IDENTIFIER_NO_INHERIT 0x004000 // like above, but excluding

#define VARIANT_AUTO            0x010000  // A
#define VARIANT_INHERIT         0x020000  // H eCSSUnit_Initial, eCSSUnit_Inherit, eCSSUnit_Unset
#define VARIANT_NONE            0x040000  // O
#define VARIANT_NORMAL          0x080000  // M
#define VARIANT_SYSFONT         0x100000  // eCSSUnit_System_Font
#define VARIANT_GRADIENT        0x200000  // eCSSUnit_Gradient
#define VARIANT_TIMING_FUNCTION 0x400000  // cubic-bezier() and steps()
#define VARIANT_ALL             0x800000  //
#define VARIANT_IMAGE_RECT    0x01000000  // eCSSUnit_Function

#define VARIANT_ZERO_ANGLE    0x02000000  // unitless zero for angles
#define VARIANT_CALC          0x04000000  // eCSSUnit_Calc
#define VARIANT_ELEMENT       0x08000000  // eCSSUnit_Element
#define VARIANT_POSITIVE_DIMENSION 0x10000000 // Only lengths greater than 0.0
#define VARIANT_NONNEGATIVE_DIMENSION 0x20000000 // Only lengths greater than or equal to 0.0

#define VARIANT_OPENTYPE_SVG_KEYWORD 0x40000000


#define VARIANT_AL   (VARIANT_AUTO | VARIANT_LENGTH)
#define VARIANT_LP   (VARIANT_LENGTH | VARIANT_PERCENT)
#define VARIANT_LN   (VARIANT_LENGTH | VARIANT_NUMBER)
#define VARIANT_AH   (VARIANT_AUTO | VARIANT_INHERIT)
#define VARIANT_AHLP (VARIANT_AH | VARIANT_LP)
#define VARIANT_AHI  (VARIANT_AH | VARIANT_INTEGER)
#define VARIANT_AHK  (VARIANT_AH | VARIANT_KEYWORD)
#define VARIANT_AHKLP (VARIANT_AHLP | VARIANT_KEYWORD)
#define VARIANT_AHL  (VARIANT_AH | VARIANT_LENGTH)
#define VARIANT_AHKL (VARIANT_AHK | VARIANT_LENGTH)
#define VARIANT_HK   (VARIANT_INHERIT | VARIANT_KEYWORD)
#define VARIANT_HKF  (VARIANT_HK | VARIANT_FREQUENCY)
#define VARIANT_HKI  (VARIANT_HK | VARIANT_INTEGER)
#define VARIANT_HKL  (VARIANT_HK | VARIANT_LENGTH)
#define VARIANT_HKLP (VARIANT_HK | VARIANT_LP)
#define VARIANT_HKLPO (VARIANT_HKLP | VARIANT_NONE)
#define VARIANT_HL   (VARIANT_INHERIT | VARIANT_LENGTH)
#define VARIANT_HI   (VARIANT_INHERIT | VARIANT_INTEGER)
#define VARIANT_HLP  (VARIANT_HL | VARIANT_PERCENT)
#define VARIANT_HLPN (VARIANT_HLP | VARIANT_NUMBER)
#define VARIANT_HLPO (VARIANT_HLP | VARIANT_NONE)
#define VARIANT_HTP  (VARIANT_INHERIT | VARIANT_TIME | VARIANT_PERCENT)
#define VARIANT_HMK  (VARIANT_HK | VARIANT_NORMAL)
#define VARIANT_HC   (VARIANT_INHERIT | VARIANT_COLOR)
#define VARIANT_HCK  (VARIANT_HK | VARIANT_COLOR)
#define VARIANT_HUK  (VARIANT_HK | VARIANT_URL)
#define VARIANT_HUO  (VARIANT_INHERIT | VARIANT_URL | VARIANT_NONE)
#define VARIANT_AHUO (VARIANT_AUTO | VARIANT_HUO)
#define VARIANT_HPN  (VARIANT_INHERIT | VARIANT_PERCENT | VARIANT_NUMBER)
#define VARIANT_PN   (VARIANT_PERCENT | VARIANT_NUMBER)
#define VARIANT_ALPN (VARIANT_AL | VARIANT_PN)
#define VARIANT_HN   (VARIANT_INHERIT | VARIANT_NUMBER)
#define VARIANT_HON  (VARIANT_HN | VARIANT_NONE)
#define VARIANT_HOS  (VARIANT_INHERIT | VARIANT_NONE | VARIANT_STRING)
#define VARIANT_LPN  (VARIANT_LP | VARIANT_NUMBER)
#define VARIANT_UK   (VARIANT_URL | VARIANT_KEYWORD)
#define VARIANT_UO   (VARIANT_URL | VARIANT_NONE)
#define VARIANT_ANGLE_OR_ZERO (VARIANT_ANGLE | VARIANT_ZERO_ANGLE)
#define VARIANT_LCALC  (VARIANT_LENGTH | VARIANT_CALC)
#define VARIANT_LPCALC (VARIANT_LCALC | VARIANT_PERCENT)
#define VARIANT_LNCALC (VARIANT_LCALC | VARIANT_NUMBER)
#define VARIANT_LPNCALC (VARIANT_LNCALC | VARIANT_PERCENT)
#define VARIANT_IMAGE   (VARIANT_URL | VARIANT_NONE | VARIANT_GRADIENT | \
                        VARIANT_IMAGE_RECT | VARIANT_ELEMENT)




#define CSS_PROPERTY_LOGICAL                      (1<<0)

#define CSS_PROPERTY_VALUE_LIST_USES_COMMAS       (1<<1) /* otherwise spaces */

#define CSS_PROPERTY_APPLIES_TO_FIRST_LETTER      (1<<2)
#define CSS_PROPERTY_APPLIES_TO_FIRST_LINE        (1<<3)
#define CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE \
  (CSS_PROPERTY_APPLIES_TO_FIRST_LETTER | CSS_PROPERTY_APPLIES_TO_FIRST_LINE)



#define CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED (1<<4)






#define CSS_PROPERTY_START_IMAGE_LOADS            (1<<5)





#define CSS_PROPERTY_IMAGE_IS_IN_ARRAY_0          (1<<6)








#define CSS_PROPERTY_LOGICAL_AXIS                 (1<<7)





#define CSS_PROPERTY_STORES_CALC                  (1<<8)












#define CSS_PROPERTY_PARSE_PROPERTY_MASK          (7<<9)
#define CSS_PROPERTY_PARSE_INACCESSIBLE           (1<<9)
#define CSS_PROPERTY_PARSE_FUNCTION               (2<<9)
#define CSS_PROPERTY_PARSE_VALUE                  (3<<9)
#define CSS_PROPERTY_PARSE_VALUE_LIST             (4<<9)



#define CSS_PROPERTY_VALUE_PARSER_FUNCTION        (1<<12)
static_assert((CSS_PROPERTY_PARSE_PROPERTY_MASK &
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



#define CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH     (1<<20)


#define CSS_PROPERTY_CREATES_STACKING_CONTEXT     (1<<21)






#define CSS_PROPERTY_ALWAYS_ENABLED_IN_UA_SHEETS  (1<<22)







#define CSS_PROPERTY_ALWAYS_ENABLED_IN_CHROME_OR_CERTIFIED_APP (1<<23)


#define CSS_PROPERTY_NUMBERS_ARE_PIXELS           (1<<24)






#define CSS_PROPERTY_LOGICAL_BLOCK_AXIS           (1<<25)







#define CSS_PROPERTY_LOGICAL_END_EDGE             (1<<26)




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
  typedef int16_t KTableValue;

  static void AddRefTable(void);
  static void ReleaseTable(void);

  enum EnabledState {
    
    
    eEnabledForAllContent = 0,
    
    eEnabledInUASheets    = 0x01,
    
    eEnabledInChromeOrCertifiedApp = 0x02,
    
    
    
    
    eIgnoreEnabledState   = 0xff
  };

  
  
  
  static nsCSSProperty LookupProperty(const nsAString& aProperty,
                                      EnabledState aEnabled);
  static nsCSSProperty LookupProperty(const nsACString& aProperty,
                                      EnabledState aEnabled);
  
  
  static bool IsCustomPropertyName(const nsAString& aProperty);
  static bool IsCustomPropertyName(const nsACString& aProperty);

  static inline bool IsShorthand(nsCSSProperty aProperty) {
    MOZ_ASSERT(0 <= aProperty && aProperty < eCSSProperty_COUNT,
               "out of range");
    return (aProperty >= eCSSProperty_COUNT_no_shorthands);
  }

  
  static bool IsInherited(nsCSSProperty aProperty);

  
  static nsCSSFontDesc LookupFontDesc(const nsAString& aProperty);
  static nsCSSFontDesc LookupFontDesc(const nsACString& aProperty);

  
  static nsCSSCounterDesc LookupCounterDesc(const nsAString& aProperty);
  static nsCSSCounterDesc LookupCounterDesc(const nsACString& aProperty);

  
  static bool IsPredefinedCounterStyle(const nsAString& aStyle);
  static bool IsPredefinedCounterStyle(const nsACString& aStyle);

  
  static const nsAFlatCString& GetStringValue(nsCSSProperty aProperty);
  static const nsAFlatCString& GetStringValue(nsCSSFontDesc aFontDesc);
  static const nsAFlatCString& GetStringValue(nsCSSCounterDesc aCounterDesc);

  
  
  
  static const nsAFlatCString& LookupPropertyValue(nsCSSProperty aProperty, int32_t aValue);

  
  
  static bool GetColorName(int32_t aPropID, nsCString &aStr);

  
  
  
  static int32_t FindIndexOfKeyword(nsCSSKeyword aKeyword,
                                    const KTableValue aTable[]);

  
  
  static bool FindKeyword(nsCSSKeyword aKeyword, const KTableValue aTable[],
                          int32_t& aValue);
  
  
  static nsCSSKeyword ValueToKeywordEnum(int32_t aValue, 
                                         const KTableValue aTable[]);
  
  static const nsAFlatCString& ValueToKeyword(int32_t aValue,
                                              const KTableValue aTable[]);

  static const nsStyleStructID kSIDTable[eCSSProperty_COUNT_no_shorthands];
  static const KTableValue* const kKeywordTableTable[eCSSProperty_COUNT_no_shorthands];
  static const nsStyleAnimType kAnimTypeTable[eCSSProperty_COUNT_no_shorthands];
  static const ptrdiff_t
    kStyleStructOffsetTable[eCSSProperty_COUNT_no_shorthands];

private:
  static const uint32_t        kFlagsTable[eCSSProperty_COUNT];

public:
  static inline bool PropHasFlags(nsCSSProperty aProperty, uint32_t aFlags)
  {
    MOZ_ASSERT(0 <= aProperty && aProperty < eCSSProperty_COUNT,
               "out of range");
    MOZ_ASSERT(!(aFlags & CSS_PROPERTY_PARSE_PROPERTY_MASK),
               "The CSS_PROPERTY_PARSE_* values are not bitflags; don't pass "
               "them to PropHasFlags.  You probably want PropertyParseType "
               "instead.");
    return (nsCSSProps::kFlagsTable[aProperty] & aFlags) == aFlags;
  }

  static inline uint32_t PropertyParseType(nsCSSProperty aProperty)
  {
    MOZ_ASSERT(0 <= aProperty && aProperty < eCSSProperty_COUNT,
               "out of range");
    return nsCSSProps::kFlagsTable[aProperty] &
           CSS_PROPERTY_PARSE_PROPERTY_MASK;
  }

  static inline uint32_t ValueRestrictions(nsCSSProperty aProperty)
  {
    MOZ_ASSERT(0 <= aProperty && aProperty < eCSSProperty_COUNT,
               "out of range");
    return nsCSSProps::kFlagsTable[aProperty] &
           CSS_PROPERTY_VALUE_RESTRICTION_MASK;
  }

private:
  
  static const uint32_t kParserVariantTable[eCSSProperty_COUNT_no_shorthands];

public:
  static inline uint32_t ParserVariant(nsCSSProperty aProperty) {
    MOZ_ASSERT(0 <= aProperty && aProperty < eCSSProperty_COUNT_no_shorthands,
               "out of range");
    return nsCSSProps::kParserVariantTable[aProperty];
  }

private:
  
  
  static const nsCSSProperty *const
    kSubpropertyTable[eCSSProperty_COUNT - eCSSProperty_COUNT_no_shorthands];

public:
  static inline
  const nsCSSProperty * SubpropertyEntryFor(nsCSSProperty aProperty) {
    MOZ_ASSERT(eCSSProperty_COUNT_no_shorthands <= aProperty &&
               aProperty < eCSSProperty_COUNT,
               "out of range");
    return nsCSSProps::kSubpropertyTable[aProperty -
                                         eCSSProperty_COUNT_no_shorthands];
  }

  
  
  
  static const nsCSSProperty * ShorthandsContaining(nsCSSProperty aProperty) {
    MOZ_ASSERT(gShorthandsContainingPool, "uninitialized");
    MOZ_ASSERT(0 <= aProperty && aProperty < eCSSProperty_COUNT_no_shorthands,
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
    MOZ_ASSERT(0 <= aSID && aSID < nsStyleStructID_Length,
               "out of range");
    return gPropertyCountInStruct[aSID];
  }
  



  static size_t PropertyIndexInStruct(nsCSSProperty aProperty) {
    MOZ_ASSERT(0 <= aProperty && aProperty < eCSSProperty_COUNT_no_shorthands,
               "out of range");
    return gPropertyIndexInStruct[aProperty];
  }

private:
  
  
  static const nsCSSProperty* const
    kLogicalGroupTable[eCSSPropertyLogicalGroup_COUNT];

public:
  

















  static const nsCSSProperty* LogicalGroup(nsCSSProperty aProperty);

private:
  static bool gPropertyEnabled[eCSSProperty_COUNT_with_aliases];

public:

  static bool IsEnabled(nsCSSProperty aProperty) {
    MOZ_ASSERT(0 <= aProperty && aProperty < eCSSProperty_COUNT_with_aliases,
               "out of range");
    return gPropertyEnabled[aProperty];
  }

  static bool IsEnabled(nsCSSProperty aProperty, EnabledState aEnabled)
  {
    if (IsEnabled(aProperty)) {
      return true;
    }
    if (aEnabled == eIgnoreEnabledState) {
      return true;
    }
    if ((aEnabled & eEnabledInUASheets) &&
        PropHasFlags(aProperty, CSS_PROPERTY_ALWAYS_ENABLED_IN_UA_SHEETS))
    {
      return true;
    }
    if ((aEnabled & eEnabledInChromeOrCertifiedApp) &&
        PropHasFlags(aProperty, CSS_PROPERTY_ALWAYS_ENABLED_IN_CHROME_OR_CERTIFIED_APP))
    {
      return true;
    }
    return false;
  }

public:





#define CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(it_, prop_, enabledstate_)       \
  for (const nsCSSProperty *it_ = nsCSSProps::SubpropertyEntryFor(prop_),     \
                            es_ = (nsCSSProperty) (enabledstate_);            \
       *it_ != eCSSProperty_UNKNOWN; ++it_)                                   \
    if (nsCSSProps::IsEnabled(*it_, (nsCSSProps::EnabledState) es_))

  
  static const KTableValue kAnimationDirectionKTable[];
  static const KTableValue kAnimationFillModeKTable[];
  static const KTableValue kAnimationIterationCountKTable[];
  static const KTableValue kAnimationPlayStateKTable[];
  static const KTableValue kAnimationTimingFunctionKTable[];
  static const KTableValue kAppearanceKTable[];
  static const KTableValue kAzimuthKTable[];
  static const KTableValue kBackfaceVisibilityKTable[];
  static const KTableValue kTransformStyleKTable[];
  static const KTableValue kBackgroundAttachmentKTable[];
  static const KTableValue kBackgroundOriginKTable[];
  static const KTableValue kBackgroundPositionKTable[];
  static const KTableValue kBackgroundRepeatKTable[];
  static const KTableValue kBackgroundRepeatPartKTable[];
  static const KTableValue kBackgroundSizeKTable[];
  static const KTableValue kBlendModeKTable[];
  static const KTableValue kBorderCollapseKTable[];
  static const KTableValue kBorderColorKTable[];
  static const KTableValue kBorderImageRepeatKTable[];
  static const KTableValue kBorderImageSliceKTable[];
  static const KTableValue kBorderStyleKTable[];
  static const KTableValue kBorderWidthKTable[];
  static const KTableValue kBoxAlignKTable[];
  static const KTableValue kBoxDecorationBreakKTable[];
  static const KTableValue kBoxDirectionKTable[];
  static const KTableValue kBoxOrientKTable[];
  static const KTableValue kBoxPackKTable[];
  static const KTableValue kClipShapeSizingKTable[];
  static const KTableValue kCounterRangeKTable[];
  static const KTableValue kCounterSpeakAsKTable[];
  static const KTableValue kCounterSymbolsSystemKTable[];
  static const KTableValue kCounterSystemKTable[];
  static const KTableValue kDominantBaselineKTable[];
  static const KTableValue kShapeRadiusKTable[];
  static const KTableValue kFillRuleKTable[];
  static const KTableValue kFilterFunctionKTable[];
  static const KTableValue kImageRenderingKTable[];
  static const KTableValue kShapeRenderingKTable[];
  static const KTableValue kStrokeLinecapKTable[];
  static const KTableValue kStrokeLinejoinKTable[];
  static const KTableValue kStrokeContextValueKTable[];
  static const KTableValue kVectorEffectKTable[];
  static const KTableValue kTextAnchorKTable[];
  static const KTableValue kTextRenderingKTable[];
  static const KTableValue kColorInterpolationKTable[];
  static const KTableValue kColumnFillKTable[];
  static const KTableValue kBoxPropSourceKTable[];
  static const KTableValue kBoxShadowTypeKTable[];
  static const KTableValue kBoxSizingKTable[];
  static const KTableValue kCaptionSideKTable[];
  static const KTableValue kClearKTable[];
  static const KTableValue kColorKTable[];
  static const KTableValue kContentKTable[];
  static const KTableValue kControlCharacterVisibilityKTable[];
  static const KTableValue kCursorKTable[];
  static const KTableValue kDirectionKTable[];
  
  
  static KTableValue kDisplayKTable[];
  static const KTableValue kElevationKTable[];
  static const KTableValue kEmptyCellsKTable[];
  static const KTableValue kAlignContentKTable[];
  static const KTableValue kAlignItemsKTable[];
  static const KTableValue kAlignSelfKTable[];
  static const KTableValue kFlexDirectionKTable[];
  static const KTableValue kFlexWrapKTable[];
  static const KTableValue kJustifyContentKTable[];
  static const KTableValue kFloatKTable[];
  static const KTableValue kFloatEdgeKTable[];
  static const KTableValue kFontKTable[];
  static const KTableValue kFontKerningKTable[];
  static const KTableValue kFontSizeKTable[];
  static const KTableValue kFontSmoothingKTable[];
  static const KTableValue kFontStretchKTable[];
  static const KTableValue kFontStyleKTable[];
  static const KTableValue kFontSynthesisKTable[];
  static const KTableValue kFontVariantKTable[];
  static const KTableValue kFontVariantAlternatesKTable[];
  static const KTableValue kFontVariantAlternatesFuncsKTable[];
  static const KTableValue kFontVariantCapsKTable[];
  static const KTableValue kFontVariantEastAsianKTable[];
  static const KTableValue kFontVariantLigaturesKTable[];
  static const KTableValue kFontVariantNumericKTable[];
  static const KTableValue kFontVariantPositionKTable[];
  static const KTableValue kFontWeightKTable[];
  static const KTableValue kGridAutoFlowKTable[];
  static const KTableValue kGridTrackBreadthKTable[];
  static const KTableValue kHyphensKTable[];
  static const KTableValue kImageOrientationKTable[];
  static const KTableValue kImageOrientationFlipKTable[];
  static const KTableValue kIsolationKTable[];
  static const KTableValue kIMEModeKTable[];
  static const KTableValue kLineHeightKTable[];
  static const KTableValue kListStylePositionKTable[];
  static const KTableValue kListStyleKTable[];
  static const KTableValue kMaskTypeKTable[];
  static const KTableValue kMathVariantKTable[];
  static const KTableValue kMathDisplayKTable[];
  static const KTableValue kContextOpacityKTable[];
  static const KTableValue kContextPatternKTable[];
  static const KTableValue kObjectFitKTable[];
  static const KTableValue kOrientKTable[];
  static const KTableValue kOutlineStyleKTable[];
  static const KTableValue kOutlineColorKTable[];
  static const KTableValue kOverflowKTable[];
  static const KTableValue kOverflowSubKTable[];
  static const KTableValue kOverflowClipBoxKTable[];
  static const KTableValue kPageBreakKTable[];
  static const KTableValue kPageBreakInsideKTable[];
  static const KTableValue kPageMarksKTable[];
  static const KTableValue kPageSizeKTable[];
  static const KTableValue kPitchKTable[];
  static const KTableValue kPointerEventsKTable[];
  
  
  static KTableValue kPositionKTable[];
  static const KTableValue kRadialGradientShapeKTable[];
  static const KTableValue kRadialGradientSizeKTable[];
  static const KTableValue kRadialGradientLegacySizeKTable[];
  static const KTableValue kResizeKTable[];
  static const KTableValue kRubyAlignKTable[];
  static const KTableValue kRubyPositionKTable[];
  static const KTableValue kScrollBehaviorKTable[];
  static const KTableValue kScrollSnapTypeKTable[];
  static const KTableValue kSpeakKTable[];
  static const KTableValue kSpeakHeaderKTable[];
  static const KTableValue kSpeakNumeralKTable[];
  static const KTableValue kSpeakPunctuationKTable[];
  static const KTableValue kSpeechRateKTable[];
  static const KTableValue kStackSizingKTable[];
  static const KTableValue kTableLayoutKTable[];
  
  
  static KTableValue kTextAlignKTable[];
  static KTableValue kTextAlignLastKTable[];
  static const KTableValue kTextCombineUprightKTable[];
  static const KTableValue kTextDecorationLineKTable[];
  static const KTableValue kTextDecorationStyleKTable[];
  static const KTableValue kTextOrientationKTable[];
  static const KTableValue kTextOverflowKTable[];
  static const KTableValue kTextTransformKTable[];
  static const KTableValue kTouchActionKTable[];
  static const KTableValue kTransitionTimingFunctionKTable[];
  static const KTableValue kUnicodeBidiKTable[];
  static const KTableValue kUserFocusKTable[];
  static const KTableValue kUserInputKTable[];
  static const KTableValue kUserModifyKTable[];
  static const KTableValue kUserSelectKTable[];
  static const KTableValue kVerticalAlignKTable[];
  static const KTableValue kVisibilityKTable[];
  static const KTableValue kVolumeKTable[];
  static const KTableValue kWhitespaceKTable[];
  static const KTableValue kWidthKTable[]; 
  static const KTableValue kWindowDraggingKTable[];
  static const KTableValue kWindowShadowKTable[];
  static const KTableValue kWordBreakKTable[];
  static const KTableValue kWordWrapKTable[];
  static const KTableValue kWritingModeKTable[];
};

inline nsCSSProps::EnabledState operator|(nsCSSProps::EnabledState a,
                                          nsCSSProps::EnabledState b)
{
  return nsCSSProps::EnabledState(int(a) | int(b));
}

inline nsCSSProps::EnabledState operator&(nsCSSProps::EnabledState a,
                                          nsCSSProps::EnabledState b)
{
  return nsCSSProps::EnabledState(int(a) & int(b));
}

inline nsCSSProps::EnabledState& operator|=(nsCSSProps::EnabledState& a,
                                            nsCSSProps::EnabledState b)
{
  return a = a | b;
}

inline nsCSSProps::EnabledState& operator&=(nsCSSProps::EnabledState& a,
                                            nsCSSProps::EnabledState b)
{
  return a = a & b;
}

#endif 
