







#include "mozilla/DebugOnly.h"

#include "nsCSSParser.h"
#include "nsCSSProps.h"
#include "nsCSSKeywords.h"
#include "nsCSSScanner.h"
#include "mozilla/css/ErrorReporter.h"
#include "mozilla/css/Loader.h"
#include "mozilla/css/StyleRule.h"
#include "mozilla/css/ImportRule.h"
#include "nsCSSRules.h"
#include "mozilla/css/NameSpaceRule.h"
#include "nsTArray.h"
#include "nsCSSStyleSheet.h"
#include "mozilla/css/Declaration.h"
#include "nsStyleConsts.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIAtom.h"
#include "nsCOMArray.h"
#include "nsColor.h"
#include "nsCSSPseudoClasses.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSAnonBoxes.h"
#include "nsINameSpaceManager.h"
#include "nsXMLNameSpaceMap.h"
#include "nsThemeConstants.h"
#include "nsError.h"
#include "nsIMediaList.h"
#include "mozilla/LookAndFeel.h"
#include "nsStyleUtil.h"
#include "nsIPrincipal.h"
#include "prprf.h"
#include "math.h"
#include "nsContentUtils.h"
#include "nsAutoPtr.h"
#include "prlog.h"
#include "CSSCalc.h"
#include "nsMediaFeatures.h"
#include "nsLayoutUtils.h"

using namespace mozilla;


#define VARIANT_KEYWORD         0x000001  // K
#define VARIANT_LENGTH          0x000002  // L
#define VARIANT_PERCENT         0x000004  // P
#define VARIANT_COLOR           0x000008  // C eCSSUnit_Color, eCSSUnit_Ident (e.g.  "red")
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
#define VARIANT_INHERIT         0x020000  // H eCSSUnit_Initial, eCSSUnit_Inherit
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
#define VARIANT_LPCALC (VARIANT_LENGTH | VARIANT_CALC | VARIANT_PERCENT)
#define VARIANT_LNCALC (VARIANT_LENGTH | VARIANT_CALC | VARIANT_NUMBER)
#define VARIANT_LPNCALC (VARIANT_LNCALC | VARIANT_PERCENT)
#define VARIANT_IMAGE (VARIANT_URL | VARIANT_NONE | VARIANT_GRADIENT | \
                       VARIANT_IMAGE_RECT | VARIANT_ELEMENT)


const uint32_t
nsCSSProps::kParserVariantTable[eCSSProperty_COUNT_no_shorthands] = {
#define CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, \
                 stylestruct_, stylestructoffset_, animtype_)                 \
  parsevariant_,
#include "nsCSSPropList.h"
#undef CSS_PROP
};



namespace {


typedef void (* RuleAppendFunc) (css::Rule* aRule, void* aData);
static void AssignRuleToPointer(css::Rule* aRule, void* aPointer);
static void AppendRuleToSheet(css::Rule* aRule, void* aParser);




class CSSParserImpl {
public:
  CSSParserImpl();
  ~CSSParserImpl();

  nsresult SetStyleSheet(nsCSSStyleSheet* aSheet);

  nsresult SetQuirkMode(bool aQuirkMode);

  nsresult SetChildLoader(mozilla::css::Loader* aChildLoader);

  
  void Reset();

  nsresult ParseSheet(const nsAString& aInput,
                      nsIURI*          aSheetURI,
                      nsIURI*          aBaseURI,
                      nsIPrincipal*    aSheetPrincipal,
                      uint32_t         aLineNumber,
                      bool             aAllowUnsafeRules);

  nsresult ParseStyleAttribute(const nsAString&  aAttributeValue,
                               nsIURI*           aDocURL,
                               nsIURI*           aBaseURL,
                               nsIPrincipal*     aNodePrincipal,
                               css::StyleRule**  aResult);

  nsresult ParseDeclarations(const nsAString&  aBuffer,
                             nsIURI*           aSheetURL,
                             nsIURI*           aBaseURL,
                             nsIPrincipal*     aSheetPrincipal,
                             css::Declaration* aDeclaration,
                             bool*           aChanged);

  nsresult ParseRule(const nsAString&        aRule,
                     nsIURI*                 aSheetURL,
                     nsIURI*                 aBaseURL,
                     nsIPrincipal*           aSheetPrincipal,
                     css::Rule**             aResult);

  nsresult ParseProperty(const nsCSSProperty aPropID,
                         const nsAString& aPropValue,
                         nsIURI* aSheetURL,
                         nsIURI* aBaseURL,
                         nsIPrincipal* aSheetPrincipal,
                         css::Declaration* aDeclaration,
                         bool* aChanged,
                         bool aIsImportant,
                         bool aIsSVGMode);

  nsresult ParseMediaList(const nsSubstring& aBuffer,
                          nsIURI* aURL, 
                          uint32_t aLineNumber, 
                          nsMediaList* aMediaList,
                          bool aHTMLMode);

  bool ParseColorString(const nsSubstring& aBuffer,
                        nsIURI* aURL, 
                        uint32_t aLineNumber, 
                        nsCSSValue& aValue);

  nsresult ParseSelectorString(const nsSubstring& aSelectorString,
                               nsIURI* aURL, 
                               uint32_t aLineNumber, 
                               nsCSSSelectorList **aSelectorList);

  already_AddRefed<nsCSSKeyframeRule>
  ParseKeyframeRule(const nsSubstring& aBuffer,
                    nsIURI*            aURL,
                    uint32_t           aLineNumber);

  bool ParseKeyframeSelectorString(const nsSubstring& aSelectorString,
                                   nsIURI* aURL, 
                                   uint32_t aLineNumber, 
                                   InfallibleTArray<float>& aSelectorList);

  bool EvaluateSupportsDeclaration(const nsAString& aProperty,
                                   const nsAString& aValue,
                                   nsIURI* aDocURL,
                                   nsIURI* aBaseURL,
                                   nsIPrincipal* aDocPrincipal);

  bool EvaluateSupportsCondition(const nsAString& aCondition,
                                 nsIURI* aDocURL,
                                 nsIURI* aBaseURL,
                                 nsIPrincipal* aDocPrincipal);

protected:
  class nsAutoParseCompoundProperty;
  friend class nsAutoParseCompoundProperty;

  class nsAutoFailingSupportsRule;
  friend class nsAutoFailingSupportsRule;

  class nsAutoSuppressErrors;
  friend class nsAutoSuppressErrors;

  void AppendRule(css::Rule* aRule);
  friend void AppendRuleToSheet(css::Rule*, void*); 

  



  class nsAutoParseCompoundProperty {
    public:
      nsAutoParseCompoundProperty(CSSParserImpl* aParser) : mParser(aParser)
      {
        NS_ASSERTION(!aParser->IsParsingCompoundProperty(),
                     "already parsing compound property");
        NS_ASSERTION(aParser, "Null parser?");
        aParser->SetParsingCompoundProperty(true);
      }

      ~nsAutoParseCompoundProperty()
      {
        mParser->SetParsingCompoundProperty(false);
      }
    private:
      CSSParserImpl* mParser;
  };

  





  class nsAutoFailingSupportsRule {
    public:
      nsAutoFailingSupportsRule(CSSParserImpl* aParser,
                                bool aCondition)
        : mParser(aParser),
          mOriginalValue(aParser->mInFailingSupportsRule)
      {
        if (!aCondition) {
          mParser->mInFailingSupportsRule = true;
        }
      }

      ~nsAutoFailingSupportsRule()
      {
        mParser->mInFailingSupportsRule = mOriginalValue;
      }

    private:
      CSSParserImpl* mParser;
      bool mOriginalValue;
  };

  



  class nsAutoSuppressErrors {
    public:
      nsAutoSuppressErrors(CSSParserImpl* aParser,
                           bool aSuppressErrors = true)
        : mParser(aParser),
          mOriginalValue(aParser->mSuppressErrors)
      {
        mParser->mSuppressErrors = aSuppressErrors;
      }

      ~nsAutoSuppressErrors()
      {
        mParser->mSuppressErrors = mOriginalValue;
      }

    private:
      CSSParserImpl* mParser;
      bool mOriginalValue;
  };

  
  void InitScanner(nsCSSScanner& aScanner,
                   css::ErrorReporter& aReporter,
                   nsIURI* aSheetURI, nsIURI* aBaseURI,
                   nsIPrincipal* aSheetPrincipal);
  void ReleaseScanner(void);
  bool IsSVGMode() const {
    return mScanner->IsSVGMode();
  }

  bool GetToken(bool aSkipWS);
  void UngetToken();

  bool ExpectSymbol(PRUnichar aSymbol, bool aSkipWS);
  bool ExpectEndProperty();
  bool CheckEndProperty();
  nsSubstring* NextIdent();

  
  bool SkipUntil(PRUnichar aStopSymbol);
  void SkipUntilOneOf(const PRUnichar* aStopSymbolChars);

  void SkipRuleSet(bool aInsideBraces);
  bool SkipAtRule(bool aInsideBlock);
  bool SkipDeclaration(bool aCheckForBraces);

  void PushGroup(css::GroupRule* aRule);
  void PopGroup();

  bool ParseRuleSet(RuleAppendFunc aAppendFunc, void* aProcessData,
                    bool aInsideBraces = false);
  bool ParseAtRule(RuleAppendFunc aAppendFunc, void* aProcessData,
                   bool aInAtRule);
  bool ParseCharsetRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseImportRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseURLOrString(nsString& aURL);
  bool GatherMedia(nsMediaList* aMedia,
                   bool aInAtRule);
  bool ParseMediaQuery(bool aInAtRule, nsMediaQuery **aQuery,
                       bool *aHitStop);
  bool ParseMediaQueryExpression(nsMediaQuery* aQuery);
  void ProcessImport(const nsString& aURLSpec,
                     nsMediaList* aMedia,
                     RuleAppendFunc aAppendFunc,
                     void* aProcessData);
  bool ParseGroupRule(css::GroupRule* aRule, RuleAppendFunc aAppendFunc,
                      void* aProcessData);
  bool ParseMediaRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseMozDocumentRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseNameSpaceRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  void ProcessNameSpace(const nsString& aPrefix,
                        const nsString& aURLSpec, RuleAppendFunc aAppendFunc,
                        void* aProcessData);

  bool ParseFontFaceRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseFontDescriptor(nsCSSFontFaceRule* aRule);
  bool ParseFontDescriptorValue(nsCSSFontDesc aDescID,
                                nsCSSValue& aValue);

  bool ParsePageRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseKeyframesRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  already_AddRefed<nsCSSKeyframeRule> ParseKeyframeRule();
  bool ParseKeyframeSelectorList(InfallibleTArray<float>& aSelectorList);

  bool ParseSupportsRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseSupportsCondition(bool& aConditionMet);
  bool ParseSupportsConditionNegation(bool& aConditionMet);
  bool ParseSupportsConditionInParens(bool& aConditionMet);
  bool ParseSupportsConditionInParensInsideParens(bool& aConditionMet);
  bool ParseSupportsConditionTerms(bool& aConditionMet);
  enum SupportsConditionTermOperator { eAnd, eOr };
  bool ParseSupportsConditionTermsAfterOperator(
                                       bool& aConditionMet,
                                       SupportsConditionTermOperator aOperator);

  enum nsSelectorParsingStatus {
    
    
    eSelectorParsingStatus_Done,
    
    eSelectorParsingStatus_Continue,
    
    
    eSelectorParsingStatus_Error
  };
  nsSelectorParsingStatus ParseIDSelector(int32_t&       aDataMask,
                                          nsCSSSelector& aSelector);

  nsSelectorParsingStatus ParseClassSelector(int32_t&       aDataMask,
                                             nsCSSSelector& aSelector);

  
  
  
  
  nsSelectorParsingStatus ParsePseudoSelector(int32_t&       aDataMask,
                                              nsCSSSelector& aSelector,
                                              bool           aIsNegated,
                                              nsIAtom**      aPseudoElement,
                                              nsAtomList**   aPseudoElementArgs,
                                              nsCSSPseudoElements::Type* aPseudoElementType);

  nsSelectorParsingStatus ParseAttributeSelector(int32_t&       aDataMask,
                                                 nsCSSSelector& aSelector);

  nsSelectorParsingStatus ParseTypeOrUniversalSelector(int32_t&       aDataMask,
                                                       nsCSSSelector& aSelector,
                                                       bool           aIsNegated);

  nsSelectorParsingStatus ParsePseudoClassWithIdentArg(nsCSSSelector& aSelector,
                                                       nsCSSPseudoClasses::Type aType);

  nsSelectorParsingStatus ParsePseudoClassWithNthPairArg(nsCSSSelector& aSelector,
                                                         nsCSSPseudoClasses::Type aType);

  nsSelectorParsingStatus ParsePseudoClassWithSelectorListArg(nsCSSSelector& aSelector,
                                                              nsCSSPseudoClasses::Type aType);

  nsSelectorParsingStatus ParseNegatedSimpleSelector(int32_t&       aDataMask,
                                                     nsCSSSelector& aSelector);

  
  
  bool ParseSelectorList(nsCSSSelectorList*& aListHead,
                           PRUnichar aStopChar);
  bool ParseSelectorGroup(nsCSSSelectorList*& aListHead);
  bool ParseSelector(nsCSSSelectorList* aList, PRUnichar aPrevCombinator);

  enum {
    eParseDeclaration_InBraces       = 1 << 0,
    eParseDeclaration_AllowImportant = 1 << 1
  };
  enum nsCSSContextType {
    eCSSContext_General,
    eCSSContext_Page
  };

  css::Declaration* ParseDeclarationBlock(uint32_t aFlags,
                                          nsCSSContextType aContext = eCSSContext_General);
  bool ParseDeclaration(css::Declaration* aDeclaration,
                        uint32_t aFlags,
                        bool aMustCallValueAppended,
                        bool* aChanged,
                        nsCSSContextType aContext = eCSSContext_General);

  bool ParseProperty(nsCSSProperty aPropID);
  bool ParsePropertyByFunction(nsCSSProperty aPropID);
  bool ParseSingleValueProperty(nsCSSValue& aValue,
                                  nsCSSProperty aPropID);

  enum PriorityParsingStatus {
    ePriority_None,
    ePriority_Important,
    ePriority_Error
  };
  PriorityParsingStatus ParsePriority();

#ifdef MOZ_XUL
  bool ParseTreePseudoElement(nsAtomList **aPseudoElementArgs);
#endif

  void InitBoxPropsAsPhysical(const nsCSSProperty *aSourceProperties);

  
  bool ParseBackground();

  struct BackgroundParseState {
    nsCSSValue&  mColor;
    nsCSSValueList* mImage;
    nsCSSValuePairList* mRepeat;
    nsCSSValueList* mAttachment;
    nsCSSValueList* mClip;
    nsCSSValueList* mOrigin;
    nsCSSValueList* mPosition;
    nsCSSValuePairList* mSize;
    BackgroundParseState(
        nsCSSValue& aColor, nsCSSValueList* aImage, nsCSSValuePairList* aRepeat,
        nsCSSValueList* aAttachment, nsCSSValueList* aClip,
        nsCSSValueList* aOrigin, nsCSSValueList* aPosition,
        nsCSSValuePairList* aSize) :
        mColor(aColor), mImage(aImage), mRepeat(aRepeat),
        mAttachment(aAttachment), mClip(aClip), mOrigin(aOrigin),
        mPosition(aPosition), mSize(aSize) {};
  };

  bool ParseBackgroundItem(BackgroundParseState& aState);

  bool ParseValueList(nsCSSProperty aPropID); 
  bool ParseBackgroundRepeat();
  bool ParseBackgroundRepeatValues(nsCSSValuePair& aValue);
  bool ParseBackgroundPosition();

  
  
  
  bool ParseBoxPositionValues(nsCSSValuePair& aOut, bool aAcceptsInherit,
                              bool aAllowExplicitCenter = true); 
  bool ParseBackgroundPositionValues(nsCSSValue& aOut, bool aAcceptsInherit);

  bool ParseBackgroundSize();
  bool ParseBackgroundSizeValues(nsCSSValuePair& aOut);
  bool ParseBorderColor();
  bool ParseBorderColors(nsCSSProperty aProperty);
  void SetBorderImageInitialValues();
  bool ParseBorderImageRepeat(bool aAcceptsInherit);
  
  
  
  
  bool ParseBorderImageSlice(bool aAcceptsInherit, bool* aConsumedTokens);
  bool ParseBorderImageWidth(bool aAcceptsInherit);
  bool ParseBorderImageOutset(bool aAcceptsInherit);
  bool ParseBorderImage();
  bool ParseBorderSpacing();
  bool ParseBorderSide(const nsCSSProperty aPropIDs[],
                         bool aSetAllSides);
  bool ParseDirectionalBorderSide(const nsCSSProperty aPropIDs[],
                                    int32_t aSourceType);
  bool ParseBorderStyle();
  bool ParseBorderWidth();

  bool ParseCalc(nsCSSValue &aValue, int32_t aVariantMask);
  bool ParseCalcAdditiveExpression(nsCSSValue& aValue,
                                     int32_t& aVariantMask);
  bool ParseCalcMultiplicativeExpression(nsCSSValue& aValue,
                                           int32_t& aVariantMask,
                                           bool *aHadFinalWS);
  bool ParseCalcTerm(nsCSSValue& aValue, int32_t& aVariantMask);
  bool RequireWhitespace();

#ifdef MOZ_FLEXBOX
  
  bool ParseFlex();
#endif

  
  bool ParseRect(nsCSSProperty aPropID);
  bool ParseColumns();
  bool ParseContent();
  bool ParseCounterData(nsCSSProperty aPropID);
  bool ParseCursor();
  bool ParseFont();
  bool ParseFontWeight(nsCSSValue& aValue);
  bool ParseOneFamily(nsAString& aFamily, bool& aOneKeyword);
  bool ParseFamily(nsCSSValue& aValue);
  bool ParseFontFeatureSettings(nsCSSValue& aValue);
  bool ParseFontSrc(nsCSSValue& aValue);
  bool ParseFontSrcFormat(InfallibleTArray<nsCSSValue>& values);
  bool ParseFontRanges(nsCSSValue& aValue);
  bool ParseListStyle();
  bool ParseMargin();
  bool ParseMarks(nsCSSValue& aValue);
  bool ParseTransform(bool aIsPrefixed);
  bool ParseOutline();
  bool ParseOverflow();
  bool ParsePadding();
  bool ParseQuotes();
  bool ParseSize();
  bool ParseTextDecoration();
  bool ParseTextDecorationLine(nsCSSValue& aValue);
  bool ParseTextOverflow(nsCSSValue& aValue);

  bool ParseShadowItem(nsCSSValue& aValue, bool aIsBoxShadow);
  bool ParseShadowList(nsCSSProperty aProperty);
  bool ParseTransitionProperty();
  bool ParseTransitionTimingFunctionValues(nsCSSValue& aValue);
  bool ParseTransitionTimingFunctionValueComponent(float& aComponent,
                                                     char aStop,
                                                     bool aCheckRange);
  bool ParseTransitionStepTimingFunctionValues(nsCSSValue& aValue);
  enum ParseAnimationOrTransitionShorthandResult {
    eParseAnimationOrTransitionShorthand_Values,
    eParseAnimationOrTransitionShorthand_Inherit,
    eParseAnimationOrTransitionShorthand_Error
  };
  ParseAnimationOrTransitionShorthandResult
    ParseAnimationOrTransitionShorthand(const nsCSSProperty* aProperties,
                                        const nsCSSValue* aInitialValues,
                                        nsCSSValue* aValues,
                                        size_t aNumProperties);
  bool ParseTransition();
  bool ParseAnimation();

  bool ParsePaint(nsCSSProperty aPropID);
  bool ParseDasharray();
  bool ParseMarker();
  bool ParsePaintOrder();

  
  void AppendValue(nsCSSProperty aPropID, const nsCSSValue& aValue);
  bool ParseBoxProperties(const nsCSSProperty aPropIDs[]);
  bool ParseGroupedBoxProperty(int32_t aVariantMask,
                               nsCSSValue& aValue);
  bool ParseDirectionalBoxProperty(nsCSSProperty aProperty,
                                     int32_t aSourceType);
  bool ParseBoxCornerRadius(const nsCSSProperty aPropID);
  bool ParseBoxCornerRadii(const nsCSSProperty aPropIDs[]);
  int32_t ParseChoice(nsCSSValue aValues[],
                      const nsCSSProperty aPropIDs[], int32_t aNumIDs);
  bool ParseColor(nsCSSValue& aValue);
  bool ParseColorComponent(uint8_t& aComponent,
                             int32_t& aType, char aStop);
  
  
  bool ParseHSLColor(nscolor& aColor, char aStop);
  
  
  bool ParseColorOpacity(uint8_t& aOpacity);
  bool ParseEnum(nsCSSValue& aValue, const int32_t aKeywordTable[]);
  bool ParseVariant(nsCSSValue& aValue,
                      int32_t aVariantMask,
                      const int32_t aKeywordTable[]);
  bool ParseNonNegativeVariant(nsCSSValue& aValue,
                                 int32_t aVariantMask,
                                 const int32_t aKeywordTable[]);
  bool ParseOneOrLargerVariant(nsCSSValue& aValue,
                                 int32_t aVariantMask,
                                 const int32_t aKeywordTable[]);
  bool ParseCounter(nsCSSValue& aValue);
  bool ParseAttr(nsCSSValue& aValue);
  bool SetValueToURL(nsCSSValue& aValue, const nsString& aURL);
  bool TranslateDimension(nsCSSValue& aValue, int32_t aVariantMask,
                            float aNumber, const nsString& aUnit);
  bool ParseImageRect(nsCSSValue& aImage);
  bool ParseElement(nsCSSValue& aValue);
  bool ParseColorStop(nsCSSValueGradient* aGradient);
  bool ParseLinearGradient(nsCSSValue& aValue, bool aIsRepeating,
                           bool aIsLegacy);
  bool ParseRadialGradient(nsCSSValue& aValue, bool aIsRepeating,
                           bool aIsLegacy);
  bool IsLegacyGradientLine(const nsCSSTokenType& aType,
                            const nsString& aId);
  bool ParseGradientColorStops(nsCSSValueGradient* aGradient,
                               nsCSSValue& aValue);

  void SetParsingCompoundProperty(bool aBool) {
    mParsingCompoundProperty = aBool;
  }
  bool IsParsingCompoundProperty(void) const {
    return mParsingCompoundProperty;
  }

  
  bool ParseSingleTransform(bool aIsPrefixed, nsCSSValue& aValue, bool& aIs3D);
  bool ParseFunction(const nsString &aFunction, const int32_t aAllowedTypes[],
                       uint16_t aMinElems, uint16_t aMaxElems,
                       nsCSSValue &aValue);
  bool ParseFunctionInternals(const int32_t aVariantMask[],
                                uint16_t aMinElems,
                                uint16_t aMaxElems,
                                InfallibleTArray<nsCSSValue>& aOutput);

  
  bool ParseTransformOrigin(bool aPerspective);

  


  int32_t GetNamespaceIdForPrefix(const nsString& aPrefix);

  
  void SetDefaultNamespaceOnSelector(nsCSSSelector& aSelector);

  
  
  nsCSSToken mToken;

  
  nsCSSScanner* mScanner;

  
  css::ErrorReporter* mReporter;

  
  nsCOMPtr<nsIURI> mBaseURI;

  
  nsCOMPtr<nsIURI> mSheetURI;

  
  nsCOMPtr<nsIPrincipal> mSheetPrincipal;

  
  nsRefPtr<nsCSSStyleSheet> mSheet;

  
  mozilla::css::Loader* mChildLoader; 

  
  
  
  
  enum nsCSSSection {
    eCSSSection_Charset,
    eCSSSection_Import,
    eCSSSection_NameSpace,
    eCSSSection_General
  };
  nsCSSSection  mSection;

  nsXMLNameSpaceMap *mNameSpaceMap;  

  
  
  bool mHavePushBack : 1;

  
  bool          mNavQuirkMode : 1;

  
  bool mHashlessColorQuirk : 1;

  
  bool mUnitlessLengthQuirk : 1;

  
  bool mUnsafeRulesEnabled : 1;

  
  bool mViewportUnitsEnabled : 1;

  
  
  bool mHTMLMediaMode : 1;

  
  
  bool          mParsingCompoundProperty : 1;

  
  
  bool mInFailingSupportsRule : 1;

  
  
  
  bool mSuppressErrors : 1;

  
  InfallibleTArray<nsRefPtr<css::GroupRule> > mGroupStack;

  
  
  
  
  
  nsCSSExpandedDataBlock mTempData;

  
  nsCSSExpandedDataBlock mData;

public:
  
  CSSParserImpl* mNextFree;
};

static void AssignRuleToPointer(css::Rule* aRule, void* aPointer)
{
  css::Rule **pointer = static_cast<css::Rule**>(aPointer);
  NS_ADDREF(*pointer = aRule);
}

static void AppendRuleToSheet(css::Rule* aRule, void* aParser)
{
  CSSParserImpl* parser = (CSSParserImpl*) aParser;
  parser->AppendRule(aRule);
}

#define REPORT_UNEXPECTED(msg_) \
  { if (!mSuppressErrors) mReporter->ReportUnexpected(#msg_); }

#define REPORT_UNEXPECTED_P(msg_, param_) \
  { if (!mSuppressErrors) mReporter->ReportUnexpected(#msg_, param_); }

#define REPORT_UNEXPECTED_TOKEN(msg_) \
  { if (!mSuppressErrors) mReporter->ReportUnexpected(#msg_, mToken); }

#define REPORT_UNEXPECTED_TOKEN_CHAR(msg_, ch_) \
  { if (!mSuppressErrors) mReporter->ReportUnexpected(#msg_, mToken, ch_); }

#define REPORT_UNEXPECTED_EOF(lf_) \
  mReporter->ReportUnexpectedEOF(#lf_)

#define REPORT_UNEXPECTED_EOF_CHAR(ch_) \
  mReporter->ReportUnexpectedEOF(ch_)

#define OUTPUT_ERROR() \
  mReporter->OutputError()

#define CLEAR_ERROR() \
  mReporter->ClearError()

CSSParserImpl::CSSParserImpl()
  : mToken(),
    mScanner(nullptr),
    mReporter(nullptr),
    mChildLoader(nullptr),
    mSection(eCSSSection_Charset),
    mNameSpaceMap(nullptr),
    mHavePushBack(false),
    mNavQuirkMode(false),
    mHashlessColorQuirk(false),
    mUnitlessLengthQuirk(false),
    mUnsafeRulesEnabled(false),
    mViewportUnitsEnabled(true),
    mHTMLMediaMode(false),
    mParsingCompoundProperty(false),
    mInFailingSupportsRule(false),
    mSuppressErrors(false),
    mNextFree(nullptr)
{
}

CSSParserImpl::~CSSParserImpl()
{
  mData.AssertInitialState();
  mTempData.AssertInitialState();
}

nsresult
CSSParserImpl::SetStyleSheet(nsCSSStyleSheet* aSheet)
{
  if (aSheet != mSheet) {
    
    mGroupStack.Clear();
    mSheet = aSheet;
    if (mSheet) {
      mNameSpaceMap = mSheet->GetNameSpaceMap();
    } else {
      mNameSpaceMap = nullptr;
    }
  } else if (mSheet) {
    mNameSpaceMap = mSheet->GetNameSpaceMap();
  }

  return NS_OK;
}

nsresult
CSSParserImpl::SetQuirkMode(bool aQuirkMode)
{
  mNavQuirkMode = aQuirkMode;
  return NS_OK;
}

nsresult
CSSParserImpl::SetChildLoader(mozilla::css::Loader* aChildLoader)
{
  mChildLoader = aChildLoader;  
  return NS_OK;
}

void
CSSParserImpl::Reset()
{
  NS_ASSERTION(!mScanner, "resetting with scanner active");
  SetStyleSheet(nullptr);
  SetQuirkMode(false);
  SetChildLoader(nullptr);
}

void
CSSParserImpl::InitScanner(nsCSSScanner& aScanner,
                           css::ErrorReporter& aReporter,
                           nsIURI* aSheetURI, nsIURI* aBaseURI,
                           nsIPrincipal* aSheetPrincipal)
{
  NS_PRECONDITION(!mHTMLMediaMode, "Bad initial state");
  NS_PRECONDITION(!mParsingCompoundProperty, "Bad initial state");
  NS_PRECONDITION(!mScanner, "already have scanner");

  mScanner = &aScanner;
  mReporter = &aReporter;
  mScanner->SetErrorReporter(mReporter);

  mBaseURI = aBaseURI;
  mSheetURI = aSheetURI;
  mSheetPrincipal = aSheetPrincipal;
  mHavePushBack = false;
}

void
CSSParserImpl::ReleaseScanner()
{
  mScanner = nullptr;
  mReporter = nullptr;
  mBaseURI = nullptr;
  mSheetURI = nullptr;
  mSheetPrincipal = nullptr;
}

nsresult
CSSParserImpl::ParseSheet(const nsAString& aInput,
                          nsIURI*          aSheetURI,
                          nsIURI*          aBaseURI,
                          nsIPrincipal*    aSheetPrincipal,
                          uint32_t         aLineNumber,
                          bool             aAllowUnsafeRules)
{
  NS_PRECONDITION(aSheetPrincipal, "Must have principal here!");
  NS_PRECONDITION(aBaseURI, "need base URI");
  NS_PRECONDITION(aSheetURI, "need sheet URI");
  NS_PRECONDITION(mSheet, "Must have sheet to parse into");
  NS_ENSURE_STATE(mSheet);

#ifdef DEBUG
  nsIURI* uri = mSheet->GetSheetURI();
  bool equal;
  NS_ASSERTION(NS_SUCCEEDED(aSheetURI->Equals(uri, &equal)) && equal,
               "Sheet URI does not match passed URI");
  NS_ASSERTION(NS_SUCCEEDED(mSheet->Principal()->Equals(aSheetPrincipal,
                                                        &equal)) &&
               equal,
               "Sheet principal does not match passed principal");
#endif

  nsCSSScanner scanner(aInput, aLineNumber);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aSheetURI);
  InitScanner(scanner, reporter, aSheetURI, aBaseURI, aSheetPrincipal);

  int32_t ruleCount = mSheet->StyleRuleCount();
  if (0 < ruleCount) {
    css::Rule* lastRule = nullptr;
    mSheet->GetStyleRuleAt(ruleCount - 1, lastRule);
    if (lastRule) {
      switch (lastRule->GetType()) {
        case css::Rule::CHARSET_RULE:
        case css::Rule::IMPORT_RULE:
          mSection = eCSSSection_Import;
          break;
        case css::Rule::NAMESPACE_RULE:
          mSection = eCSSSection_NameSpace;
          break;
        default:
          mSection = eCSSSection_General;
          break;
      }
      NS_RELEASE(lastRule);
    }
  }
  else {
    mSection = eCSSSection_Charset; 
  }

  mUnsafeRulesEnabled = aAllowUnsafeRules;

  nsCSSToken* tk = &mToken;
  for (;;) {
    
    if (!GetToken(true)) {
      OUTPUT_ERROR();
      break;
    }
    if (eCSSToken_HTMLComment == tk->mType) {
      continue; 
    }
    if (eCSSToken_AtKeyword == tk->mType) {
      ParseAtRule(AppendRuleToSheet, this, false);
      continue;
    }
    UngetToken();
    if (ParseRuleSet(AppendRuleToSheet, this)) {
      mSection = eCSSSection_General;
    }
  }
  ReleaseScanner();

  mUnsafeRulesEnabled = false;

  
  return NS_OK;
}





static bool
NonMozillaVendorIdentifier(const nsAString& ident)
{
  return (ident.First() == PRUnichar('-') &&
          !StringBeginsWith(ident, NS_LITERAL_STRING("-moz-"))) ||
         ident.First() == PRUnichar('_');

}

nsresult
CSSParserImpl::ParseStyleAttribute(const nsAString& aAttributeValue,
                                   nsIURI*          aDocURI,
                                   nsIURI*          aBaseURI,
                                   nsIPrincipal*    aNodePrincipal,
                                   css::StyleRule** aResult)
{
  NS_PRECONDITION(aNodePrincipal, "Must have principal here!");
  NS_PRECONDITION(aBaseURI, "need base URI");

  
  nsCSSScanner scanner(aAttributeValue, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aDocURI);
  InitScanner(scanner, reporter, aDocURI, aBaseURI, aNodePrincipal);

  mSection = eCSSSection_General;

  
  
  bool haveBraces;
  if (mNavQuirkMode && GetToken(true)) {
    haveBraces = eCSSToken_Symbol == mToken.mType &&
                 '{' == mToken.mSymbol;
    UngetToken();
  }
  else {
    haveBraces = false;
  }

  uint32_t parseFlags = eParseDeclaration_AllowImportant;
  if (haveBraces) {
    parseFlags |= eParseDeclaration_InBraces;
  }

  css::Declaration* declaration = ParseDeclarationBlock(parseFlags);
  if (declaration) {
    
    NS_ADDREF(*aResult = new css::StyleRule(nullptr, declaration));
  } else {
    *aResult = nullptr;
  }

  ReleaseScanner();

  
  return NS_OK;
}

nsresult
CSSParserImpl::ParseDeclarations(const nsAString&  aBuffer,
                                 nsIURI*           aSheetURI,
                                 nsIURI*           aBaseURI,
                                 nsIPrincipal*     aSheetPrincipal,
                                 css::Declaration* aDeclaration,
                                 bool*           aChanged)
{
  *aChanged = false;

  NS_PRECONDITION(aSheetPrincipal, "Must have principal here!");

  nsCSSScanner scanner(aBuffer, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aSheetURI);
  InitScanner(scanner, reporter, aSheetURI, aBaseURI, aSheetPrincipal);

  mSection = eCSSSection_General;

  mData.AssertInitialState();
  aDeclaration->ClearData();
  
  *aChanged = true;

  for (;;) {
    
    
    if (!ParseDeclaration(aDeclaration, eParseDeclaration_AllowImportant,
                          true, aChanged)) {
      if (!SkipDeclaration(false)) {
        break;
      }
    }
  }

  aDeclaration->CompressFrom(&mData);
  ReleaseScanner();
  return NS_OK;
}

nsresult
CSSParserImpl::ParseRule(const nsAString&        aRule,
                         nsIURI*                 aSheetURI,
                         nsIURI*                 aBaseURI,
                         nsIPrincipal*           aSheetPrincipal,
                         css::Rule**             aResult)
{
  NS_PRECONDITION(aSheetPrincipal, "Must have principal here!");
  NS_PRECONDITION(aBaseURI, "need base URI");

  *aResult = nullptr;

  nsCSSScanner scanner(aRule, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aSheetURI);
  InitScanner(scanner, reporter, aSheetURI, aBaseURI, aSheetPrincipal);

  mSection = eCSSSection_Charset; 

  nsCSSToken* tk = &mToken;
  
  nsresult rv = NS_OK;
  if (!GetToken(true)) {
    REPORT_UNEXPECTED(PEParseRuleWSOnly);
    OUTPUT_ERROR();
    rv = NS_ERROR_DOM_SYNTAX_ERR;
  } else {
    if (eCSSToken_AtKeyword == tk->mType) {
      
      ParseAtRule(AssignRuleToPointer, aResult, false);
    } else {
      UngetToken();
      ParseRuleSet(AssignRuleToPointer, aResult);
    }

    if (*aResult && GetToken(true)) {
      
      REPORT_UNEXPECTED_TOKEN(PERuleTrailing);
      NS_RELEASE(*aResult);
    }

    if (!*aResult) {
      rv = NS_ERROR_DOM_SYNTAX_ERR;
      OUTPUT_ERROR();
    }
  }

  ReleaseScanner();
  return rv;
}


#ifdef _MSC_VER
#pragma optimize( "", off )
#endif
nsresult
CSSParserImpl::ParseProperty(const nsCSSProperty aPropID,
                             const nsAString& aPropValue,
                             nsIURI* aSheetURI,
                             nsIURI* aBaseURI,
                             nsIPrincipal* aSheetPrincipal,
                             css::Declaration* aDeclaration,
                             bool* aChanged,
                             bool aIsImportant,
                             bool aIsSVGMode)
{
  NS_PRECONDITION(aSheetPrincipal, "Must have principal here!");
  NS_PRECONDITION(aBaseURI, "need base URI");
  NS_PRECONDITION(aDeclaration, "Need declaration to parse into!");

  mData.AssertInitialState();
  mTempData.AssertInitialState();
  aDeclaration->AssertMutable();

  nsCSSScanner scanner(aPropValue, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aSheetURI);
  InitScanner(scanner, reporter, aSheetURI, aBaseURI, aSheetPrincipal);
  mSection = eCSSSection_General;
  scanner.SetSVGMode(aIsSVGMode);

  *aChanged = false;

  
  if (eCSSProperty_UNKNOWN == aPropID || !nsCSSProps::IsEnabled(aPropID)) {
    NS_ConvertASCIItoUTF16 propName(nsCSSProps::GetStringValue(aPropID));
    REPORT_UNEXPECTED_P(PEUnknownProperty, propName);
    REPORT_UNEXPECTED(PEDeclDropped);
    OUTPUT_ERROR();
    ReleaseScanner();
    return NS_OK;
  }

  bool parsedOK = ParseProperty(aPropID);
  
  if (parsedOK && GetToken(true)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectEndValue);
    parsedOK = false;
  }

  if (!parsedOK) {
    NS_ConvertASCIItoUTF16 propName(nsCSSProps::GetStringValue(aPropID));
    REPORT_UNEXPECTED_P(PEValueParsingError, propName);
    REPORT_UNEXPECTED(PEDeclDropped);
    OUTPUT_ERROR();
    mTempData.ClearProperty(aPropID);
  } else {

    
    
    
    
    
    
    if (!aDeclaration->TryReplaceValue(aPropID, aIsImportant, mTempData,
                                       aChanged)) {
      
      aDeclaration->ExpandTo(&mData);
      *aChanged = mData.TransferFromBlock(mTempData, aPropID, aIsImportant,
                                          true, false, aDeclaration);
      aDeclaration->CompressFrom(&mData);
    }
    CLEAR_ERROR();
  }

  mTempData.AssertInitialState();

  ReleaseScanner();
  return NS_OK;
}
#ifdef _MSC_VER
#pragma optimize( "", on )
#endif

nsresult
CSSParserImpl::ParseMediaList(const nsSubstring& aBuffer,
                              nsIURI* aURI, 
                              uint32_t aLineNumber, 
                              nsMediaList* aMediaList,
                              bool aHTMLMode)
{
  
  
  
  aMediaList->Clear();

  
  nsCSSScanner scanner(aBuffer, aLineNumber);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aURI);
  InitScanner(scanner, reporter, aURI, aURI, nullptr);

  mHTMLMediaMode = aHTMLMode;

    
    

  
  
  
  
  
  
  
  

  DebugOnly<bool> parsedOK = GatherMedia(aMediaList, false);
  NS_ASSERTION(parsedOK, "GatherMedia returned false; we probably want to avoid "
                         "trashing aMediaList");

  CLEAR_ERROR();
  ReleaseScanner();
  mHTMLMediaMode = false;

  return NS_OK;
}

bool
CSSParserImpl::ParseColorString(const nsSubstring& aBuffer,
                                nsIURI* aURI, 
                                uint32_t aLineNumber, 
                                nsCSSValue& aValue)
{
  nsCSSScanner scanner(aBuffer, aLineNumber);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aURI);
  InitScanner(scanner, reporter, aURI, aURI, nullptr);

  
  bool colorParsed = ParseColor(aValue) && !GetToken(true);
  OUTPUT_ERROR();
  ReleaseScanner();
  return colorParsed;
}

nsresult
CSSParserImpl::ParseSelectorString(const nsSubstring& aSelectorString,
                                   nsIURI* aURI, 
                                   uint32_t aLineNumber, 
                                   nsCSSSelectorList **aSelectorList)
{
  nsCSSScanner scanner(aSelectorString, aLineNumber);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aURI);
  InitScanner(scanner, reporter, aURI, aURI, nullptr);

  bool success = ParseSelectorList(*aSelectorList, PRUnichar(0));

  
  
  
  
  
  
  
  
  CLEAR_ERROR();
  ReleaseScanner();

  if (success) {
    NS_ASSERTION(*aSelectorList, "Should have list!");
    return NS_OK;
  }

  NS_ASSERTION(!*aSelectorList, "Shouldn't have list!");

  return NS_ERROR_DOM_SYNTAX_ERR;
}


already_AddRefed<nsCSSKeyframeRule>
CSSParserImpl::ParseKeyframeRule(const nsSubstring&  aBuffer,
                                 nsIURI*             aURI,
                                 uint32_t            aLineNumber)
{
  nsCSSScanner scanner(aBuffer, aLineNumber);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aURI);
  InitScanner(scanner, reporter, aURI, aURI, nullptr);

  nsRefPtr<nsCSSKeyframeRule> result = ParseKeyframeRule();
  if (GetToken(true)) {
    
    result = nullptr;
  }

  OUTPUT_ERROR();
  ReleaseScanner();

  return result.forget();
}

bool
CSSParserImpl::ParseKeyframeSelectorString(const nsSubstring& aSelectorString,
                                           nsIURI* aURI, 
                                           uint32_t aLineNumber, 
                                           InfallibleTArray<float>& aSelectorList)
{
  NS_ABORT_IF_FALSE(aSelectorList.IsEmpty(), "given list should start empty");

  nsCSSScanner scanner(aSelectorString, aLineNumber);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aURI);
  InitScanner(scanner, reporter, aURI, aURI, nullptr);

  bool success = ParseKeyframeSelectorList(aSelectorList) &&
                 
                 !GetToken(true);

  OUTPUT_ERROR();
  ReleaseScanner();

  if (success) {
    NS_ASSERTION(!aSelectorList.IsEmpty(), "should not be empty");
  } else {
    aSelectorList.Clear();
  }

  return success;
}

bool
CSSParserImpl::EvaluateSupportsDeclaration(const nsAString& aProperty,
                                           const nsAString& aValue,
                                           nsIURI* aDocURL,
                                           nsIURI* aBaseURL,
                                           nsIPrincipal* aDocPrincipal)
{
  nsCSSProperty propID = nsCSSProps::LookupProperty(aProperty,
                                                    nsCSSProps::eEnabled);
  if (propID == eCSSProperty_UNKNOWN) {
    return false;
  }

  nsCSSScanner scanner(aValue, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aDocURL);
  InitScanner(scanner, reporter, aDocURL, aBaseURL, aDocPrincipal);
  nsAutoSuppressErrors suppressErrors(this);

  bool parsedOK = ParseProperty(propID) && !GetToken(true);

  CLEAR_ERROR();
  ReleaseScanner();

  mTempData.ClearProperty(propID);
  mTempData.AssertInitialState();

  return parsedOK;
}

bool
CSSParserImpl::EvaluateSupportsCondition(const nsAString& aDeclaration,
                                         nsIURI* aDocURL,
                                         nsIURI* aBaseURL,
                                         nsIPrincipal* aDocPrincipal)
{
  nsCSSScanner scanner(aDeclaration, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aDocURL);
  InitScanner(scanner, reporter, aDocURL, aBaseURL, aDocPrincipal);
  nsAutoSuppressErrors suppressErrors(this);

  bool conditionMet;
  bool parsedOK = ParseSupportsCondition(conditionMet) && !GetToken(true);

  CLEAR_ERROR();
  ReleaseScanner();

  return parsedOK && conditionMet;
}



bool
CSSParserImpl::GetToken(bool aSkipWS)
{
  if (mHavePushBack) {
    mHavePushBack = false;
    if (!aSkipWS || mToken.mType != eCSSToken_Whitespace) {
      return true;
    }
  }
  return mScanner->Next(mToken, aSkipWS);
}

void
CSSParserImpl::UngetToken()
{
  NS_PRECONDITION(!mHavePushBack, "double pushback");
  mHavePushBack = true;
}

bool
CSSParserImpl::ExpectSymbol(PRUnichar aSymbol,
                            bool aSkipWS)
{
  if (!GetToken(aSkipWS)) {
    
    
    
    
    if (aSymbol == ')' || aSymbol == ']' ||
        aSymbol == '}' || aSymbol == ';') {
      REPORT_UNEXPECTED_EOF_CHAR(aSymbol);
      return true;
    }
    else
      return false;
  }
  if (mToken.IsSymbol(aSymbol)) {
    return true;
  }
  UngetToken();
  return false;
}



bool
CSSParserImpl::CheckEndProperty()
{
  if (!GetToken(true)) {
    return true; 
  }
  if ((eCSSToken_Symbol == mToken.mType) &&
      ((';' == mToken.mSymbol) ||
       ('!' == mToken.mSymbol) ||
       ('}' == mToken.mSymbol) ||
       (')' == mToken.mSymbol))) {
    
    
    UngetToken();
    return true;
  }
  UngetToken();
  return false;
}


bool
CSSParserImpl::ExpectEndProperty()
{
  if (CheckEndProperty())
    return true;

  
  REPORT_UNEXPECTED_TOKEN(PEExpectEndValue);
  return false;
}



CSSParserImpl::PriorityParsingStatus
CSSParserImpl::ParsePriority()
{
  if (!GetToken(true)) {
    return ePriority_None; 
  }
  if (!mToken.IsSymbol('!')) {
    UngetToken();
    return ePriority_None; 
  }

  if (!GetToken(true)) {
    
    REPORT_UNEXPECTED_EOF(PEImportantEOF);
    return ePriority_Error;
  }

  if (mToken.mType != eCSSToken_Ident ||
      !mToken.mIdent.LowerCaseEqualsLiteral("important")) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedImportant);
    UngetToken();
    return ePriority_Error;
  }

  return ePriority_Important;
}

nsSubstring*
CSSParserImpl::NextIdent()
{
  
  if (!GetToken(true)) {
    return nullptr;
  }
  if (eCSSToken_Ident != mToken.mType) {
    UngetToken();
    return nullptr;
  }
  return &mToken.mIdent;
}

bool
CSSParserImpl::SkipAtRule(bool aInsideBlock)
{
  for (;;) {
    if (!GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PESkipAtRuleEOF2);
      return false;
    }
    if (eCSSToken_Symbol == mToken.mType) {
      PRUnichar symbol = mToken.mSymbol;
      if (symbol == ';') {
        break;
      }
      if (aInsideBlock && symbol == '}') {
        
        UngetToken();
        break;
      }
      if (symbol == '{') {
        SkipUntil('}');
        break;
      } else if (symbol == '(') {
        SkipUntil(')');
      } else if (symbol == '[') {
        SkipUntil(']');
      }
    } else if (eCSSToken_Function == mToken.mType ||
               eCSSToken_Bad_URL == mToken.mType) {
      SkipUntil(')');
    }
  }
  return true;
}

bool
CSSParserImpl::ParseAtRule(RuleAppendFunc aAppendFunc,
                           void* aData,
                           bool aInAtRule)
{

  nsCSSSection newSection;
  bool (CSSParserImpl::*parseFunc)(RuleAppendFunc, void*);

  if ((mSection <= eCSSSection_Charset) &&
      (mToken.mIdent.LowerCaseEqualsLiteral("charset"))) {
    parseFunc = &CSSParserImpl::ParseCharsetRule;
    newSection = eCSSSection_Import;  

  } else if ((mSection <= eCSSSection_Import) &&
             mToken.mIdent.LowerCaseEqualsLiteral("import")) {
    parseFunc = &CSSParserImpl::ParseImportRule;
    newSection = eCSSSection_Import;

  } else if ((mSection <= eCSSSection_NameSpace) &&
             mToken.mIdent.LowerCaseEqualsLiteral("namespace")) {
    parseFunc = &CSSParserImpl::ParseNameSpaceRule;
    newSection = eCSSSection_NameSpace;

  } else if (mToken.mIdent.LowerCaseEqualsLiteral("media")) {
    parseFunc = &CSSParserImpl::ParseMediaRule;
    newSection = eCSSSection_General;

  } else if (mToken.mIdent.LowerCaseEqualsLiteral("-moz-document")) {
    parseFunc = &CSSParserImpl::ParseMozDocumentRule;
    newSection = eCSSSection_General;

  } else if (mToken.mIdent.LowerCaseEqualsLiteral("font-face")) {
    parseFunc = &CSSParserImpl::ParseFontFaceRule;
    newSection = eCSSSection_General;

  } else if (mToken.mIdent.LowerCaseEqualsLiteral("page")) {
    parseFunc = &CSSParserImpl::ParsePageRule;
    newSection = eCSSSection_General;

  } else if (mToken.mIdent.LowerCaseEqualsLiteral("-moz-keyframes") ||
             mToken.mIdent.LowerCaseEqualsLiteral("keyframes")) {
    parseFunc = &CSSParserImpl::ParseKeyframesRule;
    newSection = eCSSSection_General;

  } else if (mToken.mIdent.LowerCaseEqualsLiteral("supports") &&
             CSSSupportsRule::PrefEnabled()) {
    parseFunc = &CSSParserImpl::ParseSupportsRule;
    newSection = eCSSSection_General;

  } else {
    if (!NonMozillaVendorIdentifier(mToken.mIdent)) {
      REPORT_UNEXPECTED_TOKEN(PEUnknownAtRule);
      OUTPUT_ERROR();
    }
    
    return SkipAtRule(aInAtRule);
  }

  
  
  bool unnestable = aInAtRule && newSection != eCSSSection_General;
  if (unnestable) {
    REPORT_UNEXPECTED_TOKEN(PEGroupRuleNestedAtRule);
  }
  
  if (unnestable || !(this->*parseFunc)(aAppendFunc, aData)) {
    
    OUTPUT_ERROR();
    return SkipAtRule(aInAtRule);
  }

  
  if (!aInAtRule) {
    mSection = newSection;
  }
  
  return true;
}

bool
CSSParserImpl::ParseCharsetRule(RuleAppendFunc aAppendFunc,
                                void* aData)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PECharsetRuleEOF);
    return false;
  }

  if (eCSSToken_String != mToken.mType) {
    UngetToken();
    REPORT_UNEXPECTED_TOKEN(PECharsetRuleNotString);
    return false;
  }

  nsAutoString charset = mToken.mIdent;

  if (!ExpectSymbol(';', true)) {
    return false;
  }

  nsRefPtr<css::CharsetRule> rule = new css::CharsetRule(charset);
  (*aAppendFunc)(rule, aData);

  return true;
}

bool
CSSParserImpl::ParseURLOrString(nsString& aURL)
{
  if (!GetToken(true)) {
    return false;
  }
  if (eCSSToken_String == mToken.mType || eCSSToken_URL == mToken.mType) {
    aURL = mToken.mIdent;
    return true;
  }
  UngetToken();
  return false;
}

bool
CSSParserImpl::ParseMediaQuery(bool aInAtRule,
                               nsMediaQuery **aQuery,
                               bool *aHitStop)
{
  *aQuery = nullptr;
  *aHitStop = false;

  
  
  
  if (!GetToken(true)) {
    *aHitStop = true;
    
    if (!aInAtRule)
      return true;

    
    REPORT_UNEXPECTED_EOF(PEGatherMediaEOF);
    return true;
  }

  if (eCSSToken_Symbol == mToken.mType && aInAtRule &&
      (mToken.mSymbol == ';' || mToken.mSymbol == '{' || mToken.mSymbol == '}' )) {
    *aHitStop = true;
    UngetToken();
    return true;
  }
  UngetToken();

  nsMediaQuery* query = new nsMediaQuery;
  *aQuery = query;

  if (ExpectSymbol('(', true)) {
    
    UngetToken(); 
    query->SetType(nsGkAtoms::all);
    query->SetTypeOmitted();
    
    if (!ParseMediaQueryExpression(query)) {
      OUTPUT_ERROR();
      query->SetHadUnknownExpression();
    }
  } else {
    nsCOMPtr<nsIAtom> mediaType;
    bool gotNotOrOnly = false;
    for (;;) {
      if (!GetToken(true)) {
        REPORT_UNEXPECTED_EOF(PEGatherMediaEOF);
        return false;
      }
      if (eCSSToken_Ident != mToken.mType) {
        REPORT_UNEXPECTED_TOKEN(PEGatherMediaNotIdent);
        UngetToken();
        return false;
      }
      
      nsContentUtils::ASCIIToLower(mToken.mIdent);
      mediaType = do_GetAtom(mToken.mIdent);
      if (!mediaType) {
        NS_RUNTIMEABORT("do_GetAtom failed - out of memory?");
      }
      if (gotNotOrOnly ||
          (mediaType != nsGkAtoms::_not && mediaType != nsGkAtoms::only))
        break;
      gotNotOrOnly = true;
      if (mediaType == nsGkAtoms::_not)
        query->SetNegated();
      else
        query->SetHasOnly();
    }
    query->SetType(mediaType);
  }

  for (;;) {
    if (!GetToken(true)) {
      *aHitStop = true;
      
      if (!aInAtRule)
        break;

      
      REPORT_UNEXPECTED_EOF(PEGatherMediaEOF);
      break;
    }

    if (eCSSToken_Symbol == mToken.mType && aInAtRule &&
        (mToken.mSymbol == ';' || mToken.mSymbol == '{' || mToken.mSymbol == '}')) {
      *aHitStop = true;
      UngetToken();
      break;
    }
    if (eCSSToken_Symbol == mToken.mType && mToken.mSymbol == ',') {
      
      break;
    }
    if (eCSSToken_Ident != mToken.mType ||
        !mToken.mIdent.LowerCaseEqualsLiteral("and")) {
      REPORT_UNEXPECTED_TOKEN(PEGatherMediaNotComma);
      UngetToken();
      return false;
    }
    if (!ParseMediaQueryExpression(query)) {
      OUTPUT_ERROR();
      query->SetHadUnknownExpression();
    }
  }
  return true;
}



bool
CSSParserImpl::GatherMedia(nsMediaList* aMedia,
                           bool aInAtRule)
{
  for (;;) {
    nsAutoPtr<nsMediaQuery> query;
    bool hitStop;
    if (!ParseMediaQuery(aInAtRule, getter_Transfers(query),
                         &hitStop)) {
      NS_ASSERTION(!hitStop, "should return true when hit stop");
      OUTPUT_ERROR();
      if (query) {
        query->SetHadUnknownExpression();
      }
      if (aInAtRule) {
        const PRUnichar stopChars[] =
          { PRUnichar(','), PRUnichar('{'), PRUnichar(';'), PRUnichar('}'), PRUnichar(0) };
        SkipUntilOneOf(stopChars);
      } else {
        SkipUntil(',');
      }
      
      if (mToken.mType == eCSSToken_Symbol && aInAtRule &&
          (mToken.mSymbol == '{' || mToken.mSymbol == ';'  || mToken.mSymbol == '}')) {
        UngetToken();
        hitStop = true;
      }
    }
    if (query) {
      aMedia->AppendQuery(query);
    }
    if (hitStop) {
      break;
    }
  }
  return true;
}

bool
CSSParserImpl::ParseMediaQueryExpression(nsMediaQuery* aQuery)
{
  if (!ExpectSymbol('(', true)) {
    REPORT_UNEXPECTED_TOKEN(PEMQExpectedExpressionStart);
    return false;
  }
  if (! GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEMQExpressionEOF);
    return false;
  }
  if (eCSSToken_Ident != mToken.mType) {
    REPORT_UNEXPECTED_TOKEN(PEMQExpectedFeatureName);
    UngetToken();
    SkipUntil(')');
    return false;
  }

  nsMediaExpression *expr = aQuery->NewExpression();

  
  nsContentUtils::ASCIIToLower(mToken.mIdent);
  const PRUnichar *featureString;
  if (StringBeginsWith(mToken.mIdent, NS_LITERAL_STRING("min-"))) {
    expr->mRange = nsMediaExpression::eMin;
    featureString = mToken.mIdent.get() + 4;
  } else if (StringBeginsWith(mToken.mIdent, NS_LITERAL_STRING("max-"))) {
    expr->mRange = nsMediaExpression::eMax;
    featureString = mToken.mIdent.get() + 4;
  } else {
    expr->mRange = nsMediaExpression::eEqual;
    featureString = mToken.mIdent.get();
  }

  nsCOMPtr<nsIAtom> mediaFeatureAtom = do_GetAtom(featureString);
  if (!mediaFeatureAtom) {
    NS_RUNTIMEABORT("do_GetAtom failed - out of memory?");
  }
  const nsMediaFeature *feature = nsMediaFeatures::features;
  for (; feature->mName; ++feature) {
    if (*(feature->mName) == mediaFeatureAtom) {
      break;
    }
  }
  if (!feature->mName ||
      (expr->mRange != nsMediaExpression::eEqual &&
       feature->mRangeType != nsMediaFeature::eMinMaxAllowed)) {
    REPORT_UNEXPECTED_TOKEN(PEMQExpectedFeatureName);
    SkipUntil(')');
    return false;
  }
  expr->mFeature = feature;

  if (!GetToken(true) || mToken.IsSymbol(')')) {
    
    
    if (expr->mRange != nsMediaExpression::eEqual) {
      REPORT_UNEXPECTED(PEMQNoMinMaxWithoutValue);
      return false;
    }
    expr->mValue.Reset();
    return true;
  }

  if (!mToken.IsSymbol(':')) {
    REPORT_UNEXPECTED_TOKEN(PEMQExpectedFeatureNameEnd);
    UngetToken();
    SkipUntil(')');
    return false;
  }

  bool rv;
  switch (feature->mValueType) {
    case nsMediaFeature::eLength:
      rv = ParseNonNegativeVariant(expr->mValue, VARIANT_LENGTH, nullptr);
      break;
    case nsMediaFeature::eInteger:
    case nsMediaFeature::eBoolInteger:
      rv = ParseNonNegativeVariant(expr->mValue, VARIANT_INTEGER, nullptr);
      
      if (rv &&
          feature->mValueType == nsMediaFeature::eBoolInteger &&
          expr->mValue.GetIntValue() > 1)
        rv = false;
      break;
    case nsMediaFeature::eFloat:
      rv = ParseNonNegativeVariant(expr->mValue, VARIANT_NUMBER, nullptr);
      break;
    case nsMediaFeature::eIntRatio:
      {
        
        
        nsRefPtr<nsCSSValue::Array> a = nsCSSValue::Array::Create(2);
        expr->mValue.SetArrayValue(a, eCSSUnit_Array);
        
        
        
        rv = ParseVariant(a->Item(0), VARIANT_INTEGER, nullptr) &&
             a->Item(0).GetIntValue() > 0 &&
             ExpectSymbol('/', true) &&
             ParseVariant(a->Item(1), VARIANT_INTEGER, nullptr) &&
             a->Item(1).GetIntValue() > 0;
      }
      break;
    case nsMediaFeature::eResolution:
      rv = GetToken(true);
      if (!rv)
        break;
      rv = mToken.mType == eCSSToken_Dimension && mToken.mNumber > 0.0f;
      if (!rv) {
        UngetToken();
        break;
      }
      
      
      NS_ASSERTION(!mToken.mIdent.IsEmpty(), "unit lied");
      if (mToken.mIdent.LowerCaseEqualsLiteral("dpi")) {
        expr->mValue.SetFloatValue(mToken.mNumber, eCSSUnit_Inch);
      } else if (mToken.mIdent.LowerCaseEqualsLiteral("dppx")) {
        expr->mValue.SetFloatValue(mToken.mNumber, eCSSUnit_Pixel);
      } else if (mToken.mIdent.LowerCaseEqualsLiteral("dpcm")) {
        expr->mValue.SetFloatValue(mToken.mNumber, eCSSUnit_Centimeter);
      } else {
        rv = false;
      }
      break;
    case nsMediaFeature::eEnumerated:
      rv = ParseVariant(expr->mValue, VARIANT_KEYWORD,
                        feature->mData.mKeywordTable);
      break;
    case nsMediaFeature::eIdent:
      rv = ParseVariant(expr->mValue, VARIANT_IDENTIFIER, nullptr);
      break;
  }
  if (!rv || !ExpectSymbol(')', true)) {
    REPORT_UNEXPECTED(PEMQExpectedFeatureValue);
    SkipUntil(')');
    return false;
  }

  return true;
}


bool
CSSParserImpl::ParseImportRule(RuleAppendFunc aAppendFunc, void* aData)
{
  nsRefPtr<nsMediaList> media = new nsMediaList();

  nsAutoString url;
  if (!ParseURLOrString(url)) {
    REPORT_UNEXPECTED_TOKEN(PEImportNotURI);
    return false;
  }

  if (!ExpectSymbol(';', true)) {
    if (!GatherMedia(media, true) ||
        !ExpectSymbol(';', true)) {
      REPORT_UNEXPECTED_TOKEN(PEImportUnexpected);
      
      return false;
    }

    
    
    NS_ASSERTION(media->Count() != 0, "media list must be nonempty");
  }

  ProcessImport(url, media, aAppendFunc, aData);
  return true;
}


void
CSSParserImpl::ProcessImport(const nsString& aURLSpec,
                             nsMediaList* aMedia,
                             RuleAppendFunc aAppendFunc,
                             void* aData)
{
  nsRefPtr<css::ImportRule> rule = new css::ImportRule(aMedia, aURLSpec);
  (*aAppendFunc)(rule, aData);

  
  nsCOMPtr<nsIURI> url;
  
  nsresult rv = NS_NewURI(getter_AddRefs(url), aURLSpec, nullptr, mBaseURI);

  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_MALFORMED_URI) {
      
      REPORT_UNEXPECTED_P(PEImportBadURI, aURLSpec);
      OUTPUT_ERROR();
    }
    return;
  }

  if (mChildLoader) {
    mChildLoader->LoadChildSheet(mSheet, url, aMedia, rule);
  }
}


bool
CSSParserImpl::ParseGroupRule(css::GroupRule* aRule,
                              RuleAppendFunc aAppendFunc,
                              void* aData)
{
  
  if (!ExpectSymbol('{', true)) {
    return false;
  }

  
  PushGroup(aRule);
  nsCSSSection holdSection = mSection;
  mSection = eCSSSection_General;

  for (;;) {
    
    if (! GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PEGroupRuleEOF2);
      break;
    }
    if (mToken.IsSymbol('}')) { 
      UngetToken();
      break;
    }
    if (eCSSToken_AtKeyword == mToken.mType) {
      
      ParseAtRule(aAppendFunc, aData, true);
      continue;
    }
    UngetToken();
    ParseRuleSet(AppendRuleToSheet, this, true);
  }
  PopGroup();

  if (!ExpectSymbol('}', true)) {
    mSection = holdSection;
    return false;
  }
  (*aAppendFunc)(aRule, aData);
  return true;
}


bool
CSSParserImpl::ParseMediaRule(RuleAppendFunc aAppendFunc, void* aData)
{
  nsRefPtr<nsMediaList> media = new nsMediaList();

  if (GatherMedia(media, true)) {
    
    nsRefPtr<css::MediaRule> rule = new css::MediaRule();
    
    
    if (ParseGroupRule(rule, aAppendFunc, aData)) {
      rule->SetMedia(media);
      return true;
    }
  }

  return false;
}




bool
CSSParserImpl::ParseMozDocumentRule(RuleAppendFunc aAppendFunc, void* aData)
{
  css::DocumentRule::URL *urls = nullptr;
  css::DocumentRule::URL **next = &urls;
  do {
    if (!GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PEMozDocRuleEOF);
      delete urls;
      return false;
    }
        
    if (!(eCSSToken_URL == mToken.mType ||
          (eCSSToken_Function == mToken.mType &&
           (mToken.mIdent.LowerCaseEqualsLiteral("url-prefix") ||
            mToken.mIdent.LowerCaseEqualsLiteral("domain") ||
            mToken.mIdent.LowerCaseEqualsLiteral("regexp"))))) {
      REPORT_UNEXPECTED_TOKEN(PEMozDocRuleBadFunc);
      UngetToken();
      delete urls;
      return false;
    }
    css::DocumentRule::URL *cur = *next = new css::DocumentRule::URL;
    next = &cur->next;
    if (mToken.mType == eCSSToken_URL) {
      cur->func = css::DocumentRule::eURL;
      CopyUTF16toUTF8(mToken.mIdent, cur->url);
    } else if (mToken.mIdent.LowerCaseEqualsLiteral("regexp")) {
      
      
      
      cur->func = css::DocumentRule::eRegExp;
      GetToken(true);
      
      
      CopyUTF16toUTF8(mToken.mIdent, cur->url);
      if (eCSSToken_String != mToken.mType || !ExpectSymbol(')', true)) {
        REPORT_UNEXPECTED_TOKEN(PEMozDocRuleNotString);
        SkipUntil(')');
        delete urls;
        return false;
      }
    } else {
      if (mToken.mIdent.LowerCaseEqualsLiteral("url-prefix")) {
        cur->func = css::DocumentRule::eURLPrefix;
      } else if (mToken.mIdent.LowerCaseEqualsLiteral("domain")) {
        cur->func = css::DocumentRule::eDomain;
      }

      NS_ASSERTION(!mHavePushBack, "mustn't have pushback at this point");
      if (!mScanner->NextURL(mToken) || mToken.mType != eCSSToken_URL) {
        REPORT_UNEXPECTED_TOKEN(PEMozDocRuleNotURI);
        SkipUntil(')');
        delete urls;
        return false;
      }

      
      
      
      CopyUTF16toUTF8(mToken.mIdent, cur->url);
    }
  } while (ExpectSymbol(',', true));

  nsRefPtr<css::DocumentRule> rule = new css::DocumentRule();
  rule->SetURLs(urls);

  return ParseGroupRule(rule, aAppendFunc, aData);
}


bool
CSSParserImpl::ParseNameSpaceRule(RuleAppendFunc aAppendFunc, void* aData)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEAtNSPrefixEOF);
    return false;
  }

  nsAutoString  prefix;
  nsAutoString  url;

  if (eCSSToken_Ident == mToken.mType) {
    prefix = mToken.mIdent;
    
  } else {
    UngetToken();
  }

  if (!ParseURLOrString(url) || !ExpectSymbol(';', true)) {
    if (mHavePushBack) {
      REPORT_UNEXPECTED_TOKEN(PEAtNSUnexpected);
    } else {
      REPORT_UNEXPECTED_EOF(PEAtNSURIEOF);
    }
    return false;
  }

  ProcessNameSpace(prefix, url, aAppendFunc, aData);
  return true;
}

void
CSSParserImpl::ProcessNameSpace(const nsString& aPrefix,
                                const nsString& aURLSpec,
                                RuleAppendFunc aAppendFunc,
                                void* aData)
{
  nsCOMPtr<nsIAtom> prefix;

  if (!aPrefix.IsEmpty()) {
    prefix = do_GetAtom(aPrefix);
    if (!prefix) {
      NS_RUNTIMEABORT("do_GetAtom failed - out of memory?");
    }
  }

  nsRefPtr<css::NameSpaceRule> rule = new css::NameSpaceRule(prefix, aURLSpec);
  (*aAppendFunc)(rule, aData);

  
  
  if (!mNameSpaceMap) {
    mNameSpaceMap = mSheet->GetNameSpaceMap();
  }
}



bool
CSSParserImpl::ParseFontFaceRule(RuleAppendFunc aAppendFunc, void* aData)
{
  if (!ExpectSymbol('{', true)) {
    REPORT_UNEXPECTED_TOKEN(PEBadFontBlockStart);
    return false;
  }

  nsRefPtr<nsCSSFontFaceRule> rule(new nsCSSFontFaceRule());

  for (;;) {
    if (!GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PEFontFaceEOF);
      break;
    }
    if (mToken.IsSymbol('}')) { 
      UngetToken();
      break;
    }

    
    if (mToken.IsSymbol(';'))
      continue;

    if (!ParseFontDescriptor(rule)) {
      REPORT_UNEXPECTED(PEDeclSkipped);
      OUTPUT_ERROR();
      if (!SkipDeclaration(true))
        break;
    }
  }
  if (!ExpectSymbol('}', true)) {
    REPORT_UNEXPECTED_TOKEN(PEBadFontBlockEnd);
    return false;
  }
  (*aAppendFunc)(rule, aData);
  return true;
}













bool
CSSParserImpl::ParseFontDescriptor(nsCSSFontFaceRule* aRule)
{
  if (eCSSToken_Ident != mToken.mType) {
    REPORT_UNEXPECTED_TOKEN(PEFontDescExpected);
    return false;
  }

  nsString descName = mToken.mIdent;
  if (!ExpectSymbol(':', true)) {
    REPORT_UNEXPECTED_TOKEN(PEParseDeclarationNoColon);
    OUTPUT_ERROR();
    return false;
  }

  nsCSSFontDesc descID = nsCSSProps::LookupFontDesc(descName);
  nsCSSValue value;

  if (descID == eCSSFontDesc_UNKNOWN) {
    if (NonMozillaVendorIdentifier(descName)) {
      
      SkipDeclaration(true);
      return true;
    } else {
      REPORT_UNEXPECTED_P(PEUnknownFontDesc, descName);
      return false;
    }
  }

  if (!ParseFontDescriptorValue(descID, value)) {
    REPORT_UNEXPECTED_P(PEValueParsingError, descName);
    return false;
  }

  if (!ExpectEndProperty())
    return false;

  aRule->SetDesc(descID, value);
  return true;
}


bool
CSSParserImpl::ParseKeyframesRule(RuleAppendFunc aAppendFunc, void* aData)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEKeyframeNameEOF);
    return false;
  }

  if (mToken.mType != eCSSToken_Ident) {
    REPORT_UNEXPECTED_TOKEN(PEKeyframeBadName);
    UngetToken();
    return false;
  }
  nsString name(mToken.mIdent);

  if (!ExpectSymbol('{', true)) {
    REPORT_UNEXPECTED_TOKEN(PEKeyframeBrace);
    return false;
  }

  nsRefPtr<nsCSSKeyframesRule> rule = new nsCSSKeyframesRule(name);

  while (!ExpectSymbol('}', true)) {
    nsRefPtr<nsCSSKeyframeRule> kid = ParseKeyframeRule();
    if (kid) {
      rule->AppendStyleRule(kid);
    } else {
      OUTPUT_ERROR();
      SkipRuleSet(true);
    }
  }

  (*aAppendFunc)(rule, aData);
  return true;
}

bool
CSSParserImpl::ParsePageRule(RuleAppendFunc aAppendFunc, void* aData)
{
  
  uint32_t parseFlags = eParseDeclaration_InBraces |
                        eParseDeclaration_AllowImportant;

  
  NS_ABORT_IF_FALSE(mViewportUnitsEnabled,
                    "Viewport units should be enabled outside of @page rules.");
  mViewportUnitsEnabled = false;
  nsAutoPtr<css::Declaration> declaration(
                                ParseDeclarationBlock(parseFlags,
                                                      eCSSContext_Page));
  mViewportUnitsEnabled = true;

  if (!declaration) {
    return false;
  }

  
  nsRefPtr<nsCSSPageRule> rule = new nsCSSPageRule(declaration);

  (*aAppendFunc)(rule, aData);
  return true;
}

already_AddRefed<nsCSSKeyframeRule>
CSSParserImpl::ParseKeyframeRule()
{
  InfallibleTArray<float> selectorList;
  if (!ParseKeyframeSelectorList(selectorList)) {
    REPORT_UNEXPECTED(PEBadSelectorKeyframeRuleIgnored);
    return nullptr;
  }

  
  uint32_t parseFlags = eParseDeclaration_InBraces;
  nsAutoPtr<css::Declaration> declaration(ParseDeclarationBlock(parseFlags));
  if (!declaration) {
    return nullptr;
  }

  
  nsRefPtr<nsCSSKeyframeRule> rule =
    new nsCSSKeyframeRule(selectorList, declaration);

  return rule.forget();
}

bool
CSSParserImpl::ParseKeyframeSelectorList(InfallibleTArray<float>& aSelectorList)
{
  for (;;) {
    if (!GetToken(true)) {
      
      
      return false;
    }
    float value;
    switch (mToken.mType) {
      case eCSSToken_Percentage:
        value = mToken.mNumber;
        break;
      case eCSSToken_Ident:
        if (mToken.mIdent.LowerCaseEqualsLiteral("from")) {
          value = 0.0f;
          break;
        }
        if (mToken.mIdent.LowerCaseEqualsLiteral("to")) {
          value = 1.0f;
          break;
        }
        
      default:
        UngetToken();
        
        
        return false;
    }
    aSelectorList.AppendElement(value);
    if (!ExpectSymbol(',', true)) {
      return true;
    }
  }
}




bool
CSSParserImpl::ParseSupportsRule(RuleAppendFunc aAppendFunc, void* aProcessData)
{
  bool conditionMet = false;
  nsString condition;

  mScanner->StartRecording();
  bool parsed = ParseSupportsCondition(conditionMet);

  if (!parsed) {
    mScanner->StopRecording();
    return false;
  }

  if (!ExpectSymbol('{', true)) {
    REPORT_UNEXPECTED_TOKEN(PESupportsGroupRuleStart);
    mScanner->StopRecording();
    return false;
  }

  UngetToken();
  mScanner->StopRecording(condition);

  
  if (condition.Length() != 0) {
    condition.Truncate(condition.Length() - 1);
  }

  
  condition.Trim(" ", true, true, false);

  
  
  nsAutoFailingSupportsRule failing(this, conditionMet);

  nsRefPtr<css::GroupRule> rule = new CSSSupportsRule(conditionMet, condition);
  return ParseGroupRule(rule, aAppendFunc, aProcessData);
}





bool
CSSParserImpl::ParseSupportsCondition(bool& aConditionMet)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PESupportsConditionStartEOF2);
    return false;
  }

  UngetToken();

  if (mToken.IsSymbol('(') ||
      mToken.mType == eCSSToken_Function ||
      mToken.mType == eCSSToken_URL ||
      mToken.mType == eCSSToken_Bad_URL) {
    return ParseSupportsConditionInParens(aConditionMet) &&
           ParseSupportsConditionTerms(aConditionMet);
  }

  if (mToken.mType == eCSSToken_Ident &&
      mToken.mIdent.LowerCaseEqualsLiteral("not")) {
    return ParseSupportsConditionNegation(aConditionMet);
  }

  REPORT_UNEXPECTED_TOKEN(PESupportsConditionExpectedStart);
  return false;
}




bool
CSSParserImpl::ParseSupportsConditionNegation(bool& aConditionMet)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PESupportsConditionNotEOF);
    return false;
  }

  if (mToken.mType != eCSSToken_Ident ||
      !mToken.mIdent.LowerCaseEqualsLiteral("not")) {
    REPORT_UNEXPECTED_TOKEN(PESupportsConditionExpectedNot);
    return false;
  }

  if (ParseSupportsConditionInParens(aConditionMet)) {
    aConditionMet = !aConditionMet;
    return true;
  }

  return false;
}





bool
CSSParserImpl::ParseSupportsConditionInParens(bool& aConditionMet)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PESupportsConditionInParensStartEOF);
    return false;
  }

  if (mToken.mType == eCSSToken_URL) {
    aConditionMet = false;
    return true;
  }

  if (mToken.mType == eCSSToken_Function ||
      mToken.mType == eCSSToken_Bad_URL) {
    if (!SkipUntil(')')) {
      REPORT_UNEXPECTED_EOF(PESupportsConditionInParensEOF);
      return false;
    }
    aConditionMet = false;
    return true;
  }

  if (!mToken.IsSymbol('(')) {
    REPORT_UNEXPECTED_TOKEN(PESupportsConditionExpectedOpenParenOrFunction);
    UngetToken();
    return false;
  }

  if (!ParseSupportsConditionInParensInsideParens(aConditionMet)) {
    if (!SkipUntil(')')) {
      REPORT_UNEXPECTED_EOF(PESupportsConditionInParensEOF);
      return false;
    }
    aConditionMet = false;
    return true;
  }

  if (!(ExpectSymbol(')', true))) {
    REPORT_UNEXPECTED_TOKEN(PESupportsConditionExpectedCloseParen);
    SkipUntil(')');
    return false;
  }

  return true;
}






bool
CSSParserImpl::ParseSupportsConditionInParensInsideParens(bool& aConditionMet)
{
  if (!GetToken(true)) {
    return false;
  }

  if (mToken.mType == eCSSToken_Ident) {
    if (!mToken.mIdent.LowerCaseEqualsLiteral("not")) {
      nsAutoString propertyName = mToken.mIdent;
      if (!ExpectSymbol(':', true)) {
        return false;
      }

      if (ExpectSymbol(')', true)) {
        UngetToken();
        return false;
      }

      nsCSSProperty propID = nsCSSProps::LookupProperty(propertyName,
                                                        nsCSSProps::eEnabled);
      if (propID == eCSSProperty_UNKNOWN) {
        aConditionMet = false;
        SkipUntil(')');
        UngetToken();
      } else {
        aConditionMet = ParseProperty(propID) &&
                        ParsePriority() != ePriority_Error;
        if (!aConditionMet) {
          SkipUntil(')');
          UngetToken();
        }
        mTempData.ClearProperty(propID);
        mTempData.AssertInitialState();
      }
      return true;
    }

    UngetToken();
    return ParseSupportsConditionNegation(aConditionMet);
  }

  UngetToken();
  return ParseSupportsConditionInParens(aConditionMet) &&
         ParseSupportsConditionTerms(aConditionMet);
}






bool
CSSParserImpl::ParseSupportsConditionTerms(bool& aConditionMet)
{
  if (!GetToken(true)) {
    return true;
  }

  if (mToken.mType != eCSSToken_Ident) {
    UngetToken();
    return true;
  }

  if (mToken.mIdent.LowerCaseEqualsLiteral("and")) {
    return ParseSupportsConditionTermsAfterOperator(aConditionMet, eAnd);
  }

  if (mToken.mIdent.LowerCaseEqualsLiteral("or")) {
    return ParseSupportsConditionTermsAfterOperator(aConditionMet, eOr);
  }

  UngetToken();
  return true;
}




bool
CSSParserImpl::ParseSupportsConditionTermsAfterOperator(
                         bool& aConditionMet,
                         CSSParserImpl::SupportsConditionTermOperator aOperator)
{
  const char* token = aOperator == eAnd ? "and" : "or";
  for (;;) {
    bool termConditionMet = false;
    if (!ParseSupportsConditionInParens(termConditionMet)) {
      return false;
    }
    aConditionMet = aOperator == eAnd ? aConditionMet && termConditionMet :
                                        aConditionMet || termConditionMet;

    if (!GetToken(true)) {
      return true;
    }

    if (mToken.mType != eCSSToken_Ident ||
        !mToken.mIdent.LowerCaseEqualsASCII(token)) {
      UngetToken();
      return true;
    }
  }
}

bool
CSSParserImpl::SkipUntil(PRUnichar aStopSymbol)
{
  nsCSSToken* tk = &mToken;
  nsAutoTArray<PRUnichar, 16> stack;
  stack.AppendElement(aStopSymbol);
  for (;;) {
    if (!GetToken(true)) {
      return false;
    }
    if (eCSSToken_Symbol == tk->mType) {
      PRUnichar symbol = tk->mSymbol;
      uint32_t stackTopIndex = stack.Length() - 1;
      if (symbol == stack.ElementAt(stackTopIndex)) {
        stack.RemoveElementAt(stackTopIndex);
        if (stackTopIndex == 0) {
          return true;
        }

      
      
      
      } else if ('{' == symbol) {
        stack.AppendElement('}');
      } else if ('[' == symbol) {
        stack.AppendElement(']');
      } else if ('(' == symbol) {
        stack.AppendElement(')');
      }
    } else if (eCSSToken_Function == tk->mType ||
               eCSSToken_Bad_URL == tk->mType) {
      stack.AppendElement(')');
    }
  }
}

void
CSSParserImpl::SkipUntilOneOf(const PRUnichar* aStopSymbolChars)
{
  nsCSSToken* tk = &mToken;
  nsDependentString stopSymbolChars(aStopSymbolChars);
  for (;;) {
    if (!GetToken(true)) {
      break;
    }
    if (eCSSToken_Symbol == tk->mType) {
      PRUnichar symbol = tk->mSymbol;
      if (stopSymbolChars.FindChar(symbol) != -1) {
        break;
      } else if ('{' == symbol) {
        SkipUntil('}');
      } else if ('[' == symbol) {
        SkipUntil(']');
      } else if ('(' == symbol) {
        SkipUntil(')');
      }
    } else if (eCSSToken_Function == tk->mType ||
               eCSSToken_Bad_URL == tk->mType) {
      SkipUntil(')');
    }
  }
}

bool
CSSParserImpl::SkipDeclaration(bool aCheckForBraces)
{
  nsCSSToken* tk = &mToken;
  for (;;) {
    if (!GetToken(true)) {
      if (aCheckForBraces) {
        REPORT_UNEXPECTED_EOF(PESkipDeclBraceEOF);
      }
      return false;
    }
    if (eCSSToken_Symbol == tk->mType) {
      PRUnichar symbol = tk->mSymbol;
      if (';' == symbol) {
        break;
      }
      if (aCheckForBraces) {
        if ('}' == symbol) {
          UngetToken();
          break;
        }
      }
      if ('{' == symbol) {
        SkipUntil('}');
      } else if ('(' == symbol) {
        SkipUntil(')');
      } else if ('[' == symbol) {
        SkipUntil(']');
      }
    } else if (eCSSToken_Function == tk->mType ||
               eCSSToken_Bad_URL == tk->mType) {
      SkipUntil(')');
    }
  }
  return true;
}

void
CSSParserImpl::SkipRuleSet(bool aInsideBraces)
{
  nsCSSToken* tk = &mToken;
  for (;;) {
    if (!GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PESkipRSBraceEOF);
      break;
    }
    if (eCSSToken_Symbol == tk->mType) {
      PRUnichar symbol = tk->mSymbol;
      if ('}' == symbol && aInsideBraces) {
        
        UngetToken();
        break;
      } else if ('{' == symbol) {
        SkipUntil('}');
        break;
      } else if ('(' == symbol) {
        SkipUntil(')');
      } else if ('[' == symbol) {
        SkipUntil(']');
      }
    } else if (eCSSToken_Function == tk->mType ||
               eCSSToken_Bad_URL == tk->mType) {
      SkipUntil(')');
    }
  }
}

void
CSSParserImpl::PushGroup(css::GroupRule* aRule)
{
  mGroupStack.AppendElement(aRule);
}

void
CSSParserImpl::PopGroup()
{
  uint32_t count = mGroupStack.Length();
  if (0 < count) {
    mGroupStack.RemoveElementAt(count - 1);
  }
}

void
CSSParserImpl::AppendRule(css::Rule* aRule)
{
  uint32_t count = mGroupStack.Length();
  if (0 < count) {
    mGroupStack[count - 1]->AppendStyleRule(aRule);
  }
  else {
    mSheet->AppendStyleRule(aRule);
  }
}

bool
CSSParserImpl::ParseRuleSet(RuleAppendFunc aAppendFunc, void* aData,
                            bool aInsideBraces)
{
  
  nsCSSSelectorList* slist = nullptr;
  uint32_t linenum = mScanner->GetLineNumber();
  if (! ParseSelectorList(slist, PRUnichar('{'))) {
    REPORT_UNEXPECTED(PEBadSelectorRSIgnored);
    OUTPUT_ERROR();
    SkipRuleSet(aInsideBraces);
    return false;
  }
  NS_ASSERTION(nullptr != slist, "null selector list");
  CLEAR_ERROR();

  
  uint32_t parseFlags = eParseDeclaration_InBraces |
                        eParseDeclaration_AllowImportant;
  css::Declaration* declaration = ParseDeclarationBlock(parseFlags);
  if (nullptr == declaration) {
    delete slist;
    return false;
  }

#if 0
  slist->Dump();
  fputs("{\n", stdout);
  declaration->List();
  fputs("}\n", stdout);
#endif

  

  nsRefPtr<css::StyleRule> rule = new css::StyleRule(slist, declaration);
  rule->SetLineNumber(linenum);
  (*aAppendFunc)(rule, aData);

  return true;
}

bool
CSSParserImpl::ParseSelectorList(nsCSSSelectorList*& aListHead,
                                 PRUnichar aStopChar)
{
  nsCSSSelectorList* list = nullptr;
  if (! ParseSelectorGroup(list)) {
    
    aListHead = nullptr;
    return false;
  }
  NS_ASSERTION(nullptr != list, "no selector list");
  aListHead = list;

  
  
  nsCSSToken* tk = &mToken;
  for (;;) {
    if (! GetToken(true)) {
      if (aStopChar == PRUnichar(0)) {
        return true;
      }

      REPORT_UNEXPECTED_EOF(PESelectorListExtraEOF);
      break;
    }

    if (eCSSToken_Symbol == tk->mType) {
      if (',' == tk->mSymbol) {
        nsCSSSelectorList* newList = nullptr;
        
        if (! ParseSelectorGroup(newList)) {
          break;
        }
        
        list->mNext = newList;
        list = newList;
        continue;
      } else if (aStopChar == tk->mSymbol && aStopChar != PRUnichar(0)) {
        UngetToken();
        return true;
      }
    }
    REPORT_UNEXPECTED_TOKEN(PESelectorListExtra);
    UngetToken();
    break;
  }

  delete aListHead;
  aListHead = nullptr;
  return false;
}

static bool IsUniversalSelector(const nsCSSSelector& aSelector)
{
  return bool((aSelector.mNameSpace == kNameSpaceID_Unknown) &&
                (aSelector.mLowercaseTag == nullptr) &&
                (aSelector.mIDList == nullptr) &&
                (aSelector.mClassList == nullptr) &&
                (aSelector.mAttrList == nullptr) &&
                (aSelector.mNegations == nullptr) &&
                (aSelector.mPseudoClassList == nullptr));
}

bool
CSSParserImpl::ParseSelectorGroup(nsCSSSelectorList*& aList)
{
  PRUnichar combinator = 0;
  nsAutoPtr<nsCSSSelectorList> list(new nsCSSSelectorList());

  for (;;) {
    if (!ParseSelector(list, combinator)) {
      return false;
    }

    
    if (!GetToken(false)) {
      break; 
    }

    combinator = PRUnichar(0);
    if (mToken.mType == eCSSToken_Whitespace) {
      if (!GetToken(true)) {
        break; 
      }
      combinator = PRUnichar(' ');
    }

    if (mToken.mType != eCSSToken_Symbol) {
      UngetToken(); 
    } else {
      PRUnichar symbol = mToken.mSymbol;
      if (symbol == '+' || symbol == '>' || symbol == '~') {
        combinator = mToken.mSymbol;
      } else {
        UngetToken(); 
        if (symbol == ',' || symbol == '{' || symbol == ')') {
          break; 
        }
      }
    }

    if (!combinator) {
      REPORT_UNEXPECTED_TOKEN(PESelectorListExtra);
      return false;
    }
  }

  aList = list.forget();
  return true;
}

#define SEL_MASK_NSPACE   0x01
#define SEL_MASK_ELEM     0x02
#define SEL_MASK_ID       0x04
#define SEL_MASK_CLASS    0x08
#define SEL_MASK_ATTRIB   0x10
#define SEL_MASK_PCLASS   0x20
#define SEL_MASK_PELEM    0x40




CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParseIDSelector(int32_t&       aDataMask,
                               nsCSSSelector& aSelector)
{
  NS_ASSERTION(!mToken.mIdent.IsEmpty(),
               "Empty mIdent in eCSSToken_ID token?");
  aDataMask |= SEL_MASK_ID;
  aSelector.AddID(mToken.mIdent);
  return eSelectorParsingStatus_Continue;
}




CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParseClassSelector(int32_t&       aDataMask,
                                  nsCSSSelector& aSelector)
{
  if (! GetToken(false)) { 
    REPORT_UNEXPECTED_EOF(PEClassSelEOF);
    return eSelectorParsingStatus_Error;
  }
  if (eCSSToken_Ident != mToken.mType) {  
    REPORT_UNEXPECTED_TOKEN(PEClassSelNotIdent);
    UngetToken();
    return eSelectorParsingStatus_Error;
  }
  aDataMask |= SEL_MASK_CLASS;

  aSelector.AddClass(mToken.mIdent);

  return eSelectorParsingStatus_Continue;
}





CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParseTypeOrUniversalSelector(int32_t&       aDataMask,
                                            nsCSSSelector& aSelector,
                                            bool           aIsNegated)
{
  nsAutoString buffer;
  if (mToken.IsSymbol('*')) {  
    if (ExpectSymbol('|', false)) {  
      aDataMask |= SEL_MASK_NSPACE;
      aSelector.SetNameSpace(kNameSpaceID_Unknown); 

      if (! GetToken(false)) {
        REPORT_UNEXPECTED_EOF(PETypeSelEOF);
        return eSelectorParsingStatus_Error;
      }
      if (eCSSToken_Ident == mToken.mType) {  
        aDataMask |= SEL_MASK_ELEM;

        aSelector.SetTag(mToken.mIdent);
      }
      else if (mToken.IsSymbol('*')) {  
        aDataMask |= SEL_MASK_ELEM;
        
      }
      else {
        REPORT_UNEXPECTED_TOKEN(PETypeSelNotType);
        UngetToken();
        return eSelectorParsingStatus_Error;
      }
    }
    else {  
      SetDefaultNamespaceOnSelector(aSelector);
      aDataMask |= SEL_MASK_ELEM;
      
    }
    if (! GetToken(false)) {   
      return eSelectorParsingStatus_Done;
    }
  }
  else if (eCSSToken_Ident == mToken.mType) {    
    buffer = mToken.mIdent; 

    if (ExpectSymbol('|', false)) {  
      aDataMask |= SEL_MASK_NSPACE;
      int32_t nameSpaceID = GetNamespaceIdForPrefix(buffer);
      if (nameSpaceID == kNameSpaceID_Unknown) {
        return eSelectorParsingStatus_Error;
      }
      aSelector.SetNameSpace(nameSpaceID);

      if (! GetToken(false)) {
        REPORT_UNEXPECTED_EOF(PETypeSelEOF);
        return eSelectorParsingStatus_Error;
      }
      if (eCSSToken_Ident == mToken.mType) {  
        aDataMask |= SEL_MASK_ELEM;
        aSelector.SetTag(mToken.mIdent);
      }
      else if (mToken.IsSymbol('*')) {  
        aDataMask |= SEL_MASK_ELEM;
        
      }
      else {
        REPORT_UNEXPECTED_TOKEN(PETypeSelNotType);
        UngetToken();
        return eSelectorParsingStatus_Error;
      }
    }
    else {  
      SetDefaultNamespaceOnSelector(aSelector);
      aSelector.SetTag(buffer);

      aDataMask |= SEL_MASK_ELEM;
    }
    if (! GetToken(false)) {   
      return eSelectorParsingStatus_Done;
    }
  }
  else if (mToken.IsSymbol('|')) {  
    aDataMask |= SEL_MASK_NSPACE;
    aSelector.SetNameSpace(kNameSpaceID_None);  

    
    if (! GetToken(false)) {
      REPORT_UNEXPECTED_EOF(PETypeSelEOF);
      return eSelectorParsingStatus_Error;
    }
    if (eCSSToken_Ident == mToken.mType) {  
      aDataMask |= SEL_MASK_ELEM;
      aSelector.SetTag(mToken.mIdent);
    }
    else if (mToken.IsSymbol('*')) {  
      aDataMask |= SEL_MASK_ELEM;
      
    }
    else {
      REPORT_UNEXPECTED_TOKEN(PETypeSelNotType);
      UngetToken();
      return eSelectorParsingStatus_Error;
    }
    if (! GetToken(false)) {   
      return eSelectorParsingStatus_Done;
    }
  }
  else {
    SetDefaultNamespaceOnSelector(aSelector);
  }

  if (aIsNegated) {
    
    UngetToken();
  }
  return eSelectorParsingStatus_Continue;
}





CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParseAttributeSelector(int32_t&       aDataMask,
                                      nsCSSSelector& aSelector)
{
  if (! GetToken(true)) { 
    REPORT_UNEXPECTED_EOF(PEAttributeNameEOF);
    return eSelectorParsingStatus_Error;
  }

  int32_t nameSpaceID = kNameSpaceID_None;
  nsAutoString  attr;
  if (mToken.IsSymbol('*')) { 
    nameSpaceID = kNameSpaceID_Unknown;
    if (ExpectSymbol('|', false)) {
      if (! GetToken(false)) { 
        REPORT_UNEXPECTED_EOF(PEAttributeNameEOF);
        return eSelectorParsingStatus_Error;
      }
      if (eCSSToken_Ident == mToken.mType) { 
        attr = mToken.mIdent;
      }
      else {
        REPORT_UNEXPECTED_TOKEN(PEAttributeNameExpected);
        UngetToken();
        return eSelectorParsingStatus_Error;
       }
    }
    else {
      REPORT_UNEXPECTED_TOKEN(PEAttSelNoBar);
      return eSelectorParsingStatus_Error;
    }
  }
  else if (mToken.IsSymbol('|')) { 
    if (! GetToken(false)) { 
      REPORT_UNEXPECTED_EOF(PEAttributeNameEOF);
      return eSelectorParsingStatus_Error;
    }
    if (eCSSToken_Ident == mToken.mType) { 
      attr = mToken.mIdent;
    }
    else {
      REPORT_UNEXPECTED_TOKEN(PEAttributeNameExpected);
      UngetToken();
      return eSelectorParsingStatus_Error;
    }
  }
  else if (eCSSToken_Ident == mToken.mType) { 
    attr = mToken.mIdent; 
    if (ExpectSymbol('|', false)) {  
      nameSpaceID = GetNamespaceIdForPrefix(attr);
      if (nameSpaceID == kNameSpaceID_Unknown) {
        return eSelectorParsingStatus_Error;
      }
      if (! GetToken(false)) { 
        REPORT_UNEXPECTED_EOF(PEAttributeNameEOF);
        return eSelectorParsingStatus_Error;
      }
      if (eCSSToken_Ident == mToken.mType) { 
        attr = mToken.mIdent;
      }
      else {
        REPORT_UNEXPECTED_TOKEN(PEAttributeNameExpected);
        UngetToken();
        return eSelectorParsingStatus_Error;
      }
    }
  }
  else {  
    REPORT_UNEXPECTED_TOKEN(PEAttributeNameOrNamespaceExpected);
    UngetToken();
    return eSelectorParsingStatus_Error;
  }

  if (! GetToken(true)) { 
    REPORT_UNEXPECTED_EOF(PEAttSelInnerEOF);
    return eSelectorParsingStatus_Error;
  }
  if ((eCSSToken_Symbol == mToken.mType) ||
      (eCSSToken_Includes == mToken.mType) ||
      (eCSSToken_Dashmatch == mToken.mType) ||
      (eCSSToken_Beginsmatch == mToken.mType) ||
      (eCSSToken_Endsmatch == mToken.mType) ||
      (eCSSToken_Containsmatch == mToken.mType)) {
    uint8_t func;
    if (eCSSToken_Includes == mToken.mType) {
      func = NS_ATTR_FUNC_INCLUDES;
    }
    else if (eCSSToken_Dashmatch == mToken.mType) {
      func = NS_ATTR_FUNC_DASHMATCH;
    }
    else if (eCSSToken_Beginsmatch == mToken.mType) {
      func = NS_ATTR_FUNC_BEGINSMATCH;
    }
    else if (eCSSToken_Endsmatch == mToken.mType) {
      func = NS_ATTR_FUNC_ENDSMATCH;
    }
    else if (eCSSToken_Containsmatch == mToken.mType) {
      func = NS_ATTR_FUNC_CONTAINSMATCH;
    }
    else if (']' == mToken.mSymbol) {
      aDataMask |= SEL_MASK_ATTRIB;
      aSelector.AddAttribute(nameSpaceID, attr);
      func = NS_ATTR_FUNC_SET;
    }
    else if ('=' == mToken.mSymbol) {
      func = NS_ATTR_FUNC_EQUALS;
    }
    else {
      REPORT_UNEXPECTED_TOKEN(PEAttSelUnexpected);
      UngetToken(); 
      return eSelectorParsingStatus_Error;
    }
    if (NS_ATTR_FUNC_SET != func) { 
      if (! GetToken(true)) { 
        REPORT_UNEXPECTED_EOF(PEAttSelValueEOF);
        return eSelectorParsingStatus_Error;
      }
      if ((eCSSToken_Ident == mToken.mType) || (eCSSToken_String == mToken.mType)) {
        nsAutoString  value(mToken.mIdent);
        if (! GetToken(true)) { 
          REPORT_UNEXPECTED_EOF(PEAttSelCloseEOF);
          return eSelectorParsingStatus_Error;
        }
        if (mToken.IsSymbol(']')) {
          bool isCaseSensitive = true;

          
          
          
          
          
          if (nameSpaceID == kNameSpaceID_None) {
            static const char* caseInsensitiveHTMLAttribute[] = {
              
              "lang",
              "dir",
              "http-equiv",
              "text",
              "link",
              "vlink",
              "alink",
              "compact",
              "align",
              "frame",
              "rules",
              "valign",
              "scope",
              "axis",
              "nowrap",
              "hreflang",
              "rel",
              "rev",
              "charset",
              "codetype",
              "declare",
              "valuetype",
              "shape",
              "nohref",
              "media",
              "bgcolor",
              "clear",
              "color",
              "face",
              "noshade",
              "noresize",
              "scrolling",
              "target",
              "method",
              "enctype",
              "accept-charset",
              "accept",
              "checked",
              "multiple",
              "selected",
              "disabled",
              "readonly",
              "language",
              "defer",
              "type",
              
              "direction", 
              nullptr
            };
            short i = 0;
            const char* htmlAttr;
            while ((htmlAttr = caseInsensitiveHTMLAttribute[i++])) {
              if (attr.LowerCaseEqualsASCII(htmlAttr)) {
                isCaseSensitive = false;
                break;
              }
            }
          }
          aDataMask |= SEL_MASK_ATTRIB;
          aSelector.AddAttribute(nameSpaceID, attr, func, value, isCaseSensitive);
        }
        else {
          REPORT_UNEXPECTED_TOKEN(PEAttSelNoClose);
          UngetToken();
          return eSelectorParsingStatus_Error;
        }
      }
      else {
        REPORT_UNEXPECTED_TOKEN(PEAttSelBadValue);
        UngetToken();
        return eSelectorParsingStatus_Error;
      }
    }
  }
  else {
    REPORT_UNEXPECTED_TOKEN(PEAttSelUnexpected);
    UngetToken(); 
    return eSelectorParsingStatus_Error;
   }
   return eSelectorParsingStatus_Continue;
}




CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParsePseudoSelector(int32_t&       aDataMask,
                                   nsCSSSelector& aSelector,
                                   bool           aIsNegated,
                                   nsIAtom**      aPseudoElement,
                                   nsAtomList**   aPseudoElementArgs,
                                   nsCSSPseudoElements::Type* aPseudoElementType)
{
  NS_ASSERTION(aIsNegated || (aPseudoElement && aPseudoElementArgs),
               "expected location to store pseudo element");
  NS_ASSERTION(!aIsNegated || (!aPseudoElement && !aPseudoElementArgs),
               "negated selectors shouldn't have a place to store "
               "pseudo elements");
  if (! GetToken(false)) { 
    REPORT_UNEXPECTED_EOF(PEPseudoSelEOF);
    return eSelectorParsingStatus_Error;
  }

  
  bool parsingPseudoElement = false;
  if (mToken.IsSymbol(':')) {
    parsingPseudoElement = true;
    if (! GetToken(false)) { 
      REPORT_UNEXPECTED_EOF(PEPseudoSelEOF);
      return eSelectorParsingStatus_Error;
    }
  }

  
  if (eCSSToken_Ident != mToken.mType && eCSSToken_Function != mToken.mType) {
    
    REPORT_UNEXPECTED_TOKEN(PEPseudoSelBadName);
    UngetToken();
    return eSelectorParsingStatus_Error;
  }

  
  
  nsAutoString buffer;
  buffer.Append(PRUnichar(':'));
  buffer.Append(mToken.mIdent);
  nsContentUtils::ASCIIToLower(buffer);
  nsCOMPtr<nsIAtom> pseudo = do_GetAtom(buffer);
  if (!pseudo) {
    NS_RUNTIMEABORT("do_GetAtom failed - out of memory?");
  }

  
  bool isTreePseudo = false;
  nsCSSPseudoElements::Type pseudoElementType =
    nsCSSPseudoElements::GetPseudoType(pseudo);
  nsCSSPseudoClasses::Type pseudoClassType =
    nsCSSPseudoClasses::GetPseudoType(pseudo);

  
  
  if (pseudoElementType == nsCSSPseudoElements::ePseudo_mozPlaceholder &&
      pseudoClassType == nsCSSPseudoClasses::ePseudoClass_mozPlaceholder) {
    if (parsingPseudoElement) {
      pseudoClassType = nsCSSPseudoClasses::ePseudoClass_NotPseudoClass;
    } else {
      pseudoElementType = nsCSSPseudoElements::ePseudo_NotPseudoElement;
    }
  }

#ifdef MOZ_XUL
  isTreePseudo = (pseudoElementType == nsCSSPseudoElements::ePseudo_XULTree);
  
  
  
  
  
  
  
  bool isTree = (eCSSToken_Function == mToken.mType) && isTreePseudo;
#endif
  bool isPseudoElement =
    (pseudoElementType < nsCSSPseudoElements::ePseudo_PseudoElementCount);
  
  
  bool isAnonBox = isTreePseudo ||
    (pseudoElementType == nsCSSPseudoElements::ePseudo_AnonBox &&
     mUnsafeRulesEnabled);
  bool isPseudoClass =
    (pseudoClassType != nsCSSPseudoClasses::ePseudoClass_NotPseudoClass);

  NS_ASSERTION(!isPseudoClass ||
               pseudoElementType == nsCSSPseudoElements::ePseudo_NotPseudoElement,
               "Why is this atom both a pseudo-class and a pseudo-element?");
  NS_ASSERTION(isPseudoClass + isPseudoElement + isAnonBox <= 1,
               "Shouldn't be more than one of these");

  if (!isPseudoClass && !isPseudoElement && !isAnonBox) {
    
    REPORT_UNEXPECTED_TOKEN(PEPseudoSelUnknown);
    UngetToken();
    return eSelectorParsingStatus_Error;
  }

  
  
  if ((eCSSToken_Function == mToken.mType) !=
      (
#ifdef MOZ_XUL
       isTree ||
#endif
       nsCSSPseudoClasses::ePseudoClass_notPseudo == pseudoClassType ||
       nsCSSPseudoClasses::HasStringArg(pseudoClassType) ||
       nsCSSPseudoClasses::HasNthPairArg(pseudoClassType) ||
       nsCSSPseudoClasses::HasSelectorListArg(pseudoClassType))) {
    
    REPORT_UNEXPECTED_TOKEN(PEPseudoSelNonFunc);
    UngetToken();
    return eSelectorParsingStatus_Error;
  }

  
  if (parsingPseudoElement &&
      !isPseudoElement &&
      !isAnonBox) {
    REPORT_UNEXPECTED_TOKEN(PEPseudoSelNotPE);
    UngetToken();
    return eSelectorParsingStatus_Error;
  }

  if (!parsingPseudoElement &&
      nsCSSPseudoClasses::ePseudoClass_notPseudo == pseudoClassType) {
    if (aIsNegated) { 
      REPORT_UNEXPECTED_TOKEN(PEPseudoSelDoubleNot);
      UngetToken();
      return eSelectorParsingStatus_Error;
    }
    
    nsSelectorParsingStatus parsingStatus =
      ParseNegatedSimpleSelector(aDataMask, aSelector);
    if (eSelectorParsingStatus_Continue != parsingStatus) {
      return parsingStatus;
    }
  }
  else if (!parsingPseudoElement && isPseudoClass) {
    aDataMask |= SEL_MASK_PCLASS;
    if (eCSSToken_Function == mToken.mType) {
      nsSelectorParsingStatus parsingStatus;
      if (nsCSSPseudoClasses::HasStringArg(pseudoClassType)) {
        parsingStatus =
          ParsePseudoClassWithIdentArg(aSelector, pseudoClassType);
      }
      else if (nsCSSPseudoClasses::HasNthPairArg(pseudoClassType)) {
        parsingStatus =
          ParsePseudoClassWithNthPairArg(aSelector, pseudoClassType);
      }
      else {
        NS_ABORT_IF_FALSE(nsCSSPseudoClasses::HasSelectorListArg(pseudoClassType),
                          "unexpected pseudo with function token");
        parsingStatus = ParsePseudoClassWithSelectorListArg(aSelector,
                                                            pseudoClassType);
      }
      if (eSelectorParsingStatus_Continue != parsingStatus) {
        if (eSelectorParsingStatus_Error == parsingStatus) {
          SkipUntil(')');
        }
        return parsingStatus;
      }
    }
    else {
      aSelector.AddPseudoClass(pseudoClassType);
    }
  }
  else if (isPseudoElement || isAnonBox) {
    

    if (aIsNegated) { 
      REPORT_UNEXPECTED_TOKEN(PEPseudoSelPEInNot);
      UngetToken();
      return eSelectorParsingStatus_Error;
    }
    
    
    
    
    if (!parsingPseudoElement &&
        !nsCSSPseudoElements::IsCSS2PseudoElement(pseudo)
#ifdef MOZ_XUL
        && !isTreePseudo
#endif
        ) {
      REPORT_UNEXPECTED_TOKEN(PEPseudoSelNewStyleOnly);
      UngetToken();
      return eSelectorParsingStatus_Error;
    }

    if (0 == (aDataMask & SEL_MASK_PELEM)) {
      aDataMask |= SEL_MASK_PELEM;
      NS_ADDREF(*aPseudoElement = pseudo);
      *aPseudoElementType = pseudoElementType;

#ifdef MOZ_XUL
      if (isTree) {
        
        
        
        
        if (!ParseTreePseudoElement(aPseudoElementArgs)) {
          return eSelectorParsingStatus_Error;
        }
      }
#endif

      
      if (!GetToken(true)) { 
        return eSelectorParsingStatus_Done;
      }
      if ((mToken.IsSymbol('{') || mToken.IsSymbol(','))) {
        UngetToken();
        return eSelectorParsingStatus_Done;
      }
      REPORT_UNEXPECTED_TOKEN(PEPseudoSelTrailing);
      UngetToken();
      return eSelectorParsingStatus_Error;
    }
    else {  
      REPORT_UNEXPECTED_TOKEN(PEPseudoSelMultiplePE);
      UngetToken();
      return eSelectorParsingStatus_Error;
    }
  }
#ifdef DEBUG
  else {
    
    
    
    
    
    
    
    NS_NOTREACHED("How did this happen?");
  }
#endif
  return eSelectorParsingStatus_Continue;
}




CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParseNegatedSimpleSelector(int32_t&       aDataMask,
                                          nsCSSSelector& aSelector)
{
  if (! GetToken(true)) { 
    REPORT_UNEXPECTED_EOF(PENegationEOF);
    return eSelectorParsingStatus_Error;
  }

  if (mToken.IsSymbol(')')) {
    REPORT_UNEXPECTED_TOKEN(PENegationBadArg);
    return eSelectorParsingStatus_Error;
  }

  
  
  
  
  
  
  
  nsCSSSelector *newSel = new nsCSSSelector();
  nsCSSSelector* negations = &aSelector;
  while (negations->mNegations) {
    negations = negations->mNegations;
  }
  negations->mNegations = newSel;

  nsSelectorParsingStatus parsingStatus;
  if (eCSSToken_ID == mToken.mType) { 
    parsingStatus = ParseIDSelector(aDataMask, *newSel);
  }
  else if (mToken.IsSymbol('.')) {    
    parsingStatus = ParseClassSelector(aDataMask, *newSel);
  }
  else if (mToken.IsSymbol(':')) {    
    parsingStatus = ParsePseudoSelector(aDataMask, *newSel, true,
                                        nullptr, nullptr, nullptr);
  }
  else if (mToken.IsSymbol('[')) {    
    parsingStatus = ParseAttributeSelector(aDataMask, *newSel);
    if (eSelectorParsingStatus_Error == parsingStatus) {
      
      SkipUntil(']');
    }
  }
  else {
    
    parsingStatus = ParseTypeOrUniversalSelector(aDataMask, *newSel, true);
  }
  if (eSelectorParsingStatus_Error == parsingStatus) {
    REPORT_UNEXPECTED_TOKEN(PENegationBadInner);
    SkipUntil(')');
    return parsingStatus;
  }
  
  if (!ExpectSymbol(')', true)) {
    REPORT_UNEXPECTED_TOKEN(PENegationNoClose);
    SkipUntil(')');
    return eSelectorParsingStatus_Error;
  }

  NS_ASSERTION(newSel->mNameSpace == kNameSpaceID_Unknown ||
               (!newSel->mIDList && !newSel->mClassList &&
                !newSel->mPseudoClassList && !newSel->mAttrList),
               "Need to fix the serialization code to deal with this");

  return eSelectorParsingStatus_Continue;
}




CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParsePseudoClassWithIdentArg(nsCSSSelector& aSelector,
                                            nsCSSPseudoClasses::Type aType)
{
  if (! GetToken(true)) { 
    REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
    return eSelectorParsingStatus_Error;
  }
  
  if (eCSSToken_Ident != mToken.mType) {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotIdent);
    UngetToken();
    return eSelectorParsingStatus_Error; 
  }

  
  if (aType == nsCSSPseudoClasses::ePseudoClass_mozLocaleDir ||
      aType == nsCSSPseudoClasses::ePseudoClass_dir) {
    nsContentUtils::ASCIIToLower(mToken.mIdent); 
    if (!mToken.mIdent.EqualsLiteral("ltr") &&
        !mToken.mIdent.EqualsLiteral("rtl")) {
      REPORT_UNEXPECTED_TOKEN(PEBadDirValue);
      return eSelectorParsingStatus_Error; 
    }
  }

  
  aSelector.AddPseudoClass(aType, mToken.mIdent.get());

  
  if (!ExpectSymbol(')', true)) {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassNoClose);
    return eSelectorParsingStatus_Error; 
  }

  return eSelectorParsingStatus_Continue;
}

CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParsePseudoClassWithNthPairArg(nsCSSSelector& aSelector,
                                              nsCSSPseudoClasses::Type aType)
{
  int32_t numbers[2] = { 0, 0 };
  bool lookForB = true;

  
  

  if (! GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
    return eSelectorParsingStatus_Error;
  }

  if (eCSSToken_Ident == mToken.mType || eCSSToken_Dimension == mToken.mType) {
    
    
    
    
    
    uint32_t truncAt = 0;
    if (StringBeginsWith(mToken.mIdent, NS_LITERAL_STRING("n-"))) {
      truncAt = 1;
    } else if (StringBeginsWith(mToken.mIdent, NS_LITERAL_STRING("-n-"))) {
      truncAt = 2;
    }
    if (truncAt != 0) {
      mScanner->Backup(mToken.mIdent.Length() - truncAt);
      mToken.mIdent.Truncate(truncAt);
    }
  }

  if (eCSSToken_Ident == mToken.mType) {
    if (mToken.mIdent.LowerCaseEqualsLiteral("odd")) {
      numbers[0] = 2;
      numbers[1] = 1;
      lookForB = false;
    }
    else if (mToken.mIdent.LowerCaseEqualsLiteral("even")) {
      numbers[0] = 2;
      numbers[1] = 0;
      lookForB = false;
    }
    else if (mToken.mIdent.LowerCaseEqualsLiteral("n")) {
      numbers[0] = 1;
    }
    else if (mToken.mIdent.LowerCaseEqualsLiteral("-n")) {
      numbers[0] = -1;
    }
    else {
      REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
      return eSelectorParsingStatus_Error; 
    }
  }
  else if (eCSSToken_Number == mToken.mType) {
    if (!mToken.mIntegerValid) {
      REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
      return eSelectorParsingStatus_Error; 
    }
    numbers[1] = mToken.mInteger;
    lookForB = false;
  }
  else if (eCSSToken_Dimension == mToken.mType) {
    if (!mToken.mIntegerValid || !mToken.mIdent.LowerCaseEqualsLiteral("n")) {
      REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
      return eSelectorParsingStatus_Error; 
    }
    numbers[0] = mToken.mInteger;
  }
  
  else {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
    UngetToken();
    return eSelectorParsingStatus_Error; 
  }

  if (! GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
    return eSelectorParsingStatus_Error;
  }
  if (lookForB && !mToken.IsSymbol(')')) {
    
    
    
    bool haveSign = false;
    int32_t sign = 1;
    if (mToken.IsSymbol('+') || mToken.IsSymbol('-')) {
      haveSign = true;
      if (mToken.IsSymbol('-')) {
        sign = -1;
      }
      if (! GetToken(true)) {
        REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
        return eSelectorParsingStatus_Error;
      }
    }
    if (eCSSToken_Number != mToken.mType ||
        !mToken.mIntegerValid || mToken.mHasSign == haveSign) {
      REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
      return eSelectorParsingStatus_Error; 
    }
    numbers[1] = mToken.mInteger * sign;
    if (! GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
      return eSelectorParsingStatus_Error;
    }
  }
  if (!mToken.IsSymbol(')')) {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassNoClose);
    return eSelectorParsingStatus_Error; 
  }
  aSelector.AddPseudoClass(aType, numbers);
  return eSelectorParsingStatus_Continue;
}






CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParsePseudoClassWithSelectorListArg(nsCSSSelector& aSelector,
                                                   nsCSSPseudoClasses::Type aType)
{
  nsAutoPtr<nsCSSSelectorList> slist;
  if (! ParseSelectorList(*getter_Transfers(slist), PRUnichar(')'))) {
    return eSelectorParsingStatus_Error; 
  }

  
  
  for (nsCSSSelectorList *l = slist; l; l = l->mNext) {
    nsCSSSelector *s = l->mSelectors;
    if (s->mNext || s->IsPseudoElement()) {
      return eSelectorParsingStatus_Error; 
    }
  }

  
  aSelector.AddPseudoClass(aType, slist.forget());

  
  if (!ExpectSymbol(')', true)) {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassNoClose);
    return eSelectorParsingStatus_Error; 
  }

  return eSelectorParsingStatus_Continue;
}






bool
CSSParserImpl::ParseSelector(nsCSSSelectorList* aList,
                             PRUnichar aPrevCombinator)
{
  if (! GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PESelectorEOF);
    return false;
  }

  nsCSSSelector* selector = aList->AddSelector(aPrevCombinator);
  nsCOMPtr<nsIAtom> pseudoElement;
  nsAutoPtr<nsAtomList> pseudoElementArgs;
  nsCSSPseudoElements::Type pseudoElementType =
    nsCSSPseudoElements::ePseudo_NotPseudoElement;

  int32_t dataMask = 0;
  nsSelectorParsingStatus parsingStatus =
    ParseTypeOrUniversalSelector(dataMask, *selector, false);

  while (parsingStatus == eSelectorParsingStatus_Continue) {
    if (eCSSToken_ID == mToken.mType) { 
      parsingStatus = ParseIDSelector(dataMask, *selector);
    }
    else if (mToken.IsSymbol('.')) {    
      parsingStatus = ParseClassSelector(dataMask, *selector);
    }
    else if (mToken.IsSymbol(':')) {    
      parsingStatus = ParsePseudoSelector(dataMask, *selector, false,
                                          getter_AddRefs(pseudoElement),
                                          getter_Transfers(pseudoElementArgs),
                                          &pseudoElementType);
    }
    else if (mToken.IsSymbol('[')) {    
      parsingStatus = ParseAttributeSelector(dataMask, *selector);
      if (eSelectorParsingStatus_Error == parsingStatus) {
        SkipUntil(']');
      }
    }
    else {  
      parsingStatus = eSelectorParsingStatus_Done;
      UngetToken();
      break;
    }

    if (parsingStatus != eSelectorParsingStatus_Continue) {
      break;
    }

    if (! GetToken(false)) { 
      parsingStatus = eSelectorParsingStatus_Done;
      break;
    }
  }

  if (parsingStatus == eSelectorParsingStatus_Error) {
    return false;
  }

  if (!dataMask) {
    if (selector->mNext) {
      REPORT_UNEXPECTED(PESelectorGroupExtraCombinator);
    } else {
      REPORT_UNEXPECTED(PESelectorGroupNoSelector);
    }
    return false;
  }

  if (pseudoElementType == nsCSSPseudoElements::ePseudo_AnonBox) {
    
    
    if (selector->mNext || !IsUniversalSelector(*selector)) {
      REPORT_UNEXPECTED(PEAnonBoxNotAlone);
      return false;
    }

    
    
    selector->mLowercaseTag.swap(pseudoElement);
    selector->mClassList = pseudoElementArgs.forget();
    selector->SetPseudoType(pseudoElementType);
    return true;
  }

  aList->mWeight += selector->CalcWeight();

  
  
  if (pseudoElement) {
    selector = aList->AddSelector('>');

    selector->mLowercaseTag.swap(pseudoElement);
    selector->mClassList = pseudoElementArgs.forget();
    selector->SetPseudoType(pseudoElementType);
  }

  return true;
}

css::Declaration*
CSSParserImpl::ParseDeclarationBlock(uint32_t aFlags, nsCSSContextType aContext)
{
  bool checkForBraces = (aFlags & eParseDeclaration_InBraces) != 0;

  if (checkForBraces) {
    if (!ExpectSymbol('{', true)) {
      REPORT_UNEXPECTED_TOKEN(PEBadDeclBlockStart);
      OUTPUT_ERROR();
      return nullptr;
    }
  }
  css::Declaration* declaration = new css::Declaration();
  mData.AssertInitialState();
  if (declaration) {
    for (;;) {
      bool changed;
      if (!ParseDeclaration(declaration, aFlags, true, &changed, aContext)) {
        if (!SkipDeclaration(checkForBraces)) {
          break;
        }
        if (checkForBraces) {
          if (ExpectSymbol('}', true)) {
            break;
          }
        }
        
        
      }
    }
    declaration->CompressFrom(&mData);
  }
  return declaration;
}



#define COLOR_TYPE_UNKNOWN 0
#define COLOR_TYPE_INTEGERS 1
#define COLOR_TYPE_PERCENTAGES 2

bool
CSSParserImpl::ParseColor(nsCSSValue& aValue)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEColorEOF);
    return false;
  }

  nsCSSToken* tk = &mToken;
  nscolor rgba;
  switch (tk->mType) {
    case eCSSToken_ID:
    case eCSSToken_Hash:
      
      if (NS_HexToRGB(tk->mIdent, &rgba)) {
        aValue.SetColorValue(rgba);
        return true;
      }
      break;

    case eCSSToken_Ident:
      if (NS_ColorNameToRGB(tk->mIdent, &rgba)) {
        aValue.SetStringValue(tk->mIdent, eCSSUnit_Ident);
        return true;
      }
      else {
        nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(tk->mIdent);
        if (eCSSKeyword_UNKNOWN < keyword) { 
          int32_t value;
          if (nsCSSProps::FindKeyword(keyword, nsCSSProps::kColorKTable, value)) {
            aValue.SetIntValue(value, eCSSUnit_EnumColor);
            return true;
          }
        }
      }
      break;
    case eCSSToken_Function:
      if (mToken.mIdent.LowerCaseEqualsLiteral("rgb")) {
        
        uint8_t r, g, b;
        int32_t type = COLOR_TYPE_UNKNOWN;
        if (ParseColorComponent(r, type, ',') &&
            ParseColorComponent(g, type, ',') &&
            ParseColorComponent(b, type, ')')) {
          aValue.SetColorValue(NS_RGB(r,g,b));
          return true;
        }
        SkipUntil(')');
        return false;
      }
      else if (mToken.mIdent.LowerCaseEqualsLiteral("-moz-rgba") ||
               mToken.mIdent.LowerCaseEqualsLiteral("rgba")) {
        
        uint8_t r, g, b, a;
        int32_t type = COLOR_TYPE_UNKNOWN;
        if (ParseColorComponent(r, type, ',') &&
            ParseColorComponent(g, type, ',') &&
            ParseColorComponent(b, type, ',') &&
            ParseColorOpacity(a)) {
          aValue.SetColorValue(NS_RGBA(r, g, b, a));
          return true;
        }
        SkipUntil(')');
        return false;
      }
      else if (mToken.mIdent.LowerCaseEqualsLiteral("hsl")) {
        
        
        if (ParseHSLColor(rgba, ')')) {
          aValue.SetColorValue(rgba);
          return true;
        }
        SkipUntil(')');
        return false;
      }
      else if (mToken.mIdent.LowerCaseEqualsLiteral("-moz-hsla") ||
               mToken.mIdent.LowerCaseEqualsLiteral("hsla")) {
        
        
        
        uint8_t a;
        if (ParseHSLColor(rgba, ',') &&
            ParseColorOpacity(a)) {
          aValue.SetColorValue(NS_RGBA(NS_GET_R(rgba), NS_GET_G(rgba),
                                       NS_GET_B(rgba), a));
          return true;
        }
        SkipUntil(')');
        return false;
      }
      break;
    default:
      break;
  }

  
  if (mHashlessColorQuirk) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsAutoString str;
    char buffer[20];
    switch (tk->mType) {
      case eCSSToken_Ident:
        str.Assign(tk->mIdent);
        break;

      case eCSSToken_Number:
        if (tk->mIntegerValid) {
          PR_snprintf(buffer, sizeof(buffer), "%06d", tk->mInteger);
          str.AssignWithConversion(buffer);
        }
        break;

      case eCSSToken_Dimension:
        if (tk->mIdent.Length() <= 6) {
          PR_snprintf(buffer, sizeof(buffer), "%06.0f", tk->mNumber);
          nsAutoString temp;
          temp.AssignWithConversion(buffer);
          temp.Right(str, 6 - tk->mIdent.Length());
          str.Append(tk->mIdent);
        }
        break;
      default:
        
        
        break;
    }
    if (NS_HexToRGB(str, &rgba)) {
      aValue.SetColorValue(rgba);
      return true;
    }
  }

  
  REPORT_UNEXPECTED_TOKEN(PEColorNotColor);
  UngetToken();
  return false;
}



bool
CSSParserImpl::ParseColorComponent(uint8_t& aComponent,
                                   int32_t& aType,
                                   char aStop)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEColorComponentEOF);
    return false;
  }
  float value;
  nsCSSToken* tk = &mToken;
  switch (tk->mType) {
  case eCSSToken_Number:
    switch (aType) {
      case COLOR_TYPE_UNKNOWN:
        aType = COLOR_TYPE_INTEGERS;
        break;
      case COLOR_TYPE_INTEGERS:
        break;
      case COLOR_TYPE_PERCENTAGES:
        REPORT_UNEXPECTED_TOKEN(PEExpectedPercent);
        UngetToken();
        return false;
      default:
        NS_NOTREACHED("Someone forgot to add the new color component type in here");
    }

    if (!mToken.mIntegerValid) {
      REPORT_UNEXPECTED_TOKEN(PEExpectedInt);
      UngetToken();
      return false;
    }
    value = tk->mNumber;
    break;
  case eCSSToken_Percentage:
    switch (aType) {
      case COLOR_TYPE_UNKNOWN:
        aType = COLOR_TYPE_PERCENTAGES;
        break;
      case COLOR_TYPE_INTEGERS:
        REPORT_UNEXPECTED_TOKEN(PEExpectedInt);
        UngetToken();
        return false;
      case COLOR_TYPE_PERCENTAGES:
        break;
      default:
        NS_NOTREACHED("Someone forgot to add the new color component type in here");
    }
    value = tk->mNumber * 255.0f;
    break;
  default:
    REPORT_UNEXPECTED_TOKEN(PEColorBadRGBContents);
    UngetToken();
    return false;
  }
  if (ExpectSymbol(aStop, true)) {
    if (value < 0.0f) value = 0.0f;
    if (value > 255.0f) value = 255.0f;
    aComponent = NSToIntRound(value);
    return true;
  }
  REPORT_UNEXPECTED_TOKEN_CHAR(PEColorComponentBadTerm, aStop);
  return false;
}


bool
CSSParserImpl::ParseHSLColor(nscolor& aColor,
                             char aStop)
{
  float h, s, l;

  
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEColorHueEOF);
    return false;
  }
  if (mToken.mType != eCSSToken_Number) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedNumber);
    UngetToken();
    return false;
  }
  h = mToken.mNumber;
  h /= 360.0f;
  
  h = h - floor(h);

  if (!ExpectSymbol(',', true)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedComma);
    return false;
  }

  
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEColorSaturationEOF);
    return false;
  }
  if (mToken.mType != eCSSToken_Percentage) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedPercent);
    UngetToken();
    return false;
  }
  s = mToken.mNumber;
  if (s < 0.0f) s = 0.0f;
  if (s > 1.0f) s = 1.0f;

  if (!ExpectSymbol(',', true)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedComma);
    return false;
  }

  
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEColorLightnessEOF);
    return false;
  }
  if (mToken.mType != eCSSToken_Percentage) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedPercent);
    UngetToken();
    return false;
  }
  l = mToken.mNumber;
  if (l < 0.0f) l = 0.0f;
  if (l > 1.0f) l = 1.0f;

  if (ExpectSymbol(aStop, true)) {
    aColor = NS_HSL2RGB(h, s, l);
    return true;
  }

  REPORT_UNEXPECTED_TOKEN_CHAR(PEColorComponentBadTerm, aStop);
  return false;
}


bool
CSSParserImpl::ParseColorOpacity(uint8_t& aOpacity)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEColorOpacityEOF);
    return false;
  }

  if (mToken.mType != eCSSToken_Number) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedNumber);
    UngetToken();
    return false;
  }

  if (mToken.mNumber < 0.0f) {
    mToken.mNumber = 0.0f;
  } else if (mToken.mNumber > 1.0f) {
    mToken.mNumber = 1.0f;
  }

  uint8_t value = nsStyleUtil::FloatToColorComponent(mToken.mNumber);
  
  
  NS_ASSERTION(fabs(255.0f*mToken.mNumber - value) <= 0.51f,
               "FloatToColorComponent did something weird");

  if (!ExpectSymbol(')', true)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedCloseParen);
    return false;
  }

  aOpacity = value;

  return true;
}

#ifdef MOZ_XUL
bool
CSSParserImpl::ParseTreePseudoElement(nsAtomList **aPseudoElementArgs)
{
  
  
  
  nsCSSSelector fakeSelector; 

  while (!ExpectSymbol(')', true)) {
    if (!GetToken(true)) {
      return false;
    }
    if (eCSSToken_Ident == mToken.mType) {
      fakeSelector.AddClass(mToken.mIdent);
    }
    else if (!mToken.IsSymbol(',')) {
      UngetToken();
      SkipUntil(')');
      return false;
    }
  }
  *aPseudoElementArgs = fakeSelector.mClassList;
  fakeSelector.mClassList = nullptr;
  return true;
}
#endif



bool
CSSParserImpl::ParseDeclaration(css::Declaration* aDeclaration,
                                uint32_t aFlags,
                                bool aMustCallValueAppended,
                                bool* aChanged,
                                nsCSSContextType aContext)
{
  NS_PRECONDITION(aContext == eCSSContext_General ||
                  aContext == eCSSContext_Page,
                  "Must be page or general context");
  bool checkForBraces = (aFlags & eParseDeclaration_InBraces) != 0;

  mTempData.AssertInitialState();

  
  nsCSSToken* tk = &mToken;
  nsAutoString propertyName;
  for (;;) {
    if (!GetToken(true)) {
      if (checkForBraces) {
        REPORT_UNEXPECTED_EOF(PEDeclEndEOF);
      }
      return false;
    }
    if (eCSSToken_Ident == tk->mType) {
      propertyName = tk->mIdent;
      
      if (!ExpectSymbol(':', true)) {
        REPORT_UNEXPECTED_TOKEN(PEParseDeclarationNoColon);
        REPORT_UNEXPECTED(PEDeclDropped);
        OUTPUT_ERROR();
        return false;
      }
      break;
    }
    if (tk->IsSymbol(';')) {
      
      continue;
    }

    if (!tk->IsSymbol('}')) {
      REPORT_UNEXPECTED_TOKEN(PEParseDeclarationDeclExpected);
      REPORT_UNEXPECTED(PEDeclSkipped);
      OUTPUT_ERROR();
    }
    
    UngetToken();
    return false;
  }

  
  
  nsAutoSuppressErrors suppressErrors(this, mInFailingSupportsRule);

  
  nsCSSProperty propID = nsCSSProps::LookupProperty(propertyName,
                                                    nsCSSProps::eEnabled);
  if (eCSSProperty_UNKNOWN == propID ||
     (aContext == eCSSContext_Page &&
      !nsCSSProps::PropHasFlags(propID, CSS_PROPERTY_APPLIES_TO_PAGE_RULE))) { 
    if (!NonMozillaVendorIdentifier(propertyName)) {
      REPORT_UNEXPECTED_P(PEUnknownProperty, propertyName);
      REPORT_UNEXPECTED(PEDeclDropped);
      OUTPUT_ERROR();
    }

    return false;
  }
  if (! ParseProperty(propID)) {
    
    REPORT_UNEXPECTED_P(PEValueParsingError, propertyName);
    REPORT_UNEXPECTED(PEDeclDropped);
    OUTPUT_ERROR();
    mTempData.ClearProperty(propID);
    mTempData.AssertInitialState();
    return false;
  }
  CLEAR_ERROR();

  
  PriorityParsingStatus status;
  if ((aFlags & eParseDeclaration_AllowImportant) != 0) {
    status = ParsePriority();
  }
  else {
    status = ePriority_None;
  }

  
  if (status != ePriority_Error) {
    if (!GetToken(true)) {
      
    } else if (mToken.IsSymbol(';')) {
      
    } else if (mToken.IsSymbol('}')) {
      
      UngetToken();
      if (!checkForBraces) {
        status = ePriority_Error;
      }
    } else {
      UngetToken();
      status = ePriority_Error;
    }
  }

  if (status == ePriority_Error) {
    if (checkForBraces) {
      REPORT_UNEXPECTED_TOKEN(PEBadDeclOrRuleEnd2);
    } else {
      REPORT_UNEXPECTED_TOKEN(PEBadDeclEnd);
    }
    REPORT_UNEXPECTED(PEDeclDropped);
    OUTPUT_ERROR();
    mTempData.ClearProperty(propID);
    mTempData.AssertInitialState();
    return false;
  }

  *aChanged |= mData.TransferFromBlock(mTempData, propID,
                                       status == ePriority_Important,
                                       false, aMustCallValueAppended,
                                       aDeclaration);
  return true;
}

static const nsCSSProperty kBorderTopIDs[] = {
  eCSSProperty_border_top_width,
  eCSSProperty_border_top_style,
  eCSSProperty_border_top_color
};
static const nsCSSProperty kBorderRightIDs[] = {
  eCSSProperty_border_right_width_value,
  eCSSProperty_border_right_style_value,
  eCSSProperty_border_right_color_value,
  eCSSProperty_border_right_width,
  eCSSProperty_border_right_style,
  eCSSProperty_border_right_color
};
static const nsCSSProperty kBorderBottomIDs[] = {
  eCSSProperty_border_bottom_width,
  eCSSProperty_border_bottom_style,
  eCSSProperty_border_bottom_color
};
static const nsCSSProperty kBorderLeftIDs[] = {
  eCSSProperty_border_left_width_value,
  eCSSProperty_border_left_style_value,
  eCSSProperty_border_left_color_value,
  eCSSProperty_border_left_width,
  eCSSProperty_border_left_style,
  eCSSProperty_border_left_color
};
static const nsCSSProperty kBorderStartIDs[] = {
  eCSSProperty_border_start_width_value,
  eCSSProperty_border_start_style_value,
  eCSSProperty_border_start_color_value,
  eCSSProperty_border_start_width,
  eCSSProperty_border_start_style,
  eCSSProperty_border_start_color
};
static const nsCSSProperty kBorderEndIDs[] = {
  eCSSProperty_border_end_width_value,
  eCSSProperty_border_end_style_value,
  eCSSProperty_border_end_color_value,
  eCSSProperty_border_end_width,
  eCSSProperty_border_end_style,
  eCSSProperty_border_end_color
};
static const nsCSSProperty kColumnRuleIDs[] = {
  eCSSProperty__moz_column_rule_width,
  eCSSProperty__moz_column_rule_style,
  eCSSProperty__moz_column_rule_color
};

bool
CSSParserImpl::ParseEnum(nsCSSValue& aValue,
                         const int32_t aKeywordTable[])
{
  nsSubstring* ident = NextIdent();
  if (nullptr == ident) {
    return false;
  }
  nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(*ident);
  if (eCSSKeyword_UNKNOWN < keyword) {
    int32_t value;
    if (nsCSSProps::FindKeyword(keyword, aKeywordTable, value)) {
      aValue.SetIntValue(value, eCSSUnit_Enumerated);
      return true;
    }
  }

  
  UngetToken();
  return false;
}


struct UnitInfo {
  char name[6];  
                 
  uint32_t length;
  nsCSSUnit unit;
  int32_t type;
};

#define STR_WITH_LEN(_str) \
  _str, sizeof(_str) - 1

const UnitInfo UnitData[] = {
  { STR_WITH_LEN("px"), eCSSUnit_Pixel, VARIANT_LENGTH },
  { STR_WITH_LEN("em"), eCSSUnit_EM, VARIANT_LENGTH },
  { STR_WITH_LEN("ex"), eCSSUnit_XHeight, VARIANT_LENGTH },
  { STR_WITH_LEN("pt"), eCSSUnit_Point, VARIANT_LENGTH },
  { STR_WITH_LEN("in"), eCSSUnit_Inch, VARIANT_LENGTH },
  { STR_WITH_LEN("cm"), eCSSUnit_Centimeter, VARIANT_LENGTH },
  { STR_WITH_LEN("ch"), eCSSUnit_Char, VARIANT_LENGTH },
  { STR_WITH_LEN("rem"), eCSSUnit_RootEM, VARIANT_LENGTH },
  { STR_WITH_LEN("mm"), eCSSUnit_Millimeter, VARIANT_LENGTH },
  { STR_WITH_LEN("mozmm"), eCSSUnit_PhysicalMillimeter, VARIANT_LENGTH },
  { STR_WITH_LEN("vw"), eCSSUnit_ViewportWidth, VARIANT_LENGTH },
  { STR_WITH_LEN("vh"), eCSSUnit_ViewportHeight, VARIANT_LENGTH },
  { STR_WITH_LEN("vmin"), eCSSUnit_ViewportMin, VARIANT_LENGTH },
  { STR_WITH_LEN("vmax"), eCSSUnit_ViewportMax, VARIANT_LENGTH },
  { STR_WITH_LEN("pc"), eCSSUnit_Pica, VARIANT_LENGTH },
  { STR_WITH_LEN("deg"), eCSSUnit_Degree, VARIANT_ANGLE },
  { STR_WITH_LEN("grad"), eCSSUnit_Grad, VARIANT_ANGLE },
  { STR_WITH_LEN("rad"), eCSSUnit_Radian, VARIANT_ANGLE },
  { STR_WITH_LEN("turn"), eCSSUnit_Turn, VARIANT_ANGLE },
  { STR_WITH_LEN("hz"), eCSSUnit_Hertz, VARIANT_FREQUENCY },
  { STR_WITH_LEN("khz"), eCSSUnit_Kilohertz, VARIANT_FREQUENCY },
  { STR_WITH_LEN("s"), eCSSUnit_Seconds, VARIANT_TIME },
  { STR_WITH_LEN("ms"), eCSSUnit_Milliseconds, VARIANT_TIME }
};

#undef STR_WITH_LEN

bool
CSSParserImpl::TranslateDimension(nsCSSValue& aValue,
                                  int32_t aVariantMask,
                                  float aNumber,
                                  const nsString& aUnit)
{
  nsCSSUnit units;
  int32_t   type = 0;
  if (!aUnit.IsEmpty()) {
    uint32_t i;
    for (i = 0; i < ArrayLength(UnitData); ++i) {
      if (aUnit.LowerCaseEqualsASCII(UnitData[i].name,
                                     UnitData[i].length)) {
        units = UnitData[i].unit;
        type = UnitData[i].type;
        break;
      }
    }

    if (!mViewportUnitsEnabled &&
        (eCSSUnit_ViewportWidth == units  ||
         eCSSUnit_ViewportHeight == units ||
         eCSSUnit_ViewportMin == units    ||
         eCSSUnit_ViewportMax == units)) {
      
      
      return false;
    }

    if (i == ArrayLength(UnitData)) {
      
      return false;
    }
  } else {
    
    NS_ASSERTION(0 == aNumber, "numbers without units must be 0");
    if ((VARIANT_LENGTH & aVariantMask) != 0) {
      units = eCSSUnit_Pixel;
      type = VARIANT_LENGTH;
    }
    else if ((VARIANT_ANGLE & aVariantMask) != 0) {
      NS_ASSERTION(aVariantMask & VARIANT_ZERO_ANGLE,
                   "must have allowed zero angle");
      units = eCSSUnit_Degree;
      type = VARIANT_ANGLE;
    }
    else {
      NS_ERROR("Variant mask does not include dimension; why were we called?");
      return false;
    }
  }
  if ((type & aVariantMask) != 0) {
    aValue.SetFloatValue(aNumber, units);
    return true;
  }
  return false;
}





#define VARIANT_ALL_NONNUMERIC \
  VARIANT_KEYWORD | \
  VARIANT_COLOR | \
  VARIANT_URL | \
  VARIANT_STRING | \
  VARIANT_COUNTER | \
  VARIANT_ATTR | \
  VARIANT_IDENTIFIER | \
  VARIANT_IDENTIFIER_NO_INHERIT | \
  VARIANT_AUTO | \
  VARIANT_INHERIT | \
  VARIANT_NONE | \
  VARIANT_NORMAL | \
  VARIANT_SYSFONT | \
  VARIANT_GRADIENT | \
  VARIANT_TIMING_FUNCTION | \
  VARIANT_ALL | \
  VARIANT_CALC





bool
CSSParserImpl::ParseNonNegativeVariant(nsCSSValue& aValue,
                                       int32_t aVariantMask,
                                       const int32_t aKeywordTable[])
{
  
  
  NS_ABORT_IF_FALSE((aVariantMask & ~(VARIANT_ALL_NONNUMERIC |
                                      VARIANT_NUMBER |
                                      VARIANT_LENGTH |
                                      VARIANT_PERCENT |
                                      VARIANT_INTEGER)) == 0,
                    "need to update code below to handle additional variants");

  if (ParseVariant(aValue, aVariantMask, aKeywordTable)) {
    if (eCSSUnit_Number == aValue.GetUnit() ||
        aValue.IsLengthUnit()){
      if (aValue.GetFloatValue() < 0) {
        UngetToken();
        return false;
      }
    }
    else if (aValue.GetUnit() == eCSSUnit_Percent) {
      if (aValue.GetPercentValue() < 0) {
        UngetToken();
        return false;
      }
    } else if (aValue.GetUnit() == eCSSUnit_Integer) {
      if (aValue.GetIntValue() < 0) {
        UngetToken();
        return false;
      }
    }
    return true;
  }
  return false;
}





bool
CSSParserImpl::ParseOneOrLargerVariant(nsCSSValue& aValue,
                                       int32_t aVariantMask,
                                       const int32_t aKeywordTable[])
{
  
  
  NS_ABORT_IF_FALSE((aVariantMask & ~(VARIANT_ALL_NONNUMERIC |
                                      VARIANT_NUMBER |
                                      VARIANT_INTEGER)) == 0,
                    "need to update code below to handle additional variants");

  if (ParseVariant(aValue, aVariantMask, aKeywordTable)) {
    if (aValue.GetUnit() == eCSSUnit_Integer) {
      if (aValue.GetIntValue() < 1) {
        UngetToken();
        return false;
      }
    } else if (eCSSUnit_Number == aValue.GetUnit()) {
      if (aValue.GetFloatValue() < 1.0f) {
        UngetToken();
        return false;
      }
    }
    return true;
  }
  return false;
}


bool
CSSParserImpl::ParseVariant(nsCSSValue& aValue,
                            int32_t aVariantMask,
                            const int32_t aKeywordTable[])
{
  NS_ASSERTION(!(mHashlessColorQuirk && (aVariantMask & VARIANT_COLOR)) ||
               !(aVariantMask & VARIANT_NUMBER),
               "can't distinguish colors from numbers");
  NS_ASSERTION(!(mHashlessColorQuirk && (aVariantMask & VARIANT_COLOR)) ||
               !(mUnitlessLengthQuirk && (aVariantMask & VARIANT_LENGTH)),
               "can't distinguish colors from lengths");
  NS_ASSERTION(!(mUnitlessLengthQuirk && (aVariantMask & VARIANT_LENGTH)) ||
               !(aVariantMask & VARIANT_NUMBER),
               "can't distinguish lengths from numbers");
  NS_ABORT_IF_FALSE(!(aVariantMask & VARIANT_IDENTIFIER) ||
                    !(aVariantMask & VARIANT_IDENTIFIER_NO_INHERIT),
                    "must not set both VARIANT_IDENTIFIER and "
                    "VARIANT_IDENTIFIER_NO_INHERIT");

  if (!GetToken(true)) {
    return false;
  }
  nsCSSToken* tk = &mToken;
  if (((aVariantMask & (VARIANT_AHK | VARIANT_NORMAL | VARIANT_NONE | VARIANT_ALL)) != 0) &&
      (eCSSToken_Ident == tk->mType)) {
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(tk->mIdent);
    if (eCSSKeyword_UNKNOWN < keyword) { 
      if ((aVariantMask & VARIANT_AUTO) != 0) {
        if (eCSSKeyword_auto == keyword) {
          aValue.SetAutoValue();
          return true;
        }
      }
      if ((aVariantMask & VARIANT_INHERIT) != 0) {
        
        
        
        if (eCSSKeyword_inherit == keyword) {
          aValue.SetInheritValue();
          return true;
        }
        else if (eCSSKeyword__moz_initial == keyword ||
                 eCSSKeyword_initial == keyword) { 
          aValue.SetInitialValue();
          return true;
        }
      }
      if ((aVariantMask & VARIANT_NONE) != 0) {
        if (eCSSKeyword_none == keyword) {
          aValue.SetNoneValue();
          return true;
        }
      }
      if ((aVariantMask & VARIANT_ALL) != 0) {
        if (eCSSKeyword_all == keyword) {
          aValue.SetAllValue();
          return true;
        }
      }
      if ((aVariantMask & VARIANT_NORMAL) != 0) {
        if (eCSSKeyword_normal == keyword) {
          aValue.SetNormalValue();
          return true;
        }
      }
      if ((aVariantMask & VARIANT_SYSFONT) != 0) {
        if (eCSSKeyword__moz_use_system_font == keyword &&
            !IsParsingCompoundProperty()) {
          aValue.SetSystemFontValue();
          return true;
        }
      }
      if ((aVariantMask & VARIANT_KEYWORD) != 0) {
        int32_t value;
        if (nsCSSProps::FindKeyword(keyword, aKeywordTable, value)) {
          aValue.SetIntValue(value, eCSSUnit_Enumerated);
          return true;
        }
      }
    }
  }
  
  
  if (((aVariantMask & VARIANT_NUMBER) != 0) &&
      (eCSSToken_Number == tk->mType)) {
    aValue.SetFloatValue(tk->mNumber, eCSSUnit_Number);
    return true;
  }
  if (((aVariantMask & VARIANT_INTEGER) != 0) &&
      (eCSSToken_Number == tk->mType) && tk->mIntegerValid) {
    aValue.SetIntValue(tk->mInteger, eCSSUnit_Integer);
    return true;
  }
  if (((aVariantMask & (VARIANT_LENGTH | VARIANT_ANGLE |
                        VARIANT_FREQUENCY | VARIANT_TIME)) != 0 &&
       eCSSToken_Dimension == tk->mType) ||
      ((aVariantMask & (VARIANT_LENGTH | VARIANT_ZERO_ANGLE)) != 0 &&
       eCSSToken_Number == tk->mType &&
       tk->mNumber == 0.0f)) {
    if (((aVariantMask & VARIANT_POSITIVE_DIMENSION) != 0 && 
         tk->mNumber <= 0.0) ||
        ((aVariantMask & VARIANT_NONNEGATIVE_DIMENSION) != 0 && 
         tk->mNumber < 0.0)) {
        UngetToken();
        return false;
    }
    if (TranslateDimension(aValue, aVariantMask, tk->mNumber, tk->mIdent)) {
      return true;
    }
    
    UngetToken();
    return false;
  }
  if (((aVariantMask & VARIANT_PERCENT) != 0) &&
      (eCSSToken_Percentage == tk->mType)) {
    aValue.SetPercentValue(tk->mNumber);
    return true;
  }
  if (mUnitlessLengthQuirk) { 
    if (((aVariantMask & VARIANT_LENGTH) != 0) &&
        (eCSSToken_Number == tk->mType)) {
      aValue.SetFloatValue(tk->mNumber, eCSSUnit_Pixel);
      return true;
    }
  }

  if (IsSVGMode() && !IsParsingCompoundProperty()) {
    
    
    if (((aVariantMask & VARIANT_LENGTH) != 0) &&
        (eCSSToken_Number == tk->mType)) {
      aValue.SetFloatValue(tk->mNumber, eCSSUnit_Pixel);
      return true;
    }
  }

  if (((aVariantMask & VARIANT_URL) != 0) &&
      eCSSToken_URL == tk->mType) {
    SetValueToURL(aValue, tk->mIdent);
    return true;
  }
  if ((aVariantMask & VARIANT_GRADIENT) != 0 &&
      eCSSToken_Function == tk->mType) {
    
    nsDependentString tmp(tk->mIdent, 0);
    bool isLegacy = false;
    if (StringBeginsWith(tmp, NS_LITERAL_STRING("-moz-"))) {
      tmp.Rebind(tmp, 5);
      isLegacy = true;
    }
    bool isRepeating = false;
    if (StringBeginsWith(tmp, NS_LITERAL_STRING("repeating-"))) {
      tmp.Rebind(tmp, 10);
      isRepeating = true;
    }

    if (tmp.LowerCaseEqualsLiteral("linear-gradient")) {
      return ParseLinearGradient(aValue, isRepeating, isLegacy);
    }
    if (tmp.LowerCaseEqualsLiteral("radial-gradient")) {
      return ParseRadialGradient(aValue, isRepeating, isLegacy);
    }
  }
  if ((aVariantMask & VARIANT_IMAGE_RECT) != 0 &&
      eCSSToken_Function == tk->mType &&
      tk->mIdent.LowerCaseEqualsLiteral("-moz-image-rect")) {
    return ParseImageRect(aValue);
  }
  if ((aVariantMask & VARIANT_ELEMENT) != 0 &&
      eCSSToken_Function == tk->mType &&
      tk->mIdent.LowerCaseEqualsLiteral("-moz-element")) {
    return ParseElement(aValue);
  }
  if ((aVariantMask & VARIANT_COLOR) != 0) {
    if (mHashlessColorQuirk || 
        (eCSSToken_ID == tk->mType) ||
        (eCSSToken_Hash == tk->mType) ||
        (eCSSToken_Ident == tk->mType) ||
        ((eCSSToken_Function == tk->mType) &&
         (tk->mIdent.LowerCaseEqualsLiteral("rgb") ||
          tk->mIdent.LowerCaseEqualsLiteral("hsl") ||
          tk->mIdent.LowerCaseEqualsLiteral("-moz-rgba") ||
          tk->mIdent.LowerCaseEqualsLiteral("-moz-hsla") ||
          tk->mIdent.LowerCaseEqualsLiteral("rgba") ||
          tk->mIdent.LowerCaseEqualsLiteral("hsla"))))
    {
      
      UngetToken();
      if (ParseColor(aValue)) {
        return true;
      }
      return false;
    }
  }
  if (((aVariantMask & VARIANT_STRING) != 0) &&
      (eCSSToken_String == tk->mType)) {
    nsAutoString  buffer;
    buffer.Append(tk->mIdent);
    aValue.SetStringValue(buffer, eCSSUnit_String);
    return true;
  }
  if (((aVariantMask &
        (VARIANT_IDENTIFIER | VARIANT_IDENTIFIER_NO_INHERIT)) != 0) &&
      (eCSSToken_Ident == tk->mType) &&
      ((aVariantMask & VARIANT_IDENTIFIER) != 0 ||
       !(tk->mIdent.LowerCaseEqualsLiteral("inherit") ||
         tk->mIdent.LowerCaseEqualsLiteral("initial")))) {
    aValue.SetStringValue(tk->mIdent, eCSSUnit_Ident);
    return true;
  }
  if (((aVariantMask & VARIANT_COUNTER) != 0) &&
      (eCSSToken_Function == tk->mType) &&
      (tk->mIdent.LowerCaseEqualsLiteral("counter") ||
       tk->mIdent.LowerCaseEqualsLiteral("counters"))) {
    return ParseCounter(aValue);
  }
  if (((aVariantMask & VARIANT_ATTR) != 0) &&
      (eCSSToken_Function == tk->mType) &&
      tk->mIdent.LowerCaseEqualsLiteral("attr")) {
    if (!ParseAttr(aValue)) {
      SkipUntil(')');
      return false;
    }
    return true;
  }
  if (((aVariantMask & VARIANT_TIMING_FUNCTION) != 0) &&
      (eCSSToken_Function == tk->mType)) {
    if (tk->mIdent.LowerCaseEqualsLiteral("cubic-bezier")) {
      if (!ParseTransitionTimingFunctionValues(aValue)) {
        SkipUntil(')');
        return false;
      }
      return true;
    }
    if (tk->mIdent.LowerCaseEqualsLiteral("steps")) {
      if (!ParseTransitionStepTimingFunctionValues(aValue)) {
        SkipUntil(')');
        return false;
      }
      return true;
    }
  }
  if ((aVariantMask & VARIANT_CALC) &&
      (eCSSToken_Function == tk->mType) &&
      (tk->mIdent.LowerCaseEqualsLiteral("calc") ||
       tk->mIdent.LowerCaseEqualsLiteral("-moz-calc"))) {
    
    return ParseCalc(aValue, aVariantMask & VARIANT_LP);
  }

  UngetToken();
  return false;
}


bool
CSSParserImpl::ParseCounter(nsCSSValue& aValue)
{
  nsCSSUnit unit = (mToken.mIdent.LowerCaseEqualsLiteral("counter") ?
                    eCSSUnit_Counter : eCSSUnit_Counters);

  
  for (;;) {
    if (!GetToken(true)) {
      break;
    }
    if (eCSSToken_Ident != mToken.mType) {
      UngetToken();
      break;
    }

    nsRefPtr<nsCSSValue::Array> val =
      nsCSSValue::Array::Create(unit == eCSSUnit_Counter ? 2 : 3);

    val->Item(0).SetStringValue(mToken.mIdent, eCSSUnit_Ident);

    if (eCSSUnit_Counters == unit) {
      
      if (!ExpectSymbol(',', true) || !GetToken(true)) {
        break;
      }
      if (eCSSToken_String != mToken.mType) {
        UngetToken();
        break;
      }
      val->Item(1).SetStringValue(mToken.mIdent, eCSSUnit_String);
    }

    
    int32_t type = NS_STYLE_LIST_STYLE_DECIMAL;
    if (ExpectSymbol(',', true)) {
      if (!GetToken(true)) {
        break;
      }
      nsCSSKeyword keyword;
      if (eCSSToken_Ident != mToken.mType ||
          (keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent)) ==
            eCSSKeyword_UNKNOWN ||
          !nsCSSProps::FindKeyword(keyword, nsCSSProps::kListStyleKTable,
                                   type)) {
        UngetToken();
        break;
      }
    }

    int32_t typeItem = eCSSUnit_Counters == unit ? 2 : 1;
    val->Item(typeItem).SetIntValue(type, eCSSUnit_Enumerated);

    if (!ExpectSymbol(')', true)) {
      break;
    }

    aValue.SetArrayValue(val, unit);
    return true;
  }

  SkipUntil(')');
  return false;
}

bool
CSSParserImpl::ParseAttr(nsCSSValue& aValue)
{
  if (!GetToken(true)) {
    return false;
  }

  nsAutoString attr;
  if (eCSSToken_Ident == mToken.mType) {  
    nsAutoString  holdIdent(mToken.mIdent);
    if (ExpectSymbol('|', false)) {  
      int32_t nameSpaceID = GetNamespaceIdForPrefix(holdIdent);
      if (nameSpaceID == kNameSpaceID_Unknown) {
        return false;
      }
      attr.AppendInt(nameSpaceID, 10);
      attr.Append(PRUnichar('|'));
      if (! GetToken(false)) {
        REPORT_UNEXPECTED_EOF(PEAttributeNameEOF);
        return false;
      }
      if (eCSSToken_Ident == mToken.mType) {
        attr.Append(mToken.mIdent);
      }
      else {
        REPORT_UNEXPECTED_TOKEN(PEAttributeNameExpected);
        UngetToken();
        return false;
      }
    }
    else {  
      attr = holdIdent;
    }
  }
  else if (mToken.IsSymbol('*')) {  
    
    REPORT_UNEXPECTED_TOKEN(PEAttributeNameExpected);
    UngetToken();
    return false;
  }
  else if (mToken.IsSymbol('|')) {  
    if (! GetToken(false)) {
      REPORT_UNEXPECTED_EOF(PEAttributeNameEOF);
      return false;
    }
    if (eCSSToken_Ident == mToken.mType) {
      attr.Append(mToken.mIdent);
    }
    else {
      REPORT_UNEXPECTED_TOKEN(PEAttributeNameExpected);
      UngetToken();
      return false;
    }
  }
  else {
    REPORT_UNEXPECTED_TOKEN(PEAttributeNameOrNamespaceExpected);
    UngetToken();
    return false;
  }
  if (!ExpectSymbol(')', true)) {
    return false;
  }
  aValue.SetStringValue(attr, eCSSUnit_Attr);
  return true;
}

bool
CSSParserImpl::SetValueToURL(nsCSSValue& aValue, const nsString& aURL)
{
  if (!mSheetPrincipal) {
    NS_NOTREACHED("Codepaths that expect to parse URLs MUST pass in an "
                  "origin principal");
    return false;
  }

  nsRefPtr<nsStringBuffer> buffer(nsCSSValue::BufferFromString(aURL));

  
  mozilla::css::URLValue *urlVal =
    new mozilla::css::URLValue(buffer, mBaseURI, mSheetURI, mSheetPrincipal);
  aValue.SetURLValue(urlVal);
  return true;
}





bool
CSSParserImpl::ParseImageRect(nsCSSValue& aImage)
{
  
  for (;;) {
    nsCSSValue newFunction;
    static const uint32_t kNumArgs = 5;
    nsCSSValue::Array* func =
      newFunction.InitFunction(eCSSKeyword__moz_image_rect, kNumArgs);

    
    nsCSSValue& url    = func->Item(1);
    nsCSSValue& top    = func->Item(2);
    nsCSSValue& right  = func->Item(3);
    nsCSSValue& bottom = func->Item(4);
    nsCSSValue& left   = func->Item(5);

    nsAutoString urlString;
    if (!ParseURLOrString(urlString) ||
        !SetValueToURL(url, urlString) ||
        !ExpectSymbol(',', true)) {
      break;
    }

    static const int32_t VARIANT_SIDE = VARIANT_NUMBER | VARIANT_PERCENT;
    if (!ParseNonNegativeVariant(top, VARIANT_SIDE, nullptr) ||
        !ExpectSymbol(',', true) ||
        !ParseNonNegativeVariant(right, VARIANT_SIDE, nullptr) ||
        !ExpectSymbol(',', true) ||
        !ParseNonNegativeVariant(bottom, VARIANT_SIDE, nullptr) ||
        !ExpectSymbol(',', true) ||
        !ParseNonNegativeVariant(left, VARIANT_SIDE, nullptr) ||
        !ExpectSymbol(')', true))
      break;

    aImage = newFunction;
    return true;
  }

  SkipUntil(')');
  return false;
}


bool
CSSParserImpl::ParseElement(nsCSSValue& aValue)
{
  
  for (;;) {
    if (!GetToken(true))
      break;

    if (mToken.mType == eCSSToken_ID) {
      aValue.SetStringValue(mToken.mIdent, eCSSUnit_Element);
    } else {
      UngetToken();
      break;
    }

    if (!ExpectSymbol(')', true))
      break;

    return true;
  }

  
  
  SkipUntil(')');
  return false;
}

#ifdef MOZ_FLEXBOX

bool
CSSParserImpl::ParseFlex()
{
  
  nsCSSValue tmpVal;
  if (ParseVariant(tmpVal, VARIANT_INHERIT, nullptr)) {
    AppendValue(eCSSProperty_flex_grow, tmpVal);
    AppendValue(eCSSProperty_flex_shrink, tmpVal);
    AppendValue(eCSSProperty_flex_basis, tmpVal);
    return true;
  }

  
  if (ParseVariant(tmpVal, VARIANT_NONE, nullptr)) {
    AppendValue(eCSSProperty_flex_grow, nsCSSValue(0.0f, eCSSUnit_Number));
    AppendValue(eCSSProperty_flex_shrink, nsCSSValue(0.0f, eCSSUnit_Number));
    AppendValue(eCSSProperty_flex_basis, nsCSSValue(eCSSUnit_Auto));
    return true;
  }

  
  

  
  
  
  
  nsCSSValue flexGrow(1.0f, eCSSUnit_Number);
  nsCSSValue flexShrink(1.0f, eCSSUnit_Number);
  nsCSSValue flexBasis(0.0f, eCSSUnit_Percent);

  
  
  
  
  
  
  
  
  
  

  uint32_t flexBasisVariantMask =
    (nsCSSProps::ParserVariant(eCSSProperty_flex_basis) & ~(VARIANT_INHERIT));

  
  
  
  
  
  
  
  
  
  
  if (!ParseNonNegativeVariant(tmpVal, flexBasisVariantMask | VARIANT_NUMBER,
                               nsCSSProps::kWidthKTable)) {
    
    return false;
  }

  
  bool wasFirstComponentFlexBasis = (tmpVal.GetUnit() != eCSSUnit_Number);
  (wasFirstComponentFlexBasis ? flexBasis : flexGrow) = tmpVal;

  
  bool doneParsing = false;
  if (wasFirstComponentFlexBasis) {
    if (ParseNonNegativeVariant(tmpVal, VARIANT_NUMBER, nullptr)) {
      flexGrow = tmpVal;
    } else {
      
      
      doneParsing = true;
    }
  }

  if (!doneParsing) {
    
    
    if (ParseNonNegativeVariant(tmpVal, VARIANT_NUMBER, nullptr)) {
      flexShrink = tmpVal;
    }
 
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (!wasFirstComponentFlexBasis &&
        ParseNonNegativeVariant(tmpVal, flexBasisVariantMask,
                                nsCSSProps::kWidthKTable)) {
      flexBasis = tmpVal;
    }
  }

  AppendValue(eCSSProperty_flex_grow,   flexGrow);
  AppendValue(eCSSProperty_flex_shrink, flexShrink);
  AppendValue(eCSSProperty_flex_basis,  flexBasis);

  return true;
}
#endif


bool
CSSParserImpl::ParseColorStop(nsCSSValueGradient* aGradient)
{
  nsCSSValueGradientStop* stop = aGradient->mStops.AppendElement();
  if (!ParseVariant(stop->mColor, VARIANT_COLOR, nullptr)) {
    return false;
  }

  
  
  if (!ParseVariant(stop->mLocation, VARIANT_LP | VARIANT_CALC, nullptr)) {
    stop->mLocation.SetNoneValue();
  }
  return true;
}





















bool
CSSParserImpl::ParseLinearGradient(nsCSSValue& aValue, bool aIsRepeating,
                                   bool aIsLegacy)
{
  nsRefPtr<nsCSSValueGradient> cssGradient
    = new nsCSSValueGradient(false, aIsRepeating);

  if (!GetToken(true)) {
    return false;
  }

  if (mToken.mType == eCSSToken_Ident &&
      mToken.mIdent.LowerCaseEqualsLiteral("to")) {

    
    if (!ParseBoxPositionValues(cssGradient->mBgPos, false, false)) {
      SkipUntil(')');
      return false;
    }

    
    const nsCSSValue& xValue = cssGradient->mBgPos.mXValue;
    const nsCSSValue& yValue = cssGradient->mBgPos.mYValue;
    if (xValue.GetUnit() != eCSSUnit_Enumerated ||
        !(xValue.GetIntValue() & (NS_STYLE_BG_POSITION_LEFT |
                                  NS_STYLE_BG_POSITION_CENTER |
                                  NS_STYLE_BG_POSITION_RIGHT)) ||
        yValue.GetUnit() != eCSSUnit_Enumerated ||
        !(yValue.GetIntValue() & (NS_STYLE_BG_POSITION_TOP |
                                  NS_STYLE_BG_POSITION_CENTER |
                                  NS_STYLE_BG_POSITION_BOTTOM))) {
      SkipUntil(')');
      return false;
    }

    if (!ExpectSymbol(',', true)) {
      SkipUntil(')');
      return false;
    }

    return ParseGradientColorStops(cssGradient, aValue);
  }

  if (!aIsLegacy) {
    UngetToken();

    
    if (ParseVariant(cssGradient->mAngle, VARIANT_ANGLE, nullptr) &&
        !ExpectSymbol(',', true)) {
      SkipUntil(')');
      return false;
    }

    return ParseGradientColorStops(cssGradient, aValue);
  }

  nsCSSTokenType ty = mToken.mType;
  nsString id = mToken.mIdent;
  UngetToken();

  
  bool haveGradientLine = IsLegacyGradientLine(ty, id);
  if (haveGradientLine) {
    cssGradient->mIsLegacySyntax = true;
    bool haveAngle =
      ParseVariant(cssGradient->mAngle, VARIANT_ANGLE, nullptr);

    
    if (!haveAngle || !ExpectSymbol(',', true)) {
      if (!ParseBoxPositionValues(cssGradient->mBgPos, false)) {
        SkipUntil(')');
        return false;
      }

      if (!ExpectSymbol(',', true) &&
          
          
          (haveAngle ||
           !ParseVariant(cssGradient->mAngle, VARIANT_ANGLE, nullptr) ||
           
           !ExpectSymbol(',', true))) {
        SkipUntil(')');
        return false;
      }
    }
  }

  return ParseGradientColorStops(cssGradient, aValue);
}

bool
CSSParserImpl::ParseRadialGradient(nsCSSValue& aValue, bool aIsRepeating,
                                   bool aIsLegacy)
{
  nsRefPtr<nsCSSValueGradient> cssGradient
    = new nsCSSValueGradient(true, aIsRepeating);

  
  bool haveShape =
    ParseVariant(cssGradient->GetRadialShape(), VARIANT_KEYWORD,
                 nsCSSProps::kRadialGradientShapeKTable);

  bool haveSize = ParseVariant(cssGradient->GetRadialSize(), VARIANT_KEYWORD,
                               aIsLegacy ?
                               nsCSSProps::kRadialGradientLegacySizeKTable :
                               nsCSSProps::kRadialGradientSizeKTable);
  if (haveSize) {
    if (!haveShape) {
      
      haveShape = ParseVariant(cssGradient->GetRadialShape(), VARIANT_KEYWORD,
                               nsCSSProps::kRadialGradientShapeKTable);
    }
  } else if (!aIsLegacy) {
    
    haveSize =
      ParseNonNegativeVariant(cssGradient->GetRadiusX(), VARIANT_LP, nullptr);
    if (haveSize) {
      
      bool haveYSize =
        ParseNonNegativeVariant(cssGradient->GetRadiusY(), VARIANT_LP, nullptr);
      if (!haveShape) {
        nsCSSValue shapeValue;
        haveShape = ParseVariant(shapeValue, VARIANT_KEYWORD,
                                 nsCSSProps::kRadialGradientShapeKTable);
      }
      int32_t shape =
        cssGradient->GetRadialShape().GetUnit() == eCSSUnit_Enumerated ?
        cssGradient->GetRadialShape().GetIntValue() : -1;
      if (haveYSize
            ? shape == NS_STYLE_GRADIENT_SHAPE_CIRCULAR
            : cssGradient->GetRadiusX().GetUnit() == eCSSUnit_Percent ||
              shape == NS_STYLE_GRADIENT_SHAPE_ELLIPTICAL) {
        SkipUntil(')');
        return false;
      }
      cssGradient->mIsExplicitSize = true;
    }
  }

  if ((haveShape || haveSize) && ExpectSymbol(',', true)) {
    
    return ParseGradientColorStops(cssGradient, aValue);
  }

  if (!GetToken(true)) {
    return false;
  }

  if (!aIsLegacy) {
    if (mToken.mType == eCSSToken_Ident &&
        mToken.mIdent.LowerCaseEqualsLiteral("at")) {
      
      if (!ParseBoxPositionValues(cssGradient->mBgPos, false) ||
          !ExpectSymbol(',', true)) {
        SkipUntil(')');
        return false;
      }

      return ParseGradientColorStops(cssGradient, aValue);
    }

    
    UngetToken();
    return ParseGradientColorStops(cssGradient, aValue);
  }
  MOZ_ASSERT(!cssGradient->mIsExplicitSize);

  nsCSSTokenType ty = mToken.mType;
  nsString id = mToken.mIdent;
  UngetToken();

  
  bool haveGradientLine = false;
  
  
  if (!haveShape && !haveSize) {
      haveGradientLine = IsLegacyGradientLine(ty, id);
  }
  if (haveGradientLine) {
    bool haveAngle =
      ParseVariant(cssGradient->mAngle, VARIANT_ANGLE, nullptr);

    
    if (!haveAngle || !ExpectSymbol(',', true)) {
      if (!ParseBoxPositionValues(cssGradient->mBgPos, false)) {
        SkipUntil(')');
        return false;
      }

      if (!ExpectSymbol(',', true) &&
          
          
          (haveAngle ||
           !ParseVariant(cssGradient->mAngle, VARIANT_ANGLE, nullptr) ||
           
           !ExpectSymbol(',', true))) {
        SkipUntil(')');
        return false;
      }
    }

    if (cssGradient->mAngle.GetUnit() != eCSSUnit_None) {
      cssGradient->mIsLegacySyntax = true;
    }
  }

  
  if (!haveShape && !haveSize) {
    haveShape =
      ParseVariant(cssGradient->GetRadialShape(), VARIANT_KEYWORD,
                   nsCSSProps::kRadialGradientShapeKTable);
    haveSize =
      ParseVariant(cssGradient->GetRadialSize(), VARIANT_KEYWORD,
                   nsCSSProps::kRadialGradientLegacySizeKTable);

    
    if (!haveShape) {
      haveShape =
        ParseVariant(cssGradient->GetRadialShape(), VARIANT_KEYWORD,
                     nsCSSProps::kRadialGradientShapeKTable);
    }
  }

  if ((haveShape || haveSize) && !ExpectSymbol(',', true)) {
    SkipUntil(')');
    return false;
  }

  return ParseGradientColorStops(cssGradient, aValue);
}

bool
CSSParserImpl::IsLegacyGradientLine(const nsCSSTokenType& aType,
                                    const nsString& aId)
{
  
  
  
  
  
  

  bool haveGradientLine = false;
  switch (aType) {
  case eCSSToken_Percentage:
  case eCSSToken_Number:
  case eCSSToken_Dimension:
    haveGradientLine = true;
    break;

  case eCSSToken_Function:
    if (aId.LowerCaseEqualsLiteral("calc") ||
        aId.LowerCaseEqualsLiteral("-moz-calc")) {
      haveGradientLine = true;
      break;
    }
    
  case eCSSToken_ID:
  case eCSSToken_Hash:
    
    break;

  case eCSSToken_Ident: {
    
    nsCSSKeyword kw = nsCSSKeywords::LookupKeyword(aId);
    int32_t junk;
    if (kw != eCSSKeyword_UNKNOWN &&
        nsCSSProps::FindKeyword(kw, nsCSSProps::kBackgroundPositionKTable,
                                junk)) {
      haveGradientLine = true;
    }
    break;
  }

  default:
    
    break;
  }

  return haveGradientLine;
}

bool
CSSParserImpl::ParseGradientColorStops(nsCSSValueGradient* aGradient,
                                       nsCSSValue& aValue)
{
  
  if (!ParseColorStop(aGradient) ||
      !ExpectSymbol(',', true) ||
      !ParseColorStop(aGradient)) {
    SkipUntil(')');
    return false;
  }

  
  while (ExpectSymbol(',', true)) {
    if (!ParseColorStop(aGradient)) {
      SkipUntil(')');
      return false;
    }
  }

  if (!ExpectSymbol(')', true)) {
    SkipUntil(')');
    return false;
  }

  aValue.SetGradientValue(aGradient);
  return true;
}

int32_t
CSSParserImpl::ParseChoice(nsCSSValue aValues[],
                           const nsCSSProperty aPropIDs[], int32_t aNumIDs)
{
  int32_t found = 0;
  nsAutoParseCompoundProperty compound(this);

  int32_t loop;
  for (loop = 0; loop < aNumIDs; loop++) {
    
    int32_t hadFound = found;
    int32_t index;
    for (index = 0; index < aNumIDs; index++) {
      int32_t bit = 1 << index;
      if ((found & bit) == 0) {
        if (ParseSingleValueProperty(aValues[index], aPropIDs[index])) {
          found |= bit;
          
          
          
          break;
        }
      }
    }
    if (found == hadFound) {  
      break;
    }
  }
  if (0 < found) {
    if (1 == found) { 
      if (eCSSUnit_Inherit == aValues[0].GetUnit()) { 
        for (loop = 1; loop < aNumIDs; loop++) {
          aValues[loop].SetInheritValue();
        }
        found = ((1 << aNumIDs) - 1);
      }
      else if (eCSSUnit_Initial == aValues[0].GetUnit()) { 
        for (loop = 1; loop < aNumIDs; loop++) {
          aValues[loop].SetInitialValue();
        }
        found = ((1 << aNumIDs) - 1);
      }
    }
    else {  
      for (loop = 0; loop < aNumIDs; loop++) {
        if (eCSSUnit_Inherit == aValues[loop].GetUnit()) {
          found = -1;
          break;
        }
        else if (eCSSUnit_Initial == aValues[loop].GetUnit()) {
          found = -1;
          break;
        }
      }
    }
  }
  return found;
}

void
CSSParserImpl::AppendValue(nsCSSProperty aPropID, const nsCSSValue& aValue)
{
  mTempData.AddLonghandProperty(aPropID, aValue);
}






bool
CSSParserImpl::ParseBoxProperties(const nsCSSProperty aPropIDs[])
{
  
  int32_t count = 0;
  nsCSSRect result;
  NS_FOR_CSS_SIDES (index) {
    if (! ParseSingleValueProperty(result.*(nsCSSRect::sides[index]),
                                   aPropIDs[index])) {
      break;
    }
    count++;
  }
  if ((count == 0) || (false == ExpectEndProperty())) {
    return false;
  }

  if (1 < count) { 
    NS_FOR_CSS_SIDES (index) {
      nsCSSUnit unit = (result.*(nsCSSRect::sides[index])).GetUnit();
      if (eCSSUnit_Inherit == unit || eCSSUnit_Initial == unit) {
        return false;
      }
    }
  }

  
  switch (count) {
    case 1: 
      result.mRight = result.mTop;
    case 2: 
      result.mBottom = result.mTop;
    case 3: 
      result.mLeft = result.mRight;
  }

  NS_FOR_CSS_SIDES (index) {
    AppendValue(aPropIDs[index], result.*(nsCSSRect::sides[index]));
  }
  return true;
}



bool
CSSParserImpl::ParseGroupedBoxProperty(int32_t aVariantMask,
                                        nsCSSValue& aValue)
{
  nsCSSRect& result = aValue.SetRectValue();

  int32_t count = 0;
  NS_FOR_CSS_SIDES (index) {
    if (!ParseNonNegativeVariant(result.*(nsCSSRect::sides[index]),
                                 aVariantMask, nullptr)) {
      break;
    }
    count++;
  }

  if (count == 0) {
    return false;
  }

  
  switch (count) {
    case 1: 
      result.mRight = result.mTop;
    case 2: 
      result.mBottom = result.mTop;
    case 3: 
      result.mLeft = result.mRight;
  }

  return true;
}

bool
CSSParserImpl::ParseDirectionalBoxProperty(nsCSSProperty aProperty,
                                           int32_t aSourceType)
{
  const nsCSSProperty* subprops = nsCSSProps::SubpropertyEntryFor(aProperty);
  NS_ASSERTION(subprops[3] == eCSSProperty_UNKNOWN,
               "not box property with physical vs. logical cascading");
  nsCSSValue value;
  if (!ParseSingleValueProperty(value, subprops[0]) ||
      !ExpectEndProperty())
    return false;

  AppendValue(subprops[0], value);
  nsCSSValue typeVal(aSourceType, eCSSUnit_Enumerated);
  AppendValue(subprops[1], typeVal);
  AppendValue(subprops[2], typeVal);
  return true;
}

bool
CSSParserImpl::ParseBoxCornerRadius(nsCSSProperty aPropID)
{
  nsCSSValue dimenX, dimenY;
  
  if (! ParseNonNegativeVariant(dimenX, VARIANT_HLP | VARIANT_CALC, nullptr))
    return false;

  
  if (dimenX.GetUnit() != eCSSUnit_Inherit &&
      dimenX.GetUnit() != eCSSUnit_Initial) {
    ParseNonNegativeVariant(dimenY, VARIANT_LP | VARIANT_CALC, nullptr);
  }

  if (dimenX == dimenY || dimenY.GetUnit() == eCSSUnit_Null) {
    AppendValue(aPropID, dimenX);
  } else {
    nsCSSValue value;
    value.SetPairValue(dimenX, dimenY);
    AppendValue(aPropID, value);
  }
  return true;
}

bool
CSSParserImpl::ParseBoxCornerRadii(const nsCSSProperty aPropIDs[])
{
  
  
  
  nsCSSRect dimenX, dimenY;
  int32_t countX = 0, countY = 0;

  NS_FOR_CSS_SIDES (side) {
    if (! ParseNonNegativeVariant(dimenX.*nsCSSRect::sides[side],
                                  (side > 0 ? 0 : VARIANT_INHERIT) |
                                    VARIANT_LP | VARIANT_CALC,
                                  nullptr))
      break;
    countX++;
  }
  if (countX == 0)
    return false;

  if (ExpectSymbol('/', true)) {
    NS_FOR_CSS_SIDES (side) {
      if (! ParseNonNegativeVariant(dimenY.*nsCSSRect::sides[side],
                                    VARIANT_LP | VARIANT_CALC, nullptr))
        break;
      countY++;
    }
    if (countY == 0)
      return false;
  }
  if (!ExpectEndProperty())
    return false;

  
  if (countX > 1 || countY > 0) {
    nsCSSUnit unit = dimenX.mTop.GetUnit();
    if (eCSSUnit_Inherit == unit || eCSSUnit_Initial == unit)
      return false;
  }

  
  if (countY == 0) {
    dimenY = dimenX;
    countY = countX;
  }

  
  switch (countX) {
    case 1: dimenX.mRight = dimenX.mTop;  
    case 2: dimenX.mBottom = dimenX.mTop; 
    case 3: dimenX.mLeft = dimenX.mRight; 
  }

  switch (countY) {
    case 1: dimenY.mRight = dimenY.mTop;  
    case 2: dimenY.mBottom = dimenY.mTop; 
    case 3: dimenY.mLeft = dimenY.mRight; 
  }

  NS_FOR_CSS_SIDES(side) {
    nsCSSValue& x = dimenX.*nsCSSRect::sides[side];
    nsCSSValue& y = dimenY.*nsCSSRect::sides[side];

    if (x == y) {
      AppendValue(aPropIDs[side], x);
    } else {
      nsCSSValue pair;
      pair.SetPairValue(x, y);
      AppendValue(aPropIDs[side], pair);
    }
  }
  return true;
}


static const nsCSSProperty kBorderStyleIDs[] = {
  eCSSProperty_border_top_style,
  eCSSProperty_border_right_style_value,
  eCSSProperty_border_bottom_style,
  eCSSProperty_border_left_style_value
};
static const nsCSSProperty kBorderWidthIDs[] = {
  eCSSProperty_border_top_width,
  eCSSProperty_border_right_width_value,
  eCSSProperty_border_bottom_width,
  eCSSProperty_border_left_width_value
};
static const nsCSSProperty kBorderColorIDs[] = {
  eCSSProperty_border_top_color,
  eCSSProperty_border_right_color_value,
  eCSSProperty_border_bottom_color,
  eCSSProperty_border_left_color_value
};
static const nsCSSProperty kBorderRadiusIDs[] = {
  eCSSProperty_border_top_left_radius,
  eCSSProperty_border_top_right_radius,
  eCSSProperty_border_bottom_right_radius,
  eCSSProperty_border_bottom_left_radius
};
static const nsCSSProperty kOutlineRadiusIDs[] = {
  eCSSProperty__moz_outline_radius_topLeft,
  eCSSProperty__moz_outline_radius_topRight,
  eCSSProperty__moz_outline_radius_bottomRight,
  eCSSProperty__moz_outline_radius_bottomLeft
};

bool
CSSParserImpl::ParseProperty(nsCSSProperty aPropID)
{
  
  NS_ABORT_IF_FALSE(!mHashlessColorQuirk,
                    "hashless color quirk should not be set");
  NS_ABORT_IF_FALSE(!mUnitlessLengthQuirk,
                    "unitless length quirk should not be set");
  if (mNavQuirkMode) {
    mHashlessColorQuirk =
      nsCSSProps::PropHasFlags(aPropID, CSS_PROPERTY_HASHLESS_COLOR_QUIRK);
    mUnitlessLengthQuirk =
      nsCSSProps::PropHasFlags(aPropID, CSS_PROPERTY_UNITLESS_LENGTH_QUIRK);
  }

  NS_ASSERTION(aPropID < eCSSProperty_COUNT, "index out of range");
  bool result;
  switch (nsCSSProps::PropertyParseType(aPropID)) {
    case CSS_PROPERTY_PARSE_INACCESSIBLE: {
      
      REPORT_UNEXPECTED(PEInaccessibleProperty2);
      result = false;
      break;
    }
    case CSS_PROPERTY_PARSE_FUNCTION: {
      result = ParsePropertyByFunction(aPropID);
      break;
    }
    case CSS_PROPERTY_PARSE_VALUE: {
      result = false;
      nsCSSValue value;
      if (ParseSingleValueProperty(value, aPropID)) {
        if (ExpectEndProperty()) {
          AppendValue(aPropID, value);
          result = true;
        }
        
      }
      
      break;
    }
    case CSS_PROPERTY_PARSE_VALUE_LIST: {
      result = ParseValueList(aPropID);
      break;
    }
    default: {
      result = false;
      NS_ABORT_IF_FALSE(false,
                        "Property's flags field in nsCSSPropList.h is missing "
                        "one of the CSS_PROPERTY_PARSE_* constants");
      break;
    }
  }

  if (mNavQuirkMode) {
    mHashlessColorQuirk = false;
    mUnitlessLengthQuirk = false;
  }

  return result;
}

bool
CSSParserImpl::ParsePropertyByFunction(nsCSSProperty aPropID)
{
  switch (aPropID) {  
  case eCSSProperty_background:
    return ParseBackground();
  case eCSSProperty_background_repeat:
    return ParseBackgroundRepeat();
  case eCSSProperty_background_position:
    return ParseBackgroundPosition();
  case eCSSProperty_background_size:
    return ParseBackgroundSize();
  case eCSSProperty_border:
    return ParseBorderSide(kBorderTopIDs, true);
  case eCSSProperty_border_color:
    return ParseBorderColor();
  case eCSSProperty_border_spacing:
    return ParseBorderSpacing();
  case eCSSProperty_border_style:
    return ParseBorderStyle();
  case eCSSProperty_border_bottom:
    return ParseBorderSide(kBorderBottomIDs, false);
  case eCSSProperty_border_end:
    return ParseDirectionalBorderSide(kBorderEndIDs,
                                      NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_border_left:
    return ParseDirectionalBorderSide(kBorderLeftIDs,
                                      NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_border_right:
    return ParseDirectionalBorderSide(kBorderRightIDs,
                                      NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_border_start:
    return ParseDirectionalBorderSide(kBorderStartIDs,
                                      NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_border_top:
    return ParseBorderSide(kBorderTopIDs, false);
  case eCSSProperty_border_bottom_colors:
  case eCSSProperty_border_left_colors:
  case eCSSProperty_border_right_colors:
  case eCSSProperty_border_top_colors:
    return ParseBorderColors(aPropID);
  case eCSSProperty_border_image_slice:
    return ParseBorderImageSlice(true, nullptr);
  case eCSSProperty_border_image_width:
    return ParseBorderImageWidth(true);
  case eCSSProperty_border_image_outset:
    return ParseBorderImageOutset(true);
  case eCSSProperty_border_image_repeat:
    return ParseBorderImageRepeat(true);
  case eCSSProperty_border_image:
    return ParseBorderImage();
  case eCSSProperty_border_width:
    return ParseBorderWidth();
  case eCSSProperty_border_end_color:
    return ParseDirectionalBoxProperty(eCSSProperty_border_end_color,
                                       NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_border_left_color:
    return ParseDirectionalBoxProperty(eCSSProperty_border_left_color,
                                       NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_border_right_color:
    return ParseDirectionalBoxProperty(eCSSProperty_border_right_color,
                                       NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_border_start_color:
    return ParseDirectionalBoxProperty(eCSSProperty_border_start_color,
                                       NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_border_end_width:
    return ParseDirectionalBoxProperty(eCSSProperty_border_end_width,
                                       NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_border_left_width:
    return ParseDirectionalBoxProperty(eCSSProperty_border_left_width,
                                       NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_border_right_width:
    return ParseDirectionalBoxProperty(eCSSProperty_border_right_width,
                                       NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_border_start_width:
    return ParseDirectionalBoxProperty(eCSSProperty_border_start_width,
                                       NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_border_end_style:
    return ParseDirectionalBoxProperty(eCSSProperty_border_end_style,
                                       NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_border_left_style:
    return ParseDirectionalBoxProperty(eCSSProperty_border_left_style,
                                       NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_border_right_style:
    return ParseDirectionalBoxProperty(eCSSProperty_border_right_style,
                                       NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_border_start_style:
    return ParseDirectionalBoxProperty(eCSSProperty_border_start_style,
                                       NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_border_radius:
    return ParseBoxCornerRadii(kBorderRadiusIDs);
  case eCSSProperty__moz_outline_radius:
    return ParseBoxCornerRadii(kOutlineRadiusIDs);

  case eCSSProperty_border_top_left_radius:
  case eCSSProperty_border_top_right_radius:
  case eCSSProperty_border_bottom_right_radius:
  case eCSSProperty_border_bottom_left_radius:
  case eCSSProperty__moz_outline_radius_topLeft:
  case eCSSProperty__moz_outline_radius_topRight:
  case eCSSProperty__moz_outline_radius_bottomRight:
  case eCSSProperty__moz_outline_radius_bottomLeft:
    return ParseBoxCornerRadius(aPropID);

  case eCSSProperty_box_shadow:
  case eCSSProperty_text_shadow:
    return ParseShadowList(aPropID);

  case eCSSProperty_clip:
    return ParseRect(eCSSProperty_clip);
  case eCSSProperty__moz_columns:
    return ParseColumns();
  case eCSSProperty__moz_column_rule:
    return ParseBorderSide(kColumnRuleIDs, false);
  case eCSSProperty_content:
    return ParseContent();
  case eCSSProperty_counter_increment:
  case eCSSProperty_counter_reset:
    return ParseCounterData(aPropID);
  case eCSSProperty_cursor:
    return ParseCursor();
#ifdef MOZ_FLEXBOX
  case eCSSProperty_flex:
    return ParseFlex();
#endif 
  case eCSSProperty_font:
    return ParseFont();
  case eCSSProperty_image_region:
    return ParseRect(eCSSProperty_image_region);
  case eCSSProperty_list_style:
    return ParseListStyle();
  case eCSSProperty_margin:
    return ParseMargin();
  case eCSSProperty_margin_end:
    return ParseDirectionalBoxProperty(eCSSProperty_margin_end,
                                       NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_margin_left:
    return ParseDirectionalBoxProperty(eCSSProperty_margin_left,
                                       NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_margin_right:
    return ParseDirectionalBoxProperty(eCSSProperty_margin_right,
                                       NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_margin_start:
    return ParseDirectionalBoxProperty(eCSSProperty_margin_start,
                                       NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_outline:
    return ParseOutline();
  case eCSSProperty_overflow:
    return ParseOverflow();
  case eCSSProperty_padding:
    return ParsePadding();
  case eCSSProperty_padding_end:
    return ParseDirectionalBoxProperty(eCSSProperty_padding_end,
                                       NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_padding_left:
    return ParseDirectionalBoxProperty(eCSSProperty_padding_left,
                                       NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_padding_right:
    return ParseDirectionalBoxProperty(eCSSProperty_padding_right,
                                       NS_BOXPROP_SOURCE_PHYSICAL);
  case eCSSProperty_padding_start:
    return ParseDirectionalBoxProperty(eCSSProperty_padding_start,
                                       NS_BOXPROP_SOURCE_LOGICAL);
  case eCSSProperty_quotes:
    return ParseQuotes();
  case eCSSProperty_size:
    return ParseSize();
  case eCSSProperty_text_decoration:
    return ParseTextDecoration();
  case eCSSProperty_transform:
    return ParseTransform(false);
  case eCSSProperty__moz_transform:
    return ParseTransform(true);
  case eCSSProperty_transform_origin:
    return ParseTransformOrigin(false);
  case eCSSProperty_perspective_origin:
    return ParseTransformOrigin(true);
  case eCSSProperty_transition:
    return ParseTransition();
  case eCSSProperty_animation:
    return ParseAnimation();
  case eCSSProperty_transition_property:
    return ParseTransitionProperty();
  case eCSSProperty_fill:
  case eCSSProperty_stroke:
    return ParsePaint(aPropID);
  case eCSSProperty_stroke_dasharray:
    return ParseDasharray();
  case eCSSProperty_marker:
    return ParseMarker();
  case eCSSProperty_paint_order:
    return ParsePaintOrder();
  default:
    NS_ABORT_IF_FALSE(false, "should not be called");
    return false;
  }
}


#define BG_CENTER  NS_STYLE_BG_POSITION_CENTER
#define BG_TOP     NS_STYLE_BG_POSITION_TOP
#define BG_BOTTOM  NS_STYLE_BG_POSITION_BOTTOM
#define BG_LEFT    NS_STYLE_BG_POSITION_LEFT
#define BG_RIGHT   NS_STYLE_BG_POSITION_RIGHT
#define BG_CTB    (BG_CENTER | BG_TOP | BG_BOTTOM)
#define BG_TB     (BG_TOP | BG_BOTTOM)
#define BG_CLR    (BG_CENTER | BG_LEFT | BG_RIGHT)
#define BG_LR     (BG_LEFT | BG_RIGHT)

bool
CSSParserImpl::ParseSingleValueProperty(nsCSSValue& aValue,
                                        nsCSSProperty aPropID)
{
  if (aPropID == eCSSPropertyExtra_x_none_value) {
    return ParseVariant(aValue, VARIANT_NONE | VARIANT_INHERIT, nullptr);
  }

  if (aPropID == eCSSPropertyExtra_x_auto_value) {
    return ParseVariant(aValue, VARIANT_AUTO | VARIANT_INHERIT, nullptr);
  }

  if (aPropID < 0 || aPropID >= eCSSProperty_COUNT_no_shorthands) {
    NS_ABORT_IF_FALSE(false, "not a single value property");
    return false;
  }

  if (nsCSSProps::PropHasFlags(aPropID, CSS_PROPERTY_VALUE_PARSER_FUNCTION)) {
    switch (aPropID) {
      case eCSSProperty_font_family:
        return ParseFamily(aValue);
      case eCSSProperty_font_feature_settings:
        return ParseFontFeatureSettings(aValue);
      case eCSSProperty_font_weight:
        return ParseFontWeight(aValue);
      case eCSSProperty_marks:
        return ParseMarks(aValue);
      case eCSSProperty_text_decoration_line:
        return ParseTextDecorationLine(aValue);
      case eCSSProperty_text_overflow:
        return ParseTextOverflow(aValue);
      default:
        NS_ABORT_IF_FALSE(false, "should not reach here");
        return false;
    }
  }

  uint32_t variant = nsCSSProps::ParserVariant(aPropID);
  if (variant == 0) {
    NS_ABORT_IF_FALSE(false, "not a single value property");
    return false;
  }

  
  
  
  if (aPropID == eCSSProperty_script_level && !mUnsafeRulesEnabled)
    return false;

  const int32_t *kwtable = nsCSSProps::kKeywordTableTable[aPropID];
  switch (nsCSSProps::ValueRestrictions(aPropID)) {
    default:
      NS_ABORT_IF_FALSE(false, "should not be reached");
    case 0:
      return ParseVariant(aValue, variant, kwtable);
    case CSS_PROPERTY_VALUE_NONNEGATIVE:
      return ParseNonNegativeVariant(aValue, variant, kwtable);
    case CSS_PROPERTY_VALUE_AT_LEAST_ONE:
      return ParseOneOrLargerVariant(aValue, variant, kwtable);
  }
}


struct NS_STACK_CLASS ExtractFirstFamilyData {
  nsAutoString mFamilyName;
  bool mGood;
  ExtractFirstFamilyData() : mFamilyName(), mGood(false) {}
};

static bool
ExtractFirstFamily(const nsString& aFamily,
                   bool aGeneric,
                   void* aData)
{
  ExtractFirstFamilyData* realData = (ExtractFirstFamilyData*) aData;
  if (aGeneric || realData->mFamilyName.Length() > 0) {
    realData->mGood = false;
    return false;
  }
  realData->mFamilyName.Assign(aFamily);
  realData->mGood = true;
  return true;
}



bool
CSSParserImpl::ParseFontDescriptorValue(nsCSSFontDesc aDescID,
                                        nsCSSValue& aValue)
{
  switch (aDescID) {
    
    
  case eCSSFontDesc_Family: {
    if (!ParseFamily(aValue) ||
        aValue.GetUnit() != eCSSUnit_Families)
      return false;

    
    
    nsAutoString valueStr;
    aValue.GetStringValue(valueStr);
    nsFont font(valueStr, 0, 0, 0, 0, 0, 0);
    ExtractFirstFamilyData dat;

    font.EnumerateFamilies(ExtractFirstFamily, (void*) &dat);
    if (!dat.mGood)
      return false;

    aValue.SetStringValue(dat.mFamilyName, eCSSUnit_String);
    return true;
  }

  case eCSSFontDesc_Style:
    
    return ParseVariant(aValue, VARIANT_KEYWORD | VARIANT_NORMAL,
                        nsCSSProps::kFontStyleKTable);

  case eCSSFontDesc_Weight:
    return (ParseFontWeight(aValue) &&
            aValue.GetUnit() != eCSSUnit_Inherit &&
            aValue.GetUnit() != eCSSUnit_Initial &&
            (aValue.GetUnit() != eCSSUnit_Enumerated ||
             (aValue.GetIntValue() != NS_STYLE_FONT_WEIGHT_BOLDER &&
              aValue.GetIntValue() != NS_STYLE_FONT_WEIGHT_LIGHTER)));

  case eCSSFontDesc_Stretch:
    
    return ParseVariant(aValue, VARIANT_KEYWORD,
                        nsCSSProps::kFontStretchKTable);

    
  case eCSSFontDesc_Src:
    return ParseFontSrc(aValue);

  case eCSSFontDesc_UnicodeRange:
    return ParseFontRanges(aValue);

  case eCSSFontDesc_FontFeatureSettings:
    return ParseFontFeatureSettings(aValue);

  case eCSSFontDesc_FontLanguageOverride:
    return ParseVariant(aValue, VARIANT_NORMAL | VARIANT_STRING, nullptr);

  case eCSSFontDesc_UNKNOWN:
  case eCSSFontDesc_COUNT:
    NS_NOTREACHED("bad nsCSSFontDesc code");
  }
  
  
  return false;
}

void
CSSParserImpl::InitBoxPropsAsPhysical(const nsCSSProperty *aSourceProperties)
{
  nsCSSValue physical(NS_BOXPROP_SOURCE_PHYSICAL, eCSSUnit_Enumerated);
  for (const nsCSSProperty *prop = aSourceProperties;
       *prop != eCSSProperty_UNKNOWN; ++prop) {
    AppendValue(*prop, physical);
  }
}

static nsCSSValue
BoxPositionMaskToCSSValue(int32_t aMask, bool isX)
{
  int32_t val = NS_STYLE_BG_POSITION_CENTER;
  if (isX) {
    if (aMask & BG_LEFT) {
      val = NS_STYLE_BG_POSITION_LEFT;
    }
    else if (aMask & BG_RIGHT) {
      val = NS_STYLE_BG_POSITION_RIGHT;
    }
  }
  else {
    if (aMask & BG_TOP) {
      val = NS_STYLE_BG_POSITION_TOP;
    }
    else if (aMask & BG_BOTTOM) {
      val = NS_STYLE_BG_POSITION_BOTTOM;
    }
  }

  return nsCSSValue(val, eCSSUnit_Enumerated);
}

bool
CSSParserImpl::ParseBackground()
{
  nsAutoParseCompoundProperty compound(this);

  
  nsCSSValue color;

  
  if (ParseVariant(color, VARIANT_INHERIT, nullptr)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
    for (const nsCSSProperty* subprops =
           nsCSSProps::SubpropertyEntryFor(eCSSProperty_background);
         *subprops != eCSSProperty_UNKNOWN; ++subprops) {
      AppendValue(*subprops, color);
    }
    return true;
  }

  nsCSSValue image, repeat, attachment, clip, origin, position, size;
  BackgroundParseState state(color, image.SetListValue(), 
                             repeat.SetPairListValue(),
                             attachment.SetListValue(), clip.SetListValue(),
                             origin.SetListValue(), position.SetListValue(),
                             size.SetPairListValue());

  for (;;) {
    if (!ParseBackgroundItem(state)) {
      return false;
    }
    if (CheckEndProperty()) {
      break;
    }
    
    if (color.GetUnit() != eCSSUnit_Null) {
      REPORT_UNEXPECTED_TOKEN(PEExpectEndValue);
      return false;
    }
    
    if (!ExpectSymbol(',', true)) {
      return false;
    }
    
    state.mImage->mNext = new nsCSSValueList;
    state.mImage = state.mImage->mNext;
    state.mRepeat->mNext = new nsCSSValuePairList;
    state.mRepeat = state.mRepeat->mNext;
    state.mAttachment->mNext = new nsCSSValueList;
    state.mAttachment = state.mAttachment->mNext;
    state.mClip->mNext = new nsCSSValueList;
    state.mClip = state.mClip->mNext;
    state.mOrigin->mNext = new nsCSSValueList;
    state.mOrigin = state.mOrigin->mNext;
    state.mPosition->mNext = new nsCSSValueList;
    state.mPosition = state.mPosition->mNext;
    state.mSize->mNext = new nsCSSValuePairList;
    state.mSize = state.mSize->mNext;
  }

  
  if (color.GetUnit() == eCSSUnit_Null) {
    color.SetColorValue(NS_RGBA(0,0,0,0));
  }

  AppendValue(eCSSProperty_background_image,      image);
  AppendValue(eCSSProperty_background_repeat,     repeat);
  AppendValue(eCSSProperty_background_attachment, attachment);
  AppendValue(eCSSProperty_background_clip,       clip);
  AppendValue(eCSSProperty_background_origin,     origin);
  AppendValue(eCSSProperty_background_position,   position);
  AppendValue(eCSSProperty_background_size,       size);
  AppendValue(eCSSProperty_background_color,      color);
  return true;
}


bool
CSSParserImpl::ParseBackgroundItem(CSSParserImpl::BackgroundParseState& aState)

{
  
  
  aState.mImage->mValue.SetNoneValue();
  aState.mRepeat->mXValue.SetIntValue(NS_STYLE_BG_REPEAT_REPEAT,
                                      eCSSUnit_Enumerated);
  aState.mRepeat->mYValue.Reset();
  aState.mAttachment->mValue.SetIntValue(NS_STYLE_BG_ATTACHMENT_SCROLL,
                                         eCSSUnit_Enumerated);
  aState.mClip->mValue.SetIntValue(NS_STYLE_BG_CLIP_BORDER,
                                   eCSSUnit_Enumerated);
  aState.mOrigin->mValue.SetIntValue(NS_STYLE_BG_ORIGIN_PADDING,
                                     eCSSUnit_Enumerated);
  nsRefPtr<nsCSSValue::Array> positionArr = nsCSSValue::Array::Create(4);
  aState.mPosition->mValue.SetArrayValue(positionArr, eCSSUnit_Array);
  positionArr->Item(1).SetPercentValue(0.0f);
  positionArr->Item(3).SetPercentValue(0.0f);
  aState.mSize->mXValue.SetAutoValue();
  aState.mSize->mYValue.SetAutoValue();

  bool haveColor = false,
         haveImage = false,
         haveRepeat = false,
         haveAttach = false,
         havePositionAndSize = false,
         haveOrigin = false,
         haveSomething = false;

  while (GetToken(true)) {
    nsCSSTokenType tt = mToken.mType;
    UngetToken(); 
    if (tt == eCSSToken_Symbol) {
      
      
      break;
    }

    if (tt == eCSSToken_Ident) {
      nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);
      int32_t dummy;
      if (keyword == eCSSKeyword_inherit ||
          keyword == eCSSKeyword__moz_initial ||
          keyword == eCSSKeyword_initial) {
        return false;
      } else if (keyword == eCSSKeyword_none) {
        if (haveImage)
          return false;
        haveImage = true;
        if (!ParseSingleValueProperty(aState.mImage->mValue,
                                      eCSSProperty_background_image)) {
          NS_NOTREACHED("should be able to parse");
          return false;
        }
      } else if (nsCSSProps::FindKeyword(keyword,
                   nsCSSProps::kBackgroundAttachmentKTable, dummy)) {
        if (haveAttach)
          return false;
        haveAttach = true;
        if (!ParseSingleValueProperty(aState.mAttachment->mValue,
                                      eCSSProperty_background_attachment)) {
          NS_NOTREACHED("should be able to parse");
          return false;
        }
      } else if (nsCSSProps::FindKeyword(keyword,
                   nsCSSProps::kBackgroundRepeatKTable, dummy)) {
        if (haveRepeat)
          return false;
        haveRepeat = true;
        nsCSSValuePair scratch;
        if (!ParseBackgroundRepeatValues(scratch)) {
          NS_NOTREACHED("should be able to parse");
          return false;
        }
        aState.mRepeat->mXValue = scratch.mXValue;
        aState.mRepeat->mYValue = scratch.mYValue;
      } else if (nsCSSProps::FindKeyword(keyword,
                   nsCSSProps::kBackgroundPositionKTable, dummy)) {
        if (havePositionAndSize)
          return false;
        havePositionAndSize = true;
        if (!ParseBackgroundPositionValues(aState.mPosition->mValue, false)) {
          return false;
        }
        if (ExpectSymbol('/', true)) {
          nsCSSValuePair scratch;
          if (!ParseBackgroundSizeValues(scratch)) {
            return false;
          }
          aState.mSize->mXValue = scratch.mXValue;
          aState.mSize->mYValue = scratch.mYValue;
        }
      } else if (nsCSSProps::FindKeyword(keyword,
                   nsCSSProps::kBackgroundOriginKTable, dummy)) {
        if (haveOrigin)
          return false;
        haveOrigin = true;
        if (!ParseSingleValueProperty(aState.mOrigin->mValue,
                                      eCSSProperty_background_origin)) {
          NS_NOTREACHED("should be able to parse");
          return false;
        }
        MOZ_STATIC_ASSERT(NS_STYLE_BG_CLIP_BORDER ==
                          NS_STYLE_BG_ORIGIN_BORDER &&
                          NS_STYLE_BG_CLIP_PADDING ==
                          NS_STYLE_BG_ORIGIN_PADDING &&
                          NS_STYLE_BG_CLIP_CONTENT ==
                          NS_STYLE_BG_ORIGIN_CONTENT,
                          "bg-clip and bg-origin style constants must agree");

        aState.mClip->mValue = aState.mOrigin->mValue;
      } else {
        if (haveColor)
          return false;
        haveColor = true;
        if (!ParseSingleValueProperty(aState.mColor,
                                      eCSSProperty_background_color)) {
          return false;
        }
      }
    } else if (tt == eCSSToken_URL ||
               (tt == eCSSToken_Function &&
                (mToken.mIdent.LowerCaseEqualsLiteral("linear-gradient") ||
                 mToken.mIdent.LowerCaseEqualsLiteral("radial-gradient") ||
                 mToken.mIdent.LowerCaseEqualsLiteral("repeating-linear-gradient") ||
                 mToken.mIdent.LowerCaseEqualsLiteral("repeating-radial-gradient") ||
                 mToken.mIdent.LowerCaseEqualsLiteral("-moz-linear-gradient") ||
                 mToken.mIdent.LowerCaseEqualsLiteral("-moz-radial-gradient") ||
                 mToken.mIdent.LowerCaseEqualsLiteral("-moz-repeating-linear-gradient") ||
                 mToken.mIdent.LowerCaseEqualsLiteral("-moz-repeating-radial-gradient") ||
                 mToken.mIdent.LowerCaseEqualsLiteral("-moz-image-rect") ||
                 mToken.mIdent.LowerCaseEqualsLiteral("-moz-element")))) {
      if (haveImage)
        return false;
      haveImage = true;
      if (!ParseSingleValueProperty(aState.mImage->mValue,
                                    eCSSProperty_background_image)) {
        return false;
      }
    } else if (tt == eCSSToken_Dimension ||
               tt == eCSSToken_Number ||
               tt == eCSSToken_Percentage ||
               (tt == eCSSToken_Function &&
                (mToken.mIdent.LowerCaseEqualsLiteral("calc") ||
                 mToken.mIdent.LowerCaseEqualsLiteral("-moz-calc")))) {
      if (havePositionAndSize)
        return false;
      havePositionAndSize = true;
      if (!ParseBackgroundPositionValues(aState.mPosition->mValue, false)) {
        return false;
      }
      if (ExpectSymbol('/', true)) {
        nsCSSValuePair scratch;
        if (!ParseBackgroundSizeValues(scratch)) {
          return false;
        }
        aState.mSize->mXValue = scratch.mXValue;
        aState.mSize->mYValue = scratch.mYValue;
      }
    } else {
      if (haveColor)
        return false;
      haveColor = true;
      
      
      if (!ParseSingleValueProperty(aState.mColor,
                                    eCSSProperty_background_color)) {
        return false;
      }
    }
    haveSomething = true;
  }

  return haveSomething;
}



bool
CSSParserImpl::ParseValueList(nsCSSProperty aPropID)
{
  
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValueList* item = value.SetListValue();
    for (;;) {
      if (!ParseSingleValueProperty(item->mValue, aPropID)) {
        return false;
      }
      if (CheckEndProperty()) {
        break;
      }
      if (!ExpectSymbol(',', true)) {
        return false;
      }
      item->mNext = new nsCSSValueList;
      item = item->mNext;
    }
  }
  AppendValue(aPropID, value);
  return true;
}

bool
CSSParserImpl::ParseBackgroundRepeat()
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValuePair valuePair;
    if (!ParseBackgroundRepeatValues(valuePair)) {
      return false;
    }
    nsCSSValuePairList* item = value.SetPairListValue();
    for (;;) {
      item->mXValue = valuePair.mXValue;
      item->mYValue = valuePair.mYValue;
      if (CheckEndProperty()) {
        break;
      }
      if (!ExpectSymbol(',', true)) {
        return false;
      }
      if (!ParseBackgroundRepeatValues(valuePair)) {
        return false;
      }
      item->mNext = new nsCSSValuePairList;
      item = item->mNext;
    }
  }

  AppendValue(eCSSProperty_background_repeat, value);
  return true;
}

bool
CSSParserImpl::ParseBackgroundRepeatValues(nsCSSValuePair& aValue) 
{
  nsCSSValue& xValue = aValue.mXValue;
  nsCSSValue& yValue = aValue.mYValue;
  
  if (ParseEnum(xValue, nsCSSProps::kBackgroundRepeatKTable)) {
    int32_t value = xValue.GetIntValue();
    
    if (value == NS_STYLE_BG_REPEAT_REPEAT_X ||
        value == NS_STYLE_BG_REPEAT_REPEAT_Y ||
        !ParseEnum(yValue, nsCSSProps::kBackgroundRepeatPartKTable)) {
      
      
      yValue.Reset();
    }
    return true;
  }
  
  return false;
}


bool
CSSParserImpl::ParseBackgroundPosition()
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValue itemValue;
    if (!ParseBackgroundPositionValues(itemValue, false)) {
      return false;
    }
    nsCSSValueList* item = value.SetListValue();
    for (;;) {
      item->mValue = itemValue;
      if (CheckEndProperty()) {
        break;
      }
      if (!ExpectSymbol(',', true)) {
        return false;
      }
      if (!ParseBackgroundPositionValues(itemValue, false)) {
        return false;
      }
      item->mNext = new nsCSSValueList;
      item = item->mNext;
    }
  }
  AppendValue(eCSSProperty_background_position, value);
  return true;
}

















bool CSSParserImpl::ParseBoxPositionValues(nsCSSValuePair &aOut,
                                           bool aAcceptsInherit,
                                           bool aAllowExplicitCenter)
{
  
  nsCSSValue &xValue = aOut.mXValue,
             &yValue = aOut.mYValue;
  int32_t variantMask =
    (aAcceptsInherit ? VARIANT_INHERIT : 0) | VARIANT_LP | VARIANT_CALC;
  if (ParseVariant(xValue, variantMask, nullptr)) {
    if (eCSSUnit_Inherit == xValue.GetUnit() ||
        eCSSUnit_Initial == xValue.GetUnit()) {  
      yValue = xValue;
      return true;
    }
    
    
    if (ParseVariant(yValue, VARIANT_LP | VARIANT_CALC, nullptr)) {
      
      return true;
    }

    if (ParseEnum(yValue, nsCSSProps::kBackgroundPositionKTable)) {
      int32_t yVal = yValue.GetIntValue();
      if (!(yVal & BG_CTB)) {
        
        return false;
      }
      yValue = BoxPositionMaskToCSSValue(yVal, false);
      return true;
    }

    
    
    yValue.SetPercentValue(0.5f);
    return true;
  }

  
  
  
  
  
  
  int32_t mask = 0;
  if (ParseEnum(xValue, nsCSSProps::kBackgroundPositionKTable)) {
    int32_t bit = xValue.GetIntValue();
    mask |= bit;
    if (ParseEnum(xValue, nsCSSProps::kBackgroundPositionKTable)) {
      bit = xValue.GetIntValue();
      if (mask & (bit & ~BG_CENTER)) {
        
        return false;
      }
      mask |= bit;
    }
    else {
      
      if (ParseVariant(yValue, VARIANT_LP | VARIANT_CALC, nullptr)) {
        if (!(mask & BG_CLR)) {
          
          return false;
        }

        xValue = BoxPositionMaskToCSSValue(mask, true);
        return true;
      }
    }
  }

  
  
  if ((mask == 0) || (mask == (BG_TOP | BG_BOTTOM)) ||
      (mask == (BG_LEFT | BG_RIGHT)) ||
      (!aAllowExplicitCenter && (mask & BG_CENTER))) {
    return false;
  }

  
  xValue = BoxPositionMaskToCSSValue(mask, true);
  yValue = BoxPositionMaskToCSSValue(mask, false);
  return true;
}

bool CSSParserImpl::ParseBackgroundPositionValues(nsCSSValue& aOut,
                                                  bool aAcceptsInherit)
{
  
  
  
  
  
  
  if (aAcceptsInherit && ParseVariant(aOut, VARIANT_INHERIT, nullptr)) {
    return true;
  }

  nsRefPtr<nsCSSValue::Array> value = nsCSSValue::Array::Create(4);
  aOut.SetArrayValue(value, eCSSUnit_Array);

  
  nsCSSValue &xEdge   = value->Item(0),
             &xOffset = value->Item(1),
             &yEdge   = value->Item(2),
             &yOffset = value->Item(3);

  
  uint32_t valueCount = 0;
  for (int32_t i = 0; i < 4; i++) {
    if (!ParseVariant(value->Item(i), VARIANT_LPCALC | VARIANT_KEYWORD,
                      nsCSSProps::kBackgroundPositionKTable)) {
      break;
    }
    ++valueCount;
  }

  switch (valueCount) {
    case 4:
      
      
      
      if (eCSSUnit_Enumerated != xEdge.GetUnit() ||
          BG_CENTER == xEdge.GetIntValue() ||
          eCSSUnit_Enumerated == xOffset.GetUnit() ||
          eCSSUnit_Enumerated != yEdge.GetUnit() ||
          BG_CENTER == yEdge.GetIntValue() ||
          eCSSUnit_Enumerated == yOffset.GetUnit()) {
        return false;
      }
      break;
    case 3:
      
      
      
      
      if (eCSSUnit_Enumerated != value->Item(1).GetUnit()) {
        
        
        
        if (eCSSUnit_Enumerated != value->Item(0).GetUnit() ||
            BG_CENTER == value->Item(0).GetIntValue()) {
          return false;
        }

        
        if (eCSSUnit_Enumerated != value->Item(2).GetUnit()) {
          return false;
        }

        yOffset.Reset(); 
      } else if (eCSSUnit_Enumerated != value->Item(2).GetUnit()) {
        
        
        
        if (BG_CENTER == value->Item(1).GetIntValue()) {
          return false;
        }

        
        if (eCSSUnit_Enumerated != value->Item(0).GetUnit()) {
          return false;
        }

        
        value->Item(3) = value->Item(2); 
        value->Item(2) = value->Item(1); 
        value->Item(1).Reset(); 
      } else {
        return false;
      }
      break;
    case 2:
      
      
      
      if (eCSSUnit_Enumerated == value->Item(0).GetUnit()) {
        if (eCSSUnit_Enumerated == value->Item(1).GetUnit()) {
          
          value->Item(2) = value->Item(1); 
          xOffset.Reset();
          yOffset.Reset();
        } else {
          
          
          if ((BG_TOP | BG_BOTTOM) & value->Item(0).GetIntValue()) {
            return false;
          }
          value->Item(3) = value->Item(1); 
          xOffset.Reset();
          yEdge.Reset();
        }
      } else {
        if (eCSSUnit_Enumerated == value->Item(1).GetUnit()) {
          
          
          if ((BG_LEFT | BG_RIGHT) & value->Item(1).GetIntValue()) {
            return false;
          }
          value->Item(2) = value->Item(1); 
          value->Item(1) = value->Item(0); 
          xEdge.Reset();
          yOffset.Reset();
        } else {
          
          value->Item(3) = value->Item(1); 
          value->Item(1) = value->Item(0); 
          xEdge.Reset();
          yEdge.Reset();
        }
      }
      break;
    case 1:
      
      
      if (eCSSUnit_Enumerated == value->Item(0).GetUnit()) {
        xOffset.Reset();
      } else {
        value->Item(1) = value->Item(0); 
        xEdge.Reset();
      }
      yEdge.SetIntValue(NS_STYLE_BG_POSITION_CENTER, eCSSUnit_Enumerated);
      yOffset.Reset();
      break;
    default:
      return false;
  }

  
  
  NS_ASSERTION((eCSSUnit_Enumerated == xEdge.GetUnit()  ||
                eCSSUnit_Null       == xEdge.GetUnit()) &&
               (eCSSUnit_Enumerated == yEdge.GetUnit()  ||
                eCSSUnit_Null       == yEdge.GetUnit()) &&
                eCSSUnit_Enumerated != xOffset.GetUnit()  &&
                eCSSUnit_Enumerated != yOffset.GetUnit(),
                "Unexpected units");

  
  
  
  int32_t xEdgeEnum =
          xEdge.GetUnit() == eCSSUnit_Enumerated ? xEdge.GetIntValue() : 0;
  int32_t yEdgeEnum =
          yEdge.GetUnit() == eCSSUnit_Enumerated ? yEdge.GetIntValue() : 0;
  if ((xEdgeEnum | yEdgeEnum) == (BG_LEFT | BG_RIGHT) ||
      (xEdgeEnum | yEdgeEnum) == (BG_TOP | BG_BOTTOM) ||
      (xEdgeEnum & yEdgeEnum & ~BG_CENTER)) {
    return false;
  }

  
  
  
  if (xEdgeEnum & (BG_TOP | BG_BOTTOM) ||
      yEdgeEnum & (BG_LEFT | BG_RIGHT)) {
    nsCSSValue swapEdge = xEdge;
    nsCSSValue swapOffset = xOffset;
    xEdge = yEdge;
    xOffset = yOffset;
    yEdge = swapEdge;
    yOffset = swapOffset;
  }

  return true;
}



bool
CSSParserImpl::ParseBackgroundSize()
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValuePair valuePair;
    if (!ParseBackgroundSizeValues(valuePair)) {
      return false;
    }
    nsCSSValuePairList* item = value.SetPairListValue();
    for (;;) {
      item->mXValue = valuePair.mXValue;
      item->mYValue = valuePair.mYValue;
      if (CheckEndProperty()) {
        break;
      }
      if (!ExpectSymbol(',', true)) {
        return false;
      }
      if (!ParseBackgroundSizeValues(valuePair)) {
        return false;
      }
      item->mNext = new nsCSSValuePairList;
      item = item->mNext;
    }
  }
  AppendValue(eCSSProperty_background_size, value);
  return true;
}











#define BG_SIZE_VARIANT (VARIANT_LP | VARIANT_AUTO | VARIANT_CALC)
bool CSSParserImpl::ParseBackgroundSizeValues(nsCSSValuePair &aOut)
{
  
  nsCSSValue &xValue = aOut.mXValue,
             &yValue = aOut.mYValue;
  if (ParseNonNegativeVariant(xValue, BG_SIZE_VARIANT, nullptr)) {
    
    
    if (ParseNonNegativeVariant(yValue, BG_SIZE_VARIANT, nullptr)) {
      
      return true;
    }

    
    
    yValue.SetAutoValue();
    return true;
  }

  
  if (!ParseEnum(xValue, nsCSSProps::kBackgroundSizeKTable))
    return false;
  yValue.Reset();
  return true;
}
#undef BG_SIZE_VARIANT

bool
CSSParserImpl::ParseBorderColor()
{
  static const nsCSSProperty kBorderColorSources[] = {
    eCSSProperty_border_left_color_ltr_source,
    eCSSProperty_border_left_color_rtl_source,
    eCSSProperty_border_right_color_ltr_source,
    eCSSProperty_border_right_color_rtl_source,
    eCSSProperty_UNKNOWN
  };

  
  InitBoxPropsAsPhysical(kBorderColorSources);
  return ParseBoxProperties(kBorderColorIDs);
}

void
CSSParserImpl::SetBorderImageInitialValues()
{
  
  nsCSSValue source;
  source.SetNoneValue();
  AppendValue(eCSSProperty_border_image_source, source);

  
  nsCSSValue sliceBoxValue;
  nsCSSRect& sliceBox = sliceBoxValue.SetRectValue();
  sliceBox.SetAllSidesTo(nsCSSValue(1.0f, eCSSUnit_Percent));
  nsCSSValue slice;
  nsCSSValueList* sliceList = slice.SetListValue();
  sliceList->mValue = sliceBoxValue;
  AppendValue(eCSSProperty_border_image_slice, slice);

  
  nsCSSValue width;
  nsCSSRect& widthBox = width.SetRectValue();
  widthBox.SetAllSidesTo(nsCSSValue(1.0f, eCSSUnit_Number));
  AppendValue(eCSSProperty_border_image_width, width);

  
  nsCSSValue outset;
  nsCSSRect& outsetBox = outset.SetRectValue();
  outsetBox.SetAllSidesTo(nsCSSValue(0.0f, eCSSUnit_Number));
  AppendValue(eCSSProperty_border_image_outset, outset);

  
  nsCSSValue repeat;
  nsCSSValuePair repeatPair;
  repeatPair.SetBothValuesTo(nsCSSValue(NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH,
                                        eCSSUnit_Enumerated));
  repeat.SetPairValue(&repeatPair);
  AppendValue(eCSSProperty_border_image_repeat, repeat);
}

bool
CSSParserImpl::ParseBorderImageSlice(bool aAcceptsInherit,
                                     bool* aConsumedTokens)
{
  
  nsCSSValue value;

  if (aConsumedTokens) {
    *aConsumedTokens = true;
  }

  if (aAcceptsInherit && ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    
    AppendValue(eCSSProperty_border_image_slice, value);
    return true;
  }

  
  nsCSSValue imageSliceFillValue;
  bool hasFill = ParseEnum(imageSliceFillValue,
                           nsCSSProps::kBorderImageSliceKTable);

  
  nsCSSValue imageSliceBoxValue;
  if (!ParseGroupedBoxProperty(VARIANT_PN, imageSliceBoxValue)) {
    if (!hasFill && aConsumedTokens) {
      *aConsumedTokens = false;
    }

    return false;
  }

  
  
  if (!hasFill) {
    hasFill = ParseEnum(imageSliceFillValue,
                        nsCSSProps::kBorderImageSliceKTable);
  }

  nsCSSValueList* borderImageSlice = value.SetListValue();
  
  borderImageSlice->mValue = imageSliceBoxValue;

  if (hasFill) {
    
    borderImageSlice->mNext = new nsCSSValueList;
    borderImageSlice->mNext->mValue = imageSliceFillValue;
  }

  AppendValue(eCSSProperty_border_image_slice, value);
  return true;
}

bool
CSSParserImpl::ParseBorderImageWidth(bool aAcceptsInherit)
{
  
  nsCSSValue value;

  if (aAcceptsInherit && ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    
    AppendValue(eCSSProperty_border_image_width, value);
    return true;
  }

  
  if (!ParseGroupedBoxProperty(VARIANT_ALPN, value)) {
    return false;
  }

  AppendValue(eCSSProperty_border_image_width, value);
  return true;
}

bool
CSSParserImpl::ParseBorderImageOutset(bool aAcceptsInherit)
{
  
  nsCSSValue value;

  if (aAcceptsInherit && ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    
    AppendValue(eCSSProperty_border_image_outset, value);
    return true;
  }

  
  if (!ParseGroupedBoxProperty(VARIANT_LN, value)) {
    return false;
  }

  AppendValue(eCSSProperty_border_image_outset, value);
  return true;
}

bool
CSSParserImpl::ParseBorderImageRepeat(bool aAcceptsInherit)
{
  nsCSSValue value;
  if (aAcceptsInherit && ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    
    AppendValue(eCSSProperty_border_image_repeat, value);
    return true;
  }

  nsCSSValuePair result;
  if (!ParseEnum(result.mXValue, nsCSSProps::kBorderImageRepeatKTable)) {
    return false;
  }

  
  if (!ParseEnum(result.mYValue, nsCSSProps::kBorderImageRepeatKTable)) {
    result.mYValue = result.mXValue;
  }

  value.SetPairValue(&result);
  AppendValue(eCSSProperty_border_image_repeat, value);
  return true;
}

bool
CSSParserImpl::ParseBorderImage()
{
  nsAutoParseCompoundProperty compound(this);

  
  
  
  
  
  

  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    AppendValue(eCSSProperty_border_image_source, value);
    AppendValue(eCSSProperty_border_image_slice, value);
    AppendValue(eCSSProperty_border_image_width, value);
    AppendValue(eCSSProperty_border_image_outset, value);
    AppendValue(eCSSProperty_border_image_repeat, value);
    
    return true;
  }

  
  if (CheckEndProperty()) {
    return false;
  }

  
  SetBorderImageInitialValues();

  bool foundSource = false;
  bool foundSliceWidthOutset = false;
  bool foundRepeat = false;

  
  
  nsCSSValue imageSourceValue;
  while (!CheckEndProperty()) {
    
    if (!foundSource && ParseVariant(imageSourceValue, VARIANT_UO, nullptr)) {
      AppendValue(eCSSProperty_border_image_source, imageSourceValue);
      foundSource = true;
      continue;
    }

    
    
    
    
    
    if (!foundSliceWidthOutset) {
      bool sliceConsumedTokens = false;
      if (ParseBorderImageSlice(false, &sliceConsumedTokens)) {
        foundSliceWidthOutset = true;

        
        if (ExpectSymbol('/', true)) {
          bool foundBorderImageWidth = ParseBorderImageWidth(false);

          
          if (ExpectSymbol('/', true)) {
            if (!ParseBorderImageOutset(false)) {
              return false;
            }
          } else if (!foundBorderImageWidth) {
            
            
            return false;
          }
        }

        continue;
      } else {
        
        
        if (sliceConsumedTokens) {
          return false;
        }
      }
    }

    
    if (!foundRepeat && ParseBorderImageRepeat(false)) {
      foundRepeat = true;
      continue;
    }

    return false;
  }

  return true;
}

bool
CSSParserImpl::ParseBorderSpacing()
{
  nsCSSValue xValue, yValue;
  if (!ParseNonNegativeVariant(xValue, VARIANT_HL | VARIANT_CALC, nullptr)) {
    return false;
  }

  
  
  if (xValue.IsLengthUnit() || xValue.IsCalcUnit()) {
    ParseNonNegativeVariant(yValue, VARIANT_LENGTH | VARIANT_CALC, nullptr);
  }

  if (!ExpectEndProperty()) {
    return false;
  }

  if (yValue == xValue || yValue.GetUnit() == eCSSUnit_Null) {
    AppendValue(eCSSProperty_border_spacing, xValue);
  } else {
    nsCSSValue pair;
    pair.SetPairValue(xValue, yValue);
    AppendValue(eCSSProperty_border_spacing, pair);
  }
  return true;
}

bool
CSSParserImpl::ParseBorderSide(const nsCSSProperty aPropIDs[],
                               bool aSetAllSides)
{
  const int32_t numProps = 3;
  nsCSSValue  values[numProps];

  int32_t found = ParseChoice(values, aPropIDs, numProps);
  if ((found < 1) || (false == ExpectEndProperty())) {
    return false;
  }

  if ((found & 1) == 0) { 
    values[0].SetIntValue(NS_STYLE_BORDER_WIDTH_MEDIUM, eCSSUnit_Enumerated);
  }
  if ((found & 2) == 0) { 
    values[1].SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
  }
  if ((found & 4) == 0) { 
    values[2].SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR, eCSSUnit_Enumerated);
  }

  if (aSetAllSides) {
    static const nsCSSProperty kBorderSources[] = {
      eCSSProperty_border_left_color_ltr_source,
      eCSSProperty_border_left_color_rtl_source,
      eCSSProperty_border_right_color_ltr_source,
      eCSSProperty_border_right_color_rtl_source,
      eCSSProperty_border_left_style_ltr_source,
      eCSSProperty_border_left_style_rtl_source,
      eCSSProperty_border_right_style_ltr_source,
      eCSSProperty_border_right_style_rtl_source,
      eCSSProperty_border_left_width_ltr_source,
      eCSSProperty_border_left_width_rtl_source,
      eCSSProperty_border_right_width_ltr_source,
      eCSSProperty_border_right_width_rtl_source,
      eCSSProperty_UNKNOWN
    };

    InitBoxPropsAsPhysical(kBorderSources);

    
    for (int32_t index = 0; index < 4; index++) {
      NS_ASSERTION(numProps == 3, "This code needs updating");
      AppendValue(kBorderWidthIDs[index], values[0]);
      AppendValue(kBorderStyleIDs[index], values[1]);
      AppendValue(kBorderColorIDs[index], values[2]);
    }

    static const nsCSSProperty kBorderColorsProps[] = {
      eCSSProperty_border_top_colors,
      eCSSProperty_border_right_colors,
      eCSSProperty_border_bottom_colors,
      eCSSProperty_border_left_colors
    };

    
    
    nsCSSValue extraValue;
    switch (values[0].GetUnit()) {
    case eCSSUnit_Inherit:
    case eCSSUnit_Initial:
      extraValue = values[0];
      
      AppendValue(eCSSProperty_border_image_source, extraValue);
      AppendValue(eCSSProperty_border_image_slice, extraValue);
      AppendValue(eCSSProperty_border_image_width, extraValue);
      AppendValue(eCSSProperty_border_image_outset, extraValue);
      AppendValue(eCSSProperty_border_image_repeat, extraValue);
      break;
    default:
      extraValue.SetNoneValue();
      SetBorderImageInitialValues();
      break;
    }
    NS_FOR_CSS_SIDES(side) {
      AppendValue(kBorderColorsProps[side], extraValue);
    }
  }
  else {
    
    for (int32_t index = 0; index < numProps; index++) {
      AppendValue(aPropIDs[index], values[index]);
    }
  }
  return true;
}

bool
CSSParserImpl::ParseDirectionalBorderSide(const nsCSSProperty aPropIDs[],
                                          int32_t aSourceType)
{
  const int32_t numProps = 3;
  nsCSSValue  values[numProps];

  int32_t found = ParseChoice(values, aPropIDs, numProps);
  if ((found < 1) || (false == ExpectEndProperty())) {
    return false;
  }

  if ((found & 1) == 0) { 
    values[0].SetIntValue(NS_STYLE_BORDER_WIDTH_MEDIUM, eCSSUnit_Enumerated);
  }
  if ((found & 2) == 0) { 
    values[1].SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
  }
  if ((found & 4) == 0) { 
    values[2].SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR, eCSSUnit_Enumerated);
  }
  for (int32_t index = 0; index < numProps; index++) {
    const nsCSSProperty* subprops =
      nsCSSProps::SubpropertyEntryFor(aPropIDs[index + numProps]);
    NS_ASSERTION(subprops[3] == eCSSProperty_UNKNOWN,
                 "not box property with physical vs. logical cascading");
    AppendValue(subprops[0], values[index]);
    nsCSSValue typeVal(aSourceType, eCSSUnit_Enumerated);
    AppendValue(subprops[1], typeVal);
    AppendValue(subprops[2], typeVal);
  }
  return true;
}

bool
CSSParserImpl::ParseBorderStyle()
{
  static const nsCSSProperty kBorderStyleSources[] = {
    eCSSProperty_border_left_style_ltr_source,
    eCSSProperty_border_left_style_rtl_source,
    eCSSProperty_border_right_style_ltr_source,
    eCSSProperty_border_right_style_rtl_source,
    eCSSProperty_UNKNOWN
  };

  
  InitBoxPropsAsPhysical(kBorderStyleSources);
  return ParseBoxProperties(kBorderStyleIDs);
}

bool
CSSParserImpl::ParseBorderWidth()
{
  static const nsCSSProperty kBorderWidthSources[] = {
    eCSSProperty_border_left_width_ltr_source,
    eCSSProperty_border_left_width_rtl_source,
    eCSSProperty_border_right_width_ltr_source,
    eCSSProperty_border_right_width_rtl_source,
    eCSSProperty_UNKNOWN
  };

  
  InitBoxPropsAsPhysical(kBorderWidthSources);
  return ParseBoxProperties(kBorderWidthIDs);
}

bool
CSSParserImpl::ParseBorderColors(nsCSSProperty aProperty)
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValueList *cur = value.SetListValue();
    for (;;) {
      if (!ParseVariant(cur->mValue, VARIANT_COLOR | VARIANT_KEYWORD,
                        nsCSSProps::kBorderColorKTable)) {
        return false;
      }
      if (CheckEndProperty()) {
        break;
      }
      cur->mNext = new nsCSSValueList;
      cur = cur->mNext;
    }
  }
  AppendValue(aProperty, value);
  return true;
}


bool
CSSParserImpl::ParseCalc(nsCSSValue &aValue, int32_t aVariantMask)
{
  
  
  
  
  NS_ASSERTION(!(aVariantMask & VARIANT_NUMBER), "unexpected variant mask");
  NS_ABORT_IF_FALSE(aVariantMask != 0, "unexpected variant mask");

  bool oldUnitlessLengthQuirk = mUnitlessLengthQuirk;
  mUnitlessLengthQuirk = false;

  
  do {
    
    nsRefPtr<nsCSSValue::Array> arr = nsCSSValue::Array::Create(1);

    if (!ParseCalcAdditiveExpression(arr->Item(0), aVariantMask))
      break;

    if (!ExpectSymbol(')', true))
      break;

    aValue.SetArrayValue(arr, eCSSUnit_Calc);
    mUnitlessLengthQuirk = oldUnitlessLengthQuirk;
    return true;
  } while (false);

  SkipUntil(')');
  mUnitlessLengthQuirk = oldUnitlessLengthQuirk;
  return false;
}














bool
CSSParserImpl::ParseCalcAdditiveExpression(nsCSSValue& aValue,
                                           int32_t& aVariantMask)
{
  NS_ABORT_IF_FALSE(aVariantMask != 0, "unexpected variant mask");
  nsCSSValue *storage = &aValue;
  for (;;) {
    bool haveWS;
    if (!ParseCalcMultiplicativeExpression(*storage, aVariantMask, &haveWS))
      return false;

    if (!haveWS || !GetToken(false))
      return true;
    nsCSSUnit unit;
    if (mToken.IsSymbol('+')) {
      unit = eCSSUnit_Calc_Plus;
    } else if (mToken.IsSymbol('-')) {
      unit = eCSSUnit_Calc_Minus;
    } else {
      UngetToken();
      return true;
    }
    if (!RequireWhitespace())
      return false;

    nsRefPtr<nsCSSValue::Array> arr = nsCSSValue::Array::Create(2);
    arr->Item(0) = aValue;
    storage = &arr->Item(1);
    aValue.SetArrayValue(arr, unit);
  }
}

struct ReduceNumberCalcOps : public mozilla::css::BasicFloatCalcOps,
                             public mozilla::css::CSSValueInputCalcOps
{
  result_type ComputeLeafValue(const nsCSSValue& aValue)
  {
    NS_ABORT_IF_FALSE(aValue.GetUnit() == eCSSUnit_Number, "unexpected unit");
    return aValue.GetFloatValue();
  }

  float ComputeNumber(const nsCSSValue& aValue)
  {
    return mozilla::css::ComputeCalc(aValue, *this);
  }
};














bool
CSSParserImpl::ParseCalcMultiplicativeExpression(nsCSSValue& aValue,
                                                 int32_t& aVariantMask,
                                                 bool *aHadFinalWS)
{
  NS_ABORT_IF_FALSE(aVariantMask != 0, "unexpected variant mask");
  bool gotValue = false; 
  bool afterDivision = false;

  nsCSSValue *storage = &aValue;
  for (;;) {
    int32_t variantMask;
    if (afterDivision || gotValue) {
      variantMask = VARIANT_NUMBER;
    } else {
      variantMask = aVariantMask | VARIANT_NUMBER;
    }
    if (!ParseCalcTerm(*storage, variantMask))
      return false;
    NS_ABORT_IF_FALSE(variantMask != 0,
                      "ParseCalcTerm did not set variantMask appropriately");
    NS_ABORT_IF_FALSE(!(variantMask & VARIANT_NUMBER) ||
                      !(variantMask & ~int32_t(VARIANT_NUMBER)),
                      "ParseCalcTerm did not set variantMask appropriately");

    if (variantMask & VARIANT_NUMBER) {
      
      
      ReduceNumberCalcOps ops;
      float number = mozilla::css::ComputeCalc(*storage, ops);
      if (number == 0.0 && afterDivision)
        return false;
      storage->SetFloatValue(number, eCSSUnit_Number);
    } else {
      gotValue = true;

      if (storage != &aValue) {
        
        
        NS_ABORT_IF_FALSE(storage == &aValue.GetArrayValue()->Item(1),
                          "unexpected relationship to current storage");
        nsCSSValue &leftValue = aValue.GetArrayValue()->Item(0);
        ReduceNumberCalcOps ops;
        float number = mozilla::css::ComputeCalc(leftValue, ops);
        leftValue.SetFloatValue(number, eCSSUnit_Number);
      }
    }

    bool hadWS = RequireWhitespace();
    if (!GetToken(false)) {
      *aHadFinalWS = hadWS;
      break;
    }
    nsCSSUnit unit;
    if (mToken.IsSymbol('*')) {
      unit = gotValue ? eCSSUnit_Calc_Times_R : eCSSUnit_Calc_Times_L;
      afterDivision = false;
    } else if (mToken.IsSymbol('/')) {
      unit = eCSSUnit_Calc_Divided;
      afterDivision = true;
    } else {
      UngetToken();
      *aHadFinalWS = hadWS;
      break;
    }

    nsRefPtr<nsCSSValue::Array> arr = nsCSSValue::Array::Create(2);
    arr->Item(0) = aValue;
    storage = &arr->Item(1);
    aValue.SetArrayValue(arr, unit);
  }

  
  
  if (aVariantMask & VARIANT_NUMBER) {
    if (gotValue) {
      aVariantMask &= ~int32_t(VARIANT_NUMBER);
    } else {
      aVariantMask = VARIANT_NUMBER;
    }
  } else {
    if (!gotValue) {
      
      return false;
    }
  }

  return true;
}









bool
CSSParserImpl::ParseCalcTerm(nsCSSValue& aValue, int32_t& aVariantMask)
{
  NS_ABORT_IF_FALSE(aVariantMask != 0, "unexpected variant mask");
  if (!GetToken(true))
    return false;
  
  if (mToken.IsSymbol('(')) {
    if (!ParseCalcAdditiveExpression(aValue, aVariantMask) ||
        !ExpectSymbol(')', true)) {
      SkipUntil(')');
      return false;
    }
    return true;
  }
  
  UngetToken();
  
  
  if (!ParseVariant(aValue, aVariantMask | VARIANT_NUMBER, nullptr)) {
    return false;
  }
  
  if (!(aVariantMask & VARIANT_NUMBER) && aValue.GetUnit() == eCSSUnit_Number) {
    return false;
  }
  
  
  if (aVariantMask & VARIANT_NUMBER) {
    if (aValue.GetUnit() == eCSSUnit_Number) {
      aVariantMask = VARIANT_NUMBER;
    } else {
      aVariantMask &= ~int32_t(VARIANT_NUMBER);
    }
  }
  return true;
}



bool
CSSParserImpl::RequireWhitespace()
{
  if (!GetToken(false))
    return false;
  if (mToken.mType != eCSSToken_Whitespace) {
    UngetToken();
    return false;
  }
  
  if (GetToken(true)) {
    UngetToken();
  }
  return true;
}

bool
CSSParserImpl::ParseRect(nsCSSProperty aPropID)
{
  if (! GetToken(true)) {
    return false;
  }

  nsCSSValue val;

  if (mToken.mType == eCSSToken_Ident) {
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);
    switch (keyword) {
      case eCSSKeyword_auto:
        if (!ExpectEndProperty()) {
          return false;
        }
        val.SetAutoValue();
        break;
      case eCSSKeyword_inherit:
        if (!ExpectEndProperty()) {
          return false;
        }
        val.SetInheritValue();
        break;
      case eCSSKeyword_initial:
      case eCSSKeyword__moz_initial:
        if (!ExpectEndProperty()) {
          return false;
        }
        val.SetInitialValue();
        break;
      default:
        UngetToken();
        return false;
    }
  } else if (mToken.mType == eCSSToken_Function &&
             mToken.mIdent.LowerCaseEqualsLiteral("rect")) {
    nsCSSRect& rect = val.SetRectValue();
    bool useCommas;
    NS_FOR_CSS_SIDES(side) {
      if (! ParseVariant(rect.*(nsCSSRect::sides[side]),
                         VARIANT_AL, nullptr)) {
        return false;
      }
      if (side == 0) {
        useCommas = ExpectSymbol(',', true);
      } else if (useCommas && side < 3) {
        
        
        if (!ExpectSymbol(',', true)) {
          return false;
        }
      }
    }
    if (!ExpectSymbol(')', true)) {
      return false;
    }
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    UngetToken();
    return false;
  }

  AppendValue(aPropID, val);
  return true;
}

bool
CSSParserImpl::ParseColumns()
{
  
  
  
  
  
  static const nsCSSProperty columnIDs[] = {
    eCSSPropertyExtra_x_auto_value,
    eCSSProperty__moz_column_count,
    eCSSProperty__moz_column_width
  };
  const int32_t numProps = NS_ARRAY_LENGTH(columnIDs);

  nsCSSValue values[numProps];
  int32_t found = ParseChoice(values, columnIDs, numProps);
  if (found < 1 || !ExpectEndProperty()) {
    return false;
  }
  if ((found & (1|2|4)) == (1|2|4) &&
      values[0].GetUnit() ==  eCSSUnit_Auto) {
    
    return false;
  }

  if ((found & 2) == 0) {
    
    values[1].SetAutoValue();
  }
  if ((found & 4) == 0) {
    
    values[2].SetAutoValue();
  }

  
  for (int32_t index = 1; index < numProps; index++) {
    AppendValue(columnIDs[index], values[index]);
  }
  return true;
}

#define VARIANT_CONTENT (VARIANT_STRING | VARIANT_URL | VARIANT_COUNTER | VARIANT_ATTR | \
                         VARIANT_KEYWORD)
bool
CSSParserImpl::ParseContent()
{
  
  
  static const int32_t kContentListKWs[] = {
    eCSSKeyword_open_quote, NS_STYLE_CONTENT_OPEN_QUOTE,
    eCSSKeyword_close_quote, NS_STYLE_CONTENT_CLOSE_QUOTE,
    eCSSKeyword_no_open_quote, NS_STYLE_CONTENT_NO_OPEN_QUOTE,
    eCSSKeyword_no_close_quote, NS_STYLE_CONTENT_NO_CLOSE_QUOTE,
    eCSSKeyword_UNKNOWN,-1
  };

  static const int32_t kContentSolitaryKWs[] = {
    eCSSKeyword__moz_alt_content, NS_STYLE_CONTENT_ALT_CONTENT,
    eCSSKeyword_UNKNOWN,-1
  };

  
  
  NS_ABORT_IF_FALSE(nsCSSProps::kContentKTable[
                      ArrayLength(kContentListKWs) +
                      ArrayLength(kContentSolitaryKWs) - 4] ==
                    eCSSKeyword_UNKNOWN &&
                    nsCSSProps::kContentKTable[
                      ArrayLength(kContentListKWs) +
                      ArrayLength(kContentSolitaryKWs) - 3] == -1,
                    "content keyword tables out of sync");

  nsCSSValue value;
  if (ParseVariant(value, VARIANT_HMK | VARIANT_NONE,
                   kContentSolitaryKWs)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValueList* cur = value.SetListValue();
    for (;;) {
      if (!ParseVariant(cur->mValue, VARIANT_CONTENT, kContentListKWs)) {
        return false;
      }
      if (CheckEndProperty()) {
        break;
      }
      cur->mNext = new nsCSSValueList;
      cur = cur->mNext;
    }
  }
  AppendValue(eCSSProperty_content, value);
  return true;
}

bool
CSSParserImpl::ParseCounterData(nsCSSProperty aPropID)
{
  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    if (!GetToken(true) || mToken.mType != eCSSToken_Ident) {
      return false;
    }

    nsCSSValuePairList *cur = value.SetPairListValue();
    for (;;) {
      cur->mXValue.SetStringValue(mToken.mIdent, eCSSUnit_Ident);
      if (!GetToken(true)) {
        break;
      }
      if (mToken.mType == eCSSToken_Number && mToken.mIntegerValid) {
        cur->mYValue.SetIntValue(mToken.mInteger, eCSSUnit_Integer);
      } else {
        UngetToken();
      }
      if (CheckEndProperty()) {
        break;
      }
      if (!GetToken(true) || mToken.mType != eCSSToken_Ident) {
        return false;
      }
      cur->mNext = new nsCSSValuePairList;
      cur = cur->mNext;
    }
  }
  AppendValue(aPropID, value);
  return true;
}

bool
CSSParserImpl::ParseCursor()
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValueList* cur = value.SetListValue();
    for (;;) {
      if (!ParseVariant(cur->mValue, VARIANT_UK, nsCSSProps::kCursorKTable)) {
        return false;
      }
      if (cur->mValue.GetUnit() != eCSSUnit_URL) { 
        if (ExpectEndProperty()) {
          break;
        }
        return false;
      }

      
      nsRefPtr<nsCSSValue::Array> val = nsCSSValue::Array::Create(3);
      val->Item(0) = cur->mValue;

      
      if (ParseVariant(val->Item(1), VARIANT_NUMBER, nullptr)) {
        
        if (!ParseVariant(val->Item(2), VARIANT_NUMBER, nullptr)) {
          return false;
        }
      }
      cur->mValue.SetArrayValue(val, eCSSUnit_Array);

      if (!ExpectSymbol(',', true)) { 
        return false;
      }
      cur->mNext = new nsCSSValueList;
      cur = cur->mNext;
    }
  }
  AppendValue(eCSSProperty_cursor, value);
  return true;
}


bool
CSSParserImpl::ParseFont()
{
  static const nsCSSProperty fontIDs[] = {
    eCSSProperty_font_style,
    eCSSProperty_font_variant,
    eCSSProperty_font_weight
  };

  nsCSSValue  family;
  if (ParseVariant(family, VARIANT_HK, nsCSSProps::kFontKTable)) {
    if (ExpectEndProperty()) {
      if (eCSSUnit_Inherit == family.GetUnit() ||
          eCSSUnit_Initial == family.GetUnit()) {
        AppendValue(eCSSProperty__x_system_font, nsCSSValue(eCSSUnit_None));
        AppendValue(eCSSProperty_font_family, family);
        AppendValue(eCSSProperty_font_style, family);
        AppendValue(eCSSProperty_font_variant, family);
        AppendValue(eCSSProperty_font_weight, family);
        AppendValue(eCSSProperty_font_size, family);
        AppendValue(eCSSProperty_line_height, family);
        AppendValue(eCSSProperty_font_stretch, family);
        AppendValue(eCSSProperty_font_size_adjust, family);
        AppendValue(eCSSProperty_font_feature_settings, family);
        AppendValue(eCSSProperty_font_language_override, family);
      }
      else {
        AppendValue(eCSSProperty__x_system_font, family);
        nsCSSValue systemFont(eCSSUnit_System_Font);
        AppendValue(eCSSProperty_font_family, systemFont);
        AppendValue(eCSSProperty_font_style, systemFont);
        AppendValue(eCSSProperty_font_variant, systemFont);
        AppendValue(eCSSProperty_font_weight, systemFont);
        AppendValue(eCSSProperty_font_size, systemFont);
        AppendValue(eCSSProperty_line_height, systemFont);
        AppendValue(eCSSProperty_font_stretch, systemFont);
        AppendValue(eCSSProperty_font_size_adjust, systemFont);
        AppendValue(eCSSProperty_font_feature_settings, systemFont);
        AppendValue(eCSSProperty_font_language_override, systemFont);
      }
      return true;
    }
    return false;
  }

  
  const int32_t numProps = 3;
  nsCSSValue  values[numProps];
  int32_t found = ParseChoice(values, fontIDs, numProps);
  if ((found < 0) || (eCSSUnit_Inherit == values[0].GetUnit()) ||
      (eCSSUnit_Initial == values[0].GetUnit())) { 
    return false;
  }
  if ((found & 1) == 0) {
    
    values[0].SetIntValue(NS_FONT_STYLE_NORMAL, eCSSUnit_Enumerated);
  }
  if ((found & 2) == 0) {
    
    values[1].SetIntValue(NS_FONT_VARIANT_NORMAL, eCSSUnit_Enumerated);
  }
  if ((found & 4) == 0) {
    
    values[2].SetIntValue(NS_FONT_WEIGHT_NORMAL, eCSSUnit_Enumerated);
  }

  
  nsCSSValue  size;
  if (! ParseVariant(size, VARIANT_KEYWORD | VARIANT_LP, nsCSSProps::kFontSizeKTable)) {
    return false;
  }

  
  nsCSSValue  lineHeight;
  if (ExpectSymbol('/', true)) {
    if (! ParseNonNegativeVariant(lineHeight,
                                  VARIANT_NUMBER | VARIANT_LP | VARIANT_NORMAL,
                                  nullptr)) {
      return false;
    }
  }
  else {
    lineHeight.SetNormalValue();
  }

  
  nsAutoParseCompoundProperty compound(this);
  if (ParseFamily(family)) {
    if ((eCSSUnit_Inherit != family.GetUnit()) && (eCSSUnit_Initial != family.GetUnit()) &&
        ExpectEndProperty()) {
      AppendValue(eCSSProperty__x_system_font, nsCSSValue(eCSSUnit_None));
      AppendValue(eCSSProperty_font_family, family);
      AppendValue(eCSSProperty_font_style, values[0]);
      AppendValue(eCSSProperty_font_variant, values[1]);
      AppendValue(eCSSProperty_font_weight, values[2]);
      AppendValue(eCSSProperty_font_size, size);
      AppendValue(eCSSProperty_line_height, lineHeight);
      AppendValue(eCSSProperty_font_stretch,
                  nsCSSValue(NS_FONT_STRETCH_NORMAL, eCSSUnit_Enumerated));
      AppendValue(eCSSProperty_font_size_adjust, nsCSSValue(eCSSUnit_None));
      AppendValue(eCSSProperty_font_feature_settings, nsCSSValue(eCSSUnit_Normal));
      AppendValue(eCSSProperty_font_language_override, nsCSSValue(eCSSUnit_Normal));
      return true;
    }
  }
  return false;
}

bool
CSSParserImpl::ParseFontWeight(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_HKI | VARIANT_SYSFONT,
                   nsCSSProps::kFontWeightKTable)) {
    if (eCSSUnit_Integer == aValue.GetUnit()) { 
      int32_t intValue = aValue.GetIntValue();
      if ((100 <= intValue) &&
          (intValue <= 900) &&
          (0 == (intValue % 100))) {
        return true;
      } else {
        UngetToken();
        return false;
      }
    }
    return true;
  }
  return false;
}

bool
CSSParserImpl::ParseOneFamily(nsAString& aFamily, bool& aOneKeyword)
{
  if (!GetToken(true))
    return false;

  nsCSSToken* tk = &mToken;

  aOneKeyword = false;
  if (eCSSToken_Ident == tk->mType) {
    aOneKeyword = true;
    aFamily.Append(tk->mIdent);
    for (;;) {
      if (!GetToken(false))
        break;

      if (eCSSToken_Ident == tk->mType) {
        aOneKeyword = false;
        aFamily.Append(tk->mIdent);
      } else if (eCSSToken_Whitespace == tk->mType) {
        
        
        if (!GetToken(true))
          break;

        UngetToken();
        if (eCSSToken_Ident == tk->mType)
          aFamily.Append(PRUnichar(' '));
        else
          break;
      } else {
        UngetToken();
        break;
      }
    }
    return true;

  } else if (eCSSToken_String == tk->mType) {
    aFamily.Append(tk->mSymbol); 
    aFamily.Append(tk->mIdent); 
    aFamily.Append(tk->mSymbol);
    return true;

  } else {
    UngetToken();
    return false;
  }
}

bool
CSSParserImpl::ParseFamily(nsCSSValue& aValue)
{
  nsAutoString family;
  bool single;

  
  if (!ParseOneFamily(family, single))
    return false;

  
  
  if (single) {
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(family);
    if (keyword == eCSSKeyword_inherit) {
      aValue.SetInheritValue();
      return true;
    }
    
    if (keyword == eCSSKeyword_default) {
      return false;
    }
    if (keyword == eCSSKeyword__moz_initial || keyword == eCSSKeyword_initial) {
      aValue.SetInitialValue();
      return true;
    }
    if (keyword == eCSSKeyword__moz_use_system_font &&
        !IsParsingCompoundProperty()) {
      aValue.SetSystemFontValue();
      return true;
    }
  }

  for (;;) {
    if (!ExpectSymbol(',', true))
      break;

    family.Append(PRUnichar(','));

    nsAutoString nextFamily;
    if (!ParseOneFamily(nextFamily, single))
      return false;

    
    
    if (single) {
      nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(nextFamily);
      switch (keyword) {
        case eCSSKeyword_inherit:
        case eCSSKeyword_initial:
        case eCSSKeyword_default:
        case eCSSKeyword__moz_initial:
        case eCSSKeyword__moz_use_system_font:
          return false;
        default:
          break;
      }
    }

    family.Append(nextFamily);
  }

  if (family.IsEmpty()) {
    return false;
  }
  aValue.SetStringValue(family, eCSSUnit_Families);
  return true;
}





bool
CSSParserImpl::ParseFontSrc(nsCSSValue& aValue)
{
  
  InfallibleTArray<nsCSSValue> values;
  nsCSSValue cur;
  for (;;) {
    if (!GetToken(true))
      break;

    if (mToken.mType == eCSSToken_URL) {
      SetValueToURL(cur, mToken.mIdent);
      values.AppendElement(cur);
      if (!ParseFontSrcFormat(values))
        return false;

    } else if (mToken.mType == eCSSToken_Function &&
               mToken.mIdent.LowerCaseEqualsLiteral("local")) {
      
      
      
      
      

      nsAutoString family;
      bool single;
      if (!ParseOneFamily(family, single)) {
        SkipUntil(')');
        return false;
      }
      if (!ExpectSymbol(')', true)) {
        SkipUntil(')');
        return false;
      }

      
      
      nsFont font(family, 0, 0, 0, 0, 0, 0);
      ExtractFirstFamilyData dat;

      font.EnumerateFamilies(ExtractFirstFamily, (void*) &dat);
      if (!dat.mGood)
        return false;

      cur.SetStringValue(dat.mFamilyName, eCSSUnit_Local_Font);
      values.AppendElement(cur);
    } else {
      
      UngetToken();
      return false;
    }

    if (!ExpectSymbol(',', true))
      break;
  }

  if (values.Length() == 0)
    return false;

  nsRefPtr<nsCSSValue::Array> srcVals
    = nsCSSValue::Array::Create(values.Length());

  uint32_t i;
  for (i = 0; i < values.Length(); i++)
    srcVals->Item(i) = values[i];
  aValue.SetArrayValue(srcVals, eCSSUnit_Array);
  return true;
}

bool
CSSParserImpl::ParseFontSrcFormat(InfallibleTArray<nsCSSValue> & values)
{
  if (!GetToken(true))
    return true; 
  if (mToken.mType != eCSSToken_Function ||
      !mToken.mIdent.LowerCaseEqualsLiteral("format")) {
    UngetToken();
    return true;
  }

  do {
    if (!GetToken(true))
      return false; 

    if (mToken.mType != eCSSToken_String) {
      UngetToken();
      SkipUntil(')');
      return false;
    }

    nsCSSValue cur(mToken.mIdent, eCSSUnit_Font_Format);
    values.AppendElement(cur);
  } while (ExpectSymbol(',', true));

  if (!ExpectSymbol(')', true)) {
    SkipUntil(')');
    return false;
  }

  return true;
}


bool
CSSParserImpl::ParseFontRanges(nsCSSValue& aValue)
{
  InfallibleTArray<uint32_t> ranges;
  for (;;) {
    if (!GetToken(true))
      break;

    if (mToken.mType != eCSSToken_URange) {
      UngetToken();
      break;
    }

    
    
    if (!mToken.mIntegerValid)
      return false;

    uint32_t low = mToken.mInteger;
    uint32_t high = mToken.mInteger2;

    
    
    
    
    if (low <= 0x10FFFF && low <= high) {
      if (high > 0x10FFFF)
        high = 0x10FFFF;

      ranges.AppendElement(low);
      ranges.AppendElement(high);
    }
    if (!ExpectSymbol(',', true))
      break;
  }

  if (ranges.Length() == 0)
    return false;

  nsRefPtr<nsCSSValue::Array> srcVals
    = nsCSSValue::Array::Create(ranges.Length());

  for (uint32_t i = 0; i < ranges.Length(); i++)
    srcVals->Item(i).SetIntValue(ranges[i], eCSSUnit_Integer);
  aValue.SetArrayValue(srcVals, eCSSUnit_Array);
  return true;
}










#define FEATURE_TAG_LENGTH   4

static bool
ValidFontFeatureTag(const nsString& aTag)
{
  if (aTag.Length() != FEATURE_TAG_LENGTH) {
    return false;
  }
  uint32_t i;
  for (i = 0; i < FEATURE_TAG_LENGTH; i++) {
    uint32_t ch = aTag[i];
    if (ch < 0x20 || ch > 0x7e) {
      return false;
    }
  }
  return true;
}

bool
CSSParserImpl::ParseFontFeatureSettings(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_INHERIT | VARIANT_NORMAL, nullptr)) {
    return true;
  }

  nsCSSValuePairList *cur = aValue.SetPairListValue();
  for (;;) {
    
    if (!GetToken(true)) {
      return false;
    }

    if (mToken.mType != eCSSToken_String ||
        !ValidFontFeatureTag(mToken.mIdent)) {
      UngetToken();
      return false;
    }
    cur->mXValue.SetStringValue(mToken.mIdent, eCSSUnit_String);

    if (!GetToken(true)) {
      cur->mYValue.SetIntValue(1, eCSSUnit_Integer);
      break;
    }

    
    if (mToken.mType == eCSSToken_Number && mToken.mIntegerValid &&
        mToken.mInteger >= 0) {
      cur->mYValue.SetIntValue(mToken.mInteger, eCSSUnit_Integer);
    } else if (mToken.mType == eCSSToken_Ident &&
               mToken.mIdent.LowerCaseEqualsLiteral("on")) {
      cur->mYValue.SetIntValue(1, eCSSUnit_Integer);
    } else if (mToken.mType == eCSSToken_Ident &&
               mToken.mIdent.LowerCaseEqualsLiteral("off")) {
      cur->mYValue.SetIntValue(0, eCSSUnit_Integer);
    } else {
      
      cur->mYValue.SetIntValue(1, eCSSUnit_Integer);
      UngetToken();
    }

    if (!ExpectSymbol(',', true)) {
      break;
    }

    cur->mNext = new nsCSSValuePairList;
    cur = cur->mNext;
  }

  return true;
}

bool
CSSParserImpl::ParseListStyle()
{
  
  
  
  
  
  static const nsCSSProperty listStyleIDs[] = {
    eCSSPropertyExtra_x_none_value,
    eCSSProperty_list_style_type,
    eCSSProperty_list_style_position,
    eCSSProperty_list_style_image
  };

  nsCSSValue values[NS_ARRAY_LENGTH(listStyleIDs)];
  int32_t found =
    ParseChoice(values, listStyleIDs, ArrayLength(listStyleIDs));
  if (found < 1 || !ExpectEndProperty()) {
    return false;
  }

  if ((found & (1|2|8)) == (1|2|8)) {
    if (values[0].GetUnit() == eCSSUnit_None) {
      
      
      
      return false;
    } else {
      NS_ASSERTION(found == (1|2|4|8) && values[0] == values[1] &&
                   values[0] == values[2] && values[0] == values[3],
                   "should be a special value");
    }
  }

  
  if ((found & 2) == 0) {
    if (found & 1) {
      values[1].SetIntValue(NS_STYLE_LIST_STYLE_NONE, eCSSUnit_Enumerated);
    } else {
      values[1].SetIntValue(NS_STYLE_LIST_STYLE_DISC, eCSSUnit_Enumerated);
    }
  }
  if ((found & 4) == 0) {
    values[2].SetIntValue(NS_STYLE_LIST_STYLE_POSITION_OUTSIDE,
                          eCSSUnit_Enumerated);
  }
  if ((found & 8) == 0) {
    values[3].SetNoneValue();
  }

  
  for (uint32_t index = 1; index < ArrayLength(listStyleIDs); ++index) {
    AppendValue(listStyleIDs[index], values[index]);
  }
  return true;
}

bool
CSSParserImpl::ParseMargin()
{
  static const nsCSSProperty kMarginSideIDs[] = {
    eCSSProperty_margin_top,
    eCSSProperty_margin_right_value,
    eCSSProperty_margin_bottom,
    eCSSProperty_margin_left_value
  };
  static const nsCSSProperty kMarginSources[] = {
    eCSSProperty_margin_left_ltr_source,
    eCSSProperty_margin_left_rtl_source,
    eCSSProperty_margin_right_ltr_source,
    eCSSProperty_margin_right_rtl_source,
    eCSSProperty_UNKNOWN
  };

  
  InitBoxPropsAsPhysical(kMarginSources);
  return ParseBoxProperties(kMarginSideIDs);
}

bool
CSSParserImpl::ParseMarks(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_HK, nsCSSProps::kPageMarksKTable)) {
    if (eCSSUnit_Enumerated == aValue.GetUnit()) {
      if (NS_STYLE_PAGE_MARKS_NONE != aValue.GetIntValue() &&
          false == CheckEndProperty()) {
        nsCSSValue second;
        if (ParseEnum(second, nsCSSProps::kPageMarksKTable)) {
          
          if (NS_STYLE_PAGE_MARKS_NONE != second.GetIntValue()) {
            aValue.SetIntValue(aValue.GetIntValue() | second.GetIntValue(),
                               eCSSUnit_Enumerated);
            return true;
          }
        }
        return false;
      }
    }
    return true;
  }
  return false;
}

bool
CSSParserImpl::ParseOutline()
{
  const int32_t numProps = 3;
  static const nsCSSProperty kOutlineIDs[] = {
    eCSSProperty_outline_color,
    eCSSProperty_outline_style,
    eCSSProperty_outline_width
  };

  nsCSSValue  values[numProps];
  int32_t found = ParseChoice(values, kOutlineIDs, numProps);
  if ((found < 1) || (false == ExpectEndProperty())) {
    return false;
  }

  
  if ((found & 1) == 0) { 
    values[0].SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR, eCSSUnit_Enumerated);
  }
  if ((found & 2) == 0) { 
    values[1].SetIntValue(NS_STYLE_BORDER_STYLE_NONE, eCSSUnit_Enumerated);
  }
  if ((found & 4) == 0) { 
    values[2].SetIntValue(NS_STYLE_BORDER_WIDTH_MEDIUM, eCSSUnit_Enumerated);
  }

  int32_t index;
  for (index = 0; index < numProps; index++) {
    AppendValue(kOutlineIDs[index], values[index]);
  }
  return true;
}

bool
CSSParserImpl::ParseOverflow()
{
  nsCSSValue overflow;
  if (!ParseVariant(overflow, VARIANT_HK,
                    nsCSSProps::kOverflowKTable) ||
      !ExpectEndProperty())
    return false;

  nsCSSValue overflowX(overflow);
  nsCSSValue overflowY(overflow);
  if (eCSSUnit_Enumerated == overflow.GetUnit())
    switch(overflow.GetIntValue()) {
      case NS_STYLE_OVERFLOW_SCROLLBARS_HORIZONTAL:
        overflowX.SetIntValue(NS_STYLE_OVERFLOW_SCROLL, eCSSUnit_Enumerated);
        overflowY.SetIntValue(NS_STYLE_OVERFLOW_HIDDEN, eCSSUnit_Enumerated);
        break;
      case NS_STYLE_OVERFLOW_SCROLLBARS_VERTICAL:
        overflowX.SetIntValue(NS_STYLE_OVERFLOW_HIDDEN, eCSSUnit_Enumerated);
        overflowY.SetIntValue(NS_STYLE_OVERFLOW_SCROLL, eCSSUnit_Enumerated);
        break;
    }
  AppendValue(eCSSProperty_overflow_x, overflowX);
  AppendValue(eCSSProperty_overflow_y, overflowY);
  return true;
}

bool
CSSParserImpl::ParsePadding()
{
  static const nsCSSProperty kPaddingSideIDs[] = {
    eCSSProperty_padding_top,
    eCSSProperty_padding_right_value,
    eCSSProperty_padding_bottom,
    eCSSProperty_padding_left_value
  };
  static const nsCSSProperty kPaddingSources[] = {
    eCSSProperty_padding_left_ltr_source,
    eCSSProperty_padding_left_rtl_source,
    eCSSProperty_padding_right_ltr_source,
    eCSSProperty_padding_right_rtl_source,
    eCSSProperty_UNKNOWN
  };

  
  InitBoxPropsAsPhysical(kPaddingSources);
  return ParseBoxProperties(kPaddingSideIDs);
}

bool
CSSParserImpl::ParseQuotes()
{
  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_HOS, nullptr)) {
    return false;
  }
  if (value.GetUnit() != eCSSUnit_String) {
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValue open = value;
    nsCSSValuePairList* quotes = value.SetPairListValue();
    for (;;) {
      quotes->mXValue = open;
      
      if (!ParseVariant(quotes->mYValue, VARIANT_STRING, nullptr)) {
        return false;
      }
      if (CheckEndProperty()) {
        break;
      }
      
      if (!ParseVariant(open, VARIANT_STRING, nullptr)) {
        return false;
      }
      quotes->mNext = new nsCSSValuePairList;
      quotes = quotes->mNext;
    }
  }
  AppendValue(eCSSProperty_quotes, value);
  return true;
}

bool
CSSParserImpl::ParseSize()
{
  nsCSSValue width, height;
  if (!ParseVariant(width, VARIANT_AHKL, nsCSSProps::kPageSizeKTable)) {
    return false;
  }
  if (width.IsLengthUnit()) {
    ParseVariant(height, VARIANT_LENGTH, nullptr);
  }
  if (!ExpectEndProperty()) {
    return false;
  }

  if (width == height || height.GetUnit() == eCSSUnit_Null) {
    AppendValue(eCSSProperty_size, width);
  } else {
    nsCSSValue pair;
    pair.SetPairValue(width, height);
    AppendValue(eCSSProperty_size, pair);
  }
  return true;
}

bool
CSSParserImpl::ParseTextDecoration()
{
  enum {
    eDecorationNone         = NS_STYLE_TEXT_DECORATION_LINE_NONE,
    eDecorationUnderline    = NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE,
    eDecorationOverline     = NS_STYLE_TEXT_DECORATION_LINE_OVERLINE,
    eDecorationLineThrough  = NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH,
    eDecorationBlink        = NS_STYLE_TEXT_DECORATION_LINE_BLINK,
    eDecorationPrefAnchors  = NS_STYLE_TEXT_DECORATION_LINE_PREF_ANCHORS
  };
  MOZ_STATIC_ASSERT((eDecorationNone ^ eDecorationUnderline ^
                     eDecorationOverline ^ eDecorationLineThrough ^
                     eDecorationBlink ^ eDecorationPrefAnchors) ==
                    (eDecorationNone | eDecorationUnderline |
                     eDecorationOverline | eDecorationLineThrough |
                     eDecorationBlink | eDecorationPrefAnchors),
                    "text decoration constants need to be bitmasks");

  static const int32_t kTextDecorationKTable[] = {
    eCSSKeyword_none,                   eDecorationNone,
    eCSSKeyword_underline,              eDecorationUnderline,
    eCSSKeyword_overline,               eDecorationOverline,
    eCSSKeyword_line_through,           eDecorationLineThrough,
    eCSSKeyword_blink,                  eDecorationBlink,
    eCSSKeyword__moz_anchor_decoration, eDecorationPrefAnchors,
    eCSSKeyword_UNKNOWN,-1
  };

  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_HK, kTextDecorationKTable)) {
    return false;
  }

  nsCSSValue blink, line, style, color;
  switch (value.GetUnit()) {
    case eCSSUnit_Enumerated: {
      
      
      color.SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR,
                        eCSSUnit_Enumerated);
      style.SetIntValue(NS_STYLE_TEXT_DECORATION_STYLE_SOLID,
                        eCSSUnit_Enumerated);

      int32_t intValue = value.GetIntValue();
      if (intValue == eDecorationNone) {
        blink.SetIntValue(NS_STYLE_TEXT_BLINK_NONE, eCSSUnit_Enumerated);
        line.SetIntValue(NS_STYLE_TEXT_DECORATION_LINE_NONE,
                         eCSSUnit_Enumerated);
        break;
      }

      
      nsCSSValue keyword;
      int32_t index;
      for (index = 0; index < 3; index++) {
        if (!ParseEnum(keyword, kTextDecorationKTable)) {
          break;
        }
        int32_t newValue = keyword.GetIntValue();
        if (newValue == eDecorationNone || newValue & intValue) {
          
          
          return false;
        }
        intValue |= newValue;
      }

      blink.SetIntValue((intValue & eDecorationBlink) != 0 ?
                          NS_STYLE_TEXT_BLINK_BLINK : NS_STYLE_TEXT_BLINK_NONE,
                        eCSSUnit_Enumerated);
      line.SetIntValue((intValue & ~eDecorationBlink), eCSSUnit_Enumerated);
      break;
    }
    default:
      blink = line = color = style = value;
      break;
  }

  AppendValue(eCSSProperty_text_blink, blink);
  AppendValue(eCSSProperty_text_decoration_line, line);
  AppendValue(eCSSProperty_text_decoration_color, color);
  AppendValue(eCSSProperty_text_decoration_style, style);

  return true;
}

bool
CSSParserImpl::ParseTextDecorationLine(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_HK, nsCSSProps::kTextDecorationLineKTable)) {
    if (eCSSUnit_Enumerated == aValue.GetUnit()) {
      int32_t intValue = aValue.GetIntValue();
      if (intValue != NS_STYLE_TEXT_DECORATION_LINE_NONE) {
        
        nsCSSValue  keyword;
        int32_t index;
        for (index = 0; index < 2; index++) {
          if (ParseEnum(keyword, nsCSSProps::kTextDecorationLineKTable)) {
            int32_t newValue = keyword.GetIntValue();
            if (newValue == NS_STYLE_TEXT_DECORATION_LINE_NONE ||
                newValue & intValue) {
              
              
              return false;
            }
            intValue |= newValue;
          }
          else {
            break;
          }
        }
        aValue.SetIntValue(intValue, eCSSUnit_Enumerated);
      }
    }
    return true;
  }
  return false;
}

bool
CSSParserImpl::ParseTextOverflow(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_INHERIT, nullptr)) {
    
    return true;
  }

  nsCSSValue left;
  if (!ParseVariant(left, VARIANT_KEYWORD | VARIANT_STRING,
                    nsCSSProps::kTextOverflowKTable))
    return false;

  nsCSSValue right;
  if (ParseVariant(right, VARIANT_KEYWORD | VARIANT_STRING,
                    nsCSSProps::kTextOverflowKTable))
    aValue.SetPairValue(left, right);
  else {
    aValue = left;
  }
  return true;
}







bool
CSSParserImpl::ParseFunctionInternals(const int32_t aVariantMask[],
                                      uint16_t aMinElems,
                                      uint16_t aMaxElems,
                                      InfallibleTArray<nsCSSValue> &aOutput)
{
  for (uint16_t index = 0; index < aMaxElems; ++index) {
    nsCSSValue newValue;
    if (!ParseVariant(newValue, aVariantMask[index], nullptr))
      return false;

    aOutput.AppendElement(newValue);

    
    if (!ExpectSymbol(',', true)) {
      
      
      return ExpectSymbol(')', true) && (index + 1) >= aMinElems;
    }
  }

  
  
  return false;
}



















bool
CSSParserImpl::ParseFunction(const nsString &aFunction,
                             const int32_t aAllowedTypes[],
                             uint16_t aMinElems, uint16_t aMaxElems,
                             nsCSSValue &aValue)
{
  typedef InfallibleTArray<nsCSSValue>::size_type arrlen_t;

  


  static const arrlen_t MAX_ALLOWED_ELEMS = 0xFFFE;

  



  nsString functionName(aFunction);

  


  InfallibleTArray<nsCSSValue> foundValues;
  if (!ParseFunctionInternals(aAllowedTypes, aMinElems, aMaxElems,
                              foundValues))
    return false;

  




  uint16_t numElements = (foundValues.Length() <= MAX_ALLOWED_ELEMS ?
                          foundValues.Length() + 1 : MAX_ALLOWED_ELEMS);
  nsRefPtr<nsCSSValue::Array> convertedArray =
    nsCSSValue::Array::Create(numElements);

  
  convertedArray->Item(0).SetStringValue(functionName, eCSSUnit_Ident);
  for (uint16_t index = 0; index + 1 < numElements; ++index)
    convertedArray->Item(index + 1) = foundValues[static_cast<arrlen_t>(index)];

  
  aValue.SetArrayValue(convertedArray, eCSSUnit_Function);

  
  return true;
}













static bool GetFunctionParseInformation(nsCSSKeyword aToken,
                                        bool aIsPrefixed,
                                        uint16_t &aMinElems,
                                        uint16_t &aMaxElems,
                                        const int32_t *& aVariantMask,
                                        bool &aIs3D)
{




  enum { eLengthPercentCalc,
         eLengthCalc,
         eTwoLengthPercentCalcs,
         eTwoLengthPercentCalcsOneLengthCalc,
         eAngle,
         eTwoAngles,
         eNumber,
         ePositiveLength,
         eTwoNumbers,
         eThreeNumbers,
         eThreeNumbersOneAngle,
         eMatrix,
         eMatrixPrefixed,
         eMatrix3d,
         eMatrix3dPrefixed,
         eNumVariantMasks };
  static const int32_t kMaxElemsPerFunction = 16;
  static const int32_t kVariantMasks[eNumVariantMasks][kMaxElemsPerFunction] = {
    {VARIANT_LPCALC},
    {VARIANT_LENGTH|VARIANT_CALC},
    {VARIANT_LPCALC, VARIANT_LPCALC},
    {VARIANT_LPCALC, VARIANT_LPCALC, VARIANT_LENGTH|VARIANT_CALC},
    {VARIANT_ANGLE_OR_ZERO},
    {VARIANT_ANGLE_OR_ZERO, VARIANT_ANGLE_OR_ZERO},
    {VARIANT_NUMBER},
    {VARIANT_LENGTH|VARIANT_POSITIVE_DIMENSION},
    {VARIANT_NUMBER, VARIANT_NUMBER},
    {VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER},
    {VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_ANGLE_OR_ZERO},
    {VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER,
     VARIANT_NUMBER, VARIANT_NUMBER},
    {VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER,
     VARIANT_LPNCALC, VARIANT_LPNCALC},
    {VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER,
     VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER,
     VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER,
     VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER},
    {VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER,
     VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER,
     VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER,
     VARIANT_LPNCALC, VARIANT_LPNCALC, VARIANT_LNCALC, VARIANT_NUMBER}};

#ifdef DEBUG
  static const uint8_t kVariantMaskLengths[eNumVariantMasks] =
    {1, 1, 2, 3, 1, 2, 1, 1, 2, 3, 4, 6, 6, 16, 16};
#endif

  int32_t variantIndex = eNumVariantMasks;

  aIs3D = false;

  switch (aToken) {
  case eCSSKeyword_translatex:
  case eCSSKeyword_translatey:
    
    variantIndex = eLengthPercentCalc;
    aMinElems = 1U;
    aMaxElems = 1U;
    break;
  case eCSSKeyword_translatez:
    
    variantIndex = eLengthCalc;
    aMinElems = 1U;
    aMaxElems = 1U;
    aIs3D = true;
    break;
  case eCSSKeyword_translate3d:
    
    variantIndex = eTwoLengthPercentCalcsOneLengthCalc;
    aMinElems = 3U;
    aMaxElems = 3U;
    aIs3D = true;
    break;
  case eCSSKeyword_scalez:
    aIs3D = true;
  case eCSSKeyword_scalex:
  case eCSSKeyword_scaley:
    
    variantIndex = eNumber;
    aMinElems = 1U;
    aMaxElems = 1U;
    break;
  case eCSSKeyword_scale3d:
    
    variantIndex = eThreeNumbers;
    aMinElems = 3U;
    aMaxElems = 3U;
    aIs3D = true;
    break;
  case eCSSKeyword_rotatex:
  case eCSSKeyword_rotatey:
    aIs3D = true;
  case eCSSKeyword_rotate:
  case eCSSKeyword_rotatez:
    
    variantIndex = eAngle;
    aMinElems = 1U;
    aMaxElems = 1U;
    break;
  case eCSSKeyword_rotate3d:
    variantIndex = eThreeNumbersOneAngle;
    aMinElems = 4U;
    aMaxElems = 4U;
    aIs3D = true;
    break;
  case eCSSKeyword_translate:
    
    variantIndex = eTwoLengthPercentCalcs;
    aMinElems = 1U;
    aMaxElems = 2U;
    break;
  case eCSSKeyword_skew:
    
    variantIndex = eTwoAngles;
    aMinElems = 1U;
    aMaxElems = 2U;
    break;
  case eCSSKeyword_scale:
    
    variantIndex = eTwoNumbers;
    aMinElems = 1U;
    aMaxElems = 2U;
    break;
  case eCSSKeyword_skewx:
    
    variantIndex = eAngle;
    aMinElems = 1U;
    aMaxElems = 1U;
    break;
  case eCSSKeyword_skewy:
    
    variantIndex = eAngle;
    aMinElems = 1U;
    aMaxElems = 1U;
    break;
  case eCSSKeyword_matrix:
    
    variantIndex = aIsPrefixed ? eMatrixPrefixed : eMatrix;
    aMinElems = 6U;
    aMaxElems = 6U;
    break;
  case eCSSKeyword_matrix3d:
    
    variantIndex = aIsPrefixed ? eMatrix3dPrefixed : eMatrix3d;
    aMinElems = 16U;
    aMaxElems = 16U;
    aIs3D = true;
    break;
  case eCSSKeyword_perspective:
    
    variantIndex = ePositiveLength;
    aMinElems = 1U;
    aMaxElems = 1U;
    aIs3D = true;
    break;
  default:
    
    return false;
  }

  NS_ASSERTION(aMinElems > 0, "Didn't update minimum elements!");
  NS_ASSERTION(aMaxElems > 0, "Didn't update maximum elements!");
  NS_ASSERTION(aMinElems <= aMaxElems, "aMinElems > aMaxElems!");
  NS_ASSERTION(variantIndex >= 0, "Invalid variant mask!");
  NS_ASSERTION(variantIndex < eNumVariantMasks, "Invalid variant mask!");
#ifdef DEBUG
  NS_ASSERTION(aMaxElems <= kVariantMaskLengths[variantIndex],
               "Invalid aMaxElems for this variant mask.");
#endif

  
  aVariantMask = kVariantMasks[variantIndex];

  return true;
}




bool
CSSParserImpl::ParseSingleTransform(bool aIsPrefixed,
                                    nsCSSValue& aValue, bool& aIs3D)
{
  if (!GetToken(true))
    return false;

  if (mToken.mType != eCSSToken_Function) {
    UngetToken();
    return false;
  }

  const int32_t* variantMask;
  uint16_t minElems, maxElems;
  nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);

  if (!GetFunctionParseInformation(keyword, aIsPrefixed,
                                   minElems, maxElems, variantMask, aIs3D))
    return false;

  
  
  
  nsContentUtils::ASCIIToLower(mToken.mIdent);
  switch (keyword) {
    case eCSSKeyword_rotatex:
    case eCSSKeyword_scalex:
    case eCSSKeyword_skewx:
    case eCSSKeyword_translatex:
      mToken.mIdent.Replace(mToken.mIdent.Length() - 1, 1, PRUnichar('X'));
      break;

    case eCSSKeyword_rotatey:
    case eCSSKeyword_scaley:
    case eCSSKeyword_skewy:
    case eCSSKeyword_translatey:
      mToken.mIdent.Replace(mToken.mIdent.Length() - 1, 1, PRUnichar('Y'));
      break;

    case eCSSKeyword_rotatez:
    case eCSSKeyword_scalez:
    case eCSSKeyword_translatez:
      mToken.mIdent.Replace(mToken.mIdent.Length() - 1, 1, PRUnichar('Z'));
      break;

    default:
      break;
  }

  return ParseFunction(mToken.mIdent, variantMask, minElems, maxElems, aValue);
}




bool CSSParserImpl::ParseTransform(bool aIsPrefixed)
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValueList* cur = value.SetListValue();
    for (;;) {
      bool is3D;
      if (!ParseSingleTransform(aIsPrefixed, cur->mValue, is3D)) {
        return false;
      }
      if (is3D && !nsLayoutUtils::Are3DTransformsEnabled()) {
        return false;
      }
      if (CheckEndProperty()) {
        break;
      }
      cur->mNext = new nsCSSValueList;
      cur = cur->mNext;
    }
  }
  AppendValue(eCSSProperty_transform, value);
  return true;
}

bool CSSParserImpl::ParseTransformOrigin(bool aPerspective)
{
  nsCSSValuePair position;
  if (!ParseBoxPositionValues(position, true))
    return false;

  nsCSSProperty prop = eCSSProperty_transform_origin;
  if (aPerspective) {
    if (!ExpectEndProperty()) {
      return false;
    }
    prop = eCSSProperty_perspective_origin;
  }

  
  
  
  if (position.mXValue.GetUnit() == eCSSUnit_Inherit ||
      position.mXValue.GetUnit() == eCSSUnit_Initial) {
    NS_ABORT_IF_FALSE(position.mXValue == position.mYValue,
                      "inherit/initial only half?");
    AppendValue(prop, position.mXValue);
  } else {
    nsCSSValue value;
    if (aPerspective) {
      value.SetPairValue(position.mXValue, position.mYValue);
    } else {
      nsCSSValue depth;
      if (!nsLayoutUtils::Are3DTransformsEnabled() ||
          
          !ParseVariant(depth, VARIANT_LENGTH | VARIANT_CALC, nullptr)) {
        depth.SetFloatValue(0.0f, eCSSUnit_Pixel);
      }
      value.SetTripletValue(position.mXValue, position.mYValue, depth);
    }

    AppendValue(prop, value);
  }
  return true;
}

bool
CSSParserImpl::ParseTransitionProperty()
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    
    
    
    
    nsCSSValueList* cur = value.SetListValue();
    for (;;) {
      if (!ParseVariant(cur->mValue, VARIANT_IDENTIFIER | VARIANT_ALL, nullptr)) {
        return false;
      }
      if (cur->mValue.GetUnit() == eCSSUnit_Ident) {
        nsDependentString str(cur->mValue.GetStringBufferValue());
        
        
        if (str.LowerCaseEqualsLiteral("none") ||
            str.LowerCaseEqualsLiteral("inherit") ||
            str.LowerCaseEqualsLiteral("initial")) {
          return false;
        }
      }
      if (CheckEndProperty()) {
        break;
      }
      if (!ExpectSymbol(',', true)) {
        REPORT_UNEXPECTED_TOKEN(PEExpectedComma);
        return false;
      }
      cur->mNext = new nsCSSValueList;
      cur = cur->mNext;
    }
  }
  AppendValue(eCSSProperty_transition_property, value);
  return true;
}

bool
CSSParserImpl::ParseTransitionTimingFunctionValues(nsCSSValue& aValue)
{
  NS_ASSERTION(!mHavePushBack &&
               mToken.mType == eCSSToken_Function &&
               mToken.mIdent.LowerCaseEqualsLiteral("cubic-bezier"),
               "unexpected initial state");

  nsRefPtr<nsCSSValue::Array> val = nsCSSValue::Array::Create(4);

  float x1, x2, y1, y2;
  if (!ParseTransitionTimingFunctionValueComponent(x1, ',', true) ||
      !ParseTransitionTimingFunctionValueComponent(y1, ',', false) ||
      !ParseTransitionTimingFunctionValueComponent(x2, ',', true) ||
      !ParseTransitionTimingFunctionValueComponent(y2, ')', false)) {
    return false;
  }

  val->Item(0).SetFloatValue(x1, eCSSUnit_Number);
  val->Item(1).SetFloatValue(y1, eCSSUnit_Number);
  val->Item(2).SetFloatValue(x2, eCSSUnit_Number);
  val->Item(3).SetFloatValue(y2, eCSSUnit_Number);

  aValue.SetArrayValue(val, eCSSUnit_Cubic_Bezier);

  return true;
}

bool
CSSParserImpl::ParseTransitionTimingFunctionValueComponent(float& aComponent,
                                                           char aStop,
                                                           bool aCheckRange)
{
  if (!GetToken(true)) {
    return false;
  }
  nsCSSToken* tk = &mToken;
  if (tk->mType == eCSSToken_Number) {
    float num = tk->mNumber;
    if (aCheckRange && (num < 0.0 || num > 1.0)) {
      return false;
    }
    aComponent = num;
    if (ExpectSymbol(aStop, true)) {
      return true;
    }
  }
  return false;
}

bool
CSSParserImpl::ParseTransitionStepTimingFunctionValues(nsCSSValue& aValue)
{
  NS_ASSERTION(!mHavePushBack &&
               mToken.mType == eCSSToken_Function &&
               mToken.mIdent.LowerCaseEqualsLiteral("steps"),
               "unexpected initial state");

  nsRefPtr<nsCSSValue::Array> val = nsCSSValue::Array::Create(2);

  if (!ParseOneOrLargerVariant(val->Item(0), VARIANT_INTEGER, nullptr)) {
    return false;
  }

  int32_t type = NS_STYLE_TRANSITION_TIMING_FUNCTION_STEP_END;
  if (ExpectSymbol(',', true)) {
    if (!GetToken(true)) {
      return false;
    }
    type = -1;
    if (mToken.mType == eCSSToken_Ident) {
      if (mToken.mIdent.LowerCaseEqualsLiteral("start")) {
        type = NS_STYLE_TRANSITION_TIMING_FUNCTION_STEP_START;
      } else if (mToken.mIdent.LowerCaseEqualsLiteral("end")) {
        type = NS_STYLE_TRANSITION_TIMING_FUNCTION_STEP_END;
      }
    }
    if (type == -1) {
      UngetToken();
      return false;
    }
  }
  val->Item(1).SetIntValue(type, eCSSUnit_Enumerated);

  if (!ExpectSymbol(')', true)) {
    return false;
  }

  aValue.SetArrayValue(val, eCSSUnit_Steps);
  return true;
}

static nsCSSValueList*
AppendValueToList(nsCSSValue& aContainer,
                  nsCSSValueList* aTail,
                  const nsCSSValue& aValue)
{
  nsCSSValueList* entry;
  if (aContainer.GetUnit() == eCSSUnit_Null) {
    NS_ABORT_IF_FALSE(!aTail, "should not have an entry");
    entry = aContainer.SetListValue();
  } else {
    NS_ABORT_IF_FALSE(!aTail->mNext, "should not have a next entry");
    NS_ABORT_IF_FALSE(aContainer.GetUnit() == eCSSUnit_List, "not a list");
    entry = new nsCSSValueList;
    aTail->mNext = entry;
  }
  entry->mValue = aValue;
  return entry;
}

CSSParserImpl::ParseAnimationOrTransitionShorthandResult
CSSParserImpl::ParseAnimationOrTransitionShorthand(
                 const nsCSSProperty* aProperties,
                 const nsCSSValue* aInitialValues,
                 nsCSSValue* aValues,
                 size_t aNumProperties)
{
  nsCSSValue tempValue;
  
  
  
  if (ParseVariant(tempValue, VARIANT_INHERIT, nullptr)) {
    for (uint32_t i = 0; i < aNumProperties; ++i) {
      AppendValue(aProperties[i], tempValue);
    }
    return eParseAnimationOrTransitionShorthand_Inherit;
  }

  static const size_t maxNumProperties = 7;
  NS_ABORT_IF_FALSE(aNumProperties <= maxNumProperties,
                    "can't handle this many properties");
  nsCSSValueList *cur[maxNumProperties];
  bool parsedProperty[maxNumProperties];

  for (size_t i = 0; i < aNumProperties; ++i) {
    cur[i] = nullptr;
  }
  bool atEOP = false; 
  for (;;) { 
    
    
    for (size_t i = 0; i < aNumProperties; ++i) {
      parsedProperty[i] = false;
    }
    for (;;) { 
      bool foundProperty = false;
      
      
      
      if (ExpectSymbol(',', true))
        break;
      if (CheckEndProperty()) {
        atEOP = true;
        break;
      }

      
      for (uint32_t i = 0; !foundProperty && i < aNumProperties; ++i) {
        if (!parsedProperty[i]) {
          
          if (ParseSingleValueProperty(tempValue, aProperties[i])) {
            parsedProperty[i] = true;
            cur[i] = AppendValueToList(aValues[i], cur[i], tempValue);
            foundProperty = true;
            break; 
          }
        }
      }
      if (!foundProperty) {
        
        
        return eParseAnimationOrTransitionShorthand_Error;
      }
    }

    
    
    for (uint32_t i = 0; i < aNumProperties; ++i) {
      
      
      if (!parsedProperty[i]) {
        cur[i] = AppendValueToList(aValues[i], cur[i], aInitialValues[i]);
      }
    }

    if (atEOP)
      break;
    
  }

  return eParseAnimationOrTransitionShorthand_Values;
}

bool
CSSParserImpl::ParseTransition()
{
  static const nsCSSProperty kTransitionProperties[] = {
    eCSSProperty_transition_duration,
    eCSSProperty_transition_timing_function,
    
    
    
    
    eCSSProperty_transition_delay,
    
    
    
    eCSSProperty_transition_property
  };
  static const uint32_t numProps = NS_ARRAY_LENGTH(kTransitionProperties);
  
  
  

  nsCSSValue initialValues[numProps];
  initialValues[0].SetFloatValue(0.0, eCSSUnit_Seconds);
  initialValues[1].SetIntValue(NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE,
                               eCSSUnit_Enumerated);
  initialValues[2].SetFloatValue(0.0, eCSSUnit_Seconds);
  initialValues[3].SetAllValue();

  nsCSSValue values[numProps];

  ParseAnimationOrTransitionShorthandResult spres =
    ParseAnimationOrTransitionShorthand(kTransitionProperties,
                                        initialValues, values, numProps);
  if (spres != eParseAnimationOrTransitionShorthand_Values) {
    return spres != eParseAnimationOrTransitionShorthand_Error;
  }

  
  
  
  
  
  
  {
    NS_ABORT_IF_FALSE(kTransitionProperties[3] ==
                        eCSSProperty_transition_property,
                      "array index mismatch");
    nsCSSValueList *l = values[3].GetListValue();
    bool multipleItems = !!l->mNext;
    do {
      const nsCSSValue& val = l->mValue;
      if (val.GetUnit() == eCSSUnit_None) {
        if (multipleItems) {
          
          return false;
        }

        
        values[3].SetNoneValue();
        break;
      }
      if (val.GetUnit() == eCSSUnit_Ident) {
        nsDependentString str(val.GetStringBufferValue());
        if (str.EqualsLiteral("inherit") || str.EqualsLiteral("initial")) {
          return false;
        }
      }
    } while ((l = l->mNext));
  }

  
  for (uint32_t i = 0; i < numProps; ++i) {
    AppendValue(kTransitionProperties[i], values[i]);
  }
  return true;
}

bool
CSSParserImpl::ParseAnimation()
{
  static const nsCSSProperty kAnimationProperties[] = {
    eCSSProperty_animation_duration,
    eCSSProperty_animation_timing_function,
    
    
    
    
    eCSSProperty_animation_delay,
    eCSSProperty_animation_direction,
    eCSSProperty_animation_fill_mode,
    eCSSProperty_animation_iteration_count,
    
    
    
    
    eCSSProperty_animation_name
  };
  static const uint32_t numProps = NS_ARRAY_LENGTH(kAnimationProperties);
  
  
  

  nsCSSValue initialValues[numProps];
  initialValues[0].SetFloatValue(0.0, eCSSUnit_Seconds);
  initialValues[1].SetIntValue(NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE,
                               eCSSUnit_Enumerated);
  initialValues[2].SetFloatValue(0.0, eCSSUnit_Seconds);
  initialValues[3].SetIntValue(NS_STYLE_ANIMATION_DIRECTION_NORMAL, eCSSUnit_Enumerated);
  initialValues[4].SetIntValue(NS_STYLE_ANIMATION_FILL_MODE_NONE, eCSSUnit_Enumerated);
  initialValues[5].SetFloatValue(1.0f, eCSSUnit_Number);
  initialValues[6].SetNoneValue();

  nsCSSValue values[numProps];

  ParseAnimationOrTransitionShorthandResult spres =
    ParseAnimationOrTransitionShorthand(kAnimationProperties,
                                        initialValues, values, numProps);
  if (spres != eParseAnimationOrTransitionShorthand_Values) {
    return spres != eParseAnimationOrTransitionShorthand_Error;
  }

  
  for (uint32_t i = 0; i < numProps; ++i) {
    AppendValue(kAnimationProperties[i], values[i]);
  }
  return true;
}

bool
CSSParserImpl::ParseShadowItem(nsCSSValue& aValue, bool aIsBoxShadow)
{
  
  enum {
    IndexX,
    IndexY,
    IndexRadius,
    IndexSpread,  
    IndexColor,
    IndexInset    
  };

  nsRefPtr<nsCSSValue::Array> val = nsCSSValue::Array::Create(6);

  if (aIsBoxShadow) {
    
    ParseVariant(val->Item(IndexInset), VARIANT_KEYWORD,
                 nsCSSProps::kBoxShadowTypeKTable);
  }

  nsCSSValue xOrColor;
  bool haveColor = false;
  if (!ParseVariant(xOrColor, VARIANT_COLOR | VARIANT_LENGTH | VARIANT_CALC,
                    nullptr)) {
    return false;
  }
  if (xOrColor.IsLengthUnit() || xOrColor.IsCalcUnit()) {
    val->Item(IndexX) = xOrColor;
  } else {
    
    NS_ASSERTION(xOrColor.GetUnit() == eCSSUnit_Ident ||
                 xOrColor.GetUnit() == eCSSUnit_Color ||
                 xOrColor.GetUnit() == eCSSUnit_EnumColor,
                 "Must be a color value");
    val->Item(IndexColor) = xOrColor;
    haveColor = true;

    
    if (!ParseVariant(val->Item(IndexX), VARIANT_LENGTH | VARIANT_CALC,
                      nullptr)) {
      return false;
    }
  }

  
  if (!ParseVariant(val->Item(IndexY), VARIANT_LENGTH | VARIANT_CALC,
                    nullptr)) {
    return false;
  }

  
  
  
  
  if (ParseVariant(val->Item(IndexRadius), VARIANT_LENGTH | VARIANT_CALC,
                   nullptr) &&
      val->Item(IndexRadius).IsLengthUnit() &&
      val->Item(IndexRadius).GetFloatValue() < 0) {
    return false;
  }

  if (aIsBoxShadow) {
    
    ParseVariant(val->Item(IndexSpread), VARIANT_LENGTH | VARIANT_CALC, nullptr);
  }

  if (!haveColor) {
    
    ParseVariant(val->Item(IndexColor), VARIANT_COLOR, nullptr);
  }

  if (aIsBoxShadow && val->Item(IndexInset).GetUnit() == eCSSUnit_Null) {
    
    ParseVariant(val->Item(IndexInset), VARIANT_KEYWORD,
                 nsCSSProps::kBoxShadowTypeKTable);
  }

  aValue.SetArrayValue(val, eCSSUnit_Array);
  return true;
}

bool
CSSParserImpl::ParseShadowList(nsCSSProperty aProperty)
{
  nsAutoParseCompoundProperty compound(this);
  bool isBoxShadow = aProperty == eCSSProperty_box_shadow;

  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValueList* cur = value.SetListValue();
    for (;;) {
      if (!ParseShadowItem(cur->mValue, isBoxShadow)) {
        return false;
      }
      if (CheckEndProperty()) {
        break;
      }
      if (!ExpectSymbol(',', true)) {
        return false;
      }
      cur->mNext = new nsCSSValueList;
      cur = cur->mNext;
    }
  }
  AppendValue(aProperty, value);
  return true;
}

int32_t
CSSParserImpl::GetNamespaceIdForPrefix(const nsString& aPrefix)
{
  NS_PRECONDITION(!aPrefix.IsEmpty(), "Must have a prefix here");

  int32_t nameSpaceID = kNameSpaceID_Unknown;
  if (mNameSpaceMap) {
    
    nsCOMPtr<nsIAtom> prefix = do_GetAtom(aPrefix);
    if (!prefix) {
      NS_RUNTIMEABORT("do_GetAtom failed - out of memory?");
    }
    nameSpaceID = mNameSpaceMap->FindNameSpaceID(prefix);
  }
  

  if (nameSpaceID == kNameSpaceID_Unknown) {   
    REPORT_UNEXPECTED_P(PEUnknownNamespacePrefix, aPrefix);
  }

  return nameSpaceID;
}

void
CSSParserImpl::SetDefaultNamespaceOnSelector(nsCSSSelector& aSelector)
{
  if (mNameSpaceMap) {
    aSelector.SetNameSpace(mNameSpaceMap->FindNameSpaceID(nullptr));
  } else {
    aSelector.SetNameSpace(kNameSpaceID_Unknown); 
  }
}

bool
CSSParserImpl::ParsePaint(nsCSSProperty aPropID)
{
  nsCSSValue x, y;
  if (!ParseVariant(x, VARIANT_HCK | VARIANT_NONE | VARIANT_URL,
                    nsCSSProps::kObjectPatternKTable)) {
    return false;
  }

  bool canHaveFallback = x.GetUnit() == eCSSUnit_URL ||
                         x.GetUnit() == eCSSUnit_Enumerated;
  if (canHaveFallback) {
    if (!ParseVariant(y, VARIANT_COLOR | VARIANT_NONE, nullptr))
      y.SetNoneValue();
  }
  if (!ExpectEndProperty())
    return false;

  if (!canHaveFallback) {
    AppendValue(aPropID, x);
  } else {
    nsCSSValue val;
    val.SetPairValue(x, y);
    AppendValue(aPropID, val);
  }
  return true;
}

bool
CSSParserImpl::ParseDasharray()
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_HK | VARIANT_NONE,
                   nsCSSProps::kStrokeObjectValueKTable)) {
    
    if (!ExpectEndProperty()) {
      return false;
    }
  } else {
    nsCSSValueList *cur = value.SetListValue();
    for (;;) {
      if (!ParseNonNegativeVariant(cur->mValue, VARIANT_LPN, nullptr)) {
        return false;
      }
      if (CheckEndProperty()) {
        break;
      }
      
      (void)ExpectSymbol(',', true);

      cur->mNext = new nsCSSValueList;
      cur = cur->mNext;
    }
  }
  AppendValue(eCSSProperty_stroke_dasharray, value);
  return true;
}

bool
CSSParserImpl::ParseMarker()
{
  nsCSSValue marker;
  if (ParseSingleValueProperty(marker, eCSSProperty_marker_end)) {
    if (ExpectEndProperty()) {
      AppendValue(eCSSProperty_marker_end, marker);
      AppendValue(eCSSProperty_marker_mid, marker);
      AppendValue(eCSSProperty_marker_start, marker);
      return true;
    }
  }
  return false;
}

bool
CSSParserImpl::ParsePaintOrder()
{
  MOZ_STATIC_ASSERT
    ((1 << NS_STYLE_PAINT_ORDER_BITWIDTH) > NS_STYLE_PAINT_ORDER_LAST_VALUE,
     "bitfield width insufficient for paint-order constants");

  static const int32_t kPaintOrderKTable[] = {
    eCSSKeyword_normal,  NS_STYLE_PAINT_ORDER_NORMAL,
    eCSSKeyword_fill,    NS_STYLE_PAINT_ORDER_FILL,
    eCSSKeyword_stroke,  NS_STYLE_PAINT_ORDER_STROKE,
    eCSSKeyword_markers, NS_STYLE_PAINT_ORDER_MARKERS,
    eCSSKeyword_UNKNOWN,-1
  };

  MOZ_STATIC_ASSERT(NS_ARRAY_LENGTH(kPaintOrderKTable) ==
                      2 * (NS_STYLE_PAINT_ORDER_LAST_VALUE + 2),
                    "missing paint-order values in kPaintOrderKTable");

  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_HK, kPaintOrderKTable)) {
    return false;
  }

  uint32_t seen = 0;
  uint32_t order = 0;
  uint32_t position = 0;

  
  
  MOZ_STATIC_ASSERT
    (NS_STYLE_PAINT_ORDER_BITWIDTH * NS_STYLE_PAINT_ORDER_LAST_VALUE < 32,
     "seen and order not big enough");

  if (value.GetUnit() == eCSSUnit_Enumerated) {
    uint32_t component = static_cast<uint32_t>(value.GetIntValue());
    if (component != NS_STYLE_PAINT_ORDER_NORMAL) {
      bool parsedOK = true;
      for (;;) {
        if (seen & (1 << component)) {
          
          UngetToken();
          parsedOK = false;
          break;
        }
        seen |= (1 << component);
        order |= (component << position);
        position += NS_STYLE_PAINT_ORDER_BITWIDTH;
        if (!ParseEnum(value, kPaintOrderKTable)) {
          break;
        }
        component = value.GetIntValue();
        if (component == NS_STYLE_PAINT_ORDER_NORMAL) {
          
          UngetToken();
          parsedOK = false;
          break;
        }
      }

      
      
      if (parsedOK) {
        for (component = 1;
             component <= NS_STYLE_PAINT_ORDER_LAST_VALUE;
             component++) {
          if (!(seen & (1 << component))) {
            order |= (component << position);
            position += NS_STYLE_PAINT_ORDER_BITWIDTH;
          }
        }
      }
    }

    MOZ_STATIC_ASSERT(NS_STYLE_PAINT_ORDER_NORMAL == 0,
                      "unexpected value for NS_STYLE_PAINT_ORDER_NORMAL");
    value.SetIntValue(static_cast<int32_t>(order), eCSSUnit_Enumerated);
  }

  if (!ExpectEndProperty()) {
    return false;
  }

  AppendValue(eCSSProperty_paint_order, value);
  return true;
}

} 



static CSSParserImpl* gFreeList = nullptr;

nsCSSParser::nsCSSParser(mozilla::css::Loader* aLoader,
                         nsCSSStyleSheet* aSheet)
{
  CSSParserImpl *impl = gFreeList;
  if (impl) {
    gFreeList = impl->mNextFree;
    impl->mNextFree = nullptr;
  } else {
    impl = new CSSParserImpl();
  }

  if (aLoader) {
    impl->SetChildLoader(aLoader);
    impl->SetQuirkMode(aLoader->GetCompatibilityMode() ==
                       eCompatibility_NavQuirks);
  }
  if (aSheet) {
    impl->SetStyleSheet(aSheet);
  }

  mImpl = static_cast<void*>(impl);
}

nsCSSParser::~nsCSSParser()
{
  CSSParserImpl *impl = static_cast<CSSParserImpl*>(mImpl);
  impl->Reset();
  impl->mNextFree = gFreeList;
  gFreeList = impl;
}

 void
nsCSSParser::Shutdown()
{
  CSSParserImpl *tofree = gFreeList;
  CSSParserImpl *next;
  while (tofree)
    {
      next = tofree->mNextFree;
      delete tofree;
      tofree = next;
    }
}



nsresult
nsCSSParser::SetStyleSheet(nsCSSStyleSheet* aSheet)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    SetStyleSheet(aSheet);
}

nsresult
nsCSSParser::SetQuirkMode(bool aQuirkMode)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    SetQuirkMode(aQuirkMode);
}

nsresult
nsCSSParser::SetChildLoader(mozilla::css::Loader* aChildLoader)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    SetChildLoader(aChildLoader);
}

nsresult
nsCSSParser::ParseSheet(const nsAString& aInput,
                        nsIURI*          aSheetURI,
                        nsIURI*          aBaseURI,
                        nsIPrincipal*    aSheetPrincipal,
                        uint32_t         aLineNumber,
                        bool             aAllowUnsafeRules)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseSheet(aInput, aSheetURI, aBaseURI, aSheetPrincipal, aLineNumber,
               aAllowUnsafeRules);
}

nsresult
nsCSSParser::ParseStyleAttribute(const nsAString&  aAttributeValue,
                                 nsIURI*           aDocURI,
                                 nsIURI*           aBaseURI,
                                 nsIPrincipal*     aNodePrincipal,
                                 css::StyleRule**  aResult)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseStyleAttribute(aAttributeValue, aDocURI, aBaseURI,
                        aNodePrincipal, aResult);
}

nsresult
nsCSSParser::ParseDeclarations(const nsAString&  aBuffer,
                               nsIURI*           aSheetURI,
                               nsIURI*           aBaseURI,
                               nsIPrincipal*     aSheetPrincipal,
                               css::Declaration* aDeclaration,
                               bool*           aChanged)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseDeclarations(aBuffer, aSheetURI, aBaseURI, aSheetPrincipal,
                      aDeclaration, aChanged);
}

nsresult
nsCSSParser::ParseRule(const nsAString&        aRule,
                       nsIURI*                 aSheetURI,
                       nsIURI*                 aBaseURI,
                       nsIPrincipal*           aSheetPrincipal,
                       css::Rule**             aResult)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseRule(aRule, aSheetURI, aBaseURI, aSheetPrincipal, aResult);
}

nsresult
nsCSSParser::ParseProperty(const nsCSSProperty aPropID,
                           const nsAString&    aPropValue,
                           nsIURI*             aSheetURI,
                           nsIURI*             aBaseURI,
                           nsIPrincipal*       aSheetPrincipal,
                           css::Declaration*   aDeclaration,
                           bool*               aChanged,
                           bool                aIsImportant,
                           bool                aIsSVGMode)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseProperty(aPropID, aPropValue, aSheetURI, aBaseURI,
                  aSheetPrincipal, aDeclaration, aChanged,
                  aIsImportant, aIsSVGMode);
}

nsresult
nsCSSParser::ParseMediaList(const nsSubstring& aBuffer,
                            nsIURI*            aURI,
                            uint32_t           aLineNumber,
                            nsMediaList*       aMediaList,
                            bool               aHTMLMode)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseMediaList(aBuffer, aURI, aLineNumber, aMediaList, aHTMLMode);
}

bool
nsCSSParser::ParseColorString(const nsSubstring& aBuffer,
                              nsIURI*            aURI,
                              uint32_t           aLineNumber,
                              nsCSSValue&        aValue)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseColorString(aBuffer, aURI, aLineNumber, aValue);
}

nsresult
nsCSSParser::ParseSelectorString(const nsSubstring&  aSelectorString,
                                 nsIURI*             aURI,
                                 uint32_t            aLineNumber,
                                 nsCSSSelectorList** aSelectorList)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseSelectorString(aSelectorString, aURI, aLineNumber, aSelectorList);
}

already_AddRefed<nsCSSKeyframeRule>
nsCSSParser::ParseKeyframeRule(const nsSubstring& aBuffer,
                               nsIURI*            aURI,
                               uint32_t           aLineNumber)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseKeyframeRule(aBuffer, aURI, aLineNumber);
}

bool
nsCSSParser::ParseKeyframeSelectorString(const nsSubstring& aSelectorString,
                                         nsIURI*            aURI,
                                         uint32_t           aLineNumber,
                                         InfallibleTArray<float>& aSelectorList)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseKeyframeSelectorString(aSelectorString, aURI, aLineNumber,
                                aSelectorList);
}

bool
nsCSSParser::EvaluateSupportsDeclaration(const nsAString& aProperty,
                                         const nsAString& aValue,
                                         nsIURI* aDocURL,
                                         nsIURI* aBaseURL,
                                         nsIPrincipal* aDocPrincipal)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    EvaluateSupportsDeclaration(aProperty, aValue, aDocURL, aBaseURL,
                                aDocPrincipal);
}

bool
nsCSSParser::EvaluateSupportsCondition(const nsAString& aCondition,
                                       nsIURI* aDocURL,
                                       nsIURI* aBaseURL,
                                       nsIPrincipal* aDocPrincipal)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    EvaluateSupportsCondition(aCondition, aDocURL, aBaseURL, aDocPrincipal);
}
