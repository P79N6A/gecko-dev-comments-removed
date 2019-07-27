







#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Move.h"
#include "mozilla/MathAlgorithms.h"

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
#include "mozilla/CSSStyleSheet.h"
#include "mozilla/css/Declaration.h"
#include "nsStyleConsts.h"
#include "nsNetUtil.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsColor.h"
#include "nsCSSPseudoClasses.h"
#include "nsCSSPseudoElements.h"
#include "nsNameSpaceManager.h"
#include "nsXMLNameSpaceMap.h"
#include "nsError.h"
#include "nsIMediaList.h"
#include "nsStyleUtil.h"
#include "nsIPrincipal.h"
#include "nsICSSUnprefixingService.h"
#include "prprf.h"
#include "nsContentUtils.h"
#include "nsAutoPtr.h"
#include "CSSCalc.h"
#include "nsMediaFeatures.h"
#include "nsLayoutUtils.h"
#include "mozilla/Preferences.h"
#include "nsRuleData.h"
#include "mozilla/CSSVariableValues.h"
#include "mozilla/dom/URL.h"
#include "gfxFontFamilyList.h"

using namespace mozilla;

typedef nsCSSProps::KTableValue KTableValue;


static bool sOpentypeSVGEnabled;
static bool sUnprefixingServiceEnabled;

const uint32_t
nsCSSProps::kParserVariantTable[eCSSProperty_COUNT_no_shorthands] = {
#define CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, \
                 stylestruct_, stylestructoffset_, animtype_)                 \
  parsevariant_,
#define CSS_PROP_LIST_INCLUDE_LOGICAL
#include "nsCSSPropList.h"
#undef CSS_PROP_LIST_INCLUDE_LOGICAL
#undef CSS_PROP
};





#define GRID_TEMPLATE_MAX_REPETITIONS 10000


#define MASK_END_VALUE  (-1)

enum class CSSParseResult : int32_t {
  
  Ok,
  
  NotFound,
  
  Error
};

namespace {


typedef void (* RuleAppendFunc) (css::Rule* aRule, void* aData);
static void AssignRuleToPointer(css::Rule* aRule, void* aPointer);
static void AppendRuleToSheet(css::Rule* aRule, void* aParser);

struct CSSParserInputState {
  nsCSSScannerPosition mPosition;
  nsCSSToken mToken;
  bool mHavePushBack;
};




class CSSParserImpl {
public:
  CSSParserImpl();
  ~CSSParserImpl();

  nsresult SetStyleSheet(CSSStyleSheet* aSheet);

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

  void ParseProperty(const nsCSSProperty aPropID,
                     const nsAString& aPropValue,
                     nsIURI* aSheetURL,
                     nsIURI* aBaseURL,
                     nsIPrincipal* aSheetPrincipal,
                     css::Declaration* aDeclaration,
                     bool* aChanged,
                     bool aIsImportant,
                     bool aIsSVGMode);

  void ParseMediaList(const nsSubstring& aBuffer,
                      nsIURI* aURL, 
                      uint32_t aLineNumber, 
                      nsMediaList* aMediaList,
                      bool aHTMLMode);

  bool ParseSourceSizeList(const nsAString& aBuffer,
                           nsIURI* aURI, 
                           uint32_t aLineNumber, 
                           InfallibleTArray< nsAutoPtr<nsMediaQuery> >& aQueries,
                           InfallibleTArray<nsCSSValue>& aValues,
                           bool aHTMLMode);

  void ParseVariable(const nsAString& aVariableName,
                     const nsAString& aPropValue,
                     nsIURI* aSheetURL,
                     nsIURI* aBaseURL,
                     nsIPrincipal* aSheetPrincipal,
                     css::Declaration* aDeclaration,
                     bool* aChanged,
                     bool aIsImportant);

  bool ParseFontFamilyListString(const nsSubstring& aBuffer,
                                 nsIURI* aURL, 
                                 uint32_t aLineNumber, 
                                 nsCSSValue& aValue);

  bool ParseColorString(const nsSubstring& aBuffer,
                        nsIURI* aURL, 
                        uint32_t aLineNumber, 
                        nsCSSValue& aValue,
                        bool aSuppressErrors );

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

  bool ParseCounterStyleName(const nsAString& aBuffer,
                             nsIURI* aURL,
                             nsAString& aName);

  bool ParseCounterDescriptor(nsCSSCounterDesc aDescID,
                              const nsAString& aBuffer,
                              nsIURI* aSheetURL,
                              nsIURI* aBaseURL,
                              nsIPrincipal* aSheetPrincipal,
                              nsCSSValue& aValue);

  bool ParseFontFaceDescriptor(nsCSSFontDesc aDescID,
                               const nsAString& aBuffer,
                               nsIURI* aSheetURL,
                               nsIURI* aBaseURL,
                               nsIPrincipal* aSheetPrincipal,
                               nsCSSValue& aValue);

  bool IsValueValidForProperty(const nsCSSProperty aPropID,
                               const nsAString& aPropValue);

  typedef nsCSSParser::VariableEnumFunc VariableEnumFunc;

  










  bool EnumerateVariableReferences(const nsAString& aPropertyValue,
                                   VariableEnumFunc aFunc,
                                   void* aData);

  














  bool ResolveVariableValue(const nsAString& aPropertyValue,
                            const CSSVariableValues* aVariables,
                            nsString& aResult,
                            nsCSSTokenSerializationType& aFirstToken,
                            nsCSSTokenSerializationType& aLastToken);

  






















  void ParsePropertyWithVariableReferences(nsCSSProperty aPropertyID,
                                           nsCSSProperty aShorthandPropertyID,
                                           const nsAString& aValue,
                                           const CSSVariableValues* aVariables,
                                           nsRuleData* aRuleData,
                                           nsIURI* aDocURL,
                                           nsIURI* aBaseURL,
                                           nsIPrincipal* aDocPrincipal,
                                           CSSStyleSheet* aSheet,
                                           uint32_t aLineNumber,
                                           uint32_t aLineOffset);

  nsCSSProps::EnabledState PropertyEnabledState() const {
    static_assert(nsCSSProps::eEnabledForAllContent == 0,
                  "nsCSSProps::eEnabledForAllContent should be zero for "
                  "this bitfield to work");
    nsCSSProps::EnabledState enabledState = nsCSSProps::eEnabledForAllContent;
    if (mUnsafeRulesEnabled) {
      enabledState |= nsCSSProps::eEnabledInUASheets;
    }
    if (mIsChromeOrCertifiedApp) {
      enabledState |= nsCSSProps::eEnabledInChromeOrCertifiedApp;
    }
    return enabledState;
  }

  nsCSSProperty LookupEnabledProperty(const nsAString& aProperty) {
    return nsCSSProps::LookupProperty(aProperty, PropertyEnabledState());
  }

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
      explicit nsAutoParseCompoundProperty(CSSParserImpl* aParser) : mParser(aParser)
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
      explicit nsAutoSuppressErrors(CSSParserImpl* aParser,
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

  





  class MOZ_STACK_CLASS nsAutoCSSParserInputStateRestorer {
    public:
      explicit nsAutoCSSParserInputStateRestorer(CSSParserImpl* aParser
                                                 MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : mParser(aParser),
          mShouldRestore(true)
      {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        mParser->SaveInputState(mSavedState);
      }

      void DoNotRestore()
      {
        mShouldRestore = false;
      }

      ~nsAutoCSSParserInputStateRestorer()
      {
        if (mShouldRestore) {
          mParser->RestoreSavedInputState(mSavedState);
        }
      }

    private:
      MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
      CSSParserImpl* mParser;
      CSSParserInputState mSavedState;
      bool mShouldRestore;
  };

  

















  class MOZ_STACK_CLASS nsAutoScannerChanger {
    public:
      nsAutoScannerChanger(CSSParserImpl* aParser,
                           const nsAString& aStringToScan
                           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : mParser(aParser),
          mOriginalScanner(aParser->mScanner),
          mStringScanner(aStringToScan, 0),
          mParserStateRestorer(aParser),
          mErrorSuppresser(aParser)
      {
        MOZ_ASSERT(mOriginalScanner,
                   "Shouldn't use nsAutoScannerChanger unless we already "
                   "have a scanner");
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;

        
        mParser->mScanner = &mStringScanner;
        mStringScanner.SetErrorReporter(mParser->mReporter);

        
        
        
        mParser->mHavePushBack = false;
      }

      ~nsAutoScannerChanger()
      {
        
        mParser->mScanner = mOriginalScanner;
      }

    private:
      MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
      CSSParserImpl* mParser;
      nsCSSScanner *mOriginalScanner;
      nsCSSScanner mStringScanner;
      nsAutoCSSParserInputStateRestorer mParserStateRestorer;
      nsAutoSuppressErrors mErrorSuppresser;
  };


  bool IsSVGMode() const {
    return mScanner->IsSVGMode();
  }

  



  void SaveInputState(CSSParserInputState& aState);

  



  void RestoreSavedInputState(const CSSParserInputState& aState);

  bool GetToken(bool aSkipWS);
  void UngetToken();
  bool GetNextTokenLocation(bool aSkipWS, uint32_t *linenum, uint32_t *colnum);

  bool ExpectSymbol(char16_t aSymbol, bool aSkipWS);
  bool ExpectEndProperty();
  bool CheckEndProperty();
  nsSubstring* NextIdent();

  
  bool SkipUntil(char16_t aStopSymbol);
  void SkipUntilOneOf(const char16_t* aStopSymbolChars);
  
  
  typedef nsAutoTArray<char16_t, 16> StopSymbolCharStack;
  void SkipUntilAllOf(const StopSymbolCharStack& aStopSymbolChars);
  
  
  
  bool SkipBalancedContentUntil(char16_t aStopSymbol);

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
  bool GatherMedia(nsMediaList* aMedia, bool aInAtRule);

  enum eMediaQueryType { eMediaQueryNormal,
                         
                         eMediaQueryAtRule,
                         
                         
                         
                         
                         eMediaQuerySingleCondition };
  bool ParseMediaQuery(eMediaQueryType aMode, nsMediaQuery **aQuery,
                       bool *aHitStop);
  bool ParseMediaQueryExpression(nsMediaQuery* aQuery);
  void ProcessImport(const nsString& aURLSpec,
                     nsMediaList* aMedia,
                     RuleAppendFunc aAppendFunc,
                     void* aProcessData,
                     uint32_t aLineNumber,
                     uint32_t aColumnNumber);
  bool ParseGroupRule(css::GroupRule* aRule, RuleAppendFunc aAppendFunc,
                      void* aProcessData);
  bool ParseMediaRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseMozDocumentRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseNameSpaceRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  void ProcessNameSpace(const nsString& aPrefix,
                        const nsString& aURLSpec, RuleAppendFunc aAppendFunc,
                        void* aProcessData,
                        uint32_t aLineNumber, uint32_t aColumnNumber);

  bool ParseFontFaceRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseFontFeatureValuesRule(RuleAppendFunc aAppendFunc,
                                  void* aProcessData);
  bool ParseFontFeatureValueSet(nsCSSFontFeatureValuesRule *aRule);
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

  bool ParseCounterStyleRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  bool ParseCounterStyleName(nsAString& aName, bool aForDefinition);
  bool ParseCounterStyleNameValue(nsCSSValue& aValue);
  bool ParseCounterDescriptor(nsCSSCounterStyleRule *aRule);
  bool ParseCounterDescriptorValue(nsCSSCounterDesc aDescID,
                                   nsCSSValue& aValue);
  bool ParseCounterRange(nsCSSValuePair& aPair);

  










  bool ResolveValueWithVariableReferences(
                              const CSSVariableValues* aVariables,
                              nsString& aResult,
                              nsCSSTokenSerializationType& aResultFirstToken,
                              nsCSSTokenSerializationType& aResultLastToken);
  
  bool ResolveValueWithVariableReferencesRec(
                             nsString& aResult,
                             nsCSSTokenSerializationType& aResultFirstToken,
                             nsCSSTokenSerializationType& aResultLastToken,
                             const CSSVariableValues* aVariables);

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
                           char16_t aStopChar);
  bool ParseSelectorGroup(nsCSSSelectorList*& aListHead);
  bool ParseSelector(nsCSSSelectorList* aList, char16_t aPrevCombinator);

  enum {
    eParseDeclaration_InBraces           = 1 << 0,
    eParseDeclaration_AllowImportant     = 1 << 1,
    
    eParseDeclaration_FromUnprefixingSvc = 1 << 2
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

  
  
  
  
  nsCSSKeyword LookupKeywordPrefixAware(nsAString& aKeywordStr,
                                        const KTableValue aKeywordTable[]);

  bool ShouldUseUnprefixingService();
  bool ParsePropertyWithUnprefixingService(const nsAString& aPropertyName,
                                           css::Declaration* aDeclaration,
                                           uint32_t aFlags,
                                           bool aMustCallValueAppended,
                                           bool* aChanged,
                                           nsCSSContextType aContext);

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

  
  
  bool ParsePositionValue(nsCSSValue& aOut);

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

  
  bool ParseFlex();
  
  bool ParseFlexFlow();

  
  bool ParseGridAutoFlow();

  
  
  
  
  
  
  
  
  
  
  CSSParseResult ParseGridLineNames(nsCSSValue& aValue);
  bool ParseGridLineNameListRepeat(nsCSSValueList** aTailPtr);
  bool ParseOptionalLineNameListAfterSubgrid(nsCSSValue& aValue);
  bool ParseGridTrackBreadth(nsCSSValue& aValue);
  CSSParseResult ParseGridTrackSize(nsCSSValue& aValue);
  bool ParseGridAutoColumnsRows(nsCSSProperty aPropID);
  bool ParseGridTrackListRepeat(nsCSSValueList** aTailPtr);

  
  
  
  
  
  
  
  
  
  
  
  bool ParseGridTrackListWithFirstLineNames(nsCSSValue& aValue,
                                            const nsCSSValue& aFirstLineNames);
  bool ParseGridTemplateColumnsRows(nsCSSProperty aPropID);

  
  
  bool ParseGridTemplateAreasLine(const nsAutoString& aInput,
                                  css::GridTemplateAreasValue* aResult,
                                  nsDataHashtable<nsStringHashKey, uint32_t>& aAreaIndices);
  bool ParseGridTemplateAreas();
  bool ParseGridTemplate();
  bool ParseGridTemplateAfterSlash(bool aColumnsIsTrackList);
  bool ParseGridTemplateAfterString(const nsCSSValue& aFirstLineNames);
  bool ParseGrid();
  bool ParseGridShorthandAutoProps();
  bool ParseGridLine(nsCSSValue& aValue);
  bool ParseGridColumnRowStartEnd(nsCSSProperty aPropID);
  bool ParseGridColumnRow(nsCSSProperty aStartPropID,
                          nsCSSProperty aEndPropID);
  bool ParseGridArea();

  
  bool ParseRect(nsCSSProperty aPropID);
  bool ParseColumns();
  bool ParseContent();
  bool ParseCounterData(nsCSSProperty aPropID);
  bool ParseCursor();
  bool ParseFont();
  bool ParseFontSynthesis(nsCSSValue& aValue);
  bool ParseSingleAlternate(int32_t& aWhichFeature, nsCSSValue& aValue);
  bool ParseFontVariantAlternates(nsCSSValue& aValue);
  bool MergeBitmaskValue(int32_t aNewValue, const int32_t aMasks[],
                         int32_t& aMergedValue);
  bool ParseBitmaskValues(nsCSSValue& aValue,
                          const KTableValue aKeywordTable[],
                          const int32_t aMasks[]);
  bool ParseFontVariantEastAsian(nsCSSValue& aValue);
  bool ParseFontVariantLigatures(nsCSSValue& aValue);
  bool ParseFontVariantNumeric(nsCSSValue& aValue);
  bool ParseFontVariant();
  bool ParseFontWeight(nsCSSValue& aValue);
  bool ParseOneFamily(nsAString& aFamily, bool& aOneKeyword, bool& aQuoted);
  bool ParseFamily(nsCSSValue& aValue);
  bool ParseFontFeatureSettings(nsCSSValue& aValue);
  bool ParseFontSrc(nsCSSValue& aValue);
  bool ParseFontSrcFormat(InfallibleTArray<nsCSSValue>& values);
  bool ParseFontRanges(nsCSSValue& aValue);
  bool ParseListStyle();
  bool ParseListStyleType(nsCSSValue& aValue);
  bool ParseMargin();
  bool ParseMarks(nsCSSValue& aValue);
  bool ParseClipPath();
  bool ParseTransform(bool aIsPrefixed);
  bool ParseObjectPosition();
  bool ParseOutline();
  bool ParseOverflow();
  bool ParsePadding();
  bool ParseQuotes();
  bool ParseSize();
  bool ParseTextAlign(nsCSSValue& aValue,
                      const KTableValue aTable[]);
  bool ParseTextAlign(nsCSSValue& aValue);
  bool ParseTextAlignLast(nsCSSValue& aValue);
  bool ParseTextDecoration();
  bool ParseTextDecorationLine(nsCSSValue& aValue);
  bool ParseTextCombineUpright(nsCSSValue& aValue);
  bool ParseTextOverflow(nsCSSValue& aValue);
  bool ParseTouchAction(nsCSSValue& aValue);

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
  bool ParseWillChange();

  bool ParsePaint(nsCSSProperty aPropID);
  bool ParseDasharray();
  bool ParseMarker();
  bool ParsePaintOrder();
  bool ParseAll();
  bool ParseScrollSnapType();
  bool ParseScrollSnapPoints(nsCSSValue& aValue, nsCSSProperty aPropID);
  bool ParseScrollSnapDestination(nsCSSValue& aValue);
  bool ParseScrollSnapCoordinate(nsCSSValue& aValue);

  









  bool ParseVariableDeclaration(CSSVariableDeclarations::Type* aType,
                                nsString& aValue);

  

















  bool ParseValueWithVariables(CSSVariableDeclarations::Type* aType,
                               bool* aDropBackslash,
                               nsString& aImpliedCharacters,
                               void (*aFunc)(const nsAString&, void*),
                               void* aData);

  


  bool BackslashDropped();

  


  void AppendImpliedEOFCharacters(nsAString& aResult);

  
  void AppendValue(nsCSSProperty aPropID, const nsCSSValue& aValue);
  bool ParseBoxProperties(const nsCSSProperty aPropIDs[]);
  bool ParseGroupedBoxProperty(int32_t aVariantMask,
                               nsCSSValue& aValue);
  bool ParseBoxCornerRadius(const nsCSSProperty aPropID);
  bool ParseBoxCornerRadiiInternals(nsCSSValue array[]);
  bool ParseBoxCornerRadii(const nsCSSProperty aPropIDs[]);

  int32_t ParseChoice(nsCSSValue aValues[],
                      const nsCSSProperty aPropIDs[], int32_t aNumIDs);
  bool ParseColor(nsCSSValue& aValue);
  bool ParseNumberColorComponent(uint8_t& aComponent, char aStop);
  bool ParsePercentageColorComponent(float& aComponent, char aStop);
  
  
  bool ParseHSLColor(float& aHue, float& aSaturation, float& aLightness,
                     char aStop);
  
  
  bool ParseColorOpacity(uint8_t& aOpacity);
  bool ParseColorOpacity(float& aOpacity);
  bool ParseEnum(nsCSSValue& aValue,
                 const KTableValue aKeywordTable[]);
  bool ParseVariant(nsCSSValue& aValue,
                    int32_t aVariantMask,
                    const KTableValue aKeywordTable[]);
  bool ParseNonNegativeVariant(nsCSSValue& aValue,
                               int32_t aVariantMask,
                               const KTableValue aKeywordTable[]);
  bool ParseOneOrLargerVariant(nsCSSValue& aValue,
                               int32_t aVariantMask,
                               const KTableValue aKeywordTable[]);
  bool ParseNonNegativeInteger(nsCSSValue& aValue)
  {
    return ParseNonNegativeVariant(aValue, VARIANT_INTEGER, nullptr);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool ParseCustomIdent(nsCSSValue& aValue,
                        const nsAutoString& aIdentValue,
                        const nsCSSKeyword aExcludedKeywords[] = nullptr,
                        const nsCSSProps::KTableValue aPropertyKTable[] = nullptr);
  bool ParseCounter(nsCSSValue& aValue);
  bool ParseAttr(nsCSSValue& aValue);
  bool ParseSymbols(nsCSSValue& aValue);
  bool SetValueToURL(nsCSSValue& aValue, const nsString& aURL);
  bool TranslateDimension(nsCSSValue& aValue, int32_t aVariantMask,
                            float aNumber, const nsString& aUnit);
  bool ParseImageOrientation(nsCSSValue& aAngle);
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

  
  bool ParseBasicShape(nsCSSValue& aValue, bool* aConsumedTokens);
  bool ParsePolygonFunction(nsCSSValue& aValue);
  bool ParseCircleOrEllipseFunction(nsCSSKeyword, nsCSSValue& aValue);
  bool ParseInsetFunction(nsCSSValue& aValue);

  
  bool ParseSingleTransform(bool aIsPrefixed, nsCSSValue& aValue);
  bool ParseFunction(nsCSSKeyword aFunction, const int32_t aAllowedTypes[],
                     int32_t aVariantMaskAll, uint16_t aMinElems,
                     uint16_t aMaxElems, nsCSSValue &aValue);
  bool ParseFunctionInternals(const int32_t aVariantMask[],
                              int32_t aVariantMaskAll,
                              uint16_t aMinElems,
                              uint16_t aMaxElems,
                              InfallibleTArray<nsCSSValue>& aOutput);

  
  bool ParseTransformOrigin(bool aPerspective);

  
  bool ParseFilter();
  bool ParseSingleFilter(nsCSSValue* aValue);
  bool ParseDropShadow(nsCSSValue* aValue);

  


  int32_t GetNamespaceIdForPrefix(const nsString& aPrefix);

  
  void SetDefaultNamespaceOnSelector(nsCSSSelector& aSelector);

  
  
  nsCSSToken mToken;

  
  nsCSSScanner* mScanner;

  
  css::ErrorReporter* mReporter;

  
  nsCOMPtr<nsIURI> mBaseURI;

  
  nsCOMPtr<nsIURI> mSheetURI;

  
  nsCOMPtr<nsIPrincipal> mSheetPrincipal;

  
  nsRefPtr<CSSStyleSheet> mSheet;

  
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

  
  
  
  
  bool mIsChromeOrCertifiedApp : 1;

  
  bool mViewportUnitsEnabled : 1;

  
  
  bool mHTMLMediaMode : 1;

  
  
  bool          mParsingCompoundProperty : 1;

  
  
  
  bool mInSupportsCondition : 1;

  
  
  bool mInFailingSupportsRule : 1;

  
  
  
  bool mSuppressErrors : 1;

  
  
  
  
  bool mDidUnprefixWebkitBoxInEarlierDecl; 

  
  
  
  
  
  
  
  bool mSheetPrincipalRequired;

  
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

#define OUTPUT_ERROR_WITH_POSITION(linenum_, lineoff_) \
  mReporter->OutputError(linenum_, lineoff_)

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
    mIsChromeOrCertifiedApp(false),
    mViewportUnitsEnabled(true),
    mHTMLMediaMode(false),
    mParsingCompoundProperty(false),
    mInSupportsCondition(false),
    mInFailingSupportsRule(false),
    mSuppressErrors(false),
    mDidUnprefixWebkitBoxInEarlierDecl(false),
    mSheetPrincipalRequired(true),
    mNextFree(nullptr)
{
}

CSSParserImpl::~CSSParserImpl()
{
  mData.AssertInitialState();
  mTempData.AssertInitialState();
}

nsresult
CSSParserImpl::SetStyleSheet(CSSStyleSheet* aSheet)
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
    const css::Rule* lastRule = mSheet->GetStyleRuleAt(ruleCount - 1);
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
    }
  }
  else {
    mSection = eCSSSection_Charset; 
  }

  mUnsafeRulesEnabled = aAllowUnsafeRules;
  mIsChromeOrCertifiedApp =
    dom::IsChromeURI(aSheetURI) ||
    aSheetPrincipal->GetAppStatus() == nsIPrincipal::APP_STATUS_CERTIFIED;

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
  mIsChromeOrCertifiedApp = false;

  
  return NS_OK;
}





static bool
NonMozillaVendorIdentifier(const nsAString& ident)
{
  return (ident.First() == char16_t('-') &&
          !StringBeginsWith(ident, NS_LITERAL_STRING("-moz-"))) ||
         ident.First() == char16_t('_');

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

  uint32_t parseFlags = eParseDeclaration_AllowImportant;

  css::Declaration* declaration = ParseDeclarationBlock(parseFlags);
  if (declaration) {
    
    NS_ADDREF(*aResult = new css::StyleRule(nullptr, declaration, 0, 0));
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

  MOZ_ASSERT(!mDidUnprefixWebkitBoxInEarlierDecl,
             "Someone forgot to clear the 'did unprefix webkit-box' flag");
  AutoRestore<bool> autoRestore(mDidUnprefixWebkitBoxInEarlierDecl);

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

void
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
  MOZ_ASSERT(aPropID != eCSSPropertyExtra_variable);

  mData.AssertInitialState();
  mTempData.AssertInitialState();
  aDeclaration->AssertMutable();

  nsCSSScanner scanner(aPropValue, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aSheetURI);
  InitScanner(scanner, reporter, aSheetURI, aBaseURI, aSheetPrincipal);
  mSection = eCSSSection_General;
  scanner.SetSVGMode(aIsSVGMode);

  *aChanged = false;

  
  if (eCSSProperty_UNKNOWN == aPropID ||
      !(nsCSSProps::IsEnabled(aPropID) ||
        (mUnsafeRulesEnabled &&
         nsCSSProps::PropHasFlags(aPropID,
                                  CSS_PROPERTY_ALWAYS_ENABLED_IN_UA_SHEETS)))) {
    NS_ConvertASCIItoUTF16 propName(nsCSSProps::GetStringValue(aPropID));
    REPORT_UNEXPECTED_P(PEUnknownProperty, propName);
    REPORT_UNEXPECTED(PEDeclDropped);
    OUTPUT_ERROR();
    ReleaseScanner();
    return;
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
      *aChanged = mData.TransferFromBlock(mTempData, aPropID,
                                          PropertyEnabledState(), aIsImportant,
                                          true, false, aDeclaration);
      aDeclaration->CompressFrom(&mData);
    }
    CLEAR_ERROR();
  }

  mTempData.AssertInitialState();

  ReleaseScanner();
}

void
CSSParserImpl::ParseVariable(const nsAString& aVariableName,
                             const nsAString& aPropValue,
                             nsIURI* aSheetURI,
                             nsIURI* aBaseURI,
                             nsIPrincipal* aSheetPrincipal,
                             css::Declaration* aDeclaration,
                             bool* aChanged,
                             bool aIsImportant)
{
  NS_PRECONDITION(aSheetPrincipal, "Must have principal here!");
  NS_PRECONDITION(aBaseURI, "need base URI");
  NS_PRECONDITION(aDeclaration, "Need declaration to parse into!");
  NS_PRECONDITION(nsLayoutUtils::CSSVariablesEnabled(),
                  "expected Variables to be enabled");

  mData.AssertInitialState();
  mTempData.AssertInitialState();
  aDeclaration->AssertMutable();

  nsCSSScanner scanner(aPropValue, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aSheetURI);
  InitScanner(scanner, reporter, aSheetURI, aBaseURI, aSheetPrincipal);
  mSection = eCSSSection_General;

  *aChanged = false;

  CSSVariableDeclarations::Type variableType;
  nsString variableValue;

  bool parsedOK = ParseVariableDeclaration(&variableType, variableValue);

  
  if (parsedOK && GetToken(true)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectEndValue);
    parsedOK = false;
  }

  if (!parsedOK) {
    REPORT_UNEXPECTED_P(PEValueParsingError, NS_LITERAL_STRING("--") +
                                             aVariableName);
    REPORT_UNEXPECTED(PEDeclDropped);
    OUTPUT_ERROR();
  } else {
    CLEAR_ERROR();
    aDeclaration->AddVariableDeclaration(aVariableName, variableType,
                                         variableValue, aIsImportant, true);
    *aChanged = true;
  }

  mTempData.AssertInitialState();

  ReleaseScanner();
}

void
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
}



bool
CSSParserImpl::ParseSourceSizeList(const nsAString& aBuffer,
                                   nsIURI* aURI, 
                                   uint32_t aLineNumber, 
                                   InfallibleTArray< nsAutoPtr<nsMediaQuery> >& aQueries,
                                   InfallibleTArray<nsCSSValue>& aValues,
                                   bool aHTMLMode)
{
  aQueries.Clear();
  aValues.Clear();

  
  nsCSSScanner scanner(aBuffer, aLineNumber);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aURI);
  InitScanner(scanner, reporter, aURI, aURI, nullptr);

  
  mHTMLMediaMode = aHTMLMode;

  bool hitError = false;
  for (;;) {
    nsAutoPtr<nsMediaQuery> query;
    nsCSSValue value;

    bool hitStop;
    if (!ParseMediaQuery(eMediaQuerySingleCondition, getter_Transfers(query),
                         &hitStop)) {
      NS_ASSERTION(!hitStop, "should return true when hit stop");
      hitError = true;
      break;
    }

    if (!query) {
      REPORT_UNEXPECTED_EOF(PEParseSourceSizeListEOF);
      NS_ASSERTION(hitStop,
                   "should return hitStop or an error if returning no query");
      hitError = true;
      break;
    }

    if (hitStop) {
      
      
      
      query->SetNegated();
    }

    if (!ParseNonNegativeVariant(value, VARIANT_LPCALC, nullptr)) {
      hitError = true;
      break;
    }

    aQueries.AppendElement(query.forget());
    aValues.AppendElement(value);

    if (!GetToken(true)) {
      
      break;
    }

    if (eCSSToken_Symbol != mToken.mType || mToken.mSymbol != ',') {
      REPORT_UNEXPECTED_TOKEN(PEParseSourceSizeListNotComma);
      hitError = true;
      break;
    }
  }

  if (hitError) {
    
    
    
    
    OUTPUT_ERROR();
  }

  CLEAR_ERROR();
  ReleaseScanner();
  mHTMLMediaMode = false;

  return !hitError;
}

bool
CSSParserImpl::ParseColorString(const nsSubstring& aBuffer,
                                nsIURI* aURI, 
                                uint32_t aLineNumber, 
                                nsCSSValue& aValue,
                                bool aSuppressErrors )
{
  nsCSSScanner scanner(aBuffer, aLineNumber);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aURI);
  InitScanner(scanner, reporter, aURI, aURI, nullptr);

  nsAutoSuppressErrors suppressErrors(this, aSuppressErrors);

  
  bool colorParsed = ParseColor(aValue) && !GetToken(true);

  if (aSuppressErrors) {
    CLEAR_ERROR();
  } else {
    OUTPUT_ERROR();
  }

  ReleaseScanner();
  return colorParsed;
}

bool
CSSParserImpl::ParseFontFamilyListString(const nsSubstring& aBuffer,
                                         nsIURI* aURI, 
                                         uint32_t aLineNumber, 
                                         nsCSSValue& aValue)
{
  nsCSSScanner scanner(aBuffer, aLineNumber);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aURI);
  InitScanner(scanner, reporter, aURI, aURI, nullptr);

  
  bool familyParsed = ParseFamily(aValue) && !GetToken(true);
  OUTPUT_ERROR();
  ReleaseScanner();
  return familyParsed;
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

  bool success = ParseSelectorList(*aSelectorList, char16_t(0));

  
  
  
  
  
  
  
  
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
  MOZ_ASSERT(aSelectorList.IsEmpty(), "given list should start empty");

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
  nsCSSProperty propID = LookupEnabledProperty(aProperty);
  if (propID == eCSSProperty_UNKNOWN) {
    return false;
  }

  nsCSSScanner scanner(aValue, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aDocURL);
  InitScanner(scanner, reporter, aDocURL, aBaseURL, aDocPrincipal);
  nsAutoSuppressErrors suppressErrors(this);

  bool parsedOK;

  if (propID == eCSSPropertyExtra_variable) {
    MOZ_ASSERT(Substring(aProperty, 0,
                         CSS_CUSTOM_NAME_PREFIX_LENGTH).EqualsLiteral("--"));
    const nsDependentSubstring varName =
      Substring(aProperty, CSS_CUSTOM_NAME_PREFIX_LENGTH);  
    CSSVariableDeclarations::Type variableType;
    nsString variableValue;
    parsedOK = ParseVariableDeclaration(&variableType, variableValue) &&
               !GetToken(true);
  } else {
    parsedOK = ParseProperty(propID) && !GetToken(true);

    mTempData.ClearProperty(propID);
    mTempData.AssertInitialState();
  }

  CLEAR_ERROR();
  ReleaseScanner();

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
CSSParserImpl::EnumerateVariableReferences(const nsAString& aPropertyValue,
                                           VariableEnumFunc aFunc,
                                           void* aData)
{
  nsCSSScanner scanner(aPropertyValue, 0);
  css::ErrorReporter reporter(scanner, nullptr, nullptr, nullptr);
  InitScanner(scanner, reporter, nullptr, nullptr, nullptr);
  nsAutoSuppressErrors suppressErrors(this);

  CSSVariableDeclarations::Type type;
  bool dropBackslash;
  nsString impliedCharacters;
  bool result = ParseValueWithVariables(&type, &dropBackslash,
                                        impliedCharacters, aFunc, aData) &&
                !GetToken(true);

  ReleaseScanner();

  return result;
}

static bool
SeparatorRequiredBetweenTokens(nsCSSTokenSerializationType aToken1,
                               nsCSSTokenSerializationType aToken2)
{
  
  
  
  switch (aToken1) {
    case eCSSTokenSerialization_Ident:
      return aToken2 == eCSSTokenSerialization_Ident ||
             aToken2 == eCSSTokenSerialization_Function ||
             aToken2 == eCSSTokenSerialization_URL_or_BadURL ||
             aToken2 == eCSSTokenSerialization_Symbol_Minus ||
             aToken2 == eCSSTokenSerialization_Number ||
             aToken2 == eCSSTokenSerialization_Percentage ||
             aToken2 == eCSSTokenSerialization_Dimension ||
             aToken2 == eCSSTokenSerialization_URange ||
             aToken2 == eCSSTokenSerialization_CDC ||
             aToken2 == eCSSTokenSerialization_Symbol_OpenParen;
    case eCSSTokenSerialization_AtKeyword_or_Hash:
    case eCSSTokenSerialization_Dimension:
      return aToken2 == eCSSTokenSerialization_Ident ||
             aToken2 == eCSSTokenSerialization_Function ||
             aToken2 == eCSSTokenSerialization_URL_or_BadURL ||
             aToken2 == eCSSTokenSerialization_Symbol_Minus ||
             aToken2 == eCSSTokenSerialization_Number ||
             aToken2 == eCSSTokenSerialization_Percentage ||
             aToken2 == eCSSTokenSerialization_Dimension ||
             aToken2 == eCSSTokenSerialization_URange ||
             aToken2 == eCSSTokenSerialization_CDC;
    case eCSSTokenSerialization_Symbol_Hash:
      return aToken2 == eCSSTokenSerialization_Ident ||
             aToken2 == eCSSTokenSerialization_Function ||
             aToken2 == eCSSTokenSerialization_URL_or_BadURL ||
             aToken2 == eCSSTokenSerialization_Symbol_Minus ||
             aToken2 == eCSSTokenSerialization_Number ||
             aToken2 == eCSSTokenSerialization_Percentage ||
             aToken2 == eCSSTokenSerialization_Dimension ||
             aToken2 == eCSSTokenSerialization_URange;
    case eCSSTokenSerialization_Symbol_Minus:
    case eCSSTokenSerialization_Number:
      return aToken2 == eCSSTokenSerialization_Ident ||
             aToken2 == eCSSTokenSerialization_Function ||
             aToken2 == eCSSTokenSerialization_URL_or_BadURL ||
             aToken2 == eCSSTokenSerialization_Number ||
             aToken2 == eCSSTokenSerialization_Percentage ||
             aToken2 == eCSSTokenSerialization_Dimension ||
             aToken2 == eCSSTokenSerialization_URange;
    case eCSSTokenSerialization_Symbol_At:
      return aToken2 == eCSSTokenSerialization_Ident ||
             aToken2 == eCSSTokenSerialization_Function ||
             aToken2 == eCSSTokenSerialization_URL_or_BadURL ||
             aToken2 == eCSSTokenSerialization_Symbol_Minus ||
             aToken2 == eCSSTokenSerialization_URange;
    case eCSSTokenSerialization_URange:
      return aToken2 == eCSSTokenSerialization_Ident ||
             aToken2 == eCSSTokenSerialization_Function ||
             aToken2 == eCSSTokenSerialization_Number ||
             aToken2 == eCSSTokenSerialization_Percentage ||
             aToken2 == eCSSTokenSerialization_Dimension ||
             aToken2 == eCSSTokenSerialization_Symbol_Question;
    case eCSSTokenSerialization_Symbol_Dot_or_Plus:
      return aToken2 == eCSSTokenSerialization_Number ||
             aToken2 == eCSSTokenSerialization_Percentage ||
             aToken2 == eCSSTokenSerialization_Dimension;
    case eCSSTokenSerialization_Symbol_Assorted:
    case eCSSTokenSerialization_Symbol_Asterisk:
      return aToken2 == eCSSTokenSerialization_Symbol_Equals;
    case eCSSTokenSerialization_Symbol_Bar:
      return aToken2 == eCSSTokenSerialization_Symbol_Equals ||
             aToken2 == eCSSTokenSerialization_Symbol_Bar ||
             aToken2 == eCSSTokenSerialization_DashMatch;              
    case eCSSTokenSerialization_Symbol_Slash:
      return aToken2 == eCSSTokenSerialization_Symbol_Asterisk ||
             aToken2 == eCSSTokenSerialization_ContainsMatch;          
    default:
      MOZ_ASSERT(aToken1 == eCSSTokenSerialization_Nothing ||
                 aToken1 == eCSSTokenSerialization_Whitespace ||
                 aToken1 == eCSSTokenSerialization_Percentage ||
                 aToken1 == eCSSTokenSerialization_URL_or_BadURL ||
                 aToken1 == eCSSTokenSerialization_Function ||
                 aToken1 == eCSSTokenSerialization_CDC ||
                 aToken1 == eCSSTokenSerialization_Symbol_OpenParen ||
                 aToken1 == eCSSTokenSerialization_Symbol_Question ||
                 aToken1 == eCSSTokenSerialization_Symbol_Assorted ||
                 aToken1 == eCSSTokenSerialization_Symbol_Asterisk ||
                 aToken1 == eCSSTokenSerialization_Symbol_Equals ||
                 aToken1 == eCSSTokenSerialization_Symbol_Bar ||
                 aToken1 == eCSSTokenSerialization_Symbol_Slash ||
                 aToken1 == eCSSTokenSerialization_Other ||
                 "unexpected nsCSSTokenSerializationType value");
      return false;
  }
}






static void
AppendTokens(nsAString& aResult,
             nsCSSTokenSerializationType& aResultFirstToken,
             nsCSSTokenSerializationType& aResultLastToken,
             nsCSSTokenSerializationType aValueFirstToken,
             nsCSSTokenSerializationType aValueLastToken,
             const nsAString& aValue)
{
  if (SeparatorRequiredBetweenTokens(aResultLastToken, aValueFirstToken)) {
    aResult.AppendLiteral("/**/");
  }
  aResult.Append(aValue);
  if (aResultFirstToken == eCSSTokenSerialization_Nothing) {
    aResultFirstToken = aValueFirstToken;
  }
  if (aValueLastToken != eCSSTokenSerialization_Nothing) {
    aResultLastToken = aValueLastToken;
  }
}






static void
StopRecordingAndAppendTokens(nsString& aResult,
                             nsCSSTokenSerializationType& aResultFirstToken,
                             nsCSSTokenSerializationType& aResultLastToken,
                             nsCSSTokenSerializationType aValueFirstToken,
                             nsCSSTokenSerializationType aValueLastToken,
                             nsCSSScanner* aScanner)
{
  if (SeparatorRequiredBetweenTokens(aResultLastToken, aValueFirstToken)) {
    aResult.AppendLiteral("/**/");
  }
  aScanner->StopRecording(aResult);
  if (aResultFirstToken == eCSSTokenSerialization_Nothing) {
    aResultFirstToken = aValueFirstToken;
  }
  if (aValueLastToken != eCSSTokenSerialization_Nothing) {
    aResultLastToken = aValueLastToken;
  }
}

bool
CSSParserImpl::ResolveValueWithVariableReferencesRec(
                                     nsString& aResult,
                                     nsCSSTokenSerializationType& aResultFirstToken,
                                     nsCSSTokenSerializationType& aResultLastToken,
                                     const CSSVariableValues* aVariables)
{
  
  
  MOZ_ASSERT(mScanner->IsRecording());
  MOZ_ASSERT(aResult.IsEmpty());

  
  nsAutoTArray<char16_t, 16> stack;

  
  nsString value;

  
  
  
  
  uint32_t lengthBeforeVar = 0;

  
  
  
  
  nsCSSTokenSerializationType valueFirstToken = eCSSTokenSerialization_Nothing,
                              valueLastToken  = eCSSTokenSerialization_Nothing,
                              recFirstToken   = eCSSTokenSerialization_Nothing,
                              recLastToken    = eCSSTokenSerialization_Nothing;

#define UPDATE_RECORDING_TOKENS(type)                    \
  if (recFirstToken == eCSSTokenSerialization_Nothing) { \
    recFirstToken = type;                                \
  }                                                      \
  recLastToken = type;

  while (GetToken(false)) {
    switch (mToken.mType) {
      case eCSSToken_Symbol: {
        nsCSSTokenSerializationType type = eCSSTokenSerialization_Other;
        if (mToken.mSymbol == '(') {
          stack.AppendElement(')');
          type = eCSSTokenSerialization_Symbol_OpenParen;
        } else if (mToken.mSymbol == '[') {
          stack.AppendElement(']');
        } else if (mToken.mSymbol == '{') {
          stack.AppendElement('}');
        } else if (mToken.mSymbol == ';') {
          if (stack.IsEmpty()) {
            
            
            return false;
          }
        } else if (mToken.mSymbol == '!') {
          if (stack.IsEmpty()) {
            
            
            return false;
          }
        } else if (mToken.mSymbol == ')' &&
                   stack.IsEmpty()) {
          
          nsString finalTokens;
          mScanner->StopRecording(finalTokens);
          MOZ_ASSERT(finalTokens[finalTokens.Length() - 1] == ')');
          finalTokens.Truncate(finalTokens.Length() - 1);
          aResult.Append(value);

          AppendTokens(aResult, valueFirstToken, valueLastToken,
                       recFirstToken, recLastToken, finalTokens);

          mScanner->StartRecording();
          UngetToken();
          aResultFirstToken = valueFirstToken;
          aResultLastToken = valueLastToken;
          return true;
        } else if (mToken.mSymbol == ')' ||
                   mToken.mSymbol == ']' ||
                   mToken.mSymbol == '}') {
          if (stack.IsEmpty() ||
              stack.LastElement() != mToken.mSymbol) {
            
            return false;
          }
          stack.TruncateLength(stack.Length() - 1);
        } else if (mToken.mSymbol == '#') {
          type = eCSSTokenSerialization_Symbol_Hash;
        } else if (mToken.mSymbol == '@') {
          type = eCSSTokenSerialization_Symbol_At;
        } else if (mToken.mSymbol == '.' ||
                   mToken.mSymbol == '+') {
          type = eCSSTokenSerialization_Symbol_Dot_or_Plus;
        } else if (mToken.mSymbol == '-') {
          type = eCSSTokenSerialization_Symbol_Minus;
        } else if (mToken.mSymbol == '?') {
          type = eCSSTokenSerialization_Symbol_Question;
        } else if (mToken.mSymbol == '$' ||
                   mToken.mSymbol == '^' ||
                   mToken.mSymbol == '~') {
          type = eCSSTokenSerialization_Symbol_Assorted;
        } else if (mToken.mSymbol == '=') {
          type = eCSSTokenSerialization_Symbol_Equals;
        } else if (mToken.mSymbol == '|') {
          type = eCSSTokenSerialization_Symbol_Bar;
        } else if (mToken.mSymbol == '/') {
          type = eCSSTokenSerialization_Symbol_Slash;
        } else if (mToken.mSymbol == '*') {
          type = eCSSTokenSerialization_Symbol_Asterisk;
        }
        UPDATE_RECORDING_TOKENS(type);
        break;
      }

      case eCSSToken_Function:
        if (mToken.mIdent.LowerCaseEqualsLiteral("var")) {
          
          nsString recording;
          mScanner->StopRecording(recording);
          recording.Truncate(lengthBeforeVar);
          AppendTokens(value, valueFirstToken, valueLastToken,
                       recFirstToken, recLastToken, recording);
          recFirstToken = eCSSTokenSerialization_Nothing;
          recLastToken = eCSSTokenSerialization_Nothing;

          if (!GetToken(true) ||
              mToken.mType != eCSSToken_Ident ||
              !nsCSSProps::IsCustomPropertyName(mToken.mIdent)) {
            
            
            return false;
          }

          
          
          MOZ_ASSERT(Substring(mToken.mIdent, 0,
                               CSS_CUSTOM_NAME_PREFIX_LENGTH).
                       EqualsLiteral("--"));
          nsDependentString variableName(mToken.mIdent,
                                         CSS_CUSTOM_NAME_PREFIX_LENGTH);

          
          
          
          
          nsString variableValue;
          nsCSSTokenSerializationType varFirstToken, varLastToken;
          bool valid = aVariables->Get(variableName, variableValue,
                                       varFirstToken, varLastToken) &&
                       !variableValue.IsEmpty();

          if (!GetToken(true) ||
              mToken.IsSymbol(')')) {
            mScanner->StartRecording();
            if (!valid) {
              
              return false;
            }
            
            AppendTokens(value, valueFirstToken, valueLastToken,
                         varFirstToken, varLastToken, variableValue);
          } else if (mToken.IsSymbol(',')) {
            mScanner->StartRecording();
            if (!GetToken(false) ||
                mToken.IsSymbol(')')) {
              
              return false;
            }
            UngetToken();
            if (valid) {
              
              mScanner->StopRecording();
              AppendTokens(value, valueFirstToken, valueLastToken,
                           varFirstToken, varLastToken, variableValue);
              bool ok = SkipBalancedContentUntil(')');
              mScanner->StartRecording();
              if (!ok) {
                return false;
              }
            } else {
              nsString fallback;
              if (!ResolveValueWithVariableReferencesRec(fallback,
                                                         varFirstToken,
                                                         varLastToken,
                                                         aVariables)) {
                
                
                return false;
              }
              AppendTokens(value, valueFirstToken, valueLastToken,
                           varFirstToken, varLastToken, fallback);
              
              
              DebugOnly<bool> gotToken = GetToken(false);
              MOZ_ASSERT(!gotToken || mToken.IsSymbol(')'));
            }
          } else {
            
            mScanner->StartRecording();
            return false;
          }
        } else {
          stack.AppendElement(')');
          UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_Function);
        }
        break;

      case eCSSToken_Bad_String:
      case eCSSToken_Bad_URL:
        return false;

      case eCSSToken_Whitespace:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_Whitespace);
        break;

      case eCSSToken_AtKeyword:
      case eCSSToken_Hash:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_AtKeyword_or_Hash);
        break;

      case eCSSToken_Number:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_Number);
        break;

      case eCSSToken_Dimension:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_Dimension);
        break;

      case eCSSToken_Ident:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_Ident);
        break;

      case eCSSToken_Percentage:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_Percentage);
        break;

      case eCSSToken_URange:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_URange);
        break;

      case eCSSToken_URL:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_URL_or_BadURL);
        break;

      case eCSSToken_HTMLComment:
        if (mToken.mIdent[0] == '-') {
          UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_CDC);
        } else {
          UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_Other);
        }
        break;

      case eCSSToken_Dashmatch:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_DashMatch);
        break;

      case eCSSToken_Containsmatch:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_ContainsMatch);
        break;

      default:
        NS_NOTREACHED("unexpected token type");
        
      case eCSSToken_ID:
      case eCSSToken_String:
      case eCSSToken_Includes:
      case eCSSToken_Beginsmatch:
      case eCSSToken_Endsmatch:
        UPDATE_RECORDING_TOKENS(eCSSTokenSerialization_Other);
        break;
    }

    lengthBeforeVar = mScanner->RecordingLength();
  }

#undef UPDATE_RECORDING_TOKENS

  aResult.Append(value);
  StopRecordingAndAppendTokens(aResult, valueFirstToken, valueLastToken,
                               recFirstToken, recLastToken, mScanner);

  
  if (!stack.IsEmpty()) {
    do {
      aResult.Append(stack.LastElement());
      stack.TruncateLength(stack.Length() - 1);
    } while (!stack.IsEmpty());
    valueLastToken = eCSSTokenSerialization_Other;
  }

  mScanner->StartRecording();
  aResultFirstToken = valueFirstToken;
  aResultLastToken = valueLastToken;
  return true;
}

bool
CSSParserImpl::ResolveValueWithVariableReferences(
                                        const CSSVariableValues* aVariables,
                                        nsString& aResult,
                                        nsCSSTokenSerializationType& aFirstToken,
                                        nsCSSTokenSerializationType& aLastToken)
{
  aResult.Truncate(0);

  
  mScanner->StartRecording();

  if (!GetToken(false)) {
    
    mScanner->StopRecording();
    return false;
  }

  UngetToken();

  nsString value;
  nsCSSTokenSerializationType firstToken, lastToken;
  bool ok = ResolveValueWithVariableReferencesRec(value, firstToken, lastToken, aVariables) &&
            !GetToken(true);

  mScanner->StopRecording();

  if (ok) {
    aResult = value;
    aFirstToken = firstToken;
    aLastToken = lastToken;
  }
  return ok;
}

bool
CSSParserImpl::ResolveVariableValue(const nsAString& aPropertyValue,
                                    const CSSVariableValues* aVariables,
                                    nsString& aResult,
                                    nsCSSTokenSerializationType& aFirstToken,
                                    nsCSSTokenSerializationType& aLastToken)
{
  nsCSSScanner scanner(aPropertyValue, 0);

  
  
  
  
  
  css::ErrorReporter reporter(scanner, nullptr, nullptr, nullptr);
  InitScanner(scanner, reporter, nullptr, nullptr, nullptr);

  bool valid = ResolveValueWithVariableReferences(aVariables, aResult,
                                                  aFirstToken, aLastToken);

  ReleaseScanner();
  return valid;
}

void
CSSParserImpl::ParsePropertyWithVariableReferences(
                                            nsCSSProperty aPropertyID,
                                            nsCSSProperty aShorthandPropertyID,
                                            const nsAString& aValue,
                                            const CSSVariableValues* aVariables,
                                            nsRuleData* aRuleData,
                                            nsIURI* aDocURL,
                                            nsIURI* aBaseURL,
                                            nsIPrincipal* aDocPrincipal,
                                            CSSStyleSheet* aSheet,
                                            uint32_t aLineNumber,
                                            uint32_t aLineOffset)
{
  mTempData.AssertInitialState();

  bool valid;
  nsString expandedValue;

  
  {
    nsCSSScanner scanner(aValue, 0);
    css::ErrorReporter reporter(scanner, aSheet, mChildLoader, aDocURL);
    InitScanner(scanner, reporter, aDocURL, aBaseURL, aDocPrincipal);

    nsCSSTokenSerializationType firstToken, lastToken;
    valid = ResolveValueWithVariableReferences(aVariables, expandedValue,
                                               firstToken, lastToken);
    if (!valid) {
      NS_ConvertASCIItoUTF16 propName(nsCSSProps::GetStringValue(aPropertyID));
      REPORT_UNEXPECTED(PEInvalidVariableReference);
      REPORT_UNEXPECTED_P(PEValueParsingError, propName);
      if (nsCSSProps::IsInherited(aPropertyID)) {
        REPORT_UNEXPECTED(PEValueWithVariablesFallbackInherit);
      } else {
        REPORT_UNEXPECTED(PEValueWithVariablesFallbackInitial);
      }
      OUTPUT_ERROR_WITH_POSITION(aLineNumber, aLineOffset);
    }
    ReleaseScanner();
  }

  nsCSSProperty propertyToParse =
    aShorthandPropertyID != eCSSProperty_UNKNOWN ? aShorthandPropertyID :
                                                   aPropertyID;

  
  if (valid) {
    nsCSSScanner scanner(expandedValue, 0);
    css::ErrorReporter reporter(scanner, aSheet, mChildLoader, aDocURL);
    InitScanner(scanner, reporter, aDocURL, aBaseURL, aDocPrincipal);
    valid = ParseProperty(propertyToParse);
    if (valid && GetToken(true)) {
      REPORT_UNEXPECTED_TOKEN(PEExpectEndValue);
      valid = false;
    }
    if (!valid) {
      NS_ConvertASCIItoUTF16 propName(nsCSSProps::GetStringValue(
                                                              propertyToParse));
      REPORT_UNEXPECTED_P(PEValueWithVariablesParsingError, propName);
      if (nsCSSProps::IsInherited(aPropertyID)) {
        REPORT_UNEXPECTED(PEValueWithVariablesFallbackInherit);
      } else {
        REPORT_UNEXPECTED(PEValueWithVariablesFallbackInitial);
      }
      OUTPUT_ERROR_WITH_POSITION(aLineNumber, aLineOffset);
    }
    ReleaseScanner();
  }

  
  
  
  if (!valid) {
    nsCSSValue defaultValue;
    if (nsCSSProps::IsInherited(aPropertyID)) {
      defaultValue.SetInheritValue();
    } else {
      defaultValue.SetInitialValue();
    }
    mTempData.AddLonghandProperty(aPropertyID, defaultValue);
  }

  
  mTempData.MapRuleInfoInto(aPropertyID, aRuleData);

  mTempData.ClearProperty(propertyToParse);
  mTempData.AssertInitialState();
}

bool
CSSParserImpl::ParseCounterStyleName(const nsAString& aBuffer,
                                     nsIURI* aURL,
                                     nsAString& aName)
{
  nsCSSScanner scanner(aBuffer, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aURL);
  InitScanner(scanner, reporter, aURL, aURL, nullptr);

  bool success = ParseCounterStyleName(aName, true) && !GetToken(true);

  OUTPUT_ERROR();
  ReleaseScanner();

  return success;
}

bool
CSSParserImpl::ParseCounterDescriptor(nsCSSCounterDesc aDescID,
                                      const nsAString& aBuffer,
                                      nsIURI* aSheetURL,
                                      nsIURI* aBaseURL,
                                      nsIPrincipal* aSheetPrincipal,
                                      nsCSSValue& aValue)
{
  nsCSSScanner scanner(aBuffer, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aSheetURL);
  InitScanner(scanner, reporter, aSheetURL, aBaseURL, aSheetPrincipal);

  bool success = ParseCounterDescriptorValue(aDescID, aValue) &&
                 !GetToken(true);

  OUTPUT_ERROR();
  ReleaseScanner();

  return success;
}

bool
CSSParserImpl::ParseFontFaceDescriptor(nsCSSFontDesc aDescID,
                                       const nsAString& aBuffer,
                                       nsIURI* aSheetURL,
                                       nsIURI* aBaseURL,
                                       nsIPrincipal* aSheetPrincipal,
                                       nsCSSValue& aValue)
{
  nsCSSScanner scanner(aBuffer, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, aSheetURL);
  InitScanner(scanner, reporter, aSheetURL, aBaseURL, aSheetPrincipal);

  bool success = ParseFontDescriptorValue(aDescID, aValue) &&
                 !GetToken(true);

  OUTPUT_ERROR();
  ReleaseScanner();

  return success;
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
CSSParserImpl::GetNextTokenLocation(bool aSkipWS, uint32_t *linenum, uint32_t *colnum)
{
  
  if (!GetToken(aSkipWS)) {
    return false;
  }
  UngetToken();
  
  
  *linenum = mScanner->GetLineNumber();
  *colnum = 1 + mScanner->GetColumnNumber();
  return true;
}

bool
CSSParserImpl::ExpectSymbol(char16_t aSymbol,
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
      char16_t symbol = mToken.mSymbol;
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

  } else if (mToken.mIdent.LowerCaseEqualsLiteral("font-feature-values")) {
    parseFunc = &CSSParserImpl::ParseFontFeatureValuesRule;
    newSection = eCSSSection_General;

  } else if (mToken.mIdent.LowerCaseEqualsLiteral("page")) {
    parseFunc = &CSSParserImpl::ParsePageRule;
    newSection = eCSSSection_General;

  } else if ((nsCSSProps::IsEnabled(eCSSPropertyAlias_MozAnimation) &&
              mToken.mIdent.LowerCaseEqualsLiteral("-moz-keyframes")) ||
             mToken.mIdent.LowerCaseEqualsLiteral("keyframes")) {
    parseFunc = &CSSParserImpl::ParseKeyframesRule;
    newSection = eCSSSection_General;

  } else if (mToken.mIdent.LowerCaseEqualsLiteral("supports")) {
    parseFunc = &CSSParserImpl::ParseSupportsRule;
    newSection = eCSSSection_General;

  } else if (mToken.mIdent.LowerCaseEqualsLiteral("counter-style")) {
    parseFunc = &CSSParserImpl::ParseCounterStyleRule;
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
  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum) ||
      !GetToken(true)) {
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
CSSParserImpl::ParseMediaQuery(eMediaQueryType aQueryType,
                               nsMediaQuery **aQuery,
                               bool *aHitStop)
{
  *aQuery = nullptr;
  *aHitStop = false;
  bool inAtRule = aQueryType == eMediaQueryAtRule;
  
  bool singleCondition = aQueryType == eMediaQuerySingleCondition;

  
  
  
  if (!GetToken(true)) {
    *aHitStop = true;
    
    if (!inAtRule)
      return true;

    
    REPORT_UNEXPECTED_EOF(PEGatherMediaEOF);
    return true;
  }

  if (eCSSToken_Symbol == mToken.mType && inAtRule &&
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
  } else if (singleCondition) {
    
    
    
    *aHitStop = true;
    return true;
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
      if (!gotNotOrOnly && mediaType == nsGkAtoms::_not) {
        gotNotOrOnly = true;
        query->SetNegated();
      } else if (!gotNotOrOnly && mediaType == nsGkAtoms::only) {
        gotNotOrOnly = true;
        query->SetHasOnly();
      } else if (mediaType == nsGkAtoms::_not ||
                 mediaType == nsGkAtoms::only ||
                 mediaType == nsGkAtoms::_and ||
                 mediaType == nsGkAtoms::_or) {
        REPORT_UNEXPECTED_TOKEN(PEGatherMediaReservedMediaType);
        UngetToken();
        return false;
      } else {
        
        break;
      }
    }
    query->SetType(mediaType);
  }

  for (;;) {
    if (!GetToken(true)) {
      *aHitStop = true;
      
      if (!inAtRule)
        break;

      
      REPORT_UNEXPECTED_EOF(PEGatherMediaEOF);
      break;
    }

    if (eCSSToken_Symbol == mToken.mType && inAtRule &&
        (mToken.mSymbol == ';' || mToken.mSymbol == '{' || mToken.mSymbol == '}')) {
      *aHitStop = true;
      UngetToken();
      break;
    }
    if (!singleCondition &&
        eCSSToken_Symbol == mToken.mType && mToken.mSymbol == ',') {
      
      break;
    }
    if (eCSSToken_Ident != mToken.mType ||
        !mToken.mIdent.LowerCaseEqualsLiteral("and")) {
      if (singleCondition) {
        
        
        UngetToken();
        break;
      } else {
        REPORT_UNEXPECTED_TOKEN(PEGatherMediaNotComma);
        UngetToken();
        return false;
      }
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
  eMediaQueryType type = aInAtRule ? eMediaQueryAtRule : eMediaQueryNormal;
  for (;;) {
    nsAutoPtr<nsMediaQuery> query;
    bool hitStop;
    if (!ParseMediaQuery(type, getter_Transfers(query), &hitStop)) {
      NS_ASSERTION(!hitStop, "should return true when hit stop");
      OUTPUT_ERROR();
      if (query) {
        query->SetHadUnknownExpression();
      }
      if (aInAtRule) {
        const char16_t stopChars[] =
          { char16_t(','), char16_t('{'), char16_t(';'), char16_t('}'), char16_t(0) };
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
  const char16_t *featureString;
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

  bool rv = false;
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

  uint32_t linenum, colnum;
  nsAutoString url;
  if (!GetNextTokenLocation(true, &linenum, &colnum) ||
      !ParseURLOrString(url)) {
    REPORT_UNEXPECTED_TOKEN(PEImportNotURI);
    return false;
  }

  if (!ExpectSymbol(';', true)) {
    if (!GatherMedia(media, true) ||
        !ExpectSymbol(';', true)) {
      REPORT_UNEXPECTED_TOKEN(PEImportUnexpected);
      
      return false;
    }

    
    
    NS_ASSERTION(media->Length() != 0, "media list must be nonempty");
  }

  ProcessImport(url, media, aAppendFunc, aData, linenum, colnum);
  return true;
}

void
CSSParserImpl::ProcessImport(const nsString& aURLSpec,
                             nsMediaList* aMedia,
                             RuleAppendFunc aAppendFunc,
                             void* aData,
                             uint32_t aLineNumber,
                             uint32_t aColumnNumber)
{
  nsRefPtr<css::ImportRule> rule = new css::ImportRule(aMedia, aURLSpec,
                                                       aLineNumber,
                                                       aColumnNumber);
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
  uint32_t linenum, colnum;
  if (GetNextTokenLocation(true, &linenum, &colnum) &&
      GatherMedia(media, true)) {
    
    nsRefPtr<css::MediaRule> rule = new css::MediaRule(linenum, colnum);
    
    
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

  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum)) {
    return false;
  }

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
      REPORT_UNEXPECTED_TOKEN(PEMozDocRuleBadFunc2);
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
      mScanner->NextURL(mToken);
      if (mToken.mType != eCSSToken_URL) {
        REPORT_UNEXPECTED_TOKEN(PEMozDocRuleNotURI);
        SkipUntil(')');
        delete urls;
        return false;
      }

      
      
      
      CopyUTF16toUTF8(mToken.mIdent, cur->url);
    }
  } while (ExpectSymbol(',', true));

  nsRefPtr<css::DocumentRule> rule = new css::DocumentRule(linenum, colnum);
  rule->SetURLs(urls);

  return ParseGroupRule(rule, aAppendFunc, aData);
}


bool
CSSParserImpl::ParseNameSpaceRule(RuleAppendFunc aAppendFunc, void* aData)
{
  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum) ||
      !GetToken(true)) {
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

  ProcessNameSpace(prefix, url, aAppendFunc, aData, linenum, colnum);
  return true;
}

void
CSSParserImpl::ProcessNameSpace(const nsString& aPrefix,
                                const nsString& aURLSpec,
                                RuleAppendFunc aAppendFunc,
                                void* aData,
                                uint32_t aLineNumber,
                                uint32_t aColumnNumber)
{
  nsCOMPtr<nsIAtom> prefix;

  if (!aPrefix.IsEmpty()) {
    prefix = do_GetAtom(aPrefix);
    if (!prefix) {
      NS_RUNTIMEABORT("do_GetAtom failed - out of memory?");
    }
  }

  nsRefPtr<css::NameSpaceRule> rule = new css::NameSpaceRule(prefix, aURLSpec,
                                                             aLineNumber,
                                                             aColumnNumber);
  (*aAppendFunc)(rule, aData);

  
  
  if (!mNameSpaceMap) {
    mNameSpaceMap = mSheet->GetNameSpaceMap();
  }
}



bool
CSSParserImpl::ParseFontFaceRule(RuleAppendFunc aAppendFunc, void* aData)
{
  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum) ||
      !ExpectSymbol('{', true)) {
    REPORT_UNEXPECTED_TOKEN(PEBadFontBlockStart);
    return false;
  }

  nsRefPtr<nsCSSFontFaceRule> rule(new nsCSSFontFaceRule(linenum, colnum));

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
CSSParserImpl::ParseFontFeatureValuesRule(RuleAppendFunc aAppendFunc,
                                          void* aData)
{
  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum)) {
    return false;
  }

  nsRefPtr<nsCSSFontFeatureValuesRule>
               valuesRule(new nsCSSFontFeatureValuesRule(linenum, colnum));

  
  nsCSSValue fontlistValue;

  if (!ParseFamily(fontlistValue) ||
      fontlistValue.GetUnit() != eCSSUnit_FontFamilyList)
  {
    REPORT_UNEXPECTED_TOKEN(PEFFVNoFamily);
    return false;
  }

  
  const FontFamilyList* fontlist = fontlistValue.GetFontFamilyListValue();

  
  if (fontlist->HasGeneric()) {
    REPORT_UNEXPECTED_TOKEN(PEFFVGenericInFamilyList);
    return false;
  }

  valuesRule->SetFamilyList(*fontlist);

  
  if (!ExpectSymbol('{', true)) {
    REPORT_UNEXPECTED_TOKEN(PEFFVBlockStart);
    return false;
  }

  
  
  for (;;) {
    if (!GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PEFFVUnexpectedEOF);
      break;
    }
    if (mToken.IsSymbol('}')) { 
      UngetToken();
      break;
    }

    if (!ParseFontFeatureValueSet(valuesRule)) {
      if (!SkipAtRule(false)) {
        break;
      }
    }
  }
  if (!ExpectSymbol('}', true)) {
    REPORT_UNEXPECTED_TOKEN(PEFFVUnexpectedBlockEnd);
    SkipUntil('}');
    return false;
  }

  (*aAppendFunc)(valuesRule, aData);
  return true;
}

#define NUMVALUES_NO_LIMIT  0xFFFF




bool
CSSParserImpl::ParseFontFeatureValueSet(nsCSSFontFeatureValuesRule
                                                            *aFeatureValuesRule)
{
  
  if (eCSSToken_AtKeyword != mToken.mType) {
    REPORT_UNEXPECTED_TOKEN(PEFontFeatureValuesNoAt);
    OUTPUT_ERROR();
    UngetToken();
    return false;
  }

  
  int32_t whichVariant;
  nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);
  if (keyword == eCSSKeyword_UNKNOWN ||
      !nsCSSProps::FindKeyword(keyword,
                               nsCSSProps::kFontVariantAlternatesFuncsKTable,
                               whichVariant))
  {
    if (!NonMozillaVendorIdentifier(mToken.mIdent)) {
      REPORT_UNEXPECTED_TOKEN(PEFFVUnknownFontVariantPropValue);
      OUTPUT_ERROR();
    }
    UngetToken();
    return false;
  }

  nsAutoString featureType(mToken.mIdent);

  
  if (!ExpectSymbol('{', true)) {
    REPORT_UNEXPECTED_TOKEN(PEFFVValueSetStart);
    return false;
  }

  
  int32_t limitNumValues;

  switch (keyword) {
    case eCSSKeyword_styleset:
      limitNumValues = NUMVALUES_NO_LIMIT;
      break;
    case eCSSKeyword_character_variant:
      limitNumValues = 2;
      break;
    default:
      limitNumValues = 1;
      break;
  }

  
  nsAutoTArray<gfxFontFeatureValueSet::ValueList, 5> values;

  
  for (;;) {
    nsAutoString valueId;

    if (!GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PEFFVUnexpectedEOF);
      break;
    }

    
    if (mToken.IsSymbol(';')) {
      continue;
    }

    
    if (mToken.IsSymbol('}')) {
      break;
    }

    
    if (eCSSToken_Ident != mToken.mType) {
      REPORT_UNEXPECTED_TOKEN(PEFFVExpectedIdent);
      if (!SkipDeclaration(true)) {
        break;
      }
      continue;
    }

    valueId.Assign(mToken.mIdent);

    
    if (!ExpectSymbol(':', true)) {
      REPORT_UNEXPECTED_TOKEN(PEParseDeclarationNoColon);
      OUTPUT_ERROR();
      if (!SkipDeclaration(true)) {
        break;
      }
      continue;
    }

    
    nsAutoTArray<uint32_t,4>   featureSelectors;

    nsCSSValue intValue;
    while (ParseNonNegativeVariant(intValue, VARIANT_INTEGER, nullptr)) {
      featureSelectors.AppendElement(uint32_t(intValue.GetIntValue()));
    }

    int32_t numValues = featureSelectors.Length();

    if (numValues == 0) {
      REPORT_UNEXPECTED_TOKEN(PEFFVExpectedValue);
      OUTPUT_ERROR();
      if (!SkipDeclaration(true)) {
        break;
      }
      continue;
    }

    if (numValues > limitNumValues) {
      REPORT_UNEXPECTED_P(PEFFVTooManyValues, featureType);
      OUTPUT_ERROR();
      if (!SkipDeclaration(true)) {
        break;
      }
      continue;
    }

    if (!GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PEFFVUnexpectedEOF);
      gfxFontFeatureValueSet::ValueList v(valueId, featureSelectors);
      values.AppendElement(v);
      break;
    }

    
    if (!mToken.IsSymbol(';') && !mToken.IsSymbol('}')) {
      REPORT_UNEXPECTED_TOKEN(PEFFVValueDefinitionTrailing);
      OUTPUT_ERROR();
      if (!SkipDeclaration(true)) {
        break;
      }
      continue;
    }

    gfxFontFeatureValueSet::ValueList v(valueId, featureSelectors);
    values.AppendElement(v);

    if (mToken.IsSymbol('}')) {
      break;
    }
 }

  aFeatureValuesRule->AddValueList(whichVariant, values);
  return true;
}

bool
CSSParserImpl::ParseKeyframesRule(RuleAppendFunc aAppendFunc, void* aData)
{
  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum) ||
      !GetToken(true)) {
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

  nsRefPtr<nsCSSKeyframesRule> rule = new nsCSSKeyframesRule(name,
                                                             linenum, colnum);

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
  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum)) {
    return false;
  }

  
  uint32_t parseFlags = eParseDeclaration_InBraces |
                        eParseDeclaration_AllowImportant;

  
  MOZ_ASSERT(mViewportUnitsEnabled,
             "Viewport units should be enabled outside of @page rules.");
  mViewportUnitsEnabled = false;
  nsAutoPtr<css::Declaration> declaration(
                                ParseDeclarationBlock(parseFlags,
                                                      eCSSContext_Page));
  mViewportUnitsEnabled = true;

  if (!declaration) {
    return false;
  }

  
  nsRefPtr<nsCSSPageRule> rule = new nsCSSPageRule(Move(declaration),
                                                   linenum, colnum);

  (*aAppendFunc)(rule, aData);
  return true;
}

already_AddRefed<nsCSSKeyframeRule>
CSSParserImpl::ParseKeyframeRule()
{
  InfallibleTArray<float> selectorList;
  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum) ||
      !ParseKeyframeSelectorList(selectorList)) {
    REPORT_UNEXPECTED(PEBadSelectorKeyframeRuleIgnored);
    return nullptr;
  }

  
  uint32_t parseFlags = eParseDeclaration_InBraces;
  nsAutoPtr<css::Declaration> declaration(ParseDeclarationBlock(parseFlags));
  if (!declaration) {
    return nullptr;
  }

  
  nsRefPtr<nsCSSKeyframeRule> rule =
    new nsCSSKeyframeRule(selectorList, Move(declaration), linenum, colnum);
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

  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum)) {
    return false;
  }

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

  nsRefPtr<css::GroupRule> rule = new CSSSupportsRule(conditionMet, condition,
                                                      linenum, colnum);
  return ParseGroupRule(rule, aAppendFunc, aProcessData);
}





bool
CSSParserImpl::ParseSupportsCondition(bool& aConditionMet)
{
  mInSupportsCondition = true;

  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PESupportsConditionStartEOF2);
    return false;
  }

  UngetToken();

  mScanner->ClearSeenBadToken();

  if (mToken.IsSymbol('(') ||
      mToken.mType == eCSSToken_Function ||
      mToken.mType == eCSSToken_URL ||
      mToken.mType == eCSSToken_Bad_URL) {
    bool result = ParseSupportsConditionInParens(aConditionMet) &&
                  ParseSupportsConditionTerms(aConditionMet) &&
                  !mScanner->SeenBadToken();
    mInSupportsCondition = false;
    return result;
  }

  if (mToken.mType == eCSSToken_Ident &&
      mToken.mIdent.LowerCaseEqualsLiteral("not")) {
    bool result = ParseSupportsConditionNegation(aConditionMet) &&
                  !mScanner->SeenBadToken();
    mInSupportsCondition = false;
    return result;
  }

  REPORT_UNEXPECTED_TOKEN(PESupportsConditionExpectedStart);
  mInSupportsCondition = false;
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

  if (!RequireWhitespace()) {
    REPORT_UNEXPECTED(PESupportsWhitespaceRequired);
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
    SkipUntil(')');
    aConditionMet = false;
    return true;
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

      nsCSSProperty propID = LookupEnabledProperty(propertyName);
      if (propID == eCSSProperty_UNKNOWN) {
        if (ExpectSymbol(')', true)) {
          UngetToken();
          return false;
        }
        aConditionMet = false;
        SkipUntil(')');
        UngetToken();
      } else if (propID == eCSSPropertyExtra_variable) {
        if (ExpectSymbol(')', false)) {
          UngetToken();
          return false;
        }
        CSSVariableDeclarations::Type variableType;
        nsString variableValue;
        aConditionMet =
          ParseVariableDeclaration(&variableType, variableValue) &&
          ParsePriority() != ePriority_Error;
        if (!aConditionMet) {
          SkipUntil(')');
          UngetToken();
        }
      } else {
        if (ExpectSymbol(')', true)) {
          UngetToken();
          return false;
        }
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
  if (!RequireWhitespace() || !GetToken(false)) {
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
  if (!RequireWhitespace()) {
    REPORT_UNEXPECTED(PESupportsWhitespaceRequired);
    return false;
  }

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
CSSParserImpl::ParseCounterStyleRule(RuleAppendFunc aAppendFunc, void* aData)
{
  nsAutoString name;
  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum) ||
      !ParseCounterStyleName(name, true)) {
    REPORT_UNEXPECTED_TOKEN(PECounterStyleNotIdent);
    return false;
  }

  if (!ExpectSymbol('{', true)) {
    REPORT_UNEXPECTED_TOKEN(PECounterStyleBadBlockStart);
    return false;
  }

  nsRefPtr<nsCSSCounterStyleRule> rule = new nsCSSCounterStyleRule(name,
                                                                   linenum,
                                                                   colnum);
  for (;;) {
    if (!GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PECounterStyleEOF);
      break;
    }
    if (mToken.IsSymbol('}')) {
      break;
    }
    if (mToken.IsSymbol(';')) {
      continue;
    }

    if (!ParseCounterDescriptor(rule)) {
      REPORT_UNEXPECTED(PEDeclSkipped);
      OUTPUT_ERROR();
      if (!SkipDeclaration(true)) {
        REPORT_UNEXPECTED_EOF(PECounterStyleEOF);
        break;
      }
    }
  }

  int32_t system = rule->GetSystem();
  bool isCorrect = false;
  switch (system) {
    case NS_STYLE_COUNTER_SYSTEM_CYCLIC:
    case NS_STYLE_COUNTER_SYSTEM_NUMERIC:
    case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
    case NS_STYLE_COUNTER_SYSTEM_SYMBOLIC:
    case NS_STYLE_COUNTER_SYSTEM_FIXED: {
      
      const nsCSSValue& symbols = rule->GetDesc(eCSSCounterDesc_Symbols);
      if (symbols.GetUnit() == eCSSUnit_List &&
          nsCSSCounterStyleRule::CheckDescValue(
              system, eCSSCounterDesc_Symbols, symbols)) {
        isCorrect = true;
      }
      break;
    }
    case NS_STYLE_COUNTER_SYSTEM_ADDITIVE: {
      
      const nsCSSValue& symbols =
        rule->GetDesc(eCSSCounterDesc_AdditiveSymbols);
      if (symbols.GetUnit() == eCSSUnit_PairList) {
        isCorrect = true;
      }
      break;
    }
    case NS_STYLE_COUNTER_SYSTEM_EXTENDS: {
      
      const nsCSSValue& symbols = rule->GetDesc(eCSSCounterDesc_Symbols);
      const nsCSSValue& additiveSymbols =
        rule->GetDesc(eCSSCounterDesc_AdditiveSymbols);
      if (symbols.GetUnit() == eCSSUnit_Null &&
          additiveSymbols.GetUnit() == eCSSUnit_Null) {
        isCorrect = true;
      }
      break;
    }
    default:
      NS_NOTREACHED("unknown system");
  }

  if (isCorrect) {
    (*aAppendFunc)(rule, aData);
  }
  return true;
}

bool
CSSParserImpl::ParseCounterStyleName(nsAString& aName, bool aForDefinition)
{
  if (!GetToken(true)) {
    return false;
  }

  if (mToken.mType != eCSSToken_Ident) {
    UngetToken();
    return false;
  }

  static const nsCSSKeyword kReservedNames[] = {
    eCSSKeyword_none,
    eCSSKeyword_decimal,
    eCSSKeyword_UNKNOWN
  };

  nsCSSValue value; 
  if (!ParseCustomIdent(value, mToken.mIdent,
                        aForDefinition ? kReservedNames : nullptr)) {
    REPORT_UNEXPECTED_TOKEN(PECounterStyleBadName);
    UngetToken();
    return false;
  }

  aName = mToken.mIdent;
  if (nsCSSProps::IsPredefinedCounterStyle(aName)) {
    ToLowerCase(aName);
  }
  return true;
}

bool
CSSParserImpl::ParseCounterStyleNameValue(nsCSSValue& aValue)
{
  nsString name;
  if (ParseCounterStyleName(name, false)) {
    aValue.SetStringValue(name, eCSSUnit_Ident);
    return true;
  }
  return false;
}

bool
CSSParserImpl::ParseCounterDescriptor(nsCSSCounterStyleRule* aRule)
{
  if (eCSSToken_Ident != mToken.mType) {
    REPORT_UNEXPECTED_TOKEN(PECounterDescExpected);
    return false;
  }

  nsString descName = mToken.mIdent;
  if (!ExpectSymbol(':', true)) {
    REPORT_UNEXPECTED_TOKEN(PEParseDeclarationNoColon);
    OUTPUT_ERROR();
    return false;
  }

  nsCSSCounterDesc descID = nsCSSProps::LookupCounterDesc(descName);
  nsCSSValue value;

  if (descID == eCSSCounterDesc_UNKNOWN) {
    REPORT_UNEXPECTED_P(PEUnknownCounterDesc, descName);
    return false;
  }

  if (!ParseCounterDescriptorValue(descID, value)) {
    REPORT_UNEXPECTED_P(PEValueParsingError, descName);
    return false;
  }

  if (!ExpectEndProperty()) {
    return false;
  }

  aRule->SetDesc(descID, value);
  return true;
}

bool
CSSParserImpl::ParseCounterDescriptorValue(nsCSSCounterDesc aDescID,
                                           nsCSSValue& aValue)
{
  
  
  static const int32_t VARIANT_COUNTER_SYMBOL =
    VARIANT_STRING | VARIANT_IDENTIFIER;

  switch (aDescID) {
    case eCSSCounterDesc_System: {
      nsCSSValue system;
      if (!ParseEnum(system, nsCSSProps::kCounterSystemKTable)) {
        return false;
      }
      switch (system.GetIntValue()) {
        case NS_STYLE_COUNTER_SYSTEM_FIXED: {
          nsCSSValue start;
          if (!ParseVariant(start, VARIANT_INTEGER, nullptr)) {
            start.SetIntValue(1, eCSSUnit_Integer);
          }
          aValue.SetPairValue(system, start);
          return true;
        }
        case NS_STYLE_COUNTER_SYSTEM_EXTENDS: {
          nsCSSValue name;
          if (!ParseCounterStyleNameValue(name)) {
            REPORT_UNEXPECTED_TOKEN(PECounterExtendsNotIdent);
            return false;
          }
          aValue.SetPairValue(system, name);
          return true;
        }
        default:
          aValue = system;
          return true;
      }
    }

    case eCSSCounterDesc_Negative: {
      nsCSSValue first, second;
      if (!ParseVariant(first, VARIANT_COUNTER_SYMBOL, nullptr)) {
        return false;
      }
      if (!ParseVariant(second, VARIANT_COUNTER_SYMBOL, nullptr)) {
        aValue = first;
      } else {
        aValue.SetPairValue(first, second);
      }
      return true;
    }

    case eCSSCounterDesc_Prefix:
    case eCSSCounterDesc_Suffix:
      return ParseVariant(aValue, VARIANT_COUNTER_SYMBOL, nullptr);

    case eCSSCounterDesc_Range: {
      if (ParseVariant(aValue, VARIANT_AUTO, nullptr)) {
        return true;
      }
      nsCSSValuePairList* item = aValue.SetPairListValue();
      for (;;) {
        nsCSSValuePair pair;
        if (!ParseCounterRange(pair)) {
          return false;
        }
        item->mXValue = pair.mXValue;
        item->mYValue = pair.mYValue;
        if (!ExpectSymbol(',', true)) {
          return true;
        }
        item->mNext = new nsCSSValuePairList;
        item = item->mNext;
      }
      
    }

    case eCSSCounterDesc_Pad: {
      nsCSSValue width, symbol;
      bool hasWidth = ParseNonNegativeInteger(width);
      if (!ParseVariant(symbol, VARIANT_COUNTER_SYMBOL, nullptr) ||
          (!hasWidth && !ParseNonNegativeInteger(width))) {
        return false;
      }
      aValue.SetPairValue(width, symbol);
      return true;
    }

    case eCSSCounterDesc_Fallback:
      return ParseCounterStyleNameValue(aValue);

    case eCSSCounterDesc_Symbols: {
      nsCSSValueList* item = nullptr;
      for (;;) {
        nsCSSValue value;
        if (!ParseVariant(value, VARIANT_COUNTER_SYMBOL, nullptr)) {
          return !!item;
        }
        if (!item) {
          item = aValue.SetListValue();
        } else {
          item->mNext = new nsCSSValueList;
          item = item->mNext;
        }
        item->mValue = value;
      }
      
    }

    case eCSSCounterDesc_AdditiveSymbols: {
      nsCSSValuePairList* item = nullptr;
      int32_t lastWeight = -1;
      for (;;) {
        nsCSSValue weight, symbol;
        bool hasWeight = ParseNonNegativeInteger(weight);
        if (!ParseVariant(symbol, VARIANT_COUNTER_SYMBOL, nullptr) ||
            (!hasWeight && !ParseNonNegativeInteger(weight))) {
          return false;
        }
        if (lastWeight != -1 && weight.GetIntValue() >= lastWeight) {
          REPORT_UNEXPECTED(PECounterASWeight);
          return false;
        }
        lastWeight = weight.GetIntValue();
        if (!item) {
          item = aValue.SetPairListValue();
        } else {
          item->mNext = new nsCSSValuePairList;
          item = item->mNext;
        }
        item->mXValue = weight;
        item->mYValue = symbol;
        if (!ExpectSymbol(',', true)) {
          return true;
        }
      }
      
    }

    case eCSSCounterDesc_SpeakAs:
      if (ParseVariant(aValue, VARIANT_AUTO | VARIANT_KEYWORD,
                      nsCSSProps::kCounterSpeakAsKTable)) {
        if (aValue.GetUnit() == eCSSUnit_Enumerated &&
            aValue.GetIntValue() == NS_STYLE_COUNTER_SPEAKAS_SPELL_OUT) {
          
          
          
          return false;
        }
        return true;
      }
      return ParseCounterStyleNameValue(aValue);

    default:
      NS_NOTREACHED("unknown descriptor");
      return false;
  }
}

bool
CSSParserImpl::ParseCounterRange(nsCSSValuePair& aPair)
{
  static const int32_t VARIANT_BOUND = VARIANT_INTEGER | VARIANT_KEYWORD;
  nsCSSValue lower, upper;
  if (!ParseVariant(lower, VARIANT_BOUND, nsCSSProps::kCounterRangeKTable) ||
      !ParseVariant(upper, VARIANT_BOUND, nsCSSProps::kCounterRangeKTable)) {
    return false;
  }
  if (lower.GetUnit() != eCSSUnit_Enumerated &&
      upper.GetUnit() != eCSSUnit_Enumerated &&
      lower.GetIntValue() > upper.GetIntValue()) {
    return false;
  }
  aPair = nsCSSValuePair(lower, upper);
  return true;
}

bool
CSSParserImpl::SkipUntil(char16_t aStopSymbol)
{
  nsCSSToken* tk = &mToken;
  nsAutoTArray<char16_t, 16> stack;
  stack.AppendElement(aStopSymbol);
  for (;;) {
    if (!GetToken(true)) {
      return false;
    }
    if (eCSSToken_Symbol == tk->mType) {
      char16_t symbol = tk->mSymbol;
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

bool
CSSParserImpl::SkipBalancedContentUntil(char16_t aStopSymbol)
{
  nsCSSToken* tk = &mToken;
  nsAutoTArray<char16_t, 16> stack;
  stack.AppendElement(aStopSymbol);
  for (;;) {
    if (!GetToken(true)) {
      return true;
    }
    if (eCSSToken_Symbol == tk->mType) {
      char16_t symbol = tk->mSymbol;
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
      } else if (')' == symbol ||
                 ']' == symbol ||
                 '}' == symbol) {
        UngetToken();
        return false;
      }
    } else if (eCSSToken_Function == tk->mType ||
               eCSSToken_Bad_URL == tk->mType) {
      stack.AppendElement(')');
    }
  }
}

void
CSSParserImpl::SkipUntilOneOf(const char16_t* aStopSymbolChars)
{
  nsCSSToken* tk = &mToken;
  nsDependentString stopSymbolChars(aStopSymbolChars);
  for (;;) {
    if (!GetToken(true)) {
      break;
    }
    if (eCSSToken_Symbol == tk->mType) {
      char16_t symbol = tk->mSymbol;
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

void
CSSParserImpl::SkipUntilAllOf(const StopSymbolCharStack& aStopSymbolChars)
{
  uint32_t i = aStopSymbolChars.Length();
  while (i--) {
    SkipUntil(aStopSymbolChars[i]);
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
      char16_t symbol = tk->mSymbol;
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
      char16_t symbol = tk->mSymbol;
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
  uint32_t linenum, colnum;
  if (!GetNextTokenLocation(true, &linenum, &colnum) ||
      !ParseSelectorList(slist, char16_t('{'))) {
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

  

  nsRefPtr<css::StyleRule> rule = new css::StyleRule(slist, declaration,
                                                     linenum, colnum);
  (*aAppendFunc)(rule, aData);

  return true;
}

bool
CSSParserImpl::ParseSelectorList(nsCSSSelectorList*& aListHead,
                                 char16_t aStopChar)
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
      if (aStopChar == char16_t(0)) {
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
      } else if (aStopChar == tk->mSymbol && aStopChar != char16_t(0)) {
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
  char16_t combinator = 0;
  nsAutoPtr<nsCSSSelectorList> list(new nsCSSSelectorList());

  for (;;) {
    if (!ParseSelector(list, combinator)) {
      return false;
    }

    
    if (!GetToken(false)) {
      break; 
    }

    combinator = char16_t(0);
    if (mToken.mType == eCSSToken_Whitespace) {
      if (!GetToken(true)) {
        break; 
      }
      combinator = char16_t(' ');
    }

    if (mToken.mType != eCSSToken_Symbol) {
      UngetToken(); 
    } else {
      char16_t symbol = mToken.mSymbol;
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

  bool gotEOF = false;
  if (! GetToken(true)) { 
    
    
    REPORT_UNEXPECTED_EOF(PEAttSelInnerEOF);
    gotEOF = true;
  }
  if (gotEOF ||
      (eCSSToken_Symbol == mToken.mType) ||
      (eCSSToken_Includes == mToken.mType) ||
      (eCSSToken_Dashmatch == mToken.mType) ||
      (eCSSToken_Beginsmatch == mToken.mType) ||
      (eCSSToken_Endsmatch == mToken.mType) ||
      (eCSSToken_Containsmatch == mToken.mType)) {
    uint8_t func;
    
    
    if (gotEOF || ']' == mToken.mSymbol) {
      aDataMask |= SEL_MASK_ATTRIB;
      aSelector.AddAttribute(nameSpaceID, attr);
      func = NS_ATTR_FUNC_SET;
    }
    else if (eCSSToken_Includes == mToken.mType) {
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
        bool gotClosingBracket;
        if (! GetToken(true)) { 
          
          REPORT_UNEXPECTED_EOF(PEAttSelCloseEOF);
          gotClosingBracket = true;
        } else {
          gotClosingBracket = mToken.IsSymbol(']');
        }
        if (gotClosingBracket) {
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
  buffer.Append(char16_t(':'));
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
  bool pseudoClassIsUserAction =
    nsCSSPseudoClasses::IsUserActionPseudoClass(pseudoClassType);

  if (!mUnsafeRulesEnabled &&
      ((pseudoElementType < nsCSSPseudoElements::ePseudo_PseudoElementCount &&
        nsCSSPseudoElements::PseudoElementIsUASheetOnly(pseudoElementType)) ||
       (pseudoClassType != nsCSSPseudoClasses::ePseudoClass_NotPseudoClass &&
        nsCSSPseudoClasses::PseudoClassIsUASheetOnly(pseudoClassType)))) {
    
    REPORT_UNEXPECTED_TOKEN(PEPseudoSelUnknown);
    UngetToken();
    return eSelectorParsingStatus_Error;
  }

  
  
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
    if (aSelector.IsPseudoElement()) {
      nsCSSPseudoElements::Type type = aSelector.PseudoType();
      if (!nsCSSPseudoElements::PseudoElementSupportsUserActionState(type)) {
        
        REPORT_UNEXPECTED_TOKEN(PEPseudoSelNoUserActionPC);
        UngetToken();
        return eSelectorParsingStatus_Error;
      }
      if (!pseudoClassIsUserAction) {
        
        
        REPORT_UNEXPECTED_TOKEN(PEPseudoClassNotUserAction);
        UngetToken();
        return eSelectorParsingStatus_Error;
      }
    }
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
        MOZ_ASSERT(nsCSSPseudoClasses::HasSelectorListArg(pseudoClassType),
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
      if (parsingPseudoElement && mToken.IsSymbol(':')) {
        UngetToken();
        return eSelectorParsingStatus_Continue;
      }
      if ((mToken.IsSymbol('{') || mToken.IsSymbol(','))) {
        UngetToken();
        return eSelectorParsingStatus_Done;
      }
      REPORT_UNEXPECTED_TOKEN(PEPseudoSelEndOrUserActionPC);
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
  int32_t sign[2] = { 1, 1 };
  bool hasSign[2] = { false, false };
  bool lookForB = true;

  
  

  if (! GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
    return eSelectorParsingStatus_Error;
  }

  if (mToken.IsSymbol('+') || mToken.IsSymbol('-')) {
    hasSign[0] = true;
    if (mToken.IsSymbol('-')) {
      sign[0] = -1;
    }
    if (! GetToken(false)) {
      REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
      return eSelectorParsingStatus_Error;
    }
  }

  if (eCSSToken_Ident == mToken.mType || eCSSToken_Dimension == mToken.mType) {
    
    
    
    
    
    uint32_t truncAt = 0;
    if (StringBeginsWith(mToken.mIdent, NS_LITERAL_STRING("n-"))) {
      truncAt = 1;
    } else if (StringBeginsWith(mToken.mIdent, NS_LITERAL_STRING("-n-")) && !hasSign[0]) {
      truncAt = 2;
    }
    if (truncAt != 0) {
      mScanner->Backup(mToken.mIdent.Length() - truncAt);
      mToken.mIdent.Truncate(truncAt);
    }
  }

  if (eCSSToken_Ident == mToken.mType) {
    if (mToken.mIdent.LowerCaseEqualsLiteral("odd") && !hasSign[0]) {
      numbers[0] = 2;
      numbers[1] = 1;
      lookForB = false;
    }
    else if (mToken.mIdent.LowerCaseEqualsLiteral("even") && !hasSign[0]) {
      numbers[0] = 2;
      numbers[1] = 0;
      lookForB = false;
    }
    else if (mToken.mIdent.LowerCaseEqualsLiteral("n")) {
      numbers[0] = sign[0];
    }
    else if (mToken.mIdent.LowerCaseEqualsLiteral("-n") && !hasSign[0]) {
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
    
    if (mToken.mHasSign && hasSign[0]) {
      REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
      return eSelectorParsingStatus_Error; 
    }
    int32_t intValue = mToken.mInteger * sign[0];
    
    if (! GetToken(false)) {
      numbers[1] = intValue;
      lookForB = false;
    }
    else {
      if (eCSSToken_Ident == mToken.mType && mToken.mIdent.LowerCaseEqualsLiteral("n")) {
        numbers[0] = intValue;
      }
      else if (eCSSToken_Ident == mToken.mType && StringBeginsWith(mToken.mIdent, NS_LITERAL_STRING("n-"))) {
        numbers[0] = intValue;
        mScanner->Backup(mToken.mIdent.Length() - 1);
      }
      else {
        UngetToken();
        numbers[1] = intValue;
        lookForB = false;
      }
    }
  }
  else if (eCSSToken_Dimension == mToken.mType) {
    if (!mToken.mIntegerValid || !mToken.mIdent.LowerCaseEqualsLiteral("n")) {
      REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
      return eSelectorParsingStatus_Error; 
    }
    
    if ( mToken.mHasSign && hasSign[0] ) {
      REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
      return eSelectorParsingStatus_Error; 
    }
    numbers[0] = mToken.mInteger * sign[0];
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
    
    
    
    if (mToken.IsSymbol('+') || mToken.IsSymbol('-')) {
      hasSign[1] = true;
      if (mToken.IsSymbol('-')) {
        sign[1] = -1;
      }
      if (! GetToken(true)) {
        REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
        return eSelectorParsingStatus_Error;
      }
    }
    if (eCSSToken_Number != mToken.mType ||
        !mToken.mIntegerValid || mToken.mHasSign == hasSign[1]) {
      REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
      UngetToken();
      return eSelectorParsingStatus_Error; 
    }
    numbers[1] = mToken.mInteger * sign[1];
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
  if (! ParseSelectorList(*getter_Transfers(slist), char16_t(')'))) {
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
                             char16_t aPrevCombinator)
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
      if (pseudoElement &&
          pseudoElementType != nsCSSPseudoElements::ePseudo_AnonBox) {
        
        

        aList->mWeight += selector->CalcWeight();

        selector = aList->AddSelector(':');

        selector->mLowercaseTag.swap(pseudoElement);
        selector->mClassList = pseudoElementArgs.forget();
        selector->SetPseudoType(pseudoElementType);
      }
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

  return true;
}

css::Declaration*
CSSParserImpl::ParseDeclarationBlock(uint32_t aFlags, nsCSSContextType aContext)
{
  bool checkForBraces = (aFlags & eParseDeclaration_InBraces) != 0;

  MOZ_ASSERT(!mDidUnprefixWebkitBoxInEarlierDecl,
             "Someone forgot to clear the 'did unprefix webkit-box' flag");
  AutoRestore<bool> restorer(mDidUnprefixWebkitBoxInEarlierDecl);

  if (checkForBraces) {
    if (!ExpectSymbol('{', true)) {
      REPORT_UNEXPECTED_TOKEN(PEBadDeclBlockStart);
      OUTPUT_ERROR();
      return nullptr;
    }
  }
  css::Declaration* declaration = new css::Declaration();
  mData.AssertInitialState();
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
  return declaration;
}

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
        MOZ_ASSERT(tk->mIdent.Length() == 3 || tk->mIdent.Length() == 6,
                   "unexpected hex color length");
        nsCSSUnit unit = tk->mIdent.Length() == 3 ?
                           eCSSUnit_ShortHexColor :
                           eCSSUnit_HexColor;
        aValue.SetIntegerColorValue(rgba, unit);
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
        
        if (GetToken(true)) {
          UngetToken();
        }
        if (mToken.mType == eCSSToken_Number) {
          uint8_t r, g, b;
          if (ParseNumberColorComponent(r, ',') &&
              ParseNumberColorComponent(g, ',') &&
              ParseNumberColorComponent(b, ')')) {
            aValue.SetIntegerColorValue(NS_RGB(r, g, b), eCSSUnit_RGBColor);
            return true;
          }
        } else {
          float r, g, b;
          if (ParsePercentageColorComponent(r, ',') &&
              ParsePercentageColorComponent(g, ',') &&
              ParsePercentageColorComponent(b, ')')) {
            aValue.SetFloatColorValue(r, g, b, 1.0f,
                                      eCSSUnit_PercentageRGBColor);
            return true;
          }
        }
        SkipUntil(')');
        return false;
      }
      else if (mToken.mIdent.LowerCaseEqualsLiteral("rgba")) {
        
        if (GetToken(true)) {
          UngetToken();
        }
        if (mToken.mType == eCSSToken_Number) {
          uint8_t r, g, b, a;
          if (ParseNumberColorComponent(r, ',') &&
              ParseNumberColorComponent(g, ',') &&
              ParseNumberColorComponent(b, ',') &&
              ParseColorOpacity(a)) {
            aValue.SetIntegerColorValue(NS_RGBA(r, g, b, a),
                                        eCSSUnit_RGBAColor);
            return true;
          }
        } else {
          float r, g, b, a;
          if (ParsePercentageColorComponent(r, ',') &&
              ParsePercentageColorComponent(g, ',') &&
              ParsePercentageColorComponent(b, ',') &&
              ParseColorOpacity(a)) {
            aValue.SetFloatColorValue(r, g, b, a, eCSSUnit_PercentageRGBAColor);
            return true;
          }
        }
        SkipUntil(')');
        return false;
      }
      else if (mToken.mIdent.LowerCaseEqualsLiteral("hsl")) {
        
        
        float h, s, l;
        if (ParseHSLColor(h, s, l, ')')) {
          aValue.SetFloatColorValue(h, s, l, 1.0f, eCSSUnit_HSLColor);
          return true;
        }
        SkipUntil(')');
        return false;
      }
      else if (mToken.mIdent.LowerCaseEqualsLiteral("hsla")) {
        
        
        
        float h, s, l, a;
        if (ParseHSLColor(h, s, l, ',') &&
            ParseColorOpacity(a)) {
          aValue.SetFloatColorValue(h, s, l, a, eCSSUnit_HSLAColor);
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
      aValue.SetIntegerColorValue(rgba, eCSSUnit_HexColor);
      return true;
    }
  }

  
  REPORT_UNEXPECTED_TOKEN(PEColorNotColor);
  UngetToken();
  return false;
}

bool
CSSParserImpl::ParseNumberColorComponent(uint8_t& aComponent, char aStop)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEColorComponentEOF);
    return false;
  }

  if (mToken.mType != eCSSToken_Number || !mToken.mIntegerValid) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedInt);
    UngetToken();
    return false;
  }

  float value = mToken.mNumber;
  if (value < 0.0f) value = 0.0f;
  if (value > 255.0f) value = 255.0f;

  if (ExpectSymbol(aStop, true)) {
    aComponent = NSToIntRound(value);
    return true;
  }
  REPORT_UNEXPECTED_TOKEN_CHAR(PEColorComponentBadTerm, aStop);
  return false;
}

bool
CSSParserImpl::ParsePercentageColorComponent(float& aComponent, char aStop)
{
  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEColorComponentEOF);
    return false;
  }

  if (mToken.mType != eCSSToken_Percentage) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedPercent);
    UngetToken();
    return false;
  }

  float value = mToken.mNumber;
  if (value < 0.0f) value = 0.0f;
  if (value > 1.0f) value = 1.0f;

  if (ExpectSymbol(aStop, true)) {
    aComponent = value;
    return true;
  }
  REPORT_UNEXPECTED_TOKEN_CHAR(PEColorComponentBadTerm, aStop);
  return false;
}


bool
CSSParserImpl::ParseHSLColor(float& aHue, float& aSaturation, float& aLightness,
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
    aHue = h;
    aSaturation = s;
    aLightness = l;
    return true;
  }

  REPORT_UNEXPECTED_TOKEN_CHAR(PEColorComponentBadTerm, aStop);
  return false;
}


bool
CSSParserImpl::ParseColorOpacity(uint8_t& aOpacity)
{
  float floatOpacity;
  if (!ParseColorOpacity(floatOpacity)) {
    return false;
  }

  uint8_t value = nsStyleUtil::FloatToColorComponent(floatOpacity);
  
  
  NS_ASSERTION(fabs(255.0f*mToken.mNumber - value) <= 0.51f,
               "FloatToColorComponent did something weird");

  aOpacity = value;
  return true;
}

bool
CSSParserImpl::ParseColorOpacity(float& aOpacity)
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

  if (!ExpectSymbol(')', true)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedCloseParen);
    return false;
  }

  if (mToken.mNumber < 0.0f) {
    mToken.mNumber = 0.0f;
  } else if (mToken.mNumber > 1.0f) {
    mToken.mNumber = 1.0f;
  }

  aOpacity = mToken.mNumber;
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

nsCSSKeyword
CSSParserImpl::LookupKeywordPrefixAware(nsAString& aKeywordStr,
                                        const KTableValue aKeywordTable[])
{
  nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(aKeywordStr);

  if (aKeywordTable == nsCSSProps::kDisplayKTable) {
    if (keyword == eCSSKeyword_UNKNOWN &&
        ShouldUseUnprefixingService() &&
        aKeywordStr.EqualsLiteral("-webkit-box")) {
      
      
      
      mDidUnprefixWebkitBoxInEarlierDecl = true;
      return eCSSKeyword_flex;
    }

    
    
    
    
    
    
    
    if (mDidUnprefixWebkitBoxInEarlierDecl && keyword == eCSSKeyword__moz_box) {
      MOZ_ASSERT(ShouldUseUnprefixingService(),
                 "mDidUnprefixWebkitBoxInEarlierDecl should only be set if "
                 "we're using the unprefixing service on this site");
      return eCSSKeyword_flex;
    }
  }

  return keyword;
}

bool
CSSParserImpl::ShouldUseUnprefixingService()
{
  if (!sUnprefixingServiceEnabled) {
    
    return false;
  }

  
  return mSheetPrincipal && mSheetPrincipal->IsOnCSSUnprefixingWhitelist();
}

bool
CSSParserImpl::ParsePropertyWithUnprefixingService(
  const nsAString& aPropertyName,
  css::Declaration* aDeclaration,
  uint32_t aFlags,
  bool aMustCallValueAppended,
  bool* aChanged,
  nsCSSContextType aContext)
{
  MOZ_ASSERT(ShouldUseUnprefixingService(),
             "Caller should've checked ShouldUseUnprefixingService()");

  nsCOMPtr<nsICSSUnprefixingService> unprefixingSvc =
    do_GetService(NS_CSSUNPREFIXINGSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(unprefixingSvc, false);

  
  
  nsAutoCSSParserInputStateRestorer parserStateBeforeTryingToUnprefix(this);

  
  
  
  
  
  bool checkForBraces = (aFlags & eParseDeclaration_InBraces) != 0;
  nsAutoString rightHalfOfDecl;
  mScanner->StartRecording();
  SkipDeclaration(checkForBraces);
  mScanner->StopRecording(rightHalfOfDecl);

  
  bool success;
  nsAutoString unprefixedDecl;
  nsresult rv =
    unprefixingSvc->GenerateUnprefixedDeclaration(aPropertyName,
                                                  rightHalfOfDecl,
                                                  unprefixedDecl, &success);
  if (NS_FAILED(rv) || !success) {
    return false;
  }

  
  nsAutoScannerChanger scannerChanger(this, unprefixedDecl);
  success = ParseDeclaration(aDeclaration,
                             aFlags | eParseDeclaration_FromUnprefixingSvc,
                             aMustCallValueAppended, aChanged, aContext);
  if (success) {
    
    
    parserStateBeforeTryingToUnprefix.DoNotRestore();
  }

  return success;
}



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

      if (eCSSToken_AtKeyword == tk->mType) {
        SkipAtRule(checkForBraces);
        return true;  
      }
    }
    
    UngetToken();
    return false;
  }

  
  
  nsAutoSuppressErrors suppressErrors(this, mInFailingSupportsRule);

  
  nsCSSProperty propID;

  
  CSSVariableDeclarations::Type variableType;
  nsString variableValue;

  
  bool customProperty = nsLayoutUtils::CSSVariablesEnabled() &&
                        nsCSSProps::IsCustomPropertyName(propertyName) &&
                        aContext == eCSSContext_General;

  if (customProperty) {
    if (!ParseVariableDeclaration(&variableType, variableValue)) {
      REPORT_UNEXPECTED_P(PEValueParsingError, propertyName);
      REPORT_UNEXPECTED(PEDeclDropped);
      OUTPUT_ERROR();
      return false;
    }
  } else {
    
    propID = LookupEnabledProperty(propertyName);
    if (eCSSProperty_UNKNOWN == propID ||
        eCSSPropertyExtra_variable == propID ||
        (aContext == eCSSContext_Page &&
         !nsCSSProps::PropHasFlags(propID,
                                   CSS_PROPERTY_APPLIES_TO_PAGE_RULE))) { 
      if (NonMozillaVendorIdentifier(propertyName)) {
        if (!mInSupportsCondition &&
            aContext == eCSSContext_General &&
            !(aFlags & eParseDeclaration_FromUnprefixingSvc) && 
            ShouldUseUnprefixingService()) {
          if (ParsePropertyWithUnprefixingService(propertyName,
                                                  aDeclaration, aFlags,
                                                  aMustCallValueAppended,
                                                  aChanged, aContext)) {
            return true;
          }
        }
      } else {
        REPORT_UNEXPECTED_P(PEUnknownProperty, propertyName);
        REPORT_UNEXPECTED(PEDeclDropped);
        OUTPUT_ERROR();
      }
      return false;
    }
    
    if (!ParseProperty(propID)) {
      
      REPORT_UNEXPECTED_P(PEValueParsingError, propertyName);
      REPORT_UNEXPECTED(PEDeclDropped);
      OUTPUT_ERROR();
      mTempData.ClearProperty(propID);
      mTempData.AssertInitialState();
      return false;
    }
  }

  CLEAR_ERROR();

  
  PriorityParsingStatus status;
  if ((aFlags & eParseDeclaration_AllowImportant) != 0) {
    status = ParsePriority();
  } else {
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
    if (!customProperty) {
      mTempData.ClearProperty(propID);
    }
    mTempData.AssertInitialState();
    return false;
  }

  if (customProperty) {
    MOZ_ASSERT(Substring(propertyName, 0,
                         CSS_CUSTOM_NAME_PREFIX_LENGTH).EqualsLiteral("--"));
    
    nsDependentString varName(propertyName, CSS_CUSTOM_NAME_PREFIX_LENGTH);
    aDeclaration->AddVariableDeclaration(varName, variableType, variableValue,
                                         status == ePriority_Important, false);
  } else {
    *aChanged |= mData.TransferFromBlock(mTempData, propID,
                                         PropertyEnabledState(),
                                         status == ePriority_Important,
                                         false, aMustCallValueAppended,
                                         aDeclaration);
  }

  return true;
}

static const nsCSSProperty kBorderTopIDs[] = {
  eCSSProperty_border_top_width,
  eCSSProperty_border_top_style,
  eCSSProperty_border_top_color
};
static const nsCSSProperty kBorderRightIDs[] = {
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
  eCSSProperty_border_left_width,
  eCSSProperty_border_left_style,
  eCSSProperty_border_left_color
};
static const nsCSSProperty kBorderStartIDs[] = {
  eCSSProperty_border_start_width,
  eCSSProperty_border_start_style,
  eCSSProperty_border_start_color
};
static const nsCSSProperty kBorderEndIDs[] = {
  eCSSProperty_border_end_width,
  eCSSProperty_border_end_style,
  eCSSProperty_border_end_color
};
static const nsCSSProperty kBorderBlockStartIDs[] = {
  eCSSProperty_border_block_start_width,
  eCSSProperty_border_block_start_style,
  eCSSProperty_border_block_start_color
};
static const nsCSSProperty kBorderBlockEndIDs[] = {
  eCSSProperty_border_block_end_width,
  eCSSProperty_border_block_end_style,
  eCSSProperty_border_block_end_color
};
static const nsCSSProperty kColumnRuleIDs[] = {
  eCSSProperty__moz_column_rule_width,
  eCSSProperty__moz_column_rule_style,
  eCSSProperty__moz_column_rule_color
};

bool
CSSParserImpl::ParseEnum(nsCSSValue& aValue,
                         const KTableValue aKeywordTable[])
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

    if (i == ArrayLength(UnitData)) {
      
      return false;
    }

    if (!mViewportUnitsEnabled &&
        (eCSSUnit_ViewportWidth == units  ||
         eCSSUnit_ViewportHeight == units ||
         eCSSUnit_ViewportMin == units    ||
         eCSSUnit_ViewportMax == units)) {
      
      
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
  VARIANT_CALC | \
  VARIANT_OPENTYPE_SVG_KEYWORD





bool
CSSParserImpl::ParseNonNegativeVariant(nsCSSValue& aValue,
                                       int32_t aVariantMask,
                                       const KTableValue aKeywordTable[])
{
  
  
  MOZ_ASSERT((aVariantMask & ~(VARIANT_ALL_NONNUMERIC |
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
                                       const KTableValue aKeywordTable[])
{
  
  
  MOZ_ASSERT((aVariantMask & ~(VARIANT_ALL_NONNUMERIC |
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
                            const KTableValue aKeywordTable[])
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
  MOZ_ASSERT(!(aVariantMask & VARIANT_IDENTIFIER) ||
             !(aVariantMask & VARIANT_IDENTIFIER_NO_INHERIT),
             "must not set both VARIANT_IDENTIFIER and "
             "VARIANT_IDENTIFIER_NO_INHERIT");

  if (!GetToken(true)) {
    return false;
  }
  nsCSSToken* tk = &mToken;
  if (((aVariantMask & (VARIANT_AHK | VARIANT_NORMAL | VARIANT_NONE | VARIANT_ALL)) != 0) &&
      (eCSSToken_Ident == tk->mType)) {
    nsCSSKeyword keyword = LookupKeywordPrefixAware(tk->mIdent,
                                                    aKeywordTable);

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
        else if (eCSSKeyword_initial == keyword) {
          aValue.SetInitialValue();
          return true;
        }
        else if (eCSSKeyword_unset == keyword &&
                 nsLayoutUtils::UnsetValueEnabled()) {
          aValue.SetUnsetValue();
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
      if ((aVariantMask & VARIANT_OPENTYPE_SVG_KEYWORD) != 0) {
        if (sOpentypeSVGEnabled) {
          aVariantMask |= VARIANT_KEYWORD;
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
         tk->mIdent.LowerCaseEqualsLiteral("initial") ||
         (tk->mIdent.LowerCaseEqualsLiteral("unset") &&
          nsLayoutUtils::UnsetValueEnabled())))) {
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
CSSParserImpl::ParseCustomIdent(nsCSSValue& aValue,
                                const nsAutoString& aIdentValue,
                                const nsCSSKeyword aExcludedKeywords[],
                                const nsCSSProps::KTableValue aPropertyKTable[])
{
  nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(aIdentValue);
  if (keyword == eCSSKeyword_UNKNOWN) {
    
    aValue.SetStringValue(mToken.mIdent, eCSSUnit_Ident);
    return true;
  }
  if (keyword == eCSSKeyword_inherit ||
      keyword == eCSSKeyword_initial ||
      keyword == eCSSKeyword_unset ||
      keyword == eCSSKeyword_default ||
      (aPropertyKTable &&
        nsCSSProps::FindIndexOfKeyword(keyword, aPropertyKTable) >= 0)) {
    return false;
  }
  if (aExcludedKeywords) {
    for (uint32_t i = 0;; i++) {
      nsCSSKeyword excludedKeyword = aExcludedKeywords[i];
      if (excludedKeyword == eCSSKeyword_UNKNOWN) {
        break;
      }
      if (excludedKeyword == keyword) {
        return false;
      }
    }
  }
  aValue.SetStringValue(mToken.mIdent, eCSSUnit_Ident);
  return true;
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

    
    int32_t typeItem = eCSSUnit_Counters == unit ? 2 : 1;
    nsCSSValue& type = val->Item(typeItem);
    if (ExpectSymbol(',', true)) {
      if (!ParseCounterStyleNameValue(type) && !ParseSymbols(type)) {
        break;
      }
    } else {
      type.SetStringValue(NS_LITERAL_STRING("decimal"), eCSSUnit_Ident);
    }

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
      attr.Append(char16_t('|'));
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
CSSParserImpl::ParseSymbols(nsCSSValue& aValue)
{
  if (!GetToken(true)) {
    return false;
  }
  if (mToken.mType != eCSSToken_Function &&
      !mToken.mIdent.LowerCaseEqualsLiteral("symbols")) {
    UngetToken();
    return false;
  }

  nsRefPtr<nsCSSValue::Array> params = nsCSSValue::Array::Create(2);
  nsCSSValue& type = params->Item(0);
  nsCSSValue& symbols = params->Item(1);

  if (!ParseEnum(type, nsCSSProps::kCounterSymbolsSystemKTable)) {
    type.SetIntValue(NS_STYLE_COUNTER_SYSTEM_SYMBOLIC, eCSSUnit_Enumerated);
  }

  bool first = true;
  nsCSSValueList* item = symbols.SetListValue();
  for (;;) {
    
    if (!ParseVariant(item->mValue, VARIANT_STRING, nullptr)) {
      break;
    }
    if (ExpectSymbol(')', true)) {
      if (first) {
        switch (type.GetIntValue()) {
          case NS_STYLE_COUNTER_SYSTEM_NUMERIC:
          case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
            
            return false;
        }
      }
      aValue.SetArrayValue(params, eCSSUnit_Symbols);
      return true;
    }
    item->mNext = new nsCSSValueList;
    item = item->mNext;
    first = false;
  }

  SkipUntil(')');
  return false;
}

bool
CSSParserImpl::SetValueToURL(nsCSSValue& aValue, const nsString& aURL)
{
  if (!mSheetPrincipal) {
    if (!mSheetPrincipalRequired) {
      
      return true;
    }

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
CSSParserImpl::ParseImageOrientation(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_INHERIT, nullptr)) {
    
    return true;
  }

  
  nsCSSValue angle;
  if (ParseVariant(angle, VARIANT_ANGLE, nullptr)) {
    nsCSSValue flip;

    if (ParseVariant(flip, VARIANT_KEYWORD, nsCSSProps::kImageOrientationFlipKTable)) {
      nsRefPtr<nsCSSValue::Array> array = nsCSSValue::Array::Create(2);
      array->Item(0) = angle;
      array->Item(1) = flip;
      aValue.SetArrayValue(array, eCSSUnit_Array);
    } else {
      aValue = angle;
    }
    
    return true;
  }

  
  
  nsCSSValue keyword;
  if (ParseVariant(keyword, VARIANT_KEYWORD, nsCSSProps::kImageOrientationKTable)) {
    aValue = keyword;
    return true;
  }

  
  return false;
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


bool
CSSParserImpl::ParseFlexFlow()
{
  static const nsCSSProperty kFlexFlowSubprops[] = {
    eCSSProperty_flex_direction,
    eCSSProperty_flex_wrap
  };
  const size_t numProps = MOZ_ARRAY_LENGTH(kFlexFlowSubprops);
  nsCSSValue values[numProps];

  int32_t found = ParseChoice(values, kFlexFlowSubprops, numProps);

  
  if (found < 1) {
    return false;
  }

  
  if ((found & 1) == 0) {
    values[0].SetIntValue(NS_STYLE_FLEX_DIRECTION_ROW, eCSSUnit_Enumerated);
  }
  if ((found & 2) == 0) {
    values[1].SetIntValue(NS_STYLE_FLEX_WRAP_NOWRAP, eCSSUnit_Enumerated);
  }

  
  for (size_t i = 0; i < numProps; i++) {
    AppendValue(kFlexFlowSubprops[i], values[i]);
  }
  return true;
}

bool
CSSParserImpl::ParseGridAutoFlow()
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    AppendValue(eCSSProperty_grid_auto_flow, value);
    return true;
  }

  static const int32_t mask[] = {
    NS_STYLE_GRID_AUTO_FLOW_ROW | NS_STYLE_GRID_AUTO_FLOW_COLUMN,
    MASK_END_VALUE
  };
  if (!ParseBitmaskValues(value, nsCSSProps::kGridAutoFlowKTable, mask)) {
    return false;
  }
  int32_t bitField = value.GetIntValue();

  
  if (!(bitField & (NS_STYLE_GRID_AUTO_FLOW_ROW |
                    NS_STYLE_GRID_AUTO_FLOW_COLUMN))) {
    value.SetIntValue(bitField | NS_STYLE_GRID_AUTO_FLOW_ROW,
                      eCSSUnit_Enumerated);
  }

  AppendValue(eCSSProperty_grid_auto_flow, value);
  return true;
}

CSSParseResult
CSSParserImpl::ParseGridLineNames(nsCSSValue& aValue)
{
  if (!ExpectSymbol('(', true)) {
    return CSSParseResult::NotFound;
  }
  if (!GetToken(true) || mToken.IsSymbol(')')) {
    return CSSParseResult::Ok;
  }
  

  nsCSSValueList* item;
  if (aValue.GetUnit() == eCSSUnit_List) {
    
    

    
    
    
    item = aValue.GetListValue();
    while (item->mNext) {
      item = item->mNext;
    }
    item->mNext = new nsCSSValueList;
    item = item->mNext;
  } else {
    MOZ_ASSERT(aValue.GetUnit() == eCSSUnit_Null, "Unexpected unit");
    item = aValue.SetListValue();
  }
  for (;;) {
    if (!(eCSSToken_Ident == mToken.mType &&
          ParseCustomIdent(item->mValue, mToken.mIdent))) {
      UngetToken();
      SkipUntil(')');
      return CSSParseResult::Error;
    }
    if (!GetToken(true) || mToken.IsSymbol(')')) {
      return CSSParseResult::Ok;
    }
    item->mNext = new nsCSSValueList;
    item = item->mNext;
  }
}





bool
CSSParserImpl::ParseGridLineNameListRepeat(nsCSSValueList** aTailPtr)
{
  if (!(GetToken(true) &&
        mToken.mType == eCSSToken_Number &&
        mToken.mIntegerValid &&
        mToken.mInteger > 0)) {
    SkipUntil(')');
    return false;
  }
  int32_t repetitions = std::min(mToken.mInteger,
                                 GRID_TEMPLATE_MAX_REPETITIONS);
  if (!ExpectSymbol(',', true)) {
    SkipUntil(')');
    return false;
  }

  
  nsCSSValueList* tail = *aTailPtr;
  do {
    tail->mNext = new nsCSSValueList;
    tail = tail->mNext;
    if (ParseGridLineNames(tail->mValue) != CSSParseResult::Ok) {
      SkipUntil(')');
      return false;
    }
  } while (!ExpectSymbol(')', true));
  nsCSSValueList* firstRepeatedItem = (*aTailPtr)->mNext;
  nsCSSValueList* lastRepeatedItem = tail;

  
  
  MOZ_ASSERT(repetitions > 0, "Expected positive repetitions");
  while (--repetitions) {
    nsCSSValueList* repeatedItem = firstRepeatedItem;
    for (;;) {
      tail->mNext = new nsCSSValueList;
      tail = tail->mNext;
      tail->mValue = repeatedItem->mValue;
      if (repeatedItem == lastRepeatedItem) {
        break;
      }
      repeatedItem = repeatedItem->mNext;
    }
  }
  *aTailPtr = tail;
  return true;
}


bool
CSSParserImpl::ParseOptionalLineNameListAfterSubgrid(nsCSSValue& aValue)
{
  nsCSSValueList* item = aValue.SetListValue();
  
  item->mValue.SetIntValue(NS_STYLE_GRID_TEMPLATE_SUBGRID,
                           eCSSUnit_Enumerated);
  for (;;) {
    
    if (!GetToken(true)) {
      return true;
    }
    if (mToken.mType == eCSSToken_Function &&
        mToken.mIdent.LowerCaseEqualsLiteral("repeat")) {
      if (!ParseGridLineNameListRepeat(&item)) {
        return false;
      }
    } else {
      UngetToken();

      
      nsCSSValue lineNames;
      CSSParseResult result = ParseGridLineNames(lineNames);
      if (result == CSSParseResult::NotFound) {
        return true;
      }
      if (result == CSSParseResult::Error) {
        return false;
      }
      item->mNext = new nsCSSValueList;
      item = item->mNext;
      item->mValue = lineNames;
    }
  }
}


bool
CSSParserImpl::ParseGridTrackBreadth(nsCSSValue& aValue)
{
  if (ParseNonNegativeVariant(aValue,
                              VARIANT_AUTO | VARIANT_LPCALC | VARIANT_KEYWORD,
                              nsCSSProps::kGridTrackBreadthKTable)) {
    return true;
  }

  
  if (!GetToken(true)) {
    return false;
  }
  if (!(eCSSToken_Dimension == mToken.mType &&
        mToken.mIdent.LowerCaseEqualsLiteral("fr") &&
        mToken.mNumber >= 0)) {
    UngetToken();
    return false;
  }
  aValue.SetFloatValue(mToken.mNumber, eCSSUnit_FlexFraction);
  return true;
}


CSSParseResult
CSSParserImpl::ParseGridTrackSize(nsCSSValue& aValue)
{
  
  if (ParseGridTrackBreadth(aValue)) {
    return CSSParseResult::Ok;
  }

  
  if (!GetToken(true)) {
    return CSSParseResult::NotFound;
  }
  if (!(eCSSToken_Function == mToken.mType &&
        mToken.mIdent.LowerCaseEqualsLiteral("minmax"))) {
    UngetToken();
    return CSSParseResult::NotFound;
  }
  nsCSSValue::Array* func = aValue.InitFunction(eCSSKeyword_minmax, 2);
  if (ParseGridTrackBreadth(func->Item(1)) &&
      ExpectSymbol(',', true) &&
      ParseGridTrackBreadth(func->Item(2)) &&
      ExpectSymbol(')', true)) {
    return CSSParseResult::Ok;
  }
  SkipUntil(')');
  return CSSParseResult::Error;
}

bool
CSSParserImpl::ParseGridAutoColumnsRows(nsCSSProperty aPropID)
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr) ||
      ParseGridTrackSize(value) == CSSParseResult::Ok) {
    AppendValue(aPropID, value);
    return true;
  }
  return false;
}

bool
CSSParserImpl::ParseGridTrackListWithFirstLineNames(nsCSSValue& aValue,
                                                    const nsCSSValue& aFirstLineNames)
{
  nsCSSValueList* firstLineNamesItem = aValue.SetListValue();
  firstLineNamesItem->mValue = aFirstLineNames;

  
  
  
  
  
  
  
  
  
  
  
  nsCSSValueList* item = firstLineNamesItem;
  for (;;) {
    
    if (!GetToken(true)) {
      break;
    }
    if (mToken.mType == eCSSToken_Function &&
        mToken.mIdent.LowerCaseEqualsLiteral("repeat")) {
      if (!ParseGridTrackListRepeat(&item)) {
        return false;
      }
    } else {
      UngetToken();

      
      nsCSSValue trackSize;
      CSSParseResult result = ParseGridTrackSize(trackSize);
      if (result == CSSParseResult::Error) {
        return false;
      }
      if (result == CSSParseResult::NotFound) {
        
        
        
        break;
      }
      item->mNext = new nsCSSValueList;
      item = item->mNext;
      item->mValue = trackSize;

      item->mNext = new nsCSSValueList;
      item = item->mNext;
    }
    if (ParseGridLineNames(item->mValue) == CSSParseResult::Error) {
      return false;
    }
  }

  
  if (item == firstLineNamesItem) {
    return false;
  }

  MOZ_ASSERT(aValue.GetListValue() &&
             aValue.GetListValue()->mNext &&
             aValue.GetListValue()->mNext->mNext,
             "<track-list> should have a minimum length of 3");
  return true;
}


static void
ConcatLineNames(nsCSSValue& aFirst, nsCSSValue& aSecond)
{
  if (aSecond.GetUnit() == eCSSUnit_Null) {
    
    return;
  }
  if (aFirst.GetUnit() == eCSSUnit_Null) {
    
    aFirst = aSecond;
    return;
  }

  
  nsCSSValueList* source = aSecond.GetListValue();
  nsCSSValueList* target = aFirst.GetListValue();
  
  while (target->mNext) {
    target = target->mNext;
  }
  
  
  target->mNext = new nsCSSValueList;
  target = target->mNext;
  target->mValue = source->mValue;
  
  target->mNext = source->mNext;
  source->mNext = nullptr;
}







bool
CSSParserImpl::ParseGridTrackListRepeat(nsCSSValueList** aTailPtr)
{
  if (!(GetToken(true) &&
        mToken.mType == eCSSToken_Number &&
        mToken.mIntegerValid &&
        mToken.mInteger > 0)) {
    SkipUntil(')');
    return false;
  }
  int32_t repetitions = std::min(mToken.mInteger,
                                 GRID_TEMPLATE_MAX_REPETITIONS);
  if (!ExpectSymbol(',', true)) {
    SkipUntil(')');
    return false;
  }

  
  
  
  
  nsCSSValue firstLineNames;
  nsCSSValue trackSize;
  nsCSSValue lastLineNames;
  
  if (ParseGridLineNames(firstLineNames) == CSSParseResult::Error) {
    SkipUntil(')');
    return false;
  }
  
  if (ParseGridTrackSize(trackSize) != CSSParseResult::Ok) {
    SkipUntil(')');
    return false;
  }
  
  nsAutoPtr<nsCSSValueList> firstTrackSizeItemAuto(new nsCSSValueList);
  firstTrackSizeItemAuto->mValue = trackSize;

  nsCSSValueList* item = firstTrackSizeItemAuto;
  for (;;) {
    
    if (ParseGridLineNames(lastLineNames) == CSSParseResult::Error) {
      SkipUntil(')');
      return false;
    }

    if (ExpectSymbol(')', true)) {
      break;
    }

    
    if (ParseGridTrackSize(trackSize) != CSSParseResult::Ok) {
      SkipUntil(')');
      return false;
    }

    item->mNext = new nsCSSValueList;
    item = item->mNext;
    item->mValue = lastLineNames;
    
    lastLineNames.Reset();

    item->mNext = new nsCSSValueList;
    item = item->mNext;
    item->mValue = trackSize;
  }
  nsCSSValueList* lastTrackSizeItem = item;

  
  
  
  
  
  


  
  
  
  
  
  nsCSSValue joinerLineNames;
  {
    nsCSSValueList* target = nullptr;
    if (lastLineNames.GetUnit() != eCSSUnit_Null) {
      target = joinerLineNames.SetListValue();
      nsCSSValueList* source = lastLineNames.GetListValue();
      for (;;) {
        target->mValue = source->mValue;
        source = source->mNext;
        if (!source) {
          break;
        }
        target->mNext = new nsCSSValueList;
        target = target->mNext;
      }
    }

    if (firstLineNames.GetUnit() != eCSSUnit_Null) {
      if (target) {
        target->mNext = new nsCSSValueList;
        target = target->mNext;
      } else {
        target = joinerLineNames.SetListValue();
      }
      nsCSSValueList* source = firstLineNames.GetListValue();
      for (;;) {
        target->mValue = source->mValue;
        source = source->mNext;
        if (!source) {
          break;
        }
        target->mNext = new nsCSSValueList;
        target = target->mNext;
      }
    }
  }

  
  
  nsCSSValueList* previousItemBeforeRepeat = *aTailPtr;
  ConcatLineNames(previousItemBeforeRepeat->mValue, firstLineNames);

  
  
  
  NS_ASSERTION(previousItemBeforeRepeat->mNext == nullptr,
               "Expected the end of a linked list");
  previousItemBeforeRepeat->mNext = firstTrackSizeItemAuto.forget();
  nsCSSValueList* firstTrackSizeItem = previousItemBeforeRepeat->mNext;
  nsCSSValueList* tail = lastTrackSizeItem;

  
  
  
  
  MOZ_ASSERT(repetitions > 0, "Expected positive repetitions");
  while (--repetitions) {
    tail->mNext = new nsCSSValueList;
    tail = tail->mNext;
    tail->mValue = joinerLineNames;

    nsCSSValueList* repeatedItem = firstTrackSizeItem;
    for (;;) {
      tail->mNext = new nsCSSValueList;
      tail = tail->mNext;
      tail->mValue = repeatedItem->mValue;
      if (repeatedItem == lastTrackSizeItem) {
        break;
      }
      repeatedItem = repeatedItem->mNext;
    }
  }

  
  
  tail->mNext = new nsCSSValueList;
  tail = tail->mNext;
  tail->mValue = lastLineNames;

  *aTailPtr = tail;
  return true;
}

bool
CSSParserImpl::ParseGridTemplateColumnsRows(nsCSSProperty aPropID)
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    AppendValue(aPropID, value);
    return true;
  }

  nsSubstring* ident = NextIdent();
  if (ident) {
    if (ident->LowerCaseEqualsLiteral("subgrid")) {
      if (!ParseOptionalLineNameListAfterSubgrid(value)) {
        return false;
      }
      AppendValue(aPropID, value);
      return true;
    }
    UngetToken();
  }

  nsCSSValue firstLineNames;
  if (ParseGridLineNames(firstLineNames) == CSSParseResult::Error ||
      !ParseGridTrackListWithFirstLineNames(value, firstLineNames)) {
    return false;
  }
  AppendValue(aPropID, value);
  return true;
}

bool
CSSParserImpl::ParseGridTemplateAreasLine(const nsAutoString& aInput,
                                          css::GridTemplateAreasValue* aAreas,
                                          nsDataHashtable<nsStringHashKey, uint32_t>& aAreaIndices)
{
  aAreas->mTemplates.AppendElement(mToken.mIdent);

  nsCSSGridTemplateAreaScanner scanner(aInput);
  nsCSSGridTemplateAreaToken token;
  css::GridNamedArea* currentArea = nullptr;
  uint32_t row = aAreas->NRows();
  
  
  uint32_t column = 0;
  while (scanner.Next(token)) {
    ++column;
    if (token.isTrash) {
      return false;
    }
    if (currentArea) {
      if (token.mName == currentArea->mName) {
        if (currentArea->mRowStart == row) {
          
          currentArea->mColumnEnd++;
        }
        continue;
      }
      
      
      if (currentArea->mColumnEnd != column) {
        NS_ASSERTION(currentArea->mRowStart != row,
                     "Inconsistent column end for the first row of a named area.");
        
        return false;
      }
      currentArea = nullptr;
    }
    if (!token.mName.IsEmpty()) {
      

      
      uint32_t index;
      if (aAreaIndices.Get(token.mName, &index)) {
        MOZ_ASSERT(index < aAreas->mNamedAreas.Length(),
                   "Invalid aAreaIndices hash table");
        currentArea = &aAreas->mNamedAreas[index];
        if (currentArea->mColumnStart != column ||
            currentArea->mRowEnd != row) {
          
          return false;
        }
        
        currentArea->mRowEnd++;
      } else {
        
        aAreaIndices.Put(token.mName, aAreas->mNamedAreas.Length());
        currentArea = aAreas->mNamedAreas.AppendElement();
        currentArea->mName = token.mName;
        
        
        currentArea->mColumnStart = column;
        currentArea->mColumnEnd = column + 1;
        currentArea->mRowStart = row;
        currentArea->mRowEnd = row + 1;
      }
    }
  }
  if (currentArea && currentArea->mColumnEnd != column + 1) {
    NS_ASSERTION(currentArea->mRowStart != row,
                 "Inconsistent column end for the first row of a named area.");
    
    return false;
  }

  
  
  
  
  if (row == 1) {
    aAreas->mNColumns = column;
  } else if (aAreas->mNColumns != column) {
    return false;
  }
  return true;
}

bool
CSSParserImpl::ParseGridTemplateAreas()
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    AppendValue(eCSSProperty_grid_template_areas, value);
    return true;
  }

  nsRefPtr<css::GridTemplateAreasValue> areas =
    new css::GridTemplateAreasValue();
  nsDataHashtable<nsStringHashKey, uint32_t> areaIndices;
  for (;;) {
    if (!GetToken(true)) {
      break;
    }
    if (eCSSToken_String != mToken.mType) {
      UngetToken();
      break;
    }
    if (!ParseGridTemplateAreasLine(mToken.mIdent, areas, areaIndices)) {
      return false;
    }
  }

  if (areas->NRows() == 0) {
    return false;
  }

  AppendValue(eCSSProperty_grid_template_areas, nsCSSValue(areas));
  return true;
}

bool
CSSParserImpl::ParseGridTemplate()
{
  
  
  
  
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    AppendValue(eCSSProperty_grid_template_areas, value);
    AppendValue(eCSSProperty_grid_template_columns, value);
    AppendValue(eCSSProperty_grid_template_rows, value);
    return true;
  }

  

  
  
  if (ParseVariant(value, VARIANT_NONE, nullptr)) {
    AppendValue(eCSSProperty_grid_template_columns, value);
    if (ExpectSymbol('/', true)) {
      return ParseGridTemplateAfterSlash( false);
    }
    AppendValue(eCSSProperty_grid_template_areas, value);
    AppendValue(eCSSProperty_grid_template_rows, value);
    return true;
  }

  
  
  nsSubstring* ident = NextIdent();
  if (ident) {
    if (ident->LowerCaseEqualsLiteral("subgrid")) {
      if (!ParseOptionalLineNameListAfterSubgrid(value)) {
        return false;
      }
      AppendValue(eCSSProperty_grid_template_columns, value);
      if (ExpectSymbol('/', true)) {
        return ParseGridTemplateAfterSlash( false);
      }
      if (value.GetListValue()->mNext) {
        
        
        
        return false;
      }
      
      
      AppendValue(eCSSProperty_grid_template_rows, value);
      value.SetNoneValue();
      AppendValue(eCSSProperty_grid_template_areas, value);
      return true;
    }
    UngetToken();
  }

  
  
  
  nsCSSValue firstLineNames;
  if (ParseGridLineNames(firstLineNames) == CSSParseResult::Error ||
      !GetToken(true)) {
    return false;
  }
  if (mToken.mType == eCSSToken_String) {
    
    
    value.SetNoneValue();
    AppendValue(eCSSProperty_grid_template_columns, value);
    return ParseGridTemplateAfterString(firstLineNames);
  }
  UngetToken();

  if (!(ParseGridTrackListWithFirstLineNames(value, firstLineNames) &&
        ExpectSymbol('/', true))) {
    return false;
  }
  AppendValue(eCSSProperty_grid_template_columns, value);
  return ParseGridTemplateAfterSlash( true);
}











bool
CSSParserImpl::ParseGridTemplateAfterSlash(bool aColumnsIsTrackList)
{
  nsCSSValue rowsValue;
  if (ParseVariant(rowsValue, VARIANT_NONE, nullptr)) {
    
    AppendValue(eCSSProperty_grid_template_rows, rowsValue);
    nsCSSValue areasValue(eCSSUnit_None);  
    AppendValue(eCSSProperty_grid_template_areas, areasValue);
    return true;
  }

  nsSubstring* ident = NextIdent();
  if (ident) {
    if (ident->LowerCaseEqualsLiteral("subgrid")) {
      if (!ParseOptionalLineNameListAfterSubgrid(rowsValue)) {
        return false;
      }
      
      AppendValue(eCSSProperty_grid_template_rows, rowsValue);
      nsCSSValue areasValue(eCSSUnit_None);  
      AppendValue(eCSSProperty_grid_template_areas, areasValue);
      return true;
    }
    UngetToken();
  }

  nsCSSValue firstLineNames;
  if (ParseGridLineNames(firstLineNames) == CSSParseResult::Error ||
      !GetToken(true)) {
    return false;
  }
  if (aColumnsIsTrackList && mToken.mType == eCSSToken_String) {
    
    return ParseGridTemplateAfterString(firstLineNames);
  }
  UngetToken();

  if (!ParseGridTrackListWithFirstLineNames(rowsValue, firstLineNames)) {
    return false;
  }

  
  AppendValue(eCSSProperty_grid_template_rows, rowsValue);
  nsCSSValue areasValue(eCSSUnit_None);  
  AppendValue(eCSSProperty_grid_template_areas, areasValue);
  return true;
}





bool
CSSParserImpl::ParseGridTemplateAfterString(const nsCSSValue& aFirstLineNames)
{
  MOZ_ASSERT(mToken.mType == eCSSToken_String,
             "ParseGridTemplateAfterString called with a non-string token");

  nsCSSValue rowsValue;
  nsRefPtr<css::GridTemplateAreasValue> areas =
    new css::GridTemplateAreasValue();
  nsDataHashtable<nsStringHashKey, uint32_t> areaIndices;
  nsCSSValueList* rowsItem = rowsValue.SetListValue();
  rowsItem->mValue = aFirstLineNames;

  for (;;) {
    if (!ParseGridTemplateAreasLine(mToken.mIdent, areas, areaIndices)) {
      return false;
    }

    rowsItem->mNext = new nsCSSValueList;
    rowsItem = rowsItem->mNext;
    CSSParseResult result = ParseGridTrackSize(rowsItem->mValue);
    if (result == CSSParseResult::Error) {
      return false;
    }
    if (result == CSSParseResult::NotFound) {
      rowsItem->mValue.SetAutoValue();
    }

    rowsItem->mNext = new nsCSSValueList;
    rowsItem = rowsItem->mNext;
    result = ParseGridLineNames(rowsItem->mValue);
    if (result == CSSParseResult::Error) {
      return false;
    }
    if (result == CSSParseResult::Ok) {
      
      result = ParseGridLineNames(rowsItem->mValue);
      if (result == CSSParseResult::Error) {
        return false;
      }
      if (result == CSSParseResult::Ok) {
        
        
        if (!GetToken(true)) {
          return false;
        }
        if (eCSSToken_String != mToken.mType) {
          UngetToken();
          return false;
        }
        continue;
      }
    }

    
    
    if (!GetToken(true)) {
      break;
    }
    if (eCSSToken_String != mToken.mType) {
      UngetToken();
      break;
    }
  }

  AppendValue(eCSSProperty_grid_template_areas, nsCSSValue(areas));
  AppendValue(eCSSProperty_grid_template_rows, rowsValue);
  return true;
}



bool
CSSParserImpl::ParseGrid()
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    for (const nsCSSProperty* subprops =
           nsCSSProps::SubpropertyEntryFor(eCSSProperty_grid);
         *subprops != eCSSProperty_UNKNOWN; ++subprops) {
      AppendValue(*subprops, value);
    }
    return true;
  }

  
  if (!GetToken(true)) {
    return false;
  }

  
  
  if (mToken.mType == eCSSToken_Ident) {
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);
    if (keyword == eCSSKeyword_dense ||
        keyword == eCSSKeyword_column ||
        keyword == eCSSKeyword_row) {
      UngetToken();
      return ParseGridAutoFlow() && ParseGridShorthandAutoProps();
    }
  }
  UngetToken();

  
  
  value.SetIntValue(NS_STYLE_GRID_AUTO_FLOW_ROW, eCSSUnit_Enumerated);
  AppendValue(eCSSProperty_grid_auto_flow, value);
  value.SetAutoValue();
  AppendValue(eCSSProperty_grid_auto_columns, value);
  AppendValue(eCSSProperty_grid_auto_rows, value);
  return ParseGridTemplate();
}




bool
CSSParserImpl::ParseGridShorthandAutoProps()
{
  nsCSSValue autoColumnsValue;
  nsCSSValue autoRowsValue;
  CSSParseResult result = ParseGridTrackSize(autoColumnsValue);
  if (result == CSSParseResult::Error) {
    return false;
  }
  if (result == CSSParseResult::NotFound) {
    autoColumnsValue.SetAutoValue();
    autoRowsValue.SetAutoValue();
  } else {
    if (!ExpectSymbol('/', true)) {
      autoRowsValue.SetAutoValue();
    } else if (ParseGridTrackSize(autoRowsValue) != CSSParseResult::Ok) {
      return false;
    }
  }
  AppendValue(eCSSProperty_grid_auto_columns, autoColumnsValue);
  AppendValue(eCSSProperty_grid_auto_rows, autoRowsValue);
  nsCSSValue templateValue(eCSSUnit_None);  
  AppendValue(eCSSProperty_grid_template_areas, templateValue);
  AppendValue(eCSSProperty_grid_template_columns, templateValue);
  AppendValue(eCSSProperty_grid_template_rows, templateValue);
  return true;
}










bool
CSSParserImpl::ParseGridLine(nsCSSValue& aValue)
{
  
  
  
  
  
  
  
  
  
  
  

  if (ParseVariant(aValue, VARIANT_AUTO, nullptr)) {
    return true;
  }

  static const nsCSSKeyword kGridLineKeywords[] = {
    eCSSKeyword_span,
    eCSSKeyword_UNKNOWN  
  };
  bool hasSpan = false;
  bool hasIdent = false;
  Maybe<int32_t> integer;
  nsCSSValue ident;

  if (!GetToken(true)) {
    return false;
  }
  if (mToken.mType == eCSSToken_Ident &&
      mToken.mIdent.LowerCaseEqualsLiteral("span")) {
    hasSpan = true;
    if (!GetToken(true)) {
      return false;
    }
  }

  do {
    if (!hasIdent &&
        mToken.mType == eCSSToken_Ident &&
        ParseCustomIdent(ident, mToken.mIdent, kGridLineKeywords)) {
      hasIdent = true;
    } else if (integer.isNothing() &&
               mToken.mType == eCSSToken_Number &&
               mToken.mIntegerValid &&
               mToken.mInteger != 0) {
      integer.emplace(mToken.mInteger);
    } else {
      UngetToken();
      break;
    }
  } while (!(integer.isSome() && hasIdent) && GetToken(true));

  
  if (!(integer.isSome() || hasIdent)) {
    return false;
  }

  if (!hasSpan && GetToken(true)) {
    if (mToken.mType == eCSSToken_Ident &&
        mToken.mIdent.LowerCaseEqualsLiteral("span")) {
      hasSpan = true;
    } else {
      UngetToken();
    }
  }

  nsCSSValueList* item = aValue.SetListValue();
  if (hasSpan) {
    
    if (integer.isSome() && integer.ref() < 0) {
      return false;
    }
    
    
    item->mValue.SetIntValue(1, eCSSUnit_Enumerated);
    item->mNext = new nsCSSValueList;
    item = item->mNext;
  }
  if (integer.isSome()) {
    item->mValue.SetIntValue(integer.ref(), eCSSUnit_Integer);
    if (hasIdent) {
      item->mNext = new nsCSSValueList;
      item = item->mNext;
    }
  }
  if (hasIdent) {
    item->mValue = ident;
  }
  return true;
}

bool
CSSParserImpl::ParseGridColumnRowStartEnd(nsCSSProperty aPropID)
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr) ||
      ParseGridLine(value)) {
    AppendValue(aPropID, value);
    return true;
  }
  return false;
}




static void
HandleGridLineFallback(const nsCSSValue& aFallback, nsCSSValue& aValue)
{
  if (aFallback.GetUnit() == eCSSUnit_List &&
      aFallback.GetListValue()->mValue.GetUnit() == eCSSUnit_Ident &&
      !aFallback.GetListValue()->mNext) {
    aValue = aFallback;
  } else {
    aValue.SetAutoValue();
  }
}

bool
CSSParserImpl::ParseGridColumnRow(nsCSSProperty aStartPropID,
                                  nsCSSProperty aEndPropID)
{
  nsCSSValue value;
  nsCSSValue secondValue;
  if (ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    AppendValue(aStartPropID, value);
    AppendValue(aEndPropID, value);
    return true;
  }

  if (!ParseGridLine(value)) {
    return false;
  }
  if (GetToken(true)) {
    if (mToken.IsSymbol('/')) {
      if (ParseGridLine(secondValue)) {
        AppendValue(aStartPropID, value);
        AppendValue(aEndPropID, secondValue);
        return true;
      } else {
        return false;
      }
    }
    UngetToken();
  }

  
  
  HandleGridLineFallback(value, secondValue);

  AppendValue(aStartPropID, value);
  AppendValue(aEndPropID, secondValue);
  return true;
}

bool
CSSParserImpl::ParseGridArea()
{
  nsCSSValue values[4];
  if (ParseVariant(values[0], VARIANT_INHERIT, nullptr)) {
    AppendValue(eCSSProperty_grid_row_start, values[0]);
    AppendValue(eCSSProperty_grid_column_start, values[0]);
    AppendValue(eCSSProperty_grid_row_end, values[0]);
    AppendValue(eCSSProperty_grid_column_end, values[0]);
    return true;
  }

  int32_t i = 0;
  for (;;) {
    if (!ParseGridLine(values[i])) {
      return false;
    }
    if (++i == 4 || !GetToken(true)) {
      break;
    }
    if (!mToken.IsSymbol('/')) {
      UngetToken();
      break;
    }
  }

  MOZ_ASSERT(i >= 1, "should have parsed at least one grid-line (or returned)");
  if (i < 2) {
    HandleGridLineFallback(values[0], values[1]);
  }
  if (i < 3) {
    HandleGridLineFallback(values[0], values[2]);
  }
  if (i < 4) {
    HandleGridLineFallback(values[1], values[3]);
  }

  AppendValue(eCSSProperty_grid_row_start, values[0]);
  AppendValue(eCSSProperty_grid_column_start, values[1]);
  AppendValue(eCSSProperty_grid_row_end, values[2]);
  AppendValue(eCSSProperty_grid_column_end, values[3]);
  return true;
}


bool
CSSParserImpl::ParseColorStop(nsCSSValueGradient* aGradient)
{
  nsCSSValueGradientStop* stop = aGradient->mStops.AppendElement();
  if (!ParseVariant(stop->mColor, VARIANT_COLOR, nullptr)) {
    stop->mIsInterpolationHint = true;
  }

  
  
  if (!ParseVariant(stop->mLocation, VARIANT_LP | VARIANT_CALC, nullptr)) {
    if (stop->mIsInterpolationHint) {
      return false;
    }
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
    
    
    int32_t shape =
      cssGradient->GetRadialShape().GetUnit() == eCSSUnit_Enumerated ?
      cssGradient->GetRadialShape().GetIntValue() : -1;
    
    cssGradient->mIsExplicitSize = true;
    haveSize =
      ParseNonNegativeVariant(cssGradient->GetRadiusX(), VARIANT_LP, nullptr);
    if (!haveSize) {
      
      
      
      
      
      
      
      
      cssGradient->mIsExplicitSize = false;
    } else {
      
      bool haveYSize =
        ParseNonNegativeVariant(cssGradient->GetRadiusY(), VARIANT_LP, nullptr);
      if (!haveShape) {
        nsCSSValue shapeValue;
        haveShape = ParseVariant(shapeValue, VARIANT_KEYWORD,
                                 nsCSSProps::kRadialGradientShapeKTable);
        if (haveShape) {
          shape = shapeValue.GetIntValue();
        }
      }
      if (haveYSize
            ? shape == NS_STYLE_GRADIENT_SHAPE_CIRCULAR
            : cssGradient->GetRadiusX().GetUnit() == eCSSUnit_Percent ||
              shape == NS_STYLE_GRADIENT_SHAPE_ELLIPTICAL) {
        SkipUntil(')');
        return false;
      }
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

  
  bool previousPointWasInterpolationHint = true;
  for (size_t x = 0; x < aGradient->mStops.Length(); x++) {
    bool isInterpolationHint = aGradient->mStops[x].mIsInterpolationHint;
    if (isInterpolationHint && previousPointWasInterpolationHint) {
      return false;
    }
    previousPointWasInterpolationHint = isInterpolationHint;
  }

  if (previousPointWasInterpolationHint) {
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
      else if (eCSSUnit_Unset == aValues[0].GetUnit()) { 
        for (loop = 1; loop < aNumIDs; loop++) {
          aValues[loop].SetUnsetValue();
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
        else if (eCSSUnit_Unset == aValues[loop].GetUnit()) {
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
  if (count == 0) {
    return false;
  }

  if (1 < count) { 
    NS_FOR_CSS_SIDES (index) {
      nsCSSUnit unit = (result.*(nsCSSRect::sides[index])).GetUnit();
      if (eCSSUnit_Inherit == unit ||
          eCSSUnit_Initial == unit ||
          eCSSUnit_Unset == unit) {
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
CSSParserImpl::ParseBoxCornerRadius(nsCSSProperty aPropID)
{
  nsCSSValue dimenX, dimenY;
  
  if (! ParseNonNegativeVariant(dimenX, VARIANT_HLP | VARIANT_CALC, nullptr))
    return false;

  
  if (dimenX.GetUnit() != eCSSUnit_Inherit &&
      dimenX.GetUnit() != eCSSUnit_Initial &&
      dimenX.GetUnit() != eCSSUnit_Unset) {
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
CSSParserImpl::ParseBoxCornerRadiiInternals(nsCSSValue array[])
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

  
  if (countX > 1 || countY > 0) {
    nsCSSUnit unit = dimenX.mTop.GetUnit();
    if (eCSSUnit_Inherit == unit ||
        eCSSUnit_Initial == unit ||
        eCSSUnit_Unset == unit)
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
      array[side] = x;
    } else {
      nsCSSValue pair;
      pair.SetPairValue(x, y);
      array[side] = pair;
    }
  }
  return true;
}

bool
CSSParserImpl::ParseBoxCornerRadii(const nsCSSProperty aPropIDs[])
{
  nsCSSValue value[4];
  if (!ParseBoxCornerRadiiInternals(value)) {
    return false;
  }

  NS_FOR_CSS_SIDES(side) {
    AppendValue(aPropIDs[side], value[side]);
  }
  return true;
}


static const nsCSSProperty kBorderStyleIDs[] = {
  eCSSProperty_border_top_style,
  eCSSProperty_border_right_style,
  eCSSProperty_border_bottom_style,
  eCSSProperty_border_left_style
};
static const nsCSSProperty kBorderWidthIDs[] = {
  eCSSProperty_border_top_width,
  eCSSProperty_border_right_width,
  eCSSProperty_border_bottom_width,
  eCSSProperty_border_left_width
};
static const nsCSSProperty kBorderColorIDs[] = {
  eCSSProperty_border_top_color,
  eCSSProperty_border_right_color,
  eCSSProperty_border_bottom_color,
  eCSSProperty_border_left_color
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

void
CSSParserImpl::SaveInputState(CSSParserInputState& aState)
{
  aState.mToken = mToken;
  aState.mHavePushBack = mHavePushBack;
  mScanner->SavePosition(aState.mPosition);
}

void
CSSParserImpl::RestoreSavedInputState(const CSSParserInputState& aState)
{
  mToken = aState.mToken;
  mHavePushBack = aState.mHavePushBack;
  mScanner->RestoreSavedPosition(aState.mPosition);
}

bool
CSSParserImpl::ParseProperty(nsCSSProperty aPropID)
{
  
  MOZ_ASSERT(!mHashlessColorQuirk,
             "hashless color quirk should not be set");
  MOZ_ASSERT(!mUnitlessLengthQuirk,
             "unitless length quirk should not be set");
  MOZ_ASSERT(aPropID != eCSSPropertyExtra_variable);

  if (mNavQuirkMode) {
    mHashlessColorQuirk =
      nsCSSProps::PropHasFlags(aPropID, CSS_PROPERTY_HASHLESS_COLOR_QUIRK);
    mUnitlessLengthQuirk =
      nsCSSProps::PropHasFlags(aPropID, CSS_PROPERTY_UNITLESS_LENGTH_QUIRK);
  }

  
  
  
  CSSParserInputState stateBeforeProperty;
  SaveInputState(stateBeforeProperty);
  mScanner->ClearSeenVariableReference();

  NS_ASSERTION(aPropID < eCSSProperty_COUNT, "index out of range");
  bool allowVariables = true;
  bool result;
  switch (nsCSSProps::PropertyParseType(aPropID)) {
    case CSS_PROPERTY_PARSE_INACCESSIBLE: {
      
      REPORT_UNEXPECTED(PEInaccessibleProperty2);
      allowVariables = false;
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
        AppendValue(aPropID, value);
        result = true;
      }
      
      break;
    }
    case CSS_PROPERTY_PARSE_VALUE_LIST: {
      result = ParseValueList(aPropID);
      break;
    }
    default: {
      result = false;
      allowVariables = false;
      MOZ_ASSERT(false,
                 "Property's flags field in nsCSSPropList.h is missing "
                 "one of the CSS_PROPERTY_PARSE_* constants");
      break;
    }
  }

  if (result) {
    
    
    
    
    
    
    
    
    
    
    
    
    if (!ExpectEndProperty()) {
      result = false;
    }
  }

  bool seenVariable = mScanner->SeenVariableReference() ||
    (stateBeforeProperty.mHavePushBack &&
     stateBeforeProperty.mToken.mType == eCSSToken_Function &&
     stateBeforeProperty.mToken.mIdent.LowerCaseEqualsLiteral("var"));
  bool parseAsTokenStream;

  if (!result && allowVariables) {
    parseAsTokenStream = true;
    if (!seenVariable) {
      
      
      
      CSSParserInputState stateAtError;
      SaveInputState(stateAtError);

      const char16_t stopChars[] = { ';', '!', '}', ')', 0 };
      SkipUntilOneOf(stopChars);
      UngetToken();
      parseAsTokenStream = mScanner->SeenVariableReference();

      if (!parseAsTokenStream) {
        
        
        
        RestoreSavedInputState(stateAtError);
      }
    }
  } else {
    parseAsTokenStream = false;
  }

  if (parseAsTokenStream) {
    
    
    
    RestoreSavedInputState(stateBeforeProperty);

    if (!mInSupportsCondition) {
      mScanner->StartRecording();
    }

    CSSVariableDeclarations::Type type;
    bool dropBackslash;
    nsString impliedCharacters;
    nsCSSValue value;
    if (ParseValueWithVariables(&type, &dropBackslash, impliedCharacters,
                                nullptr, nullptr)) {
      MOZ_ASSERT(type == CSSVariableDeclarations::eTokenStream,
                 "a non-custom property reparsed since it contained variable "
                 "references should not have been 'initial' or 'inherit'");

      nsString propertyValue;

      if (!mInSupportsCondition) {
        
        
        mScanner->StopRecording(propertyValue);
        if (dropBackslash) {
          MOZ_ASSERT(!propertyValue.IsEmpty() &&
                     propertyValue[propertyValue.Length() - 1] == '\\');
          propertyValue.Truncate(propertyValue.Length() - 1);
        }
        propertyValue.Append(impliedCharacters);
      }

      if (mHavePushBack) {
        
        
        
        
        MOZ_ASSERT(mToken.IsSymbol('!') ||
                   mToken.IsSymbol(')') ||
                   mToken.IsSymbol(';') ||
                   mToken.IsSymbol(']') ||
                   mToken.IsSymbol('}'));
        if (!mInSupportsCondition) {
          MOZ_ASSERT(!propertyValue.IsEmpty());
          MOZ_ASSERT(propertyValue[propertyValue.Length() - 1] ==
                     mToken.mSymbol);
          propertyValue.Truncate(propertyValue.Length() - 1);
        }
      }

      if (!mInSupportsCondition) {
        if (nsCSSProps::IsShorthand(aPropID)) {
          
          
          CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, aPropID,
                                               PropertyEnabledState()) {
            nsCSSValueTokenStream* tokenStream = new nsCSSValueTokenStream;
            tokenStream->mPropertyID = *p;
            tokenStream->mShorthandPropertyID = aPropID;
            tokenStream->mTokenStream = propertyValue;
            tokenStream->mBaseURI = mBaseURI;
            tokenStream->mSheetURI = mSheetURI;
            tokenStream->mSheetPrincipal = mSheetPrincipal;
            tokenStream->mSheet = mSheet;
            tokenStream->mLineNumber = stateBeforeProperty.mPosition.LineNumber();
            tokenStream->mLineOffset = stateBeforeProperty.mPosition.LineOffset();
            value.SetTokenStreamValue(tokenStream);
            AppendValue(*p, value);
          }
        } else {
          nsCSSValueTokenStream* tokenStream = new nsCSSValueTokenStream;
          tokenStream->mPropertyID = aPropID;
          tokenStream->mTokenStream = propertyValue;
          tokenStream->mBaseURI = mBaseURI;
          tokenStream->mSheetURI = mSheetURI;
          tokenStream->mSheetPrincipal = mSheetPrincipal;
          tokenStream->mSheet = mSheet;
          tokenStream->mLineNumber = stateBeforeProperty.mPosition.LineNumber();
          tokenStream->mLineOffset = stateBeforeProperty.mPosition.LineOffset();
          value.SetTokenStreamValue(tokenStream);
          AppendValue(aPropID, value);
        }
      }
      result = true;
    } else {
      if (!mInSupportsCondition) {
        mScanner->StopRecording();
      }
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
  case eCSSProperty_border_block_end:
    return ParseBorderSide(kBorderBlockEndIDs, false);
  case eCSSProperty_border_block_start:
    return ParseBorderSide(kBorderBlockStartIDs, false);
  case eCSSProperty_border_bottom:
    return ParseBorderSide(kBorderBottomIDs, false);
  case eCSSProperty_border_end:
    return ParseBorderSide(kBorderEndIDs, false);
  case eCSSProperty_border_start:
    return ParseBorderSide(kBorderStartIDs, false);
  case eCSSProperty_border_left:
    return ParseBorderSide(kBorderLeftIDs, false);
  case eCSSProperty_border_right:
    return ParseBorderSide(kBorderRightIDs, false);
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
  case eCSSProperty_filter:
    return ParseFilter();
  case eCSSProperty_flex:
    return ParseFlex();
  case eCSSProperty_flex_flow:
    return ParseFlexFlow();
  case eCSSProperty_font:
    return ParseFont();
  case eCSSProperty_font_variant:
    return ParseFontVariant();
  case eCSSProperty_grid_auto_flow:
    return ParseGridAutoFlow();
  case eCSSProperty_grid_auto_columns:
  case eCSSProperty_grid_auto_rows:
    return ParseGridAutoColumnsRows(aPropID);
  case eCSSProperty_grid_template_areas:
    return ParseGridTemplateAreas();
  case eCSSProperty_grid_template_columns:
  case eCSSProperty_grid_template_rows:
    return ParseGridTemplateColumnsRows(aPropID);
  case eCSSProperty_grid_template:
    return ParseGridTemplate();
  case eCSSProperty_grid:
    return ParseGrid();
  case eCSSProperty_grid_column_start:
  case eCSSProperty_grid_column_end:
  case eCSSProperty_grid_row_start:
  case eCSSProperty_grid_row_end:
    return ParseGridColumnRowStartEnd(aPropID);
  case eCSSProperty_grid_column:
    return ParseGridColumnRow(eCSSProperty_grid_column_start,
                              eCSSProperty_grid_column_end);
  case eCSSProperty_grid_row:
    return ParseGridColumnRow(eCSSProperty_grid_row_start,
                              eCSSProperty_grid_row_end);
  case eCSSProperty_grid_area:
    return ParseGridArea();
  case eCSSProperty_image_region:
    return ParseRect(eCSSProperty_image_region);
  case eCSSProperty_list_style:
    return ParseListStyle();
  case eCSSProperty_margin:
    return ParseMargin();
  case eCSSProperty_object_position:
    return ParseObjectPosition();
  case eCSSProperty_outline:
    return ParseOutline();
  case eCSSProperty_overflow:
    return ParseOverflow();
  case eCSSProperty_padding:
    return ParsePadding();
  case eCSSProperty_quotes:
    return ParseQuotes();
  case eCSSProperty_size:
    return ParseSize();
  case eCSSProperty_text_decoration:
    return ParseTextDecoration();
  case eCSSProperty_will_change:
    return ParseWillChange();
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
  case eCSSProperty_clip_path:
    return ParseClipPath();
  case eCSSProperty_scroll_snap_type:
    return ParseScrollSnapType();
  case eCSSProperty_all:
    return ParseAll();
  default:
    MOZ_ASSERT(false, "should not be called");
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
    MOZ_ASSERT(false, "not a single value property");
    return false;
  }

  if (nsCSSProps::PropHasFlags(aPropID, CSS_PROPERTY_VALUE_PARSER_FUNCTION)) {
    switch (aPropID) {
      case eCSSProperty_font_family:
        return ParseFamily(aValue);
      case eCSSProperty_font_synthesis:
        return ParseFontSynthesis(aValue);
      case eCSSProperty_font_variant_alternates:
        return ParseFontVariantAlternates(aValue);
      case eCSSProperty_font_variant_east_asian:
        return ParseFontVariantEastAsian(aValue);
      case eCSSProperty_font_variant_ligatures:
        return ParseFontVariantLigatures(aValue);
      case eCSSProperty_font_variant_numeric:
        return ParseFontVariantNumeric(aValue);
      case eCSSProperty_font_feature_settings:
        return ParseFontFeatureSettings(aValue);
      case eCSSProperty_font_weight:
        return ParseFontWeight(aValue);
      case eCSSProperty_image_orientation:
        return ParseImageOrientation(aValue);
      case eCSSProperty_list_style_type:
        return ParseListStyleType(aValue);
      case eCSSProperty_marks:
        return ParseMarks(aValue);
      case eCSSProperty_scroll_snap_points_x:
        return ParseScrollSnapPoints(aValue, eCSSProperty_scroll_snap_points_x);
      case eCSSProperty_scroll_snap_points_y:
        return ParseScrollSnapPoints(aValue, eCSSProperty_scroll_snap_points_y);
      case eCSSProperty_scroll_snap_destination:
        return ParseScrollSnapDestination(aValue);
      case eCSSProperty_scroll_snap_coordinate:
        return ParseScrollSnapCoordinate(aValue);
      case eCSSProperty_text_align:
        return ParseTextAlign(aValue);
      case eCSSProperty_text_align_last:
        return ParseTextAlignLast(aValue);
      case eCSSProperty_text_decoration_line:
        return ParseTextDecorationLine(aValue);
      case eCSSProperty_text_combine_upright:
        return ParseTextCombineUpright(aValue);
      case eCSSProperty_text_overflow:
        return ParseTextOverflow(aValue);
      case eCSSProperty_touch_action:
        return ParseTouchAction(aValue);
      default:
        MOZ_ASSERT(false, "should not reach here");
        return false;
    }
  }

  uint32_t variant = nsCSSProps::ParserVariant(aPropID);
  if (variant == 0) {
    MOZ_ASSERT(false, "not a single value property");
    return false;
  }

  
  
  
  
  if (!mUnsafeRulesEnabled &&
      (aPropID == eCSSProperty_script_level ||
       aPropID == eCSSProperty_math_display))
    return false;

  const KTableValue *kwtable = nsCSSProps::kKeywordTableTable[aPropID];
  switch (nsCSSProps::ValueRestrictions(aPropID)) {
    default:
      MOZ_ASSERT(false, "should not be reached");
    case 0:
      return ParseVariant(aValue, variant, kwtable);
    case CSS_PROPERTY_VALUE_NONNEGATIVE:
      return ParseNonNegativeVariant(aValue, variant, kwtable);
    case CSS_PROPERTY_VALUE_AT_LEAST_ONE:
      return ParseOneOrLargerVariant(aValue, variant, kwtable);
  }
}



bool
CSSParserImpl::ParseFontDescriptorValue(nsCSSFontDesc aDescID,
                                        nsCSSValue& aValue)
{
  switch (aDescID) {
    
    
  case eCSSFontDesc_Family: {
    nsCSSValue value;
    if (!ParseFamily(value) ||
        value.GetUnit() != eCSSUnit_FontFamilyList)
      return false;

    
    const FontFamilyList* f = value.GetFontFamilyListValue();
    const nsTArray<FontFamilyName>& fontlist = f->GetFontlist();

    if (fontlist.Length() != 1 || !fontlist[0].IsNamed()) {
      return false;
    }

    aValue.SetStringValue(fontlist[0].mName, eCSSUnit_String);
    return true;
  }

  case eCSSFontDesc_Style:
    
    return ParseVariant(aValue, VARIANT_KEYWORD | VARIANT_NORMAL,
                        nsCSSProps::kFontStyleKTable);

  case eCSSFontDesc_Weight:
    return (ParseFontWeight(aValue) &&
            aValue.GetUnit() != eCSSUnit_Inherit &&
            aValue.GetUnit() != eCSSUnit_Initial &&
            aValue.GetUnit() != eCSSUnit_Unset &&
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
    
    if (color.GetUnit() != eCSSUnit_Null) {
      break;
    }
    
    if (!ExpectSymbol(',', true)) {
      break;
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
    color.SetIntegerColorValue(NS_RGBA(0,0,0,0), eCSSUnit_RGBAColor);
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
          keyword == eCSSKeyword_initial ||
          keyword == eCSSKeyword_unset) {
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
        if (!ParsePositionValue(aState.mPosition->mValue)) {
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

        
        

        
        MOZ_ASSERT(nsCSSProps::kKeywordTableTable[
                     eCSSProperty_background_origin] ==
                   nsCSSProps::kBackgroundOriginKTable);
        MOZ_ASSERT(nsCSSProps::kKeywordTableTable[
                     eCSSProperty_background_clip] ==
                   nsCSSProps::kBackgroundOriginKTable);
        static_assert(NS_STYLE_BG_CLIP_BORDER ==
                      NS_STYLE_BG_ORIGIN_BORDER &&
                      NS_STYLE_BG_CLIP_PADDING ==
                      NS_STYLE_BG_ORIGIN_PADDING &&
                      NS_STYLE_BG_CLIP_CONTENT ==
                      NS_STYLE_BG_ORIGIN_CONTENT,
                      "bg-clip and bg-origin style constants must agree");

        if (!ParseSingleValueProperty(aState.mClip->mValue,
                                      eCSSProperty_background_clip)) {
          
          
          
          aState.mClip->mValue = aState.mOrigin->mValue;
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
      if (!ParsePositionValue(aState.mPosition->mValue)) {
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
  
  if (!ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    nsCSSValueList* item = value.SetListValue();
    for (;;) {
      if (!ParseSingleValueProperty(item->mValue, aPropID)) {
        return false;
      }
      if (!ExpectSymbol(',', true)) {
        break;
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
  
  if (!ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    nsCSSValuePair valuePair;
    if (!ParseBackgroundRepeatValues(valuePair)) {
      return false;
    }
    nsCSSValuePairList* item = value.SetPairListValue();
    for (;;) {
      item->mXValue = valuePair.mXValue;
      item->mYValue = valuePair.mYValue;
      if (!ExpectSymbol(',', true)) {
        break;
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
  
  if (!ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    nsCSSValue itemValue;
    if (!ParsePositionValue(itemValue)) {
      return false;
    }
    nsCSSValueList* item = value.SetListValue();
    for (;;) {
      item->mValue = itemValue;
      if (!ExpectSymbol(',', true)) {
        break;
      }
      if (!ParsePositionValue(itemValue)) {
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
        eCSSUnit_Initial == xValue.GetUnit() ||
        eCSSUnit_Unset == xValue.GetUnit()) {  
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



bool
CSSParserImpl::ParsePositionValue(nsCSSValue& aOut)
{
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
  
  if (!ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    nsCSSValuePair valuePair;
    if (!ParseBackgroundSizeValues(valuePair)) {
      return false;
    }
    nsCSSValuePairList* item = value.SetPairListValue();
    for (;;) {
      item->mXValue = valuePair.mXValue;
      item->mYValue = valuePair.mYValue;
      if (!ExpectSymbol(',', true)) {
        break;
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
    
    if (!foundSource && ParseVariant(imageSourceValue, VARIANT_IMAGE, nullptr)) {
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
  if (found < 1) {
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
    case eCSSUnit_Unset:
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
CSSParserImpl::ParseBorderStyle()
{
  return ParseBoxProperties(kBorderStyleIDs);
}

bool
CSSParserImpl::ParseBorderWidth()
{
  return ParseBoxProperties(kBorderWidthIDs);
}

bool
CSSParserImpl::ParseBorderColors(nsCSSProperty aProperty)
{
  nsCSSValue value;
  
  if (!ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
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
  MOZ_ASSERT(aVariantMask != 0, "unexpected variant mask");

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
  MOZ_ASSERT(aVariantMask != 0, "unexpected variant mask");
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
    MOZ_ASSERT(aValue.GetUnit() == eCSSUnit_Number, "unexpected unit");
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
  MOZ_ASSERT(aVariantMask != 0, "unexpected variant mask");
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
    MOZ_ASSERT(variantMask != 0,
               "ParseCalcTerm did not set variantMask appropriately");
    MOZ_ASSERT(!(variantMask & VARIANT_NUMBER) ||
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
        
        
        MOZ_ASSERT(storage == &aValue.GetArrayValue()->Item(1),
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
  MOZ_ASSERT(aVariantMask != 0, "unexpected variant mask");
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
  nsCSSValue val;
  if (ParseVariant(val, VARIANT_INHERIT | VARIANT_AUTO, nullptr)) {
    AppendValue(aPropID, val);
    return true;
  }

  if (! GetToken(true)) {
    return false;
  }

  if (mToken.mType == eCSSToken_Function &&
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
  const int32_t numProps = MOZ_ARRAY_LENGTH(columnIDs);

  nsCSSValue values[numProps];
  int32_t found = ParseChoice(values, columnIDs, numProps);
  if (found < 1) {
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
  
  
  static const KTableValue kContentListKWs[] = {
    eCSSKeyword_open_quote, NS_STYLE_CONTENT_OPEN_QUOTE,
    eCSSKeyword_close_quote, NS_STYLE_CONTENT_CLOSE_QUOTE,
    eCSSKeyword_no_open_quote, NS_STYLE_CONTENT_NO_OPEN_QUOTE,
    eCSSKeyword_no_close_quote, NS_STYLE_CONTENT_NO_CLOSE_QUOTE,
    eCSSKeyword_UNKNOWN,-1
  };

  static const KTableValue kContentSolitaryKWs[] = {
    eCSSKeyword__moz_alt_content, NS_STYLE_CONTENT_ALT_CONTENT,
    eCSSKeyword_UNKNOWN,-1
  };

  
  
  MOZ_ASSERT(nsCSSProps::kContentKTable[
               ArrayLength(kContentListKWs) +
               ArrayLength(kContentSolitaryKWs) - 4] == eCSSKeyword_UNKNOWN &&
             nsCSSProps::kContentKTable[
               ArrayLength(kContentListKWs) +
               ArrayLength(kContentSolitaryKWs) - 3] == -1,
             "content keyword tables out of sync");

  nsCSSValue value;
  
  
  if (!ParseVariant(value, VARIANT_HMK | VARIANT_NONE,
                    kContentSolitaryKWs)) {
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
  static const nsCSSKeyword kCounterDataKTable[] = {
    eCSSKeyword_none,
    eCSSKeyword_UNKNOWN
  };
  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    if (!GetToken(true)) {
      return false;
    }
    if (mToken.mType != eCSSToken_Ident) {
      UngetToken();
      return false;
    }

    nsCSSValuePairList *cur = value.SetPairListValue();
    for (;;) {
      if (!ParseCustomIdent(cur->mXValue, mToken.mIdent, kCounterDataKTable)) {
        return false;
      }
      if (!GetToken(true)) {
        break;
      }
      if (mToken.mType == eCSSToken_Number && mToken.mIntegerValid) {
        cur->mYValue.SetIntValue(mToken.mInteger, eCSSUnit_Integer);
      } else {
        UngetToken();
      }
      if (!GetToken(true)) {
        break;
      }
      if (mToken.mType != eCSSToken_Ident) {
        UngetToken();
        break;
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
  
  if (!ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    nsCSSValueList* cur = value.SetListValue();
    for (;;) {
      if (!ParseVariant(cur->mValue, VARIANT_UK, nsCSSProps::kCursorKTable)) {
        return false;
      }
      if (cur->mValue.GetUnit() != eCSSUnit_URL) { 
        break;
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
    eCSSProperty_font_variant_caps,
    eCSSProperty_font_weight
  };

  nsCSSValue  family;
  if (ParseVariant(family, VARIANT_HK, nsCSSProps::kFontKTable)) {
    if (eCSSUnit_Inherit == family.GetUnit() ||
        eCSSUnit_Initial == family.GetUnit() ||
        eCSSUnit_Unset == family.GetUnit()) {
      AppendValue(eCSSProperty__x_system_font, nsCSSValue(eCSSUnit_None));
      AppendValue(eCSSProperty_font_family, family);
      AppendValue(eCSSProperty_font_style, family);
      AppendValue(eCSSProperty_font_weight, family);
      AppendValue(eCSSProperty_font_size, family);
      AppendValue(eCSSProperty_line_height, family);
      AppendValue(eCSSProperty_font_stretch, family);
      AppendValue(eCSSProperty_font_size_adjust, family);
      AppendValue(eCSSProperty_font_feature_settings, family);
      AppendValue(eCSSProperty_font_language_override, family);
      AppendValue(eCSSProperty_font_kerning, family);
      AppendValue(eCSSProperty_font_synthesis, family);
      AppendValue(eCSSProperty_font_variant_alternates, family);
      AppendValue(eCSSProperty_font_variant_caps, family);
      AppendValue(eCSSProperty_font_variant_east_asian, family);
      AppendValue(eCSSProperty_font_variant_ligatures, family);
      AppendValue(eCSSProperty_font_variant_numeric, family);
      AppendValue(eCSSProperty_font_variant_position, family);
    }
    else {
      AppendValue(eCSSProperty__x_system_font, family);
      nsCSSValue systemFont(eCSSUnit_System_Font);
      AppendValue(eCSSProperty_font_family, systemFont);
      AppendValue(eCSSProperty_font_style, systemFont);
      AppendValue(eCSSProperty_font_weight, systemFont);
      AppendValue(eCSSProperty_font_size, systemFont);
      AppendValue(eCSSProperty_line_height, systemFont);
      AppendValue(eCSSProperty_font_stretch, systemFont);
      AppendValue(eCSSProperty_font_size_adjust, systemFont);
      AppendValue(eCSSProperty_font_feature_settings, systemFont);
      AppendValue(eCSSProperty_font_language_override, systemFont);
      AppendValue(eCSSProperty_font_kerning, systemFont);
      AppendValue(eCSSProperty_font_synthesis, systemFont);
      AppendValue(eCSSProperty_font_variant_alternates, systemFont);
      AppendValue(eCSSProperty_font_variant_caps, systemFont);
      AppendValue(eCSSProperty_font_variant_east_asian, systemFont);
      AppendValue(eCSSProperty_font_variant_ligatures, systemFont);
      AppendValue(eCSSProperty_font_variant_numeric, systemFont);
      AppendValue(eCSSProperty_font_variant_position, systemFont);
    }
    return true;
  }

  
  const int32_t numProps = 3;
  nsCSSValue  values[numProps];
  int32_t found = ParseChoice(values, fontIDs, numProps);
  if (found < 0 ||
      eCSSUnit_Inherit == values[0].GetUnit() ||
      eCSSUnit_Initial == values[0].GetUnit() ||
      eCSSUnit_Unset == values[0].GetUnit()) { 
    return false;
  }
  if ((found & 1) == 0) {
    
    values[0].SetIntValue(NS_FONT_STYLE_NORMAL, eCSSUnit_Enumerated);
  }
  if ((found & 2) == 0) {
    
    values[1].SetNormalValue();
  } else {
    if (values[1].GetUnit() == eCSSUnit_Enumerated &&
        values[1].GetIntValue() != NS_FONT_VARIANT_CAPS_SMALLCAPS) {
      
      
      
      return false;
    }
  }
  if ((found & 4) == 0) {
    
    values[2].SetIntValue(NS_FONT_WEIGHT_NORMAL, eCSSUnit_Enumerated);
  }

  
  nsCSSValue  size;
  if (! ParseNonNegativeVariant(size, VARIANT_KEYWORD | VARIANT_LP,
                                nsCSSProps::kFontSizeKTable)) {
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
    if (eCSSUnit_Inherit != family.GetUnit() &&
        eCSSUnit_Initial != family.GetUnit() &&
        eCSSUnit_Unset != family.GetUnit()) {
      AppendValue(eCSSProperty__x_system_font, nsCSSValue(eCSSUnit_None));
      AppendValue(eCSSProperty_font_family, family);
      AppendValue(eCSSProperty_font_style, values[0]);
      AppendValue(eCSSProperty_font_variant_caps, values[1]);
      AppendValue(eCSSProperty_font_weight, values[2]);
      AppendValue(eCSSProperty_font_size, size);
      AppendValue(eCSSProperty_line_height, lineHeight);
      AppendValue(eCSSProperty_font_stretch,
                  nsCSSValue(NS_FONT_STRETCH_NORMAL, eCSSUnit_Enumerated));
      AppendValue(eCSSProperty_font_size_adjust, nsCSSValue(eCSSUnit_None));
      AppendValue(eCSSProperty_font_feature_settings, nsCSSValue(eCSSUnit_Normal));
      AppendValue(eCSSProperty_font_language_override, nsCSSValue(eCSSUnit_Normal));
      AppendValue(eCSSProperty_font_kerning,
                  nsCSSValue(NS_FONT_KERNING_AUTO, eCSSUnit_Enumerated));
      AppendValue(eCSSProperty_font_synthesis,
                  nsCSSValue(NS_FONT_SYNTHESIS_WEIGHT | NS_FONT_SYNTHESIS_STYLE,
                             eCSSUnit_Enumerated));
      AppendValue(eCSSProperty_font_variant_alternates,
                  nsCSSValue(eCSSUnit_Normal));
      AppendValue(eCSSProperty_font_variant_east_asian,
                  nsCSSValue(eCSSUnit_Normal));
      AppendValue(eCSSProperty_font_variant_ligatures,
                  nsCSSValue(eCSSUnit_Normal));
      AppendValue(eCSSProperty_font_variant_numeric,
                  nsCSSValue(eCSSUnit_Normal));
      AppendValue(eCSSProperty_font_variant_position,
                  nsCSSValue(eCSSUnit_Normal));
      return true;
    }
  }
  return false;
}

bool
CSSParserImpl::ParseFontSynthesis(nsCSSValue& aValue)
{
  if (!ParseVariant(aValue, VARIANT_HK | VARIANT_NONE,
                    nsCSSProps::kFontSynthesisKTable)) {
    return false;
  }

  
  if (eCSSUnit_None == aValue.GetUnit() ||
      eCSSUnit_Initial == aValue.GetUnit() ||
      eCSSUnit_Inherit == aValue.GetUnit() ||
      eCSSUnit_Unset == aValue.GetUnit())
  {
    return true;
  }

  
  int32_t intValue = aValue.GetIntValue();
  nsCSSValue nextValue;

  if (ParseEnum(nextValue, nsCSSProps::kFontSynthesisKTable)) {
    int32_t nextIntValue = nextValue.GetIntValue();
    if (nextIntValue & intValue) {
      return false;
    }
    aValue.SetIntValue(nextIntValue | intValue, eCSSUnit_Enumerated);
  }

  return true;
}


























#define MAX_ALLOWED_FEATURES 512

static uint16_t
MaxElementsForAlternateType(nsCSSKeyword keyword)
{
  uint16_t maxElems = 1;
  if (keyword == eCSSKeyword_styleset ||
      keyword == eCSSKeyword_character_variant) {
    maxElems = MAX_ALLOWED_FEATURES;
  }
  return maxElems;
}

bool
CSSParserImpl::ParseSingleAlternate(int32_t& aWhichFeature,
                                    nsCSSValue& aValue)
{
  if (!GetToken(true)) {
    return false;
  }

  bool isIdent = (mToken.mType == eCSSToken_Ident);
  if (mToken.mType != eCSSToken_Function && !isIdent) {
    UngetToken();
    return false;
  }

  
  

  nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);
  if (!(eCSSKeyword_UNKNOWN < keyword &&
        nsCSSProps::FindKeyword(keyword,
                                (isIdent ?
                                 nsCSSProps::kFontVariantAlternatesKTable :
                                 nsCSSProps::kFontVariantAlternatesFuncsKTable),
                                aWhichFeature)))
  {
    
    UngetToken();
    return false;
  }

  if (isIdent) {
    aValue.SetIntValue(aWhichFeature, eCSSUnit_Enumerated);
    return true;
  }

  return ParseFunction(keyword, nullptr, VARIANT_IDENTIFIER,
                       1, MaxElementsForAlternateType(keyword), aValue);
}

bool
CSSParserImpl::ParseFontVariantAlternates(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_INHERIT | VARIANT_NORMAL, nullptr)) {
    return true;
  }

  
  nsCSSValue listValue;
  int32_t feature, featureFlags = 0;

  
  listValue.SetListValue();

  nsCSSValueList* list = nullptr;
  nsCSSValue value;
  while (ParseSingleAlternate(feature, value)) {

    
    if (feature == 0 ||
        feature & featureFlags) {
      return false;
    }

    featureFlags |= feature;

    
    if (value.GetUnit() == eCSSUnit_Function) {
      if (!list) {
        list = listValue.GetListValue();
      } else {
        list->mNext = new nsCSSValueList;
        list = list->mNext;
      }
      list->mValue = value;
    }
  }

  if (featureFlags == 0) {
    
    return false;
  }

  nsCSSValue featureValue;
  featureValue.SetIntValue(featureFlags, eCSSUnit_Enumerated);
  aValue.SetPairValue(featureValue, listValue);

  return true;
}

bool
CSSParserImpl::MergeBitmaskValue(int32_t aNewValue,
                                 const int32_t aMasks[],
                                 int32_t& aMergedValue)
{
  
  if (aNewValue & aMergedValue) {
    return false;
  }

  const int32_t *m = aMasks;
  int32_t c = 0;

  while (*m != MASK_END_VALUE) {
    if (*m & aNewValue) {
      c = aMergedValue & *m;
      break;
    }
    m++;
  }

  if (c) {
    return false;
  }

  aMergedValue |= aNewValue;
  return true;
}




bool
CSSParserImpl::ParseBitmaskValues(nsCSSValue& aValue,
                                  const KTableValue aKeywordTable[],
                                  const int32_t aMasks[])
{
  
  if (!ParseEnum(aValue, aKeywordTable)) {
    return false;
  }

  
  nsCSSValue nextValue;
  int32_t mergedValue = aValue.GetIntValue();

  while (ParseEnum(nextValue, aKeywordTable))
  {
    if (!MergeBitmaskValue(nextValue.GetIntValue(), aMasks, mergedValue)) {
      return false;
    }
  }

  aValue.SetIntValue(mergedValue, eCSSUnit_Enumerated);

  return true;
}

static const int32_t maskEastAsian[] = {
  NS_FONT_VARIANT_EAST_ASIAN_VARIANT_MASK,
  NS_FONT_VARIANT_EAST_ASIAN_WIDTH_MASK,
  MASK_END_VALUE
};

bool
CSSParserImpl::ParseFontVariantEastAsian(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_INHERIT | VARIANT_NORMAL, nullptr)) {
    return true;
  }

  NS_ASSERTION(maskEastAsian[ArrayLength(maskEastAsian) - 1] ==
                 MASK_END_VALUE,
               "incorrectly terminated array");

  return ParseBitmaskValues(aValue, nsCSSProps::kFontVariantEastAsianKTable,
                            maskEastAsian);
}

static const int32_t maskLigatures[] = {
  NS_FONT_VARIANT_LIGATURES_COMMON_MASK,
  NS_FONT_VARIANT_LIGATURES_DISCRETIONARY_MASK,
  NS_FONT_VARIANT_LIGATURES_HISTORICAL_MASK,
  NS_FONT_VARIANT_LIGATURES_CONTEXTUAL_MASK,
  MASK_END_VALUE
};

bool
CSSParserImpl::ParseFontVariantLigatures(nsCSSValue& aValue)
{
  if (ParseVariant(aValue,
                   VARIANT_INHERIT | VARIANT_NORMAL | VARIANT_NONE,
                   nullptr)) {
    return true;
  }

  NS_ASSERTION(maskLigatures[ArrayLength(maskLigatures) - 1] ==
                 MASK_END_VALUE,
               "incorrectly terminated array");

  return ParseBitmaskValues(aValue, nsCSSProps::kFontVariantLigaturesKTable,
                            maskLigatures);
}

static const int32_t maskNumeric[] = {
  NS_FONT_VARIANT_NUMERIC_FIGURE_MASK,
  NS_FONT_VARIANT_NUMERIC_SPACING_MASK,
  NS_FONT_VARIANT_NUMERIC_FRACTION_MASK,
  MASK_END_VALUE
};

bool
CSSParserImpl::ParseFontVariantNumeric(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_INHERIT | VARIANT_NORMAL, nullptr)) {
    return true;
  }

  NS_ASSERTION(maskNumeric[ArrayLength(maskNumeric) - 1] ==
                 MASK_END_VALUE,
               "incorrectly terminated array");

  return ParseBitmaskValues(aValue, nsCSSProps::kFontVariantNumericKTable,
                            maskNumeric);
}

bool
CSSParserImpl::ParseFontVariant()
{
  
  nsCSSValue value;
  nsCSSValue normal(eCSSUnit_Normal);

  if (ParseVariant(value,
                   VARIANT_INHERIT | VARIANT_NORMAL | VARIANT_NONE,
                   nullptr)) {
    AppendValue(eCSSProperty_font_variant_ligatures, value);
    if (eCSSUnit_None == value.GetUnit()) {
      
      
      value.SetNormalValue();
    }
    AppendValue(eCSSProperty_font_variant_alternates, value);
    AppendValue(eCSSProperty_font_variant_caps, value);
    AppendValue(eCSSProperty_font_variant_east_asian, value);
    AppendValue(eCSSProperty_font_variant_numeric, value);
    AppendValue(eCSSProperty_font_variant_position, value);
    return true;
  }

  
  int32_t altFeatures = 0, capsFeatures = 0, eastAsianFeatures = 0,
          ligFeatures = 0, numericFeatures = 0, posFeatures = 0;
  nsCSSValue altListValue;
  nsCSSValueList* altList = nullptr;

  
  altListValue.SetListValue();

  bool foundValid = false; 
  while (GetToken(true)) {
    
    bool isFunction = (mToken.mType == eCSSToken_Function);
    if (mToken.mType != eCSSToken_Ident && !isFunction) {
      UngetToken();
      break;
    }

    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);
    if (keyword == eCSSKeyword_UNKNOWN) {
      UngetToken();
      return false;
    }

    int32_t feature;

    
    if (isFunction) {
      if (!nsCSSProps::FindKeyword(keyword,
                                   nsCSSProps::kFontVariantAlternatesFuncsKTable,
                                   feature) ||
          (feature & altFeatures)) {
        UngetToken();
        return false;
      }

      altFeatures |= feature;
      nsCSSValue funcValue;
      if (!ParseFunction(keyword, nullptr, VARIANT_IDENTIFIER, 1,
                         MaxElementsForAlternateType(keyword), funcValue) ||
          funcValue.GetUnit() != eCSSUnit_Function) {
        UngetToken();
        return false;
      }

      if (!altList) {
        altList = altListValue.GetListValue();
      } else {
        altList->mNext = new nsCSSValueList;
        altList = altList->mNext;
      }
      altList->mValue = funcValue;
    } else if (nsCSSProps::FindKeyword(keyword,
                                       nsCSSProps::kFontVariantCapsKTable,
                                       feature)) {
      if (capsFeatures != 0) {
        
        UngetToken();
        return false;
      }
      capsFeatures = feature;
    } else if (nsCSSProps::FindKeyword(keyword,
                                       nsCSSProps::kFontVariantAlternatesKTable,
                                       feature)) {
      if (feature & altFeatures) {
        
        UngetToken();
        return false;
      }
      altFeatures |= feature;
    } else if (nsCSSProps::FindKeyword(keyword,
                                       nsCSSProps::kFontVariantEastAsianKTable,
                                       feature)) {
      if (!MergeBitmaskValue(feature, maskEastAsian, eastAsianFeatures)) {
        
        UngetToken();
        return false;
      }
    } else if (nsCSSProps::FindKeyword(keyword,
                                       nsCSSProps::kFontVariantLigaturesKTable,
                                       feature)) {
      if (keyword == eCSSKeyword_none ||
          !MergeBitmaskValue(feature, maskLigatures, ligFeatures)) {
        
        UngetToken();
        return false;
      }
    } else if (nsCSSProps::FindKeyword(keyword,
                                       nsCSSProps::kFontVariantNumericKTable,
                                       feature)) {
      if (!MergeBitmaskValue(feature, maskNumeric, numericFeatures)) {
        
        UngetToken();
        return false;
      }
    } else if (nsCSSProps::FindKeyword(keyword,
                                       nsCSSProps::kFontVariantPositionKTable,
                                       feature)) {
      if (posFeatures != 0) {
        
        UngetToken();
        return false;
      }
      posFeatures = feature;
    } else {
      
      UngetToken();
      return false;
    }

    foundValid = true;
  }

  if (!foundValid) {
    return false;
  }

  if (altFeatures) {
    nsCSSValue featureValue;
    featureValue.SetIntValue(altFeatures, eCSSUnit_Enumerated);
    value.SetPairValue(featureValue, altListValue);
    AppendValue(eCSSProperty_font_variant_alternates, value);
  } else {
    AppendValue(eCSSProperty_font_variant_alternates, normal);
  }

  if (capsFeatures) {
    value.SetIntValue(capsFeatures, eCSSUnit_Enumerated);
    AppendValue(eCSSProperty_font_variant_caps, value);
  } else {
    AppendValue(eCSSProperty_font_variant_caps, normal);
  }

  if (eastAsianFeatures) {
    value.SetIntValue(eastAsianFeatures, eCSSUnit_Enumerated);
    AppendValue(eCSSProperty_font_variant_east_asian, value);
  } else {
    AppendValue(eCSSProperty_font_variant_east_asian, normal);
  }

  if (ligFeatures) {
    value.SetIntValue(ligFeatures, eCSSUnit_Enumerated);
    AppendValue(eCSSProperty_font_variant_ligatures, value);
  } else {
    AppendValue(eCSSProperty_font_variant_ligatures, normal);
  }

  if (numericFeatures) {
    value.SetIntValue(numericFeatures, eCSSUnit_Enumerated);
    AppendValue(eCSSProperty_font_variant_numeric, value);
  } else {
    AppendValue(eCSSProperty_font_variant_numeric, normal);
  }

  if (posFeatures) {
    value.SetIntValue(posFeatures, eCSSUnit_Enumerated);
    AppendValue(eCSSProperty_font_variant_position, value);
  } else {
    AppendValue(eCSSProperty_font_variant_position, normal);
  }

  return true;
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
CSSParserImpl::ParseOneFamily(nsAString& aFamily,
                              bool& aOneKeyword,
                              bool& aQuoted)
{
  if (!GetToken(true))
    return false;

  nsCSSToken* tk = &mToken;

  aOneKeyword = false;
  aQuoted = false;
  if (eCSSToken_Ident == tk->mType) {
    aOneKeyword = true;
    aFamily.Append(tk->mIdent);
    for (;;) {
      if (!GetToken(false))
        break;

      if (eCSSToken_Ident == tk->mType) {
        aOneKeyword = false;
        
        
        
        
        
        
        
        aFamily.Append(char16_t(' '));
        aFamily.Append(tk->mIdent);
      } else if (eCSSToken_Whitespace != tk->mType) {
        UngetToken();
        break;
      }
    }
    return true;

  } else if (eCSSToken_String == tk->mType) {
    aQuoted = true;
    aFamily.Append(tk->mIdent); 
    return true;

  } else {
    UngetToken();
    return false;
  }
}


static bool
AppendGeneric(nsCSSKeyword aKeyword, FontFamilyList *aFamilyList)
{
  switch (aKeyword) {
    case eCSSKeyword_serif:
      aFamilyList->Append(FontFamilyName(eFamily_serif));
      return true;
    case eCSSKeyword_sans_serif:
      aFamilyList->Append(FontFamilyName(eFamily_sans_serif));
      return true;
    case eCSSKeyword_monospace:
      aFamilyList->Append(FontFamilyName(eFamily_monospace));
      return true;
    case eCSSKeyword_cursive:
      aFamilyList->Append(FontFamilyName(eFamily_cursive));
      return true;
    case eCSSKeyword_fantasy:
      aFamilyList->Append(FontFamilyName(eFamily_fantasy));
      return true;
    case eCSSKeyword__moz_fixed:
      aFamilyList->Append(FontFamilyName(eFamily_moz_fixed));
      return true;
    default:
      break;
  }

  return false;
}

bool
CSSParserImpl::ParseFamily(nsCSSValue& aValue)
{
  nsRefPtr<css::FontFamilyListRefCnt> familyList =
    new css::FontFamilyListRefCnt();
  nsAutoString family;
  bool single, quoted;

  
  if (!ParseOneFamily(family, single, quoted))
    return false;

  
  
  bool foundGeneric = false;
  if (single) {
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(family);
    switch (keyword) {
      case eCSSKeyword_inherit:
        aValue.SetInheritValue();
        return true;
      case eCSSKeyword_default:
        
        return false;
      case eCSSKeyword_initial:
        aValue.SetInitialValue();
        return true;
      case eCSSKeyword_unset:
        if (nsLayoutUtils::UnsetValueEnabled()) {
          aValue.SetUnsetValue();
          return true;
        }
        break;
      case eCSSKeyword__moz_use_system_font:
        if (!IsParsingCompoundProperty()) {
          aValue.SetSystemFontValue();
          return true;
        }
        break;
      default:
        foundGeneric = AppendGeneric(keyword, familyList);
    }
  }

  if (!foundGeneric) {
    familyList->Append(
      FontFamilyName(family, (quoted ? eQuotedName : eUnquotedName)));
  }

  for (;;) {
    if (!ExpectSymbol(',', true))
      break;

    nsAutoString nextFamily;
    if (!ParseOneFamily(nextFamily, single, quoted))
      return false;

    
    
    foundGeneric = false;
    if (single) {
      nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(nextFamily);
      switch (keyword) {
        case eCSSKeyword_inherit:
        case eCSSKeyword_initial:
        case eCSSKeyword_default:
        case eCSSKeyword__moz_use_system_font:
          return false;
        case eCSSKeyword_unset:
          if (nsLayoutUtils::UnsetValueEnabled()) {
            return false;
          }
          break;
        default:
          foundGeneric = AppendGeneric(keyword, familyList);
          break;
      }
    }

    if (!foundGeneric) {
      familyList->Append(
        FontFamilyName(nextFamily, (quoted ? eQuotedName : eUnquotedName)));
    }
  }

  if (familyList->IsEmpty()) {
    return false;
  }

  aValue.SetFontFamilyListValue(familyList);
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
      bool single, quoted;
      if (!ParseOneFamily(family, single, quoted)) {
        SkipUntil(')');
        return false;
      }
      if (!ExpectSymbol(')', true)) {
        SkipUntil(')');
        return false;
      }

      
      if (single) {
        nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(family);
        switch (keyword) {
          case eCSSKeyword_serif:
          case eCSSKeyword_sans_serif:
          case eCSSKeyword_monospace:
          case eCSSKeyword_cursive:
          case eCSSKeyword_fantasy:
          case eCSSKeyword__moz_fixed:
            return false;
          default:
            break;
        }
      }

      cur.SetStringValue(family, eCSSUnit_Local_Font);
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
    eCSSProperty_list_style_position,
    eCSSProperty_list_style_type,
    eCSSProperty_list_style_image
  };

  nsCSSValue values[MOZ_ARRAY_LENGTH(listStyleIDs)];
  int32_t found =
    ParseChoice(values, listStyleIDs, ArrayLength(listStyleIDs));
  if (found < 1) {
    return false;
  }

  if ((found & (1|4|8)) == (1|4|8)) {
    if (values[0].GetUnit() == eCSSUnit_None) {
      
      
      
      return false;
    } else {
      NS_ASSERTION(found == (1|2|4|8) && values[0] == values[1] &&
                   values[0] == values[2] && values[0] == values[3],
                   "should be a special value");
    }
  }

  if ((found & 2) == 0) {
    values[1].SetIntValue(NS_STYLE_LIST_STYLE_POSITION_OUTSIDE,
                          eCSSUnit_Enumerated);
  }
  if ((found & 4) == 0) {
    
    nsString type = (found & 1) ?
      NS_LITERAL_STRING("none") : NS_LITERAL_STRING("disc");
    values[2].SetStringValue(type, eCSSUnit_Ident);
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
CSSParserImpl::ParseListStyleType(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_INHERIT | VARIANT_STRING, nullptr)) {
    return true;
  }

  if (ParseCounterStyleNameValue(aValue) || ParseSymbols(aValue)) {
    return true;
  }

  return false;
}

bool
CSSParserImpl::ParseMargin()
{
  static const nsCSSProperty kMarginSideIDs[] = {
    eCSSProperty_margin_top,
    eCSSProperty_margin_right,
    eCSSProperty_margin_bottom,
    eCSSProperty_margin_left
  };

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
CSSParserImpl::ParseObjectPosition()
{
  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_INHERIT, nullptr) &&
      !ParsePositionValue(value)) {
    return false;
  }
  AppendValue(eCSSProperty_object_position, value);
  return true;
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
  if (found < 1) {
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
  if (!ParseVariant(overflow, VARIANT_HK, nsCSSProps::kOverflowKTable)) {
    return false;
  }

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
    eCSSProperty_padding_right,
    eCSSProperty_padding_bottom,
    eCSSProperty_padding_left
  };

  return ParseBoxProperties(kPaddingSideIDs);
}

bool
CSSParserImpl::ParseQuotes()
{
  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_HOS, nullptr)) {
    return false;
  }
  if (value.GetUnit() == eCSSUnit_String) {
    nsCSSValue open = value;
    nsCSSValuePairList* quotes = value.SetPairListValue();
    for (;;) {
      quotes->mXValue = open;
      
      if (!ParseVariant(quotes->mYValue, VARIANT_STRING, nullptr)) {
        return false;
      }
      
      if (!ParseVariant(open, VARIANT_STRING, nullptr)) {
        break;
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
  static const nsCSSProperty kTextDecorationIDs[] = {
    eCSSProperty_text_decoration_line,
    eCSSProperty_text_decoration_style,
    eCSSProperty_text_decoration_color
  };
  const int32_t numProps = MOZ_ARRAY_LENGTH(kTextDecorationIDs);
  nsCSSValue values[numProps];

  int32_t found = ParseChoice(values, kTextDecorationIDs, numProps);
  if (found < 1) {
    return false;
  }

  
  if ((found & 1) == 0) { 
    values[0].SetIntValue(NS_STYLE_TEXT_DECORATION_LINE_NONE,
                          eCSSUnit_Enumerated);
  }
  if ((found & 2) == 0) { 
    values[1].SetIntValue(NS_STYLE_TEXT_DECORATION_STYLE_SOLID,
                          eCSSUnit_Enumerated);
  }
  if ((found & 4) == 0) { 
    values[2].SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR,
                          eCSSUnit_Enumerated);
  }

  for (int32_t index = 0; index < numProps; index++) {
    AppendValue(kTextDecorationIDs[index], values[index]);
  }
  return true;
}

bool
CSSParserImpl::ParseTextAlign(nsCSSValue& aValue, const KTableValue aTable[])
{
  if (ParseVariant(aValue, VARIANT_INHERIT, nullptr)) {
    
    return true;
  }

  nsCSSValue left;
  if (!ParseVariant(left, VARIANT_KEYWORD, aTable)) {
    return false;
  }

  if (!nsLayoutUtils::IsTextAlignTrueValueEnabled()) {
    aValue = left;
    return true;
  }

  nsCSSValue right;
  if (ParseVariant(right, VARIANT_KEYWORD, aTable)) {
    
    if (left.GetIntValue() == NS_STYLE_TEXT_ALIGN_TRUE &&
        right.GetIntValue() == NS_STYLE_TEXT_ALIGN_TRUE) {
      return false;
    }
    aValue.SetPairValue(left, right);
  } else {
    
    if (left.GetIntValue() == NS_STYLE_TEXT_ALIGN_TRUE) {
      return false;
    }
    aValue = left;
  }
  return true;
}

bool
CSSParserImpl::ParseTextAlign(nsCSSValue& aValue)
{
  return ParseTextAlign(aValue, nsCSSProps::kTextAlignKTable);
}

bool
CSSParserImpl::ParseTextAlignLast(nsCSSValue& aValue)
{
  return ParseTextAlign(aValue, nsCSSProps::kTextAlignLastKTable);
}

bool
CSSParserImpl::ParseTextDecorationLine(nsCSSValue& aValue)
{
  static_assert((NS_STYLE_TEXT_DECORATION_LINE_NONE ^
                 NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE ^
                 NS_STYLE_TEXT_DECORATION_LINE_OVERLINE ^
                 NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH ^
                 NS_STYLE_TEXT_DECORATION_LINE_BLINK ^
                 NS_STYLE_TEXT_DECORATION_LINE_PREF_ANCHORS) ==
                (NS_STYLE_TEXT_DECORATION_LINE_NONE |
                 NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE |
                 NS_STYLE_TEXT_DECORATION_LINE_OVERLINE |
                 NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH |
                 NS_STYLE_TEXT_DECORATION_LINE_BLINK |
                 NS_STYLE_TEXT_DECORATION_LINE_PREF_ANCHORS),
                "text decoration constants need to be bitmasks");
  if (ParseVariant(aValue, VARIANT_HK, nsCSSProps::kTextDecorationLineKTable)) {
    if (eCSSUnit_Enumerated == aValue.GetUnit()) {
      int32_t intValue = aValue.GetIntValue();
      if (intValue != NS_STYLE_TEXT_DECORATION_LINE_NONE) {
        
        nsCSSValue  keyword;
        int32_t index;
        for (index = 0; index < 3; index++) {
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
CSSParserImpl::ParseTouchAction(nsCSSValue& aValue)
{
  
  

  if (!ParseVariant(aValue, VARIANT_HK, nsCSSProps::kTouchActionKTable)) {
    return false;
  }

  
  
  if (eCSSUnit_Enumerated != aValue.GetUnit()) {
    return true;
  }

  int32_t intValue = aValue.GetIntValue();
  nsCSSValue nextValue;
  if (ParseEnum(nextValue, nsCSSProps::kTouchActionKTable)) {
    int32_t nextIntValue = nextValue.GetIntValue();

    
    if (nextIntValue & intValue) {
      return false;
    }

    
    if ((intValue | nextIntValue) & (NS_STYLE_TOUCH_ACTION_NONE |
                                     NS_STYLE_TOUCH_ACTION_AUTO |
                                     NS_STYLE_TOUCH_ACTION_MANIPULATION)) {
      return false;
    }

    aValue.SetIntValue(nextIntValue | intValue, eCSSUnit_Enumerated);
  }

  return true;
}

bool
CSSParserImpl::ParseTextCombineUpright(nsCSSValue& aValue)
{
  if (!ParseVariant(aValue, VARIANT_HK,
                    nsCSSProps::kTextCombineUprightKTable)) {
    return false;
  }

  
  if (eCSSUnit_Enumerated == aValue.GetUnit() &&
      aValue.GetIntValue() == NS_STYLE_TEXT_COMBINE_UPRIGHT_DIGITS_2) {
    if (!GetToken(true)) {
      return true;
    }
    if (mToken.mType == eCSSToken_Number && mToken.mIntegerValid) {
      switch (mToken.mInteger) {
        case 2:  
          break;
        case 3:
          aValue.SetIntValue(NS_STYLE_TEXT_COMBINE_UPRIGHT_DIGITS_3,
                             eCSSUnit_Enumerated);
          break;
        case 4:
          aValue.SetIntValue(NS_STYLE_TEXT_COMBINE_UPRIGHT_DIGITS_4,
                             eCSSUnit_Enumerated);
          break;
        default:
          
          return false;
      }
    } else {
      UngetToken();
    }
  }
  return true;
}








bool
CSSParserImpl::ParseFunctionInternals(const int32_t aVariantMask[],
                                      int32_t aVariantMaskAll,
                                      uint16_t aMinElems,
                                      uint16_t aMaxElems,
                                      InfallibleTArray<nsCSSValue> &aOutput)
{
  NS_ASSERTION((aVariantMask && !aVariantMaskAll) ||
               (!aVariantMask && aVariantMaskAll),
               "only one of the two variant mask parameters can be set");

  for (uint16_t index = 0; index < aMaxElems; ++index) {
    nsCSSValue newValue;
    int32_t m = aVariantMaskAll ? aVariantMaskAll : aVariantMask[index];
    if (!ParseVariant(newValue, m, nullptr)) {
      break;
    }

    aOutput.AppendElement(newValue);

    if (ExpectSymbol(',', true)) {
      
      continue;
    }

    if (ExpectSymbol(')', true)) {
      
      return (index + 1) >= aMinElems;
    }

    
    break;
  }

  
  
  SkipUntil(')');
  return false;
}




















bool
CSSParserImpl::ParseFunction(nsCSSKeyword aFunction,
                             const int32_t aAllowedTypes[],
                             int32_t aAllowedTypesAll,
                             uint16_t aMinElems, uint16_t aMaxElems,
                             nsCSSValue &aValue)
{
  NS_ASSERTION((aAllowedTypes && !aAllowedTypesAll) ||
               (!aAllowedTypes && aAllowedTypesAll),
               "only one of the two allowed type parameter can be set");
  typedef InfallibleTArray<nsCSSValue>::size_type arrlen_t;

  


  static const arrlen_t MAX_ALLOWED_ELEMS = 0xFFFE;

  







  AutoInfallibleTArray<nsCSSValue, 16> foundValues;
  if (!ParseFunctionInternals(aAllowedTypes, aAllowedTypesAll, aMinElems,
                              aMaxElems, foundValues)) {
    return false;
  }

  



  uint16_t numArgs = std::min(foundValues.Length(), MAX_ALLOWED_ELEMS);
  nsRefPtr<nsCSSValue::Array> convertedArray =
    aValue.InitFunction(aFunction, numArgs);

  
  for (uint16_t index = 0; index < numArgs; ++index)
    convertedArray->Item(index + 1) = foundValues[static_cast<arrlen_t>(index)];

  
  return true;
}













static bool GetFunctionParseInformation(nsCSSKeyword aToken,
                                        bool aIsPrefixed,
                                        uint16_t &aMinElems,
                                        uint16_t &aMaxElems,
                                        const int32_t *& aVariantMask)
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
    break;
  case eCSSKeyword_translate3d:
    
    variantIndex = eTwoLengthPercentCalcsOneLengthCalc;
    aMinElems = 3U;
    aMaxElems = 3U;
    break;
  case eCSSKeyword_scalez:
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
    break;
  case eCSSKeyword_rotatex:
  case eCSSKeyword_rotatey:
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
    break;
  case eCSSKeyword_perspective:
    
    variantIndex = ePositiveLength;
    aMinElems = 1U;
    aMaxElems = 1U;
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

bool CSSParserImpl::ParseWillChange()
{
  nsCSSValue listValue;
  nsCSSValueList* currentListValue = listValue.SetListValue();
  bool first = true;
  for (;;) {
    const uint32_t variantMask = VARIANT_IDENTIFIER |
                                 VARIANT_INHERIT |
                                 VARIANT_NONE |
                                 VARIANT_ALL |
                                 VARIANT_AUTO;
    nsCSSValue value;
    if (!ParseVariant(value, variantMask, nullptr)) {
      return false;
    }

    if (value.GetUnit() == eCSSUnit_None ||
        value.GetUnit() == eCSSUnit_All)
    {
      return false;
    }

    if (value.GetUnit() != eCSSUnit_Ident) {
      if (first) {
        AppendValue(eCSSProperty_will_change, value);
        return true;
      } else {
        return false;
      }
    }

    nsString str;
    value.GetStringValue(str);
    if (str.LowerCaseEqualsLiteral("default")) {
      return false;
    }

    currentListValue->mValue = value;

    if (!ExpectSymbol(',', true)) {
      break;
    }
    currentListValue->mNext = new nsCSSValueList;
    currentListValue = currentListValue->mNext;
    first = false;
  }

  AppendValue(eCSSProperty_will_change, listValue);
  return true;
}




bool
CSSParserImpl::ParseSingleTransform(bool aIsPrefixed, nsCSSValue& aValue)
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
                                   minElems, maxElems, variantMask))
    return false;

  return ParseFunction(keyword, variantMask, 0, minElems, maxElems, aValue);
}




bool CSSParserImpl::ParseTransform(bool aIsPrefixed)
{
  nsCSSValue value;
  
  if (!ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    nsCSSValueSharedList* list = new nsCSSValueSharedList;
    value.SetSharedListValue(list);
    list->mHead = new nsCSSValueList;
    nsCSSValueList* cur = list->mHead;
    for (;;) {
      if (!ParseSingleTransform(aIsPrefixed, cur->mValue)) {
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



bool
CSSParserImpl::ParsePolygonFunction(nsCSSValue& aValue)
{
  uint16_t numArgs = 1;

  nsCSSValue fillRuleValue;
  if (ParseEnum(fillRuleValue, nsCSSProps::kFillRuleKTable)) {
    numArgs++;

    
    if (!ExpectSymbol(',', true)) {
      REPORT_UNEXPECTED_TOKEN(PEExpectedComma);
      SkipUntil(')');
      return false;
    }
  }

  nsCSSValue coordinates;
  nsCSSValuePairList* item = coordinates.SetPairListValue();
  for (;;) {
    nsCSSValue xValue, yValue;
    if (!ParseVariant(xValue, VARIANT_LPCALC, nullptr) ||
        !ParseVariant(yValue, VARIANT_LPCALC, nullptr)) {
      REPORT_UNEXPECTED_TOKEN(PECoordinatePair);
      SkipUntil(')');
      return false;
    }
    item->mXValue = xValue;
    item->mYValue = yValue;

    
    if (!ExpectSymbol(',', true)) {
      
      if (!ExpectSymbol(')', true)) {
        REPORT_UNEXPECTED_TOKEN(PEExpectedCloseParen);
        SkipUntil(')');
        return false;
      }
      break;
    }
    item->mNext = new nsCSSValuePairList;
    item = item->mNext;
  }

  nsRefPtr<nsCSSValue::Array> functionArray =
    aValue.InitFunction(eCSSKeyword_polygon, numArgs);
  functionArray->Item(numArgs) = coordinates;
  if (numArgs > 1) {
    functionArray->Item(1) = fillRuleValue;
  }

  return true;
}

bool
CSSParserImpl::ParseCircleOrEllipseFunction(nsCSSKeyword aKeyword,
                                            nsCSSValue& aValue)
{
  nsCSSValue radiusX, radiusY, position;
  bool hasRadius = false, hasPosition = false;

  int32_t mask = VARIANT_LPCALC | VARIANT_NONNEGATIVE_DIMENSION |
                 VARIANT_KEYWORD;
  if (ParseVariant(radiusX, mask, nsCSSProps::kShapeRadiusKTable)) {
    if (aKeyword == eCSSKeyword_ellipse) {
      if (!ParseVariant(radiusY, mask, nsCSSProps::kShapeRadiusKTable)) {
        REPORT_UNEXPECTED_TOKEN(PEExpectedRadius);
        SkipUntil(')');
        return false;
      }
    }
    hasRadius = true;
  }

  if (!ExpectSymbol(')', true)) {
    if (!GetToken(true)) {
      REPORT_UNEXPECTED_EOF(PEPositionEOF);
      return false;
    }

    if (mToken.mType != eCSSToken_Ident ||
        !mToken.mIdent.LowerCaseEqualsLiteral("at") ||
        !ParsePositionValue(position)) {
      REPORT_UNEXPECTED_TOKEN(PEExpectedPosition);
      SkipUntil(')');
      return false;
    }
    if (!ExpectSymbol(')', true)) {
      REPORT_UNEXPECTED_TOKEN(PEExpectedCloseParen);
      SkipUntil(')');
      return false;
    }
    hasPosition = true;
  }

  size_t count = aKeyword == eCSSKeyword_circle ? 2 : 3;
  nsRefPtr<nsCSSValue::Array> functionArray =
    aValue.InitFunction(aKeyword, count);
  if (hasRadius) {
    functionArray->Item(1) = radiusX;
    if (aKeyword == eCSSKeyword_ellipse) {
      functionArray->Item(2) = radiusY;
    }
  }
  if (hasPosition) {
    functionArray->Item(count) = position;
  }

  return true;
}

bool
CSSParserImpl::ParseInsetFunction(nsCSSValue& aValue)
{
  nsRefPtr<nsCSSValue::Array> functionArray =
    aValue.InitFunction(eCSSKeyword_inset, 5);

  if (ParseVariant(functionArray->Item(1), VARIANT_LPCALC, nullptr)) {
    
    ParseVariant(functionArray->Item(2), VARIANT_LPCALC, nullptr) &&
    ParseVariant(functionArray->Item(3), VARIANT_LPCALC, nullptr) &&
    ParseVariant(functionArray->Item(4), VARIANT_LPCALC, nullptr);
  } else {
    REPORT_UNEXPECTED_TOKEN(PEExpectedShapeArg);
    SkipUntil(')');
    return false;
  }

  if (!ExpectSymbol(')', true)) {
    if (!GetToken(true)) {
      NS_NOTREACHED("ExpectSymbol should have returned true");
      return false;
    }

    nsRefPtr<nsCSSValue::Array> radiusArray = nsCSSValue::Array::Create(4);
    functionArray->Item(5).SetArrayValue(radiusArray, eCSSUnit_Array);
    if (mToken.mType != eCSSToken_Ident ||
        !mToken.mIdent.LowerCaseEqualsLiteral("round") ||
        !ParseBoxCornerRadiiInternals(radiusArray->ItemStorage())) {
      REPORT_UNEXPECTED_TOKEN(PEExpectedRadius);
      SkipUntil(')');
      return false;
    }

    if (!ExpectSymbol(')', true)) {
      REPORT_UNEXPECTED_TOKEN(PEExpectedCloseParen);
      SkipUntil(')');
      return false;
    }
  }

  return true;
}

bool
CSSParserImpl::ParseBasicShape(nsCSSValue& aValue, bool* aConsumedTokens)
{
  if (!GetToken(true)) {
    return false;
  }

  if (mToken.mType != eCSSToken_Function) {
    UngetToken();
    return false;
  }

  
  *aConsumedTokens = true;
  nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);
  switch (keyword) {
  case eCSSKeyword_polygon:
    return ParsePolygonFunction(aValue);
  case eCSSKeyword_circle:
  case eCSSKeyword_ellipse:
    return ParseCircleOrEllipseFunction(keyword, aValue);
  case eCSSKeyword_inset:
    return ParseInsetFunction(aValue);
  default:
    return false;
  }
}


bool CSSParserImpl::ParseClipPath()
{
  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_HUO, nullptr)) {
    if (!nsLayoutUtils::CSSClipPathShapesEnabled()) {
      
      
      REPORT_UNEXPECTED_TOKEN(PEExpectedNoneOrURL);
      return false;
    }

    nsCSSValueList* cur = value.SetListValue();

    nsCSSValue referenceBox;
    bool hasBox = ParseEnum(referenceBox, nsCSSProps::kClipShapeSizingKTable);

    nsCSSValue basicShape;
    bool basicShapeConsumedTokens = false;
    bool hasShape = ParseBasicShape(basicShape, &basicShapeConsumedTokens);

    
    
    if ((!hasShape && basicShapeConsumedTokens) || (!hasBox && !hasShape)) {
      return false;
    }

    
    if (hasBox) {
      cur->mValue = referenceBox;
    }

    if (hasShape) {
      if (hasBox) {
        cur->mNext = new nsCSSValueList;
        cur = cur->mNext;
      }
      cur->mValue = basicShape;
    }

    
    if (!hasBox &&
        ParseEnum(referenceBox, nsCSSProps::kClipShapeSizingKTable)) {
        cur->mNext = new nsCSSValueList;
        cur = cur->mNext;
        cur->mValue = referenceBox;
    }
  }

  AppendValue(eCSSProperty_clip_path, value);
  return true;
}

bool CSSParserImpl::ParseTransformOrigin(bool aPerspective)
{
  nsCSSValuePair position;
  if (!ParseBoxPositionValues(position, true))
    return false;

  nsCSSProperty prop = eCSSProperty_transform_origin;
  if (aPerspective) {
    prop = eCSSProperty_perspective_origin;
  }

  
  
  
  if (position.mXValue.GetUnit() == eCSSUnit_Inherit ||
      position.mXValue.GetUnit() == eCSSUnit_Initial ||
      position.mXValue.GetUnit() == eCSSUnit_Unset) {
    MOZ_ASSERT(position.mXValue == position.mYValue,
               "inherit/initial/unset only half?");
    AppendValue(prop, position.mXValue);
  } else {
    nsCSSValue value;
    if (aPerspective) {
      value.SetPairValue(position.mXValue, position.mYValue);
    } else {
      nsCSSValue depth;
      if (!ParseVariant(depth, VARIANT_LENGTH | VARIANT_CALC, nullptr)) {
        depth.SetFloatValue(0.0f, eCSSUnit_Pixel);
      }
      value.SetTripletValue(position.mXValue, position.mYValue, depth);
    }

    AppendValue(prop, value);
  }
  return true;
}






bool
CSSParserImpl::ParseDropShadow(nsCSSValue* aValue)
{
  
  
  nsCSSValue shadow;
  nsCSSValueList* cur = shadow.SetListValue();
  if (!ParseShadowItem(cur->mValue, false))
    return false;

  if (!ExpectSymbol(')', true))
    return false;

  nsCSSValue::Array* dropShadow = aValue->InitFunction(eCSSKeyword_drop_shadow, 1);

  
  dropShadow->Item(1) = shadow;

  return true;
}





bool
CSSParserImpl::ParseSingleFilter(nsCSSValue* aValue)
{
  if (ParseVariant(*aValue, VARIANT_URL, nullptr)) {
    return true;
  }

  if (!nsLayoutUtils::CSSFiltersEnabled()) {
    
    REPORT_UNEXPECTED_TOKEN(PEExpectedNoneOrURL);
    return false;
  }

  if (!GetToken(true)) {
    REPORT_UNEXPECTED_EOF(PEFilterEOF);
    return false;
  }

  if (mToken.mType != eCSSToken_Function) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedNoneOrURLOrFilterFunction);
    UngetToken();
    return false;
  }

  nsCSSKeyword functionName = nsCSSKeywords::LookupKeyword(mToken.mIdent);
  
  
  if (functionName == eCSSKeyword_drop_shadow) {
    if (ParseDropShadow(aValue)) {
      return true;
    } else {
      
      REPORT_UNEXPECTED_TOKEN(PEExpectedNoneOrURLOrFilterFunction);
      SkipUntil(')');
      return false;
    }
  }

  
  int32_t variantMask = VARIANT_PN;
  bool rejectNegativeArgument = true;
  bool clampArgumentToOne = false;
  switch (functionName) {
    case eCSSKeyword_blur:
      variantMask = VARIANT_LCALC | VARIANT_NONNEGATIVE_DIMENSION;
      
      rejectNegativeArgument = false;
      break;
    case eCSSKeyword_brightness:
    case eCSSKeyword_contrast:
    case eCSSKeyword_saturate:
      break;
    case eCSSKeyword_grayscale:
    case eCSSKeyword_invert:
    case eCSSKeyword_sepia:
    case eCSSKeyword_opacity:
      clampArgumentToOne = true;
      break;
    case eCSSKeyword_hue_rotate:
      variantMask = VARIANT_ANGLE;
      rejectNegativeArgument = false;
      break;
    default:
      
      REPORT_UNEXPECTED_TOKEN(PEExpectedNoneOrURLOrFilterFunction);
      SkipUntil(')');
      return false;
  }

  
  uint16_t minElems = 1U;
  uint16_t maxElems = 1U;
  uint32_t allVariants = 0;
  if (!ParseFunction(functionName, &variantMask, allVariants,
                     minElems, maxElems, *aValue)) {
    REPORT_UNEXPECTED(PEFilterFunctionArgumentsParsingError);
    return false;
  }

  
  MOZ_ASSERT(aValue->GetUnit() == eCSSUnit_Function,
             "expected a filter function");
  MOZ_ASSERT(aValue->UnitHasArrayValue(),
             "filter function should be an array");
  MOZ_ASSERT(aValue->GetArrayValue()->Count() == 2,
             "filter function should have exactly one argument");
  nsCSSValue& arg = aValue->GetArrayValue()->Item(1);

  if (rejectNegativeArgument &&
      ((arg.GetUnit() == eCSSUnit_Percent && arg.GetPercentValue() < 0.0f) ||
       (arg.GetUnit() == eCSSUnit_Number && arg.GetFloatValue() < 0.0f))) {
    REPORT_UNEXPECTED(PEExpectedNonnegativeNP);
    return false;
  }

  if (clampArgumentToOne) {
    if (arg.GetUnit() == eCSSUnit_Number &&
        arg.GetFloatValue() > 1.0f) {
      arg.SetFloatValue(1.0f, arg.GetUnit());
    } else if (arg.GetUnit() == eCSSUnit_Percent &&
               arg.GetPercentValue() > 1.0f) {
      arg.SetPercentValue(1.0f);
    }
  }

  return true;
}













bool
CSSParserImpl::ParseFilter()
{
  nsCSSValue value;
  
  if (!ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    nsCSSValueList* cur = value.SetListValue();
    while (cur) {
      if (!ParseSingleFilter(&cur->mValue)) {
        return false;
      }
      if (CheckEndProperty()) {
        break;
      }
      if (!nsLayoutUtils::CSSFiltersEnabled()) {
        
        
        REPORT_UNEXPECTED_TOKEN(PEExpectEndValue);
        return false;
      }
      cur->mNext = new nsCSSValueList;
      cur = cur->mNext;
    }
  }
  AppendValue(eCSSProperty_filter, value);
  return true;
}

bool
CSSParserImpl::ParseTransitionProperty()
{
  nsCSSValue value;
  
  if (!ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    
    
    
    
    nsCSSValueList* cur = value.SetListValue();
    for (;;) {
      if (!ParseVariant(cur->mValue, VARIANT_IDENTIFIER | VARIANT_ALL, nullptr)) {
        return false;
      }
      if (cur->mValue.GetUnit() == eCSSUnit_Ident) {
        nsDependentString str(cur->mValue.GetStringBufferValue());
        
        
        if (str.LowerCaseEqualsLiteral("none") ||
            str.LowerCaseEqualsLiteral("inherit") ||
            str.LowerCaseEqualsLiteral("initial") ||
            (str.LowerCaseEqualsLiteral("unset") &&
             nsLayoutUtils::UnsetValueEnabled())) {
          return false;
        }
      }
      if (!ExpectSymbol(',', true)) {
        break;
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
    MOZ_ASSERT(!aTail, "should not have an entry");
    entry = aContainer.SetListValue();
  } else {
    MOZ_ASSERT(!aTail->mNext, "should not have a next entry");
    MOZ_ASSERT(aContainer.GetUnit() == eCSSUnit_List, "not a list");
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

  static const size_t maxNumProperties = 8;
  MOZ_ASSERT(aNumProperties <= maxNumProperties,
             "can't handle this many properties");
  nsCSSValueList *cur[maxNumProperties];
  bool parsedProperty[maxNumProperties];

  for (size_t i = 0; i < aNumProperties; ++i) {
    cur[i] = nullptr;
  }
  bool atEOP = false; 
  for (;;) { 
    
    
    bool haveAnyProperty = false;
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
            haveAnyProperty = true;
            break; 
          }
        }
      }
      if (!foundProperty) {
        
        
        return eParseAnimationOrTransitionShorthand_Error;
      }
    }

    if (!haveAnyProperty) {
      
      return eParseAnimationOrTransitionShorthand_Error;
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
  static const uint32_t numProps = MOZ_ARRAY_LENGTH(kTransitionProperties);
  
  
  

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
    MOZ_ASSERT(kTransitionProperties[3] == eCSSProperty_transition_property,
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
        if (str.EqualsLiteral("inherit") ||
            str.EqualsLiteral("initial") ||
            (str.EqualsLiteral("unset") &&
             nsLayoutUtils::UnsetValueEnabled())) {
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
    eCSSProperty_animation_play_state,
    
    
    
    
    eCSSProperty_animation_name
  };
  static const uint32_t numProps = MOZ_ARRAY_LENGTH(kAnimationProperties);
  
  
  

  nsCSSValue initialValues[numProps];
  initialValues[0].SetFloatValue(0.0, eCSSUnit_Seconds);
  initialValues[1].SetIntValue(NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE,
                               eCSSUnit_Enumerated);
  initialValues[2].SetFloatValue(0.0, eCSSUnit_Seconds);
  initialValues[3].SetIntValue(NS_STYLE_ANIMATION_DIRECTION_NORMAL, eCSSUnit_Enumerated);
  initialValues[4].SetIntValue(NS_STYLE_ANIMATION_FILL_MODE_NONE, eCSSUnit_Enumerated);
  initialValues[5].SetFloatValue(1.0f, eCSSUnit_Number);
  initialValues[6].SetIntValue(NS_STYLE_ANIMATION_PLAY_STATE_RUNNING, eCSSUnit_Enumerated);
  initialValues[7].SetNoneValue();

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
                 xOrColor.GetUnit() == eCSSUnit_EnumColor ||
                 xOrColor.IsNumericColorUnit(),
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
  
  if (!ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    nsCSSValueList* cur = value.SetListValue();
    for (;;) {
      if (!ParseShadowItem(cur->mValue, isBoxShadow)) {
        return false;
      }
      if (!ExpectSymbol(',', true)) {
        break;
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

  if (!ParseVariant(x, VARIANT_HC | VARIANT_NONE | VARIANT_URL |
                       VARIANT_OPENTYPE_SVG_KEYWORD,
                    nsCSSProps::kContextPatternKTable)) {
    return false;
  }

  bool canHaveFallback = x.GetUnit() == eCSSUnit_URL ||
                         x.GetUnit() == eCSSUnit_Enumerated;
  if (canHaveFallback) {
    if (!ParseVariant(y, VARIANT_COLOR | VARIANT_NONE, nullptr))
      y.SetNoneValue();
  }

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

  
  if (!ParseVariant(value, VARIANT_INHERIT | VARIANT_NONE |
                           VARIANT_OPENTYPE_SVG_KEYWORD,
                    nsCSSProps::kStrokeContextValueKTable)) {
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
    AppendValue(eCSSProperty_marker_end, marker);
    AppendValue(eCSSProperty_marker_mid, marker);
    AppendValue(eCSSProperty_marker_start, marker);
    return true;
  }
  return false;
}

bool
CSSParserImpl::ParsePaintOrder()
{
  static_assert
    ((1 << NS_STYLE_PAINT_ORDER_BITWIDTH) > NS_STYLE_PAINT_ORDER_LAST_VALUE,
     "bitfield width insufficient for paint-order constants");

  static const KTableValue kPaintOrderKTable[] = {
    eCSSKeyword_normal,  NS_STYLE_PAINT_ORDER_NORMAL,
    eCSSKeyword_fill,    NS_STYLE_PAINT_ORDER_FILL,
    eCSSKeyword_stroke,  NS_STYLE_PAINT_ORDER_STROKE,
    eCSSKeyword_markers, NS_STYLE_PAINT_ORDER_MARKERS,
    eCSSKeyword_UNKNOWN,-1
  };

  static_assert(MOZ_ARRAY_LENGTH(kPaintOrderKTable) ==
                  2 * (NS_STYLE_PAINT_ORDER_LAST_VALUE + 2),
                "missing paint-order values in kPaintOrderKTable");

  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_HK, kPaintOrderKTable)) {
    return false;
  }

  uint32_t seen = 0;
  uint32_t order = 0;
  uint32_t position = 0;

  
  
  static_assert
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

    static_assert(NS_STYLE_PAINT_ORDER_NORMAL == 0,
                  "unexpected value for NS_STYLE_PAINT_ORDER_NORMAL");
    value.SetIntValue(static_cast<int32_t>(order), eCSSUnit_Enumerated);
  }

  AppendValue(eCSSProperty_paint_order, value);
  return true;
}

bool
CSSParserImpl::BackslashDropped()
{
  return mScanner->GetEOFCharacters() &
         nsCSSScanner::eEOFCharacters_DropBackslash;
}

void
CSSParserImpl::AppendImpliedEOFCharacters(nsAString& aResult)
{
  nsCSSScanner::AppendImpliedEOFCharacters(mScanner->GetEOFCharacters(),
                                           aResult);
}

bool
CSSParserImpl::ParseAll()
{
  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_INHERIT, nullptr)) {
    return false;
  }

  
  
  
  CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, eCSSProperty_all,
                                       nsCSSProps::eEnabledForAllContent) {
    AppendValue(*p, value);
  }
  return true;
}

bool
CSSParserImpl::ParseVariableDeclaration(CSSVariableDeclarations::Type* aType,
                                        nsString& aValue)
{
  CSSVariableDeclarations::Type type;
  nsString variableValue;
  bool dropBackslash;
  nsString impliedCharacters;

  
  if (!mInSupportsCondition) {
    mScanner->StartRecording();
  }
  if (!ParseValueWithVariables(&type, &dropBackslash, impliedCharacters,
                               nullptr, nullptr)) {
    if (!mInSupportsCondition) {
      mScanner->StopRecording();
    }
    return false;
  }

  if (!mInSupportsCondition) {
    if (type == CSSVariableDeclarations::eTokenStream) {
      
      mScanner->StopRecording(variableValue);
      if (dropBackslash) {
        MOZ_ASSERT(!variableValue.IsEmpty() &&
                   variableValue[variableValue.Length() - 1] == '\\');
        variableValue.Truncate(variableValue.Length() - 1);
      }
      variableValue.Append(impliedCharacters);
    } else {
      
      
      mScanner->StopRecording();
    }
  }

  if (mHavePushBack && type == CSSVariableDeclarations::eTokenStream) {
    
    
    
    MOZ_ASSERT(mToken.IsSymbol('!') ||
               mToken.IsSymbol(')') ||
               mToken.IsSymbol(';') ||
               mToken.IsSymbol(']') ||
               mToken.IsSymbol('}'));
    if (!mInSupportsCondition) {
      MOZ_ASSERT(!variableValue.IsEmpty());
      MOZ_ASSERT(variableValue[variableValue.Length() - 1] == mToken.mSymbol);
      variableValue.Truncate(variableValue.Length() - 1);
    }
  }

  *aType = type;
  aValue = variableValue;
  return true;
}

bool
CSSParserImpl::ParseScrollSnapType()
{
  nsCSSValue value;
  if (!ParseVariant(value, VARIANT_HK, nsCSSProps::kScrollSnapTypeKTable)) {
    return false;
  }
  AppendValue(eCSSProperty_scroll_snap_type_x, value);
  AppendValue(eCSSProperty_scroll_snap_type_y, value);
  return true;
}

bool
CSSParserImpl::ParseScrollSnapPoints(nsCSSValue& aValue, nsCSSProperty aPropID)
{
  if (ParseVariant(aValue, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    return true;
  }
  if (!GetToken(true)) {
    return false;
  }
  if (mToken.mType == eCSSToken_Function &&
      nsCSSKeywords::LookupKeyword(mToken.mIdent) == eCSSKeyword_repeat) {
    nsCSSValue lengthValue;
    if (!ParseNonNegativeVariant(lengthValue,
                                 VARIANT_LENGTH | VARIANT_PERCENT | VARIANT_CALC,
                                 nullptr)) {
      REPORT_UNEXPECTED(PEExpectedNonnegativeNP);
      SkipUntil(')');
      return false;
    }
    if (!ExpectSymbol(')', true)) {
      REPORT_UNEXPECTED(PEExpectedCloseParen);
      SkipUntil(')');
      return false;
    }
    nsRefPtr<nsCSSValue::Array> functionArray =
      aValue.InitFunction(eCSSKeyword_repeat, 1);
    functionArray->Item(1) = lengthValue;
    return true;
  }
  UngetToken();
  return false;
}


bool
CSSParserImpl::ParseScrollSnapDestination(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_INHERIT, nullptr)) {
    return true;
  }
  nsCSSValue itemValue;
  if (!ParsePositionValue(aValue)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedPosition);
    return false;
  }
  return true;
}



bool
CSSParserImpl::ParseScrollSnapCoordinate(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_INHERIT | VARIANT_NONE, nullptr)) {
    return true;
  }
  nsCSSValue itemValue;
  if (!ParsePositionValue(itemValue)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedPosition);
    return false;
  }
  nsCSSValueList* item = aValue.SetListValue();
  for (;;) {
    item->mValue = itemValue;
    if (!ExpectSymbol(',', true)) {
      break;
    }
    if (!ParsePositionValue(itemValue)) {
      REPORT_UNEXPECTED_TOKEN(PEExpectedPosition);
      return false;
    }
    item->mNext = new nsCSSValueList;
    item = item->mNext;
  }
  return true;
}

bool
CSSParserImpl::ParseValueWithVariables(CSSVariableDeclarations::Type* aType,
                                       bool* aDropBackslash,
                                       nsString& aImpliedCharacters,
                                       void (*aFunc)(const nsAString&, void*),
                                       void* aData)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  StopSymbolCharStack stack;

  
  
  
  nsAutoTArray<uint32_t, 16> references;

  if (!GetToken(false)) {
    
    REPORT_UNEXPECTED_EOF(PEVariableEOF);
    return false;
  }

  if (mToken.mType == eCSSToken_Symbol &&
      (mToken.mSymbol == '!' ||
       mToken.mSymbol == ')' ||
       mToken.mSymbol == ';' ||
       mToken.mSymbol == ']' ||
       mToken.mSymbol == '}')) {
    
    UngetToken();
    REPORT_UNEXPECTED_TOKEN(PEVariableEmpty);
    return false;
  }

  if (mToken.mType == eCSSToken_Whitespace) {
    if (!GetToken(true)) {
      
      MOZ_ASSERT(!BackslashDropped());
      *aType = CSSVariableDeclarations::eTokenStream;
      *aDropBackslash = false;
      AppendImpliedEOFCharacters(aImpliedCharacters);
      return true;
    }
  }

  
  
  CSSVariableDeclarations::Type type = CSSVariableDeclarations::eTokenStream;
  if (mToken.mType == eCSSToken_Ident) {
    if (mToken.mIdent.LowerCaseEqualsLiteral("initial")) {
      type = CSSVariableDeclarations::eInitial;
    } else if (mToken.mIdent.LowerCaseEqualsLiteral("inherit")) {
      type = CSSVariableDeclarations::eInherit;
    } else if (mToken.mIdent.LowerCaseEqualsLiteral("unset")) {
      type = CSSVariableDeclarations::eUnset;
    }
  }

  if (type != CSSVariableDeclarations::eTokenStream) {
    if (!GetToken(true)) {
      
      MOZ_ASSERT(!BackslashDropped());
      *aType = type;
      *aDropBackslash = false;
      AppendImpliedEOFCharacters(aImpliedCharacters);
      return true;
    }
    UngetToken();
    if (mToken.mType == eCSSToken_Symbol &&
        (mToken.mSymbol == '!' ||
         mToken.mSymbol == ')' ||
         mToken.mSymbol == ';' ||
         mToken.mSymbol == ']' ||
         mToken.mSymbol == '}')) {
      
      
      MOZ_ASSERT(!BackslashDropped());
      *aType = type;
      *aDropBackslash = false;
      return true;
    }
  }

  do {
    switch (mToken.mType) {
      case eCSSToken_Symbol:
        if (mToken.mSymbol == '(') {
          stack.AppendElement(')');
        } else if (mToken.mSymbol == '[') {
          stack.AppendElement(']');
        } else if (mToken.mSymbol == '{') {
          stack.AppendElement('}');
        } else if (mToken.mSymbol == ';' ||
                   mToken.mSymbol == '!') {
          if (stack.IsEmpty()) {
            UngetToken();
            MOZ_ASSERT(!BackslashDropped());
            *aType = CSSVariableDeclarations::eTokenStream;
            *aDropBackslash = false;
            return true;
          } else if (!references.IsEmpty() &&
                     references.LastElement() == stack.Length() - 1) {
            REPORT_UNEXPECTED_TOKEN(PEInvalidVariableTokenFallback);
            SkipUntilAllOf(stack);
            return false;
          }
        } else if (mToken.mSymbol == ')' ||
                   mToken.mSymbol == ']' ||
                   mToken.mSymbol == '}') {
          for (;;) {
            if (stack.IsEmpty()) {
              UngetToken();
              MOZ_ASSERT(!BackslashDropped());
              *aType = CSSVariableDeclarations::eTokenStream;
              *aDropBackslash = false;
              return true;
            }
            char16_t c = stack.LastElement();
            stack.TruncateLength(stack.Length() - 1);
            if (!references.IsEmpty() &&
                references.LastElement() == stack.Length()) {
              references.TruncateLength(references.Length() - 1);
            }
            if (mToken.mSymbol == c) {
              break;
            }
          }
        }
        break;

      case eCSSToken_Function:
        if (mToken.mIdent.LowerCaseEqualsLiteral("var")) {
          if (!GetToken(true)) {
            
            REPORT_UNEXPECTED_EOF(PEExpectedVariableNameEOF);
            return false;
          }
          if (mToken.mType != eCSSToken_Ident ||
              !nsCSSProps::IsCustomPropertyName(mToken.mIdent)) {
            
            
            UngetToken();
            REPORT_UNEXPECTED_TOKEN(PEExpectedVariableName);
            SkipUntil(')');
            SkipUntilAllOf(stack);
            return false;
          }
          if (aFunc) {
            MOZ_ASSERT(Substring(mToken.mIdent, 0,
                                 CSS_CUSTOM_NAME_PREFIX_LENGTH).
                         EqualsLiteral("--"));
            
            const nsDependentSubstring varName =
              Substring(mToken.mIdent, CSS_CUSTOM_NAME_PREFIX_LENGTH);
            aFunc(varName, aData);
          }
          if (!GetToken(true)) {
            
            stack.AppendElement(')');
          } else if (mToken.IsSymbol(',')) {
            
            if (!GetToken(false) || mToken.IsSymbol(')')) {
              
              REPORT_UNEXPECTED(PEExpectedVariableFallback);
              SkipUntilAllOf(stack);
              return false;
            }
            UngetToken();
            references.AppendElement(stack.Length());
            stack.AppendElement(')');
          } else if (mToken.IsSymbol(')')) {
            
          } else {
            
            REPORT_UNEXPECTED_TOKEN(PEExpectedVariableCommaOrCloseParen);
            SkipUntil(')');
            SkipUntilAllOf(stack);
            return false;
          }
        } else {
          stack.AppendElement(')');
        }
        break;

      case eCSSToken_Bad_String:
        SkipUntilAllOf(stack);
        return false;

      case eCSSToken_Bad_URL:
        SkipUntil(')');
        SkipUntilAllOf(stack);
        return false;

      default:
        break;
    }
  } while (GetToken(true));

  
  *aDropBackslash = BackslashDropped();
  AppendImpliedEOFCharacters(aImpliedCharacters);
  uint32_t i = stack.Length();
  while (i--) {
    aImpliedCharacters.Append(stack[i]);
  }

  *aType = type;
  return true;
}

bool
CSSParserImpl::IsValueValidForProperty(const nsCSSProperty aPropID,
                                       const nsAString& aPropValue)
{
  mData.AssertInitialState();
  mTempData.AssertInitialState();

  nsCSSScanner scanner(aPropValue, 0);
  css::ErrorReporter reporter(scanner, mSheet, mChildLoader, nullptr);
  InitScanner(scanner, reporter, nullptr, nullptr, nullptr);

  
  
  
  
  
  
  AutoRestore<bool> autoRestore(mSheetPrincipalRequired);
  mSheetPrincipalRequired = false;

  nsAutoSuppressErrors suppressErrors(this);

  mSection = eCSSSection_General;
  scanner.SetSVGMode(false);

  
  if (eCSSProperty_UNKNOWN == aPropID) {
    ReleaseScanner();
    return false;
  }

  
  bool parsedOK = ParseProperty(aPropID);

  
  parsedOK = parsedOK && ParsePriority() != ePriority_Error;

  
  parsedOK = parsedOK && !GetToken(true);

  mTempData.ClearProperty(aPropID);
  mTempData.AssertInitialState();
  mData.AssertInitialState();

  CLEAR_ERROR();
  ReleaseScanner();

  return parsedOK;
}

} 



static CSSParserImpl* gFreeList = nullptr;

 void
nsCSSParser::Startup()
{
  Preferences::AddBoolVarCache(&sOpentypeSVGEnabled,
                               "gfx.font_rendering.opentype_svg.enabled");
  Preferences::AddBoolVarCache(&sUnprefixingServiceEnabled,
                               "layout.css.unprefixing-service.enabled");
}

nsCSSParser::nsCSSParser(mozilla::css::Loader* aLoader,
                         CSSStyleSheet* aSheet)
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
nsCSSParser::SetStyleSheet(CSSStyleSheet* aSheet)
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

void
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
  static_cast<CSSParserImpl*>(mImpl)->
    ParseProperty(aPropID, aPropValue, aSheetURI, aBaseURI,
                  aSheetPrincipal, aDeclaration, aChanged,
                  aIsImportant, aIsSVGMode);
}

void
nsCSSParser::ParseVariable(const nsAString&    aVariableName,
                           const nsAString&    aPropValue,
                           nsIURI*             aSheetURI,
                           nsIURI*             aBaseURI,
                           nsIPrincipal*       aSheetPrincipal,
                           css::Declaration*   aDeclaration,
                           bool*               aChanged,
                           bool                aIsImportant)
{
  static_cast<CSSParserImpl*>(mImpl)->
    ParseVariable(aVariableName, aPropValue, aSheetURI, aBaseURI,
                  aSheetPrincipal, aDeclaration, aChanged, aIsImportant);
}

void
nsCSSParser::ParseMediaList(const nsSubstring& aBuffer,
                            nsIURI*            aURI,
                            uint32_t           aLineNumber,
                            nsMediaList*       aMediaList,
                            bool               aHTMLMode)
{
  static_cast<CSSParserImpl*>(mImpl)->
    ParseMediaList(aBuffer, aURI, aLineNumber, aMediaList, aHTMLMode);
}

bool
nsCSSParser::ParseSourceSizeList(const nsAString& aBuffer,
                                 nsIURI* aURI,
                                 uint32_t aLineNumber,
                                 InfallibleTArray< nsAutoPtr<nsMediaQuery> >& aQueries,
                                 InfallibleTArray<nsCSSValue>& aValues,
                                 bool aHTMLMode)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseSourceSizeList(aBuffer, aURI, aLineNumber, aQueries, aValues,
                        aHTMLMode);
}

bool
nsCSSParser::ParseFontFamilyListString(const nsSubstring& aBuffer,
                                       nsIURI*            aURI,
                                       uint32_t           aLineNumber,
                                       nsCSSValue&        aValue)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseFontFamilyListString(aBuffer, aURI, aLineNumber, aValue);
}

bool
nsCSSParser::ParseColorString(const nsSubstring& aBuffer,
                              nsIURI*            aURI,
                              uint32_t           aLineNumber,
                              nsCSSValue&        aValue,
                              bool               aSuppressErrors )
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseColorString(aBuffer, aURI, aLineNumber, aValue, aSuppressErrors);
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

bool
nsCSSParser::EnumerateVariableReferences(const nsAString& aPropertyValue,
                                         VariableEnumFunc aFunc,
                                         void* aData)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    EnumerateVariableReferences(aPropertyValue, aFunc, aData);
}

bool
nsCSSParser::ResolveVariableValue(const nsAString& aPropertyValue,
                                  const CSSVariableValues* aVariables,
                                  nsString& aResult,
                                  nsCSSTokenSerializationType& aFirstToken,
                                  nsCSSTokenSerializationType& aLastToken)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ResolveVariableValue(aPropertyValue, aVariables,
                         aResult, aFirstToken, aLastToken);
}

void
nsCSSParser::ParsePropertyWithVariableReferences(
                                            nsCSSProperty aPropertyID,
                                            nsCSSProperty aShorthandPropertyID,
                                            const nsAString& aValue,
                                            const CSSVariableValues* aVariables,
                                            nsRuleData* aRuleData,
                                            nsIURI* aDocURL,
                                            nsIURI* aBaseURL,
                                            nsIPrincipal* aDocPrincipal,
                                            CSSStyleSheet* aSheet,
                                            uint32_t aLineNumber,
                                            uint32_t aLineOffset)
{
  static_cast<CSSParserImpl*>(mImpl)->
    ParsePropertyWithVariableReferences(aPropertyID, aShorthandPropertyID,
                                        aValue, aVariables, aRuleData, aDocURL,
                                        aBaseURL, aDocPrincipal, aSheet,
                                        aLineNumber, aLineOffset);
}

bool
nsCSSParser::ParseCounterStyleName(const nsAString& aBuffer,
                                   nsIURI* aURL,
                                   nsAString& aName)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseCounterStyleName(aBuffer, aURL, aName);
}

bool
nsCSSParser::ParseCounterDescriptor(nsCSSCounterDesc aDescID,
                                    const nsAString& aBuffer,
                                    nsIURI* aSheetURL,
                                    nsIURI* aBaseURL,
                                    nsIPrincipal* aSheetPrincipal,
                                    nsCSSValue& aValue)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseCounterDescriptor(aDescID, aBuffer,
                           aSheetURL, aBaseURL, aSheetPrincipal, aValue);
}

bool
nsCSSParser::ParseFontFaceDescriptor(nsCSSFontDesc aDescID,
                                     const nsAString& aBuffer,
                                     nsIURI* aSheetURL,
                                     nsIURI* aBaseURL,
                                     nsIPrincipal* aSheetPrincipal,
                                     nsCSSValue& aValue)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    ParseFontFaceDescriptor(aDescID, aBuffer,
                           aSheetURL, aBaseURL, aSheetPrincipal, aValue);
}

bool
nsCSSParser::IsValueValidForProperty(const nsCSSProperty aPropID,
                                     const nsAString&    aPropValue)
{
  return static_cast<CSSParserImpl*>(mImpl)->
    IsValueValidForProperty(aPropID, aPropValue);
}
