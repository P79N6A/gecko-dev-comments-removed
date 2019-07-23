













































#include "nsICSSParser.h"
#include "nsCSSProps.h"
#include "nsCSSKeywords.h"
#include "nsCSSScanner.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleRule.h"
#include "nsICSSImportRule.h"
#include "nsCSSRules.h"
#include "nsICSSNameSpaceRule.h"
#include "nsIUnicharInputStream.h"
#include "nsICSSStyleSheet.h"
#include "nsCSSDeclaration.h"
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
#include "nsContentErrors.h"
#include "nsPrintfCString.h"
#include "nsIMediaList.h"
#include "nsILookAndFeel.h"
#include "nsStyleUtil.h"
#include "nsIPrincipal.h"
#include "prprf.h"
#include "math.h"
#include "nsContentUtils.h"
#include "nsDOMError.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "prlog.h"


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
#define VARIANT_AUTO            0x010000  // A
#define VARIANT_INHERIT         0x020000  // H eCSSUnit_Initial, eCSSUnit_Inherit
#define VARIANT_NONE            0x040000  // O
#define VARIANT_NORMAL          0x080000  // M
#define VARIANT_SYSFONT         0x100000  // eCSSUnit_System_Font
#define VARIANT_GRADIENT        0x200000  // eCSSUnit_Gradient


#define VARIANT_AL   (VARIANT_AUTO | VARIANT_LENGTH)
#define VARIANT_LP   (VARIANT_LENGTH | VARIANT_PERCENT)
#define VARIANT_AH   (VARIANT_AUTO | VARIANT_INHERIT)
#define VARIANT_AHLP (VARIANT_AH | VARIANT_LP)
#define VARIANT_AHI  (VARIANT_AH | VARIANT_INTEGER)
#define VARIANT_AHK  (VARIANT_AH | VARIANT_KEYWORD)
#define VARIANT_AHKLP (VARIANT_AHLP | VARIANT_KEYWORD)
#define VARIANT_AUK  (VARIANT_AUTO | VARIANT_URL | VARIANT_KEYWORD)
#define VARIANT_AHUK (VARIANT_AH | VARIANT_URL | VARIANT_KEYWORD)
#define VARIANT_AHL  (VARIANT_AH | VARIANT_LENGTH)
#define VARIANT_AHKL (VARIANT_AHK | VARIANT_LENGTH)
#define VARIANT_HK   (VARIANT_INHERIT | VARIANT_KEYWORD)
#define VARIANT_HKF  (VARIANT_HK | VARIANT_FREQUENCY)
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
#define VARIANT_HMKI (VARIANT_HMK | VARIANT_INTEGER)
#define VARIANT_HC   (VARIANT_INHERIT | VARIANT_COLOR)
#define VARIANT_HCK  (VARIANT_HK | VARIANT_COLOR)
#define VARIANT_HUO  (VARIANT_INHERIT | VARIANT_URL | VARIANT_NONE)
#define VARIANT_AHUO (VARIANT_AUTO | VARIANT_HUO)
#define VARIANT_HPN  (VARIANT_INHERIT | VARIANT_PERCENT | VARIANT_NUMBER)
#define VARIANT_HOK  (VARIANT_HK | VARIANT_NONE)
#define VARIANT_HN   (VARIANT_INHERIT | VARIANT_NUMBER)
#define VARIANT_HON  (VARIANT_HN | VARIANT_NONE)
#define VARIANT_HOS  (VARIANT_INHERIT | VARIANT_NONE | VARIANT_STRING)




class CSSParserImpl : public nsICSSParser {
public:
  CSSParserImpl();
  virtual ~CSSParserImpl();

  NS_DECL_ISUPPORTS

  NS_IMETHOD SetStyleSheet(nsICSSStyleSheet* aSheet);

  NS_IMETHOD SetCaseSensitive(PRBool aCaseSensitive);

  NS_IMETHOD SetQuirkMode(PRBool aQuirkMode);

#ifdef  MOZ_SVG
  NS_IMETHOD SetSVGMode(PRBool aSVGMode);
#endif

  NS_IMETHOD SetChildLoader(nsICSSLoader* aChildLoader);

  NS_IMETHOD Parse(nsIUnicharInputStream* aInput,
                   nsIURI*                aSheetURI,
                   nsIURI*                aBaseURI,
                   nsIPrincipal*          aSheetPrincipal,
                   PRUint32               aLineNumber,
                   PRBool                 aAllowUnsafeRules);

  NS_IMETHOD ParseStyleAttribute(const nsAString&  aAttributeValue,
                                 nsIURI*           aDocURL,
                                 nsIURI*           aBaseURL,
                                 nsIPrincipal*     aNodePrincipal,
                                 nsICSSStyleRule** aResult);

  NS_IMETHOD ParseAndAppendDeclaration(const nsAString&  aBuffer,
                                       nsIURI*           aSheetURL,
                                       nsIURI*           aBaseURL,
                                       nsIPrincipal*     aSheetPrincipal,
                                       nsCSSDeclaration* aDeclaration,
                                       PRBool            aParseOnlyOneDecl,
                                       PRBool*           aChanged,
                                       PRBool            aClearOldDecl);

  NS_IMETHOD ParseRule(const nsAString&        aRule,
                       nsIURI*                 aSheetURL,
                       nsIURI*                 aBaseURL,
                       nsIPrincipal*           aSheetPrincipal,
                       nsCOMArray<nsICSSRule>& aResult);

  NS_IMETHOD ParseProperty(const nsCSSProperty aPropID,
                           const nsAString& aPropValue,
                           nsIURI* aSheetURL,
                           nsIURI* aBaseURL,
                           nsIPrincipal* aSheetPrincipal,
                           nsCSSDeclaration* aDeclaration,
                           PRBool* aChanged);

  NS_IMETHOD ParseMediaList(const nsSubstring& aBuffer,
                            nsIURI* aURL, 
                            PRUint32 aLineNumber, 
                            nsMediaList* aMediaList,
                            PRBool aHTMLMode);

  NS_IMETHOD ParseColorString(const nsSubstring& aBuffer,
                              nsIURI* aURL, 
                              PRUint32 aLineNumber, 
                              nscolor* aColor);

  NS_IMETHOD ParseSelectorString(const nsSubstring& aSelectorString,
                                 nsIURI* aURL, 
                                 PRUint32 aLineNumber, 
                                 nsCSSSelectorList **aSelectorList);

  void AppendRule(nsICSSRule* aRule);

protected:
  class nsAutoParseCompoundProperty;
  friend class nsAutoParseCompoundProperty;

  



  class nsAutoParseCompoundProperty {
    public:
      nsAutoParseCompoundProperty(CSSParserImpl* aParser) : mParser(aParser)
      {
        NS_ASSERTION(!aParser->IsParsingCompoundProperty(),
                     "already parsing compound property");
        NS_ASSERTION(aParser, "Null parser?");
        aParser->SetParsingCompoundProperty(PR_TRUE);
      }

      ~nsAutoParseCompoundProperty()
      {
        mParser->SetParsingCompoundProperty(PR_FALSE);
      }
    private:
      CSSParserImpl* mParser;
  };

  void InitScanner(nsIUnicharInputStream* aInput, nsIURI* aSheetURI,
                   PRUint32 aLineNumber, nsIURI* aBaseURI,
                   nsIPrincipal* aSheetPrincipal);
  
  void InitScanner(const nsSubstring& aString, nsIURI* aSheetURI,
                   PRUint32 aLineNumber, nsIURI* aBaseURI,
                   nsIPrincipal* aSheetPrincipal);
  void ReleaseScanner(void);
#ifdef MOZ_SVG
  PRBool IsSVGMode() const {
    return mScanner.IsSVGMode();
  }
#endif

  PRBool GetToken(PRBool aSkipWS);
  void UngetToken();

  
  
  
  
  
  
  
  PRBool GetURLInParens(nsString& aURL);

  void AssertInitialState() {
    NS_PRECONDITION(!mHTMLMediaMode, "Bad initial state");
    NS_PRECONDITION(!mUnresolvablePrefixException, "Bad initial state");
    NS_PRECONDITION(!mParsingCompoundProperty, "Bad initial state");
  }

  PRBool ExpectSymbol(PRUnichar aSymbol, PRBool aSkipWS);
  PRBool ExpectEndProperty();
  PRBool CheckEndProperty();
  nsSubstring* NextIdent();
  void SkipUntil(PRUnichar aStopSymbol);
  void SkipUntilOneOf(const PRUnichar* aStopSymbolChars);
  void SkipRuleSet(PRBool aInsideBraces);
  PRBool SkipAtRule();
  PRBool SkipDeclaration(PRBool aCheckForBraces);
  PRBool GetNonCloseParenToken(PRBool aSkipWS);

  PRBool PushGroup(nsICSSGroupRule* aRule);
  void PopGroup(void);

  PRBool ParseRuleSet(RuleAppendFunc aAppendFunc, void* aProcessData,
                      PRBool aInsideBraces = PR_FALSE);
  PRBool ParseAtRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  PRBool ParseCharsetRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  PRBool ParseImportRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  PRBool GatherURL(nsString& aURL);
  PRBool GatherMedia(nsMediaList* aMedia,
                     PRUnichar aStopSymbol);
  PRBool ParseMediaQuery(PRUnichar aStopSymbol, nsMediaQuery **aQuery,
                         PRBool *aParsedSomething, PRBool *aHitStop);
  PRBool ParseMediaQueryExpression(nsMediaQuery* aQuery);
  PRBool ProcessImport(const nsString& aURLSpec,
                       nsMediaList* aMedia,
                       RuleAppendFunc aAppendFunc,
                       void* aProcessData);
  PRBool ParseGroupRule(nsICSSGroupRule* aRule, RuleAppendFunc aAppendFunc,
                        void* aProcessData);
  PRBool ParseMediaRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  PRBool ParseMozDocumentRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  PRBool ParseNameSpaceRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  PRBool ProcessNameSpace(const nsString& aPrefix,
                          const nsString& aURLSpec, RuleAppendFunc aAppendFunc,
                          void* aProcessData);

  PRBool ParseFontFaceRule(RuleAppendFunc aAppendFunc, void* aProcessData);
  PRBool ParseFontDescriptor(nsCSSFontFaceRule* aRule);
  PRBool ParseFontDescriptorValue(nsCSSFontDesc aDescID,
                                  nsCSSValue& aValue);

  PRBool ParsePageRule(RuleAppendFunc aAppendFunc, void* aProcessData);

  enum nsSelectorParsingStatus {
    
    
    eSelectorParsingStatus_Done,
    
    eSelectorParsingStatus_Continue,
    
    eSelectorParsingStatus_Empty,
    
    
    eSelectorParsingStatus_Error
  };
  nsSelectorParsingStatus ParseIDSelector(PRInt32&       aDataMask,
                                          nsCSSSelector& aSelector);

  nsSelectorParsingStatus ParseClassSelector(PRInt32&       aDataMask,
                                             nsCSSSelector& aSelector);

  nsSelectorParsingStatus ParsePseudoSelector(PRInt32&       aDataMask,
                                              nsCSSSelector& aSelector,
                                              PRBool         aIsNegated);

  nsSelectorParsingStatus ParseAttributeSelector(PRInt32&       aDataMask,
                                                 nsCSSSelector& aSelector);

  nsSelectorParsingStatus ParseTypeOrUniversalSelector(PRInt32&       aDataMask,
                                                       nsCSSSelector& aSelector,
                                                       PRBool         aIsNegated);

  nsSelectorParsingStatus ParsePseudoClassWithIdentArg(nsCSSSelector& aSelector,
                                                       nsIAtom*       aPseudo);

  nsSelectorParsingStatus ParsePseudoClassWithNthPairArg(nsCSSSelector& aSelector,
                                                         nsIAtom*       aPseudo);

  nsSelectorParsingStatus ParseNegatedSimpleSelector(PRInt32&       aDataMask,
                                                     nsCSSSelector& aSelector);

  nsSelectorParsingStatus ParseSelector(nsCSSSelector& aSelectorResult);

  
  
  PRBool ParseSelectorList(nsCSSSelectorList*& aListHead,
                           PRBool aTerminateAtBrace);
  PRBool ParseSelectorGroup(nsCSSSelectorList*& aListHead);
  nsCSSDeclaration* ParseDeclarationBlock(PRBool aCheckForBraces);
  PRBool ParseDeclaration(nsCSSDeclaration* aDeclaration,
                          PRBool aCheckForBraces,
                          PRBool aMustCallValueAppended,
                          PRBool* aChanged);
  
  
  void ClearTempData(nsCSSProperty aPropID);
  
  
  
  
  
  void TransferTempData(nsCSSDeclaration* aDeclaration,
                        nsCSSProperty aPropID, PRBool aIsImportant,
                        PRBool aMustCallValueAppended,
                        PRBool* aChanged);
  void DoTransferTempData(nsCSSDeclaration* aDeclaration,
                          nsCSSProperty aPropID, PRBool aIsImportant,
                          PRBool aMustCallValueAppended,
                          PRBool* aChanged);
  
  
  
  
  void CopyValue(void *aSource, void *aDest, nsCSSProperty aPropID,
                 PRBool* aChanged);
  PRBool ParseProperty(nsCSSProperty aPropID);
  PRBool ParseSingleValueProperty(nsCSSValue& aValue,
                                  nsCSSProperty aPropID);

#ifdef MOZ_XUL
  PRBool ParseTreePseudoElement(nsCSSSelector& aSelector);
#endif

  void InitBoxPropsAsPhysical(const nsCSSProperty *aSourceProperties);

  
  PRBool ParseAzimuth(nsCSSValue& aValue);
  PRBool ParseBackground();

  struct BackgroundItem;
  friend struct BackgroundItem;
  struct BackgroundItem {
    nsCSSValue mImage;
    nsCSSValue mRepeat;
    nsCSSValue mAttachment;
    nsCSSValuePair mPosition;
    nsCSSValue mClip;
    nsCSSValue mOrigin;
    nsCSSValuePair mSize;
    
    
    PRBool mLastItem;
  };
  struct BackgroundItemSimpleValueInfo {
    nsCSSValue BackgroundItem::*member;
    nsCSSProperty propID;
  };

  PRBool ParseBackgroundItem(BackgroundItem& aItem, PRBool aFirstItem);

  PRBool ParseBackgroundList(nsCSSProperty aPropID); 
  PRBool ParseBackgroundPosition();
  PRBool ParseBoxPositionValues(nsCSSValuePair& aOut, PRBool aAcceptsInherit);
  PRBool ParseBackgroundSize();
  PRBool ParseBackgroundSizeValues(nsCSSValuePair& aOut);
  PRBool ParseBorderColor();
  PRBool ParseBorderColors(nsCSSValueList** aResult,
                           nsCSSProperty aProperty);
  PRBool ParseBorderImage();
  PRBool ParseBorderSpacing();
  PRBool ParseBorderSide(const nsCSSProperty aPropIDs[],
                         PRBool aSetAllSides);
  PRBool ParseDirectionalBorderSide(const nsCSSProperty aPropIDs[],
                                    PRInt32 aSourceType);
  PRBool ParseBorderStyle();
  PRBool ParseBorderWidth();
  
  PRBool ParseRect(nsCSSRect& aRect,
                   nsCSSProperty aPropID);
  PRBool DoParseRect(nsCSSRect& aRect);
  PRBool ParseContent();
  PRBool ParseCounterData(nsCSSValuePairList** aResult,
                          nsCSSProperty aPropID);
  PRBool ParseCue();
  PRBool ParseCursor();
  PRBool ParseFont();
  PRBool ParseFontWeight(nsCSSValue& aValue);
  PRBool ParseOneFamily(nsAString& aValue);
  PRBool ParseFamily(nsCSSValue& aValue);
  PRBool ParseFontSrc(nsCSSValue& aValue);
  PRBool ParseFontSrcFormat(nsTArray<nsCSSValue>& values);
  PRBool ParseFontRanges(nsCSSValue& aValue);
  PRBool ParseListStyle();
  PRBool ParseMargin();
  PRBool ParseMarks(nsCSSValue& aValue);
  PRBool ParseMozTransform();
  PRBool ParseOutline();
  PRBool ParseOverflow();
  PRBool ParsePadding();
  PRBool ParsePause();
  PRBool ParseQuotes();
  PRBool ParseSize();
  PRBool ParseTextDecoration(nsCSSValue& aValue);

  nsCSSValueList* ParseCSSShadowList(PRBool aIsBoxShadow);
  PRBool ParseTextShadow();
  PRBool ParseBoxShadow();

#ifdef MOZ_SVG
  PRBool ParsePaint(nsCSSValuePair* aResult,
                    nsCSSProperty aPropID);
  PRBool ParseDasharray();
  PRBool ParseMarker();
#endif

  
  void AppendValue(nsCSSProperty aPropID, const nsCSSValue& aValue);
  PRBool ParseBoxProperties(nsCSSRect& aResult,
                            const nsCSSProperty aPropIDs[]);
  PRBool ParseDirectionalBoxProperty(nsCSSProperty aProperty,
                                     PRInt32 aSourceType);
  PRBool ParseBoxCornerRadius(const nsCSSProperty aPropID);
  PRBool ParseBoxCornerRadii(nsCSSCornerSizes& aRadii,
                             const nsCSSProperty aPropIDs[]);
  PRInt32 ParseChoice(nsCSSValue aValues[],
                      const nsCSSProperty aPropIDs[], PRInt32 aNumIDs);
  PRBool ParseColor(nsCSSValue& aValue);
  PRBool ParseColorComponent(PRUint8& aComponent,
                             PRInt32& aType, char aStop);
  
  
  PRBool ParseHSLColor(nscolor& aColor, char aStop);
  
  
  PRBool ParseColorOpacity(PRUint8& aOpacity);
  PRBool ParseEnum(nsCSSValue& aValue, const PRInt32 aKeywordTable[]);
  PRBool ParseVariant(nsCSSValue& aValue,
                      PRInt32 aVariantMask,
                      const PRInt32 aKeywordTable[]);
  PRBool ParseNonNegativeVariant(nsCSSValue& aValue,
                                 PRInt32 aVariantMask,
                                 const PRInt32 aKeywordTable[]);
  PRBool ParsePositiveNonZeroVariant(nsCSSValue& aValue,
                                     PRInt32 aVariantMask,
                                     const PRInt32 aKeywordTable[]);
  PRBool ParseCounter(nsCSSValue& aValue);
  PRBool ParseAttr(nsCSSValue& aValue);
  PRBool ParseURL(nsCSSValue& aValue);
  PRBool TranslateDimension(nsCSSValue& aValue, PRInt32 aVariantMask,
                            float aNumber, const nsString& aUnit);
  PRBool ParseGradientStop(nsCSSValueGradient* aGradient);
  PRBool ParseGradient(nsCSSValue& aValue, PRBool aIsRadial);

  void SetParsingCompoundProperty(PRBool aBool) {
    NS_ASSERTION(aBool == PR_TRUE || aBool == PR_FALSE, "bad PRBool value");
    mParsingCompoundProperty = aBool;
  }
  PRBool IsParsingCompoundProperty(void) const {
    return mParsingCompoundProperty;
  }

  
  PRBool ReadSingleTransform(nsCSSValueList**& aTail);
  PRBool ParseFunction(const nsString &aFunction, const PRInt32 aAllowedTypes[],
                       PRUint16 aMinElems, PRUint16 aMaxElems,
                       nsCSSValue &aValue);
  PRBool ParseFunctionInternals(const PRInt32 aVariantMask[],
                                PRUint16 aMinElems,
                                PRUint16 aMaxElems,
                                nsTArray<nsCSSValue>& aOutput);

  
  PRBool ParseMozTransformOrigin();


  








  PRBool GetNamespaceIdForPrefix(const nsString& aPrefix,
                                 PRInt32* aNameSpaceID);

  
  void SetDefaultNamespaceOnSelector(nsCSSSelector& aSelector);

  
  
  nsCSSToken mToken;

  
  nsCSSScanner mScanner;

  
  nsCOMPtr<nsIURI> mBaseURL;

  
  nsCOMPtr<nsIURI> mSheetURL;

  
  nsCOMPtr<nsIPrincipal> mSheetPrincipal;

  
  nsCOMPtr<nsICSSStyleSheet> mSheet;

  
  nsICSSLoader* mChildLoader; 

  
  
  
  
  enum nsCSSSection {
    eCSSSection_Charset,
    eCSSSection_Import,
    eCSSSection_NameSpace,
    eCSSSection_General
  };
  nsCSSSection  mSection;

  nsXMLNameSpaceMap *mNameSpaceMap;  

  
  
  PRPackedBool mHavePushBack : 1;

  
  PRPackedBool  mNavQuirkMode : 1;

  
  PRPackedBool mUnsafeRulesEnabled : 1;

  
  
  PRPackedBool mHTMLMediaMode : 1;

  
  PRPackedBool  mCaseSensitive : 1;

  
  
  PRPackedBool  mParsingCompoundProperty : 1;

  
  
  PRPackedBool  mUnresolvablePrefixException : 1;

  
  nsCOMArray<nsICSSGroupRule> mGroupStack;

  
  
  
  
  
  nsCSSExpandedDataBlock mTempData;

  
  nsCSSExpandedDataBlock mData;

#ifdef DEBUG
  PRPackedBool mScannerInited;
#endif
};

static void AppendRuleToArray(nsICSSRule* aRule, void* aArray)
{
  static_cast<nsCOMArray<nsICSSRule>*>(aArray)->AppendObject(aRule);
}

static void AppendRuleToSheet(nsICSSRule* aRule, void* aParser)
{
  CSSParserImpl* parser = (CSSParserImpl*) aParser;
  parser->AppendRule(aRule);
}

nsresult
NS_NewCSSParser(nsICSSParser** aInstancePtrResult)
{
  CSSParserImpl *it = new CSSParserImpl();

  if (it == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(NS_GET_IID(nsICSSParser), (void **) aInstancePtrResult);
}

#ifdef CSS_REPORT_PARSE_ERRORS

#define REPORT_UNEXPECTED(msg_) \
  mScanner.ReportUnexpected(#msg_)

#define REPORT_UNEXPECTED_P(msg_, params_) \
  mScanner.ReportUnexpectedParams(#msg_, params_, NS_ARRAY_LENGTH(params_))

#define REPORT_UNEXPECTED_EOF(lf_) \
  mScanner.ReportUnexpectedEOF(#lf_)

#define REPORT_UNEXPECTED_EOF_CHAR(ch_) \
  mScanner.ReportUnexpectedEOF(ch_)

#define REPORT_UNEXPECTED_TOKEN(msg_) \
  mScanner.ReportUnexpectedToken(mToken, #msg_)

#define REPORT_UNEXPECTED_TOKEN_P(msg_, params_) \
  mScanner.ReportUnexpectedTokenParams(mToken, #msg_, \
                                       params_, NS_ARRAY_LENGTH(params_))


#define OUTPUT_ERROR() \
  mScanner.OutputError()

#define CLEAR_ERROR() \
  mScanner.ClearError()

#else

#define REPORT_UNEXPECTED(msg_)
#define REPORT_UNEXPECTED_P(msg_, params_)
#define REPORT_UNEXPECTED_EOF(lf_)
#define REPORT_UNEXPECTED_EOF_CHAR(ch_)
#define REPORT_UNEXPECTED_TOKEN(msg_)
#define REPORT_UNEXPECTED_TOKEN_P(msg_, params_)
#define OUTPUT_ERROR()
#define CLEAR_ERROR()

#endif

CSSParserImpl::CSSParserImpl()
  : mToken(),
    mScanner(),
    mChildLoader(nsnull),
    mSection(eCSSSection_Charset),
    mNameSpaceMap(nsnull),
    mHavePushBack(PR_FALSE),
    mNavQuirkMode(PR_FALSE),
    mUnsafeRulesEnabled(PR_FALSE),
    mHTMLMediaMode(PR_FALSE),
    mCaseSensitive(PR_FALSE),
    mParsingCompoundProperty(PR_FALSE),
    mUnresolvablePrefixException(PR_FALSE)
#ifdef DEBUG
    , mScannerInited(PR_FALSE)
#endif
{
}

NS_IMPL_ISUPPORTS1(CSSParserImpl, nsICSSParser)

CSSParserImpl::~CSSParserImpl()
{
  mData.AssertInitialState();
  mTempData.AssertInitialState();
}

NS_IMETHODIMP
CSSParserImpl::SetStyleSheet(nsICSSStyleSheet* aSheet)
{
  if (aSheet != mSheet) {
    
    mGroupStack.Clear();
    mSheet = aSheet;
    if (mSheet) {
      mNameSpaceMap = mSheet->GetNameSpaceMap();
    } else {
      mNameSpaceMap = nsnull;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
CSSParserImpl::SetCaseSensitive(PRBool aCaseSensitive)
{
  NS_ASSERTION(aCaseSensitive == PR_TRUE || aCaseSensitive == PR_FALSE, "bad PRBool value");
  mCaseSensitive = aCaseSensitive;
  return NS_OK;
}

NS_IMETHODIMP
CSSParserImpl::SetQuirkMode(PRBool aQuirkMode)
{
  NS_ASSERTION(aQuirkMode == PR_TRUE || aQuirkMode == PR_FALSE, "bad PRBool value");
  mNavQuirkMode = aQuirkMode;
  return NS_OK;
}

#ifdef MOZ_SVG
NS_IMETHODIMP
CSSParserImpl::SetSVGMode(PRBool aSVGMode)
{
  NS_ASSERTION(aSVGMode == PR_TRUE || aSVGMode == PR_FALSE,
               "bad PRBool value");
  mScanner.SetSVGMode(aSVGMode);
  return NS_OK;
}
#endif

NS_IMETHODIMP
CSSParserImpl::SetChildLoader(nsICSSLoader* aChildLoader)
{
  mChildLoader = aChildLoader;  
  return NS_OK;
}

void
CSSParserImpl::InitScanner(nsIUnicharInputStream* aInput, nsIURI* aSheetURI,
                           PRUint32 aLineNumber, nsIURI* aBaseURI,
                           nsIPrincipal* aSheetPrincipal)
{
  NS_ASSERTION(! mScannerInited, "already have scanner");

  mScanner.Init(aInput, nsnull, 0, aSheetURI, aLineNumber);
#ifdef DEBUG
  mScannerInited = PR_TRUE;
#endif
  mBaseURL = aBaseURI;
  mSheetURL = aSheetURI;
  mSheetPrincipal = aSheetPrincipal;

  mHavePushBack = PR_FALSE;
}

void
CSSParserImpl::InitScanner(const nsSubstring& aString, nsIURI* aSheetURI,
                           PRUint32 aLineNumber, nsIURI* aBaseURI,
                           nsIPrincipal* aSheetPrincipal)
{
  
  
  NS_ASSERTION(! mScannerInited, "already have scanner");

  mScanner.Init(nsnull, aString.BeginReading(), aString.Length(), aSheetURI, aLineNumber);

#ifdef DEBUG
  mScannerInited = PR_TRUE;
#endif
  mBaseURL = aBaseURI;
  mSheetURL = aSheetURI;
  mSheetPrincipal = aSheetPrincipal;

  mHavePushBack = PR_FALSE;
}

void
CSSParserImpl::ReleaseScanner(void)
{
  mScanner.Close();
#ifdef DEBUG
  mScannerInited = PR_FALSE;
#endif
  mBaseURL = nsnull;
  mSheetURL = nsnull;
  mSheetPrincipal = nsnull;
}


NS_IMETHODIMP
CSSParserImpl::Parse(nsIUnicharInputStream* aInput,
                     nsIURI*                aSheetURI,
                     nsIURI*                aBaseURI,
                     nsIPrincipal*          aSheetPrincipal,
                     PRUint32               aLineNumber,
                     PRBool                 aAllowUnsafeRules)
{
  NS_PRECONDITION(aSheetPrincipal, "Must have principal here!");

  NS_ASSERTION(nsnull != aBaseURI, "need base URL");
  NS_ASSERTION(nsnull != aSheetURI, "need sheet URL");
  AssertInitialState();

  NS_PRECONDITION(mSheet, "Must have sheet to parse into");
  NS_ENSURE_STATE(mSheet);

#ifdef DEBUG
  nsCOMPtr<nsIURI> uri;
  mSheet->GetSheetURI(getter_AddRefs(uri));
  PRBool equal;
  NS_ASSERTION(NS_SUCCEEDED(aSheetURI->Equals(uri, &equal)) && equal,
               "Sheet URI does not match passed URI");
  NS_ASSERTION(NS_SUCCEEDED(mSheet->Principal()->Equals(aSheetPrincipal,
                                                        &equal)) &&
               equal,
               "Sheet principal does not match passed principal");
#endif

  InitScanner(aInput, aSheetURI, aLineNumber, aBaseURI, aSheetPrincipal);

  PRInt32 ruleCount = 0;
  mSheet->StyleRuleCount(ruleCount);
  if (0 < ruleCount) {
    nsICSSRule* lastRule = nsnull;
    mSheet->GetStyleRuleAt(ruleCount - 1, lastRule);
    if (lastRule) {
      PRInt32 type;
      lastRule->GetType(type);
      switch (type) {
        case nsICSSRule::CHARSET_RULE:
        case nsICSSRule::IMPORT_RULE:
          mSection = eCSSSection_Import;
          break;
        case nsICSSRule::NAMESPACE_RULE:
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
    
    if (!GetToken(PR_TRUE)) {
      OUTPUT_ERROR();
      break;
    }
    if (eCSSToken_HTMLComment == tk->mType) {
      continue; 
    }
    if (eCSSToken_AtKeyword == tk->mType) {
      ParseAtRule(AppendRuleToSheet, this);
      continue;
    }
    UngetToken();
    if (ParseRuleSet(AppendRuleToSheet, this)) {
      mSection = eCSSSection_General;
    }
  }
  ReleaseScanner();

  mUnsafeRulesEnabled = PR_FALSE;

  
  return NS_OK;
}





static PRBool
NonMozillaVendorIdentifier(const nsAString& ident)
{
  return (ident.First() == PRUnichar('-') &&
          !StringBeginsWith(ident, NS_LITERAL_STRING("-moz-"))) ||
         ident.First() == PRUnichar('_');

}

NS_IMETHODIMP
CSSParserImpl::ParseStyleAttribute(const nsAString& aAttributeValue,
                                   nsIURI*                  aDocURL,
                                   nsIURI*                  aBaseURL,
                                   nsIPrincipal*            aNodePrincipal,
                                   nsICSSStyleRule**        aResult)
{
  NS_PRECONDITION(aNodePrincipal, "Must have principal here!");
  AssertInitialState();

  NS_ASSERTION(nsnull != aBaseURL, "need base URL");

  
  InitScanner(aAttributeValue, aDocURL, 0, aBaseURL, aNodePrincipal);

  mSection = eCSSSection_General;

  
  
  PRBool haveBraces;
  if (mNavQuirkMode && GetToken(PR_TRUE)) {
    haveBraces = eCSSToken_Symbol == mToken.mType &&
                 '{' == mToken.mSymbol;
    UngetToken();
  }
  else {
    haveBraces = PR_FALSE;
  }

  nsCSSDeclaration* declaration = ParseDeclarationBlock(haveBraces);
  if (declaration) {
    
    nsICSSStyleRule* rule = nsnull;
    nsresult rv = NS_NewCSSStyleRule(&rule, nsnull, declaration);
    if (NS_FAILED(rv)) {
      declaration->RuleAbort();
      ReleaseScanner();
      return rv;
    }
    *aResult = rule;
  }
  else {
    *aResult = nsnull;
  }

  ReleaseScanner();

  
  return NS_OK;
}

NS_IMETHODIMP
CSSParserImpl::ParseAndAppendDeclaration(const nsAString&  aBuffer,
                                         nsIURI*           aSheetURL,
                                         nsIURI*           aBaseURL,
                                         nsIPrincipal*     aSheetPrincipal,
                                         nsCSSDeclaration* aDeclaration,
                                         PRBool            aParseOnlyOneDecl,
                                         PRBool*           aChanged,
                                         PRBool            aClearOldDecl)
{
  NS_PRECONDITION(aSheetPrincipal, "Must have principal here!");
  AssertInitialState();

  *aChanged = PR_FALSE;

  InitScanner(aBuffer, aSheetURL, 0, aBaseURL, aSheetPrincipal);

  mSection = eCSSSection_General;

  if (aClearOldDecl) {
    mData.AssertInitialState();
    aDeclaration->ClearData();
    
    *aChanged = PR_TRUE;
  } else {
    aDeclaration->ExpandTo(&mData);
  }

  nsresult rv = NS_OK;
  do {
    
    
    if (!ParseDeclaration(aDeclaration, PR_FALSE, aClearOldDecl, aChanged)) {
      rv = mScanner.GetLowLevelError();
      if (NS_FAILED(rv))
        break;

      if (!SkipDeclaration(PR_FALSE)) {
        rv = mScanner.GetLowLevelError();
        break;
      }
    }
  } while (!aParseOnlyOneDecl);
  aDeclaration->CompressFrom(&mData);

  ReleaseScanner();
  return rv;
}

NS_IMETHODIMP
CSSParserImpl::ParseRule(const nsAString&        aRule,
                         nsIURI*                 aSheetURL,
                         nsIURI*                 aBaseURL,
                         nsIPrincipal*           aSheetPrincipal,
                         nsCOMArray<nsICSSRule>& aResult)
{
  NS_PRECONDITION(aSheetPrincipal, "Must have principal here!");
  AssertInitialState();

  NS_ASSERTION(nsnull != aBaseURL, "need base URL");

  InitScanner(aRule, aSheetURL, 0, aBaseURL, aSheetPrincipal);

  mSection = eCSSSection_Charset; 

  nsCSSToken* tk = &mToken;
  
  if (!GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED(PEParseRuleWSOnly);
    OUTPUT_ERROR();
  } else if (eCSSToken_AtKeyword == tk->mType) {
    ParseAtRule(AppendRuleToArray, &aResult);
  }
  else {
    UngetToken();
    ParseRuleSet(AppendRuleToArray, &aResult);
  }
  OUTPUT_ERROR();
  ReleaseScanner();
  
  return NS_OK;
}

NS_IMETHODIMP
CSSParserImpl::ParseProperty(const nsCSSProperty aPropID,
                             const nsAString& aPropValue,
                             nsIURI* aSheetURL,
                             nsIURI* aBaseURL,
                             nsIPrincipal* aSheetPrincipal,
                             nsCSSDeclaration* aDeclaration,
                             PRBool* aChanged)
{
  NS_PRECONDITION(aSheetPrincipal, "Must have principal here!");
  AssertInitialState();

  NS_ASSERTION(nsnull != aBaseURL, "need base URL");
  NS_ASSERTION(nsnull != aDeclaration, "Need declaration to parse into!");
  *aChanged = PR_FALSE;

  InitScanner(aPropValue, aSheetURL, 0, aBaseURL, aSheetPrincipal);

  mSection = eCSSSection_General;

  if (eCSSProperty_UNKNOWN == aPropID) { 
    NS_ConvertASCIItoUTF16 propName(nsCSSProps::GetStringValue(aPropID));
    const PRUnichar *params[] = {
      propName.get()
    };
    REPORT_UNEXPECTED_P(PEUnknownProperty, params);
    REPORT_UNEXPECTED(PEDeclDropped);
    OUTPUT_ERROR();
    ReleaseScanner();
    return NS_OK;
  }

  mData.AssertInitialState();
  mTempData.AssertInitialState();

  
  
  
  
  
  
  void* valueSlot = aDeclaration->SlotForValue(aPropID);
  if (!valueSlot) {
    
    aDeclaration->ExpandTo(&mData);
  }
  nsresult result = NS_OK;
  PRBool parsedOK = ParseProperty(aPropID);
  if (parsedOK && !GetToken(PR_TRUE)) {
    if (valueSlot) {
      CopyValue(mTempData.PropertyAt(aPropID), valueSlot, aPropID, aChanged);
      mTempData.ClearPropertyBit(aPropID);
    } else {
      TransferTempData(aDeclaration, aPropID, PR_FALSE, PR_FALSE, aChanged);
    }
  } else {
    if (parsedOK) {
      
      REPORT_UNEXPECTED_TOKEN(PEExpectEndValue);
    }
    NS_ConvertASCIItoUTF16 propName(nsCSSProps::GetStringValue(aPropID));
    const PRUnichar *params[] = {
      propName.get()
    };
    REPORT_UNEXPECTED_P(PEValueParsingError, params);
    REPORT_UNEXPECTED(PEDeclDropped);
    OUTPUT_ERROR();
    ClearTempData(aPropID);
    result = mScanner.GetLowLevelError();
  }
  CLEAR_ERROR();

  if (!valueSlot) {
    aDeclaration->CompressFrom(&mData);
  }

  ReleaseScanner();
  return result;
}

NS_IMETHODIMP
CSSParserImpl::ParseMediaList(const nsSubstring& aBuffer,
                              nsIURI* aURL, 
                              PRUint32 aLineNumber, 
                              nsMediaList* aMediaList,
                              PRBool aHTMLMode)
{
  
  
  aMediaList->Clear();

  
  InitScanner(aBuffer, aURL, aLineNumber, aURL, nsnull);

  AssertInitialState();
  NS_ASSERTION(aHTMLMode == PR_TRUE || aHTMLMode == PR_FALSE,
               "invalid PRBool");
  mHTMLMediaMode = aHTMLMode;

    
    

  
  
  
  
  
  
  
  

  if (!GatherMedia(aMediaList, PRUnichar(0))) {
    aMediaList->Clear();
    aMediaList->SetNonEmpty(); 
    if (!mHTMLMediaMode) {
      OUTPUT_ERROR();
    }
  }
  nsresult rv = mScanner.GetLowLevelError();
  CLEAR_ERROR();
  ReleaseScanner();
  mHTMLMediaMode = PR_FALSE;

  return rv;
}

NS_IMETHODIMP
CSSParserImpl::ParseColorString(const nsSubstring& aBuffer,
                                nsIURI* aURL, 
                                PRUint32 aLineNumber, 
                                nscolor* aColor)
{
  AssertInitialState();
  InitScanner(aBuffer, aURL, aLineNumber, aURL, nsnull);

  nsCSSValue value;
  PRBool colorParsed = ParseColor(value);
  nsresult rv = mScanner.GetLowLevelError();
  OUTPUT_ERROR();
  ReleaseScanner();

  if (!colorParsed) {
    return NS_FAILED(rv) ? rv : NS_ERROR_FAILURE;
  }

  if (value.GetUnit() == eCSSUnit_Ident) {
    nscolor rgba;
    if (NS_ColorNameToRGB(nsDependentString(value.GetStringBufferValue()), &rgba)) {
      (*aColor) = rgba;
      rv = NS_OK;
    }
  } else if (value.GetUnit() == eCSSUnit_Color) {
    (*aColor) = value.GetColorValue();
    rv = NS_OK;
  } else if (value.GetUnit() == eCSSUnit_EnumColor) {
    PRInt32 intValue = value.GetIntValue();
    if (intValue >= 0) {
      nsCOMPtr<nsILookAndFeel> lfSvc = do_GetService("@mozilla.org/widget/lookandfeel;1");
      if (lfSvc) {
        nscolor rgba;
        rv = lfSvc->GetColor((nsILookAndFeel::nsColorID) value.GetIntValue(), rgba);
        if (NS_SUCCEEDED(rv))
          (*aColor) = rgba;
      }
    } else {
      
      
      
      rv = NS_ERROR_FAILURE;
    }
  }

  return rv;
}

NS_IMETHODIMP
CSSParserImpl::ParseSelectorString(const nsSubstring& aSelectorString,
                                   nsIURI* aURL, 
                                   PRUint32 aLineNumber, 
                                   nsCSSSelectorList **aSelectorList)
{
  InitScanner(aSelectorString, aURL, aLineNumber, aURL, nsnull);

  AssertInitialState();

  mUnresolvablePrefixException = PR_TRUE;

  PRBool success = ParseSelectorList(*aSelectorList, PR_FALSE);
  nsresult rv = mScanner.GetLowLevelError();
  OUTPUT_ERROR();
  ReleaseScanner();

  mUnresolvablePrefixException = PR_FALSE;

  if (success) {
    NS_ASSERTION(*aSelectorList, "Should have list!");
    return NS_OK;
  }

  NS_ASSERTION(!*aSelectorList, "Shouldn't have list!");
  if (NS_SUCCEEDED(rv)) {
    rv = NS_ERROR_DOM_SYNTAX_ERR;
  }
  return rv;
}



PRBool
CSSParserImpl::GetToken(PRBool aSkipWS)
{
  for (;;) {
    if (!mHavePushBack) {
      if (!mScanner.Next(mToken)) {
        break;
      }
    }
    mHavePushBack = PR_FALSE;
    if (aSkipWS && (eCSSToken_WhiteSpace == mToken.mType)) {
      continue;
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::GetURLInParens(nsString& aURL)
{
  if (!ExpectSymbol('(', PR_FALSE))
    return PR_FALSE;

  NS_ASSERTION(!mHavePushBack,
               "ExpectSymbol returning success shouldn't leave pushback");
  do {
    if (! mScanner.NextURL(mToken)) {
      return PR_FALSE;
    }
  } while (eCSSToken_WhiteSpace == mToken.mType);

  aURL = mToken.mIdent;

  if ((eCSSToken_String != mToken.mType && eCSSToken_URL != mToken.mType) ||
      !ExpectSymbol(')', PR_TRUE)) {
    
    

    
    
    SkipUntil(')');
    return PR_FALSE;
  }

  return PR_TRUE;
}

void
CSSParserImpl::UngetToken()
{
  NS_PRECONDITION(mHavePushBack == PR_FALSE, "double pushback");
  mHavePushBack = PR_TRUE;
}

PRBool
CSSParserImpl::ExpectSymbol(PRUnichar aSymbol,
                            PRBool aSkipWS)
{
  if (!GetToken(aSkipWS)) {
    
    
    
    
    if (aSymbol == ')' || aSymbol == ']' ||
        aSymbol == '}' || aSymbol == ';') {
      REPORT_UNEXPECTED_EOF_CHAR(aSymbol);
      return PR_TRUE;
    }
    else
      return PR_FALSE;
  }
  if (mToken.IsSymbol(aSymbol)) {
    return PR_TRUE;
  }
  UngetToken();
  return PR_FALSE;
}



PRBool
CSSParserImpl::CheckEndProperty()
{
  if (!GetToken(PR_TRUE)) {
    return PR_TRUE; 
  }
  if ((eCSSToken_Symbol == mToken.mType) &&
      ((';' == mToken.mSymbol) ||
       ('!' == mToken.mSymbol) ||
       ('}' == mToken.mSymbol))) {
    
    
    UngetToken();
    return PR_TRUE;
  }
  UngetToken();
  return PR_FALSE;
}


PRBool
CSSParserImpl::ExpectEndProperty()
{
  if (CheckEndProperty())
    return PR_TRUE;

  
  REPORT_UNEXPECTED_TOKEN(PEExpectEndValue);
  return PR_FALSE;
}


nsSubstring*
CSSParserImpl::NextIdent()
{
  
  if (!GetToken(PR_TRUE)) {
    return nsnull;
  }
  if (eCSSToken_Ident != mToken.mType) {
    UngetToken();
    return nsnull;
  }
  return &mToken.mIdent;
}

PRBool
CSSParserImpl::SkipAtRule()
{
  for (;;) {
    if (!GetToken(PR_TRUE)) {
      REPORT_UNEXPECTED_EOF(PESkipAtRuleEOF);
      return PR_FALSE;
    }
    if (eCSSToken_Symbol == mToken.mType) {
      PRUnichar symbol = mToken.mSymbol;
      if (symbol == ';') {
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
    }
  }
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseAtRule(RuleAppendFunc aAppendFunc,
                           void* aData)
{
  nsCSSSection newSection;
  PRBool (CSSParserImpl::*parseFunc)(RuleAppendFunc, void*);

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

  } else {
    if (!NonMozillaVendorIdentifier(mToken.mIdent)) {
      REPORT_UNEXPECTED_TOKEN(PEUnknownAtRule);
      OUTPUT_ERROR();
    }
    
    return SkipAtRule();
  }

  if (!(this->*parseFunc)(aAppendFunc, aData)) {
    
    OUTPUT_ERROR();
    return SkipAtRule();
  }

  mSection = newSection;
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseCharsetRule(RuleAppendFunc aAppendFunc,
                                void* aData)
{
  if (!GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PECharsetRuleEOF);
    return PR_FALSE;
  }

  if (eCSSToken_String != mToken.mType) {
    REPORT_UNEXPECTED_TOKEN(PECharsetRuleNotString);
    return PR_FALSE;
  }

  nsAutoString charset = mToken.mIdent;

  if (!ExpectSymbol(';', PR_TRUE)) {
    return PR_FALSE;
  }

  nsCOMPtr<nsICSSRule> rule;
  NS_NewCSSCharsetRule(getter_AddRefs(rule), charset);

  if (rule) {
    (*aAppendFunc)(rule, aData);
  }

  return PR_TRUE;
}

PRBool
CSSParserImpl::GatherURL(nsString& aURL)
{
  if (!GetToken(PR_TRUE)) {
    return PR_FALSE;
  }
  if (eCSSToken_String == mToken.mType) {
    aURL = mToken.mIdent;
    return PR_TRUE;
  }
  else if (eCSSToken_Function == mToken.mType &&
           mToken.mIdent.LowerCaseEqualsLiteral("url") &&
           GetURLInParens(aURL)) {
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseMediaQuery(PRUnichar aStopSymbol,
                               nsMediaQuery **aQuery,
                               PRBool *aParsedSomething,
                               PRBool *aHitStop)
{
  *aQuery = nsnull;
  *aParsedSomething = PR_FALSE;
  *aHitStop = PR_FALSE;

  
  
  
  if (!GetToken(PR_TRUE)) {
    *aHitStop = PR_TRUE;
    
    if (aStopSymbol == PRUnichar(0))
      return PR_TRUE;

    
    REPORT_UNEXPECTED_EOF(PEGatherMediaEOF);
    return PR_TRUE;
  }

  if (eCSSToken_Symbol == mToken.mType &&
      mToken.mSymbol == aStopSymbol) {
    *aHitStop = PR_TRUE;
    UngetToken();
    return PR_TRUE;
  }
  UngetToken();

  *aParsedSomething = PR_TRUE;

  nsAutoPtr<nsMediaQuery> query(new nsMediaQuery);
  if (!query) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }

  if (ExpectSymbol('(', PR_TRUE)) {
    
    UngetToken(); 
    query->SetType(nsGkAtoms::all);
    query->SetTypeOmitted();
    
    if (!ParseMediaQueryExpression(query)) {
      OUTPUT_ERROR();
      query->SetHadUnknownExpression();
    }
  } else {
    nsCOMPtr<nsIAtom> mediaType;
    PRBool gotNotOrOnly = PR_FALSE;
    for (;;) {
      if (!GetToken(PR_TRUE)) {
        REPORT_UNEXPECTED_EOF(PEGatherMediaEOF);
        return PR_FALSE;
      }
      if (eCSSToken_Ident != mToken.mType) {
        REPORT_UNEXPECTED_TOKEN(PEGatherMediaNotIdent);
        UngetToken();
        return PR_FALSE;
      }
      
      ToLowerCase(mToken.mIdent);
      mediaType = do_GetAtom(mToken.mIdent);
      if (gotNotOrOnly ||
          (mediaType != nsGkAtoms::_not && mediaType != nsGkAtoms::only))
        break;
      gotNotOrOnly = PR_TRUE;
      if (mediaType == nsGkAtoms::_not)
        query->SetNegated();
      else
        query->SetHasOnly();
    }
    query->SetType(mediaType);
  }

  for (;;) {
    if (!GetToken(PR_TRUE)) {
      *aHitStop = PR_TRUE;
      
      if (aStopSymbol == PRUnichar(0))
        break;

      
      REPORT_UNEXPECTED_EOF(PEGatherMediaEOF);
      break;
    }

    if (eCSSToken_Symbol == mToken.mType &&
        mToken.mSymbol == aStopSymbol) {
      *aHitStop = PR_TRUE;
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
      return PR_FALSE;
    }
    if (!ParseMediaQueryExpression(query)) {
      OUTPUT_ERROR();
      query->SetHadUnknownExpression();
    }
  }
  *aQuery = query.forget();
  return PR_TRUE;
}



PRBool
CSSParserImpl::GatherMedia(nsMediaList* aMedia,
                           PRUnichar aStopSymbol)
{
  for (;;) {
    nsAutoPtr<nsMediaQuery> query;
    PRBool parsedSomething, hitStop;
    if (!ParseMediaQuery(aStopSymbol, getter_Transfers(query),
                         &parsedSomething, &hitStop)) {
      NS_ASSERTION(!hitStop, "should return true when hit stop");
      if (NS_FAILED(mScanner.GetLowLevelError())) {
        return PR_FALSE;
      }
      const PRUnichar stopChars[] =
        { PRUnichar(','), aStopSymbol , PRUnichar(0) };
      SkipUntilOneOf(stopChars);
      
      if (mToken.mType == eCSSToken_Symbol && mToken.mSymbol == aStopSymbol) {
        UngetToken();
        hitStop = PR_TRUE;
      }
    }
    if (parsedSomething) {
      aMedia->SetNonEmpty();
    }
    if (query) {
      nsresult rv = aMedia->AppendQuery(query);
      if (NS_FAILED(rv)) {
        mScanner.SetLowLevelError(rv);
        return PR_FALSE;
      }
    }
    if (hitStop) {
      break;
    }
  }
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseMediaQueryExpression(nsMediaQuery* aQuery)
{
  if (!ExpectSymbol('(', PR_TRUE)) {
    REPORT_UNEXPECTED_TOKEN(PEMQExpectedExpressionStart);
    return PR_FALSE;
  }
  if (! GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PEMQExpressionEOF);
    return PR_FALSE;
  }
  if (eCSSToken_Ident != mToken.mType) {
    REPORT_UNEXPECTED_TOKEN(PEMQExpectedFeatureName);
    SkipUntil(')');
    return PR_FALSE;
  }

  nsMediaExpression *expr = aQuery->NewExpression();
  if (!expr) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    SkipUntil(')');
    return PR_FALSE;
  }

  
  ToLowerCase(mToken.mIdent);
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
    return PR_FALSE;
  }
  expr->mFeature = feature;

  if (!GetToken(PR_TRUE) || mToken.IsSymbol(')')) {
    
    
    if (expr->mRange != nsMediaExpression::eEqual) {
      REPORT_UNEXPECTED(PEMQNoMinMaxWithoutValue);
      return PR_FALSE;
    }
    expr->mValue.Reset();
    return PR_TRUE;
  }

  if (!mToken.IsSymbol(':')) {
    REPORT_UNEXPECTED_TOKEN(PEMQExpectedFeatureNameEnd);
    SkipUntil(')');
    return PR_FALSE;
  }

  PRBool rv;
  switch (feature->mValueType) {
    case nsMediaFeature::eLength:
      rv = ParseNonNegativeVariant(expr->mValue, VARIANT_LENGTH, nsnull);
      break;
    case nsMediaFeature::eInteger:
    case nsMediaFeature::eBoolInteger:
      rv = ParseNonNegativeVariant(expr->mValue, VARIANT_INTEGER, nsnull);
      
      if (rv &&
          feature->mValueType == nsMediaFeature::eBoolInteger &&
          expr->mValue.GetIntValue() > 1)
        rv = PR_FALSE;
      break;
    case nsMediaFeature::eIntRatio:
      {
        
        
        nsRefPtr<nsCSSValue::Array> a = nsCSSValue::Array::Create(2);
        if (!a) {
          mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
          SkipUntil(')');
          return PR_FALSE;
        }
        expr->mValue.SetArrayValue(a, eCSSUnit_Array);
        
        
        
        rv = ParseVariant(a->Item(0), VARIANT_INTEGER, nsnull) &&
             a->Item(0).GetIntValue() > 0 &&
             ExpectSymbol('/', PR_TRUE) &&
             ParseVariant(a->Item(1), VARIANT_INTEGER, nsnull) &&
             a->Item(1).GetIntValue() > 0;
      }
      break;
    case nsMediaFeature::eResolution:
      rv = GetToken(PR_TRUE) && mToken.IsDimension() &&
           mToken.mIntegerValid && mToken.mNumber > 0.0f;
      if (rv) {
        
        
        NS_ASSERTION(!mToken.mIdent.IsEmpty(), "IsDimension lied");
        if (mToken.mIdent.LowerCaseEqualsLiteral("dpi")) {
          expr->mValue.SetFloatValue(mToken.mNumber, eCSSUnit_Inch);
        } else if (mToken.mIdent.LowerCaseEqualsLiteral("dpcm")) {
          expr->mValue.SetFloatValue(mToken.mNumber, eCSSUnit_Centimeter);
        } else {
          rv = PR_FALSE;
        }
      }
      break;
    case nsMediaFeature::eEnumerated:
      rv = ParseVariant(expr->mValue, VARIANT_KEYWORD,
                        feature->mKeywordTable);
      break;
  }
  if (!rv || !ExpectSymbol(')', PR_TRUE)) {
    REPORT_UNEXPECTED(PEMQExpectedFeatureValue);
    SkipUntil(')');
    return PR_FALSE;
  }

  return PR_TRUE;
}


PRBool
CSSParserImpl::ParseImportRule(RuleAppendFunc aAppendFunc, void* aData)
{
  nsCOMPtr<nsMediaList> media = new nsMediaList();
  if (!media) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }

  nsAutoString url;
  if (!GatherURL(url)) {
    REPORT_UNEXPECTED_TOKEN(PEImportNotURI);
    return PR_FALSE;
  }

  if (!ExpectSymbol(';', PR_TRUE)) {
    if (!GatherMedia(media, ';') ||
        !ExpectSymbol(';', PR_TRUE)) {
      REPORT_UNEXPECTED_TOKEN(PEImportUnexpected);
      
      return PR_FALSE;
    }

    
    
    NS_ASSERTION(media->Count() != 0, "media list must be nonempty");
  }

  ProcessImport(url, media, aAppendFunc, aData);
  return PR_TRUE;
}


PRBool
CSSParserImpl::ProcessImport(const nsString& aURLSpec,
                             nsMediaList* aMedia,
                             RuleAppendFunc aAppendFunc,
                             void* aData)
{
  nsCOMPtr<nsICSSImportRule> rule;
  nsresult rv = NS_NewCSSImportRule(getter_AddRefs(rule), aURLSpec, aMedia);
  if (NS_FAILED(rv)) {
    mScanner.SetLowLevelError(rv);
    return PR_FALSE;
  }
  (*aAppendFunc)(rule, aData);

  if (mChildLoader) {
    nsCOMPtr<nsIURI> url;
    
    rv = NS_NewURI(getter_AddRefs(url), aURLSpec, nsnull, mBaseURL);

    if (NS_FAILED(rv)) {
      
      
      mScanner.SetLowLevelError(rv);
      return PR_FALSE;
    }

    mChildLoader->LoadChildSheet(mSheet, url, aMedia, rule);
  }

  return PR_TRUE;
}


PRBool
CSSParserImpl::ParseGroupRule(nsICSSGroupRule* aRule,
                              RuleAppendFunc aAppendFunc,
                              void* aData)
{
  
  if (!ExpectSymbol('{', PR_TRUE)) {
    return PR_FALSE;
  }

  
  if (!PushGroup(aRule)) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }
  nsCSSSection holdSection = mSection;
  mSection = eCSSSection_General;

  for (;;) {
    
    if (! GetToken(PR_TRUE)) {
      REPORT_UNEXPECTED_EOF(PEGroupRuleEOF);
      break;
    }
    if (mToken.IsSymbol('}')) { 
      UngetToken();
      break;
    }
    if (eCSSToken_AtKeyword == mToken.mType) {
      SkipAtRule(); 
      continue;
    }
    UngetToken();
    ParseRuleSet(AppendRuleToSheet, this, PR_TRUE);
  }
  PopGroup();

  if (!ExpectSymbol('}', PR_TRUE)) {
    mSection = holdSection;
    return PR_FALSE;
  }
  (*aAppendFunc)(aRule, aData);
  return PR_TRUE;
}


PRBool
CSSParserImpl::ParseMediaRule(RuleAppendFunc aAppendFunc, void* aData)
{
  nsCOMPtr<nsMediaList> media = new nsMediaList();
  if (!media) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }

  if (GatherMedia(media, '{')) {
    
    nsRefPtr<nsCSSMediaRule> rule(new nsCSSMediaRule());
    
    
    if (rule && ParseGroupRule(rule, aAppendFunc, aData)) {
      rule->SetMedia(media);
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}




PRBool
CSSParserImpl::ParseMozDocumentRule(RuleAppendFunc aAppendFunc, void* aData)
{
  nsCSSDocumentRule::URL *urls = nsnull;
  nsCSSDocumentRule::URL **next = &urls;
  do {
    if (!GetToken(PR_TRUE) ||
        eCSSToken_Function != mToken.mType ||
        !(mToken.mIdent.LowerCaseEqualsLiteral("url") ||
          mToken.mIdent.LowerCaseEqualsLiteral("url-prefix") ||
          mToken.mIdent.LowerCaseEqualsLiteral("domain"))) {
      REPORT_UNEXPECTED_TOKEN(PEMozDocRuleBadFunc);
      delete urls;
      return PR_FALSE;
    }
    nsCSSDocumentRule::URL *cur = *next = new nsCSSDocumentRule::URL;
    if (!cur) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      delete urls;
      return PR_FALSE;
    }
    next = &cur->next;
    if (mToken.mIdent.LowerCaseEqualsLiteral("url")) {
      cur->func = nsCSSDocumentRule::eURL;
    } else if (mToken.mIdent.LowerCaseEqualsLiteral("url-prefix")) {
      cur->func = nsCSSDocumentRule::eURLPrefix;
    } else if (mToken.mIdent.LowerCaseEqualsLiteral("domain")) {
      cur->func = nsCSSDocumentRule::eDomain;
    }

    nsAutoString url;
    if (!GetURLInParens(url)) {
      REPORT_UNEXPECTED_TOKEN(PEMozDocRuleNotURI);
      delete urls;
      return PR_FALSE;
    }

    
    
    
    CopyUTF16toUTF8(url, cur->url);
  } while (ExpectSymbol(',', PR_TRUE));

  nsRefPtr<nsCSSDocumentRule> rule(new nsCSSDocumentRule());
  if (!rule) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    delete urls;
    return PR_FALSE;
  }
  rule->SetURLs(urls);

  return ParseGroupRule(rule, aAppendFunc, aData);
}


PRBool
CSSParserImpl::ParseNameSpaceRule(RuleAppendFunc aAppendFunc, void* aData)
{
  if (!GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PEAtNSPrefixEOF);
    return PR_FALSE;
  }

  nsAutoString  prefix;
  nsAutoString  url;

  if (eCSSToken_Ident == mToken.mType) {
    prefix = mToken.mIdent;
    
    if (! GetToken(PR_TRUE)) {
      REPORT_UNEXPECTED_EOF(PEAtNSURIEOF);
      return PR_FALSE;
    }
  }

  if (eCSSToken_String == mToken.mType) {
    url = mToken.mIdent;
    if (ExpectSymbol(';', PR_TRUE)) {
      ProcessNameSpace(prefix, url, aAppendFunc, aData);
      return PR_TRUE;
    }
  }
  else if ((eCSSToken_Function == mToken.mType) &&
           (mToken.mIdent.LowerCaseEqualsLiteral("url"))) {
    if (GetURLInParens(url) &&
        ExpectSymbol(';', PR_TRUE)) {
      ProcessNameSpace(prefix, url, aAppendFunc, aData);
      return PR_TRUE;
    }
  }
  REPORT_UNEXPECTED_TOKEN(PEAtNSUnexpected);

  return PR_FALSE;
}

PRBool
CSSParserImpl::ProcessNameSpace(const nsString& aPrefix,
                                const nsString& aURLSpec,
                                RuleAppendFunc aAppendFunc,
                                void* aData)
{
  PRBool result = PR_FALSE;

  nsCOMPtr<nsICSSNameSpaceRule> rule;
  nsCOMPtr<nsIAtom> prefix;

  if (!aPrefix.IsEmpty()) {
    prefix = do_GetAtom(aPrefix);
  }

  NS_NewCSSNameSpaceRule(getter_AddRefs(rule), prefix, aURLSpec);
  if (rule) {
    (*aAppendFunc)(rule, aData);

    
    
    if (!mNameSpaceMap) {
      mNameSpaceMap = mSheet->GetNameSpaceMap();
    }
  }

  return result;
}



PRBool
CSSParserImpl::ParseFontFaceRule(RuleAppendFunc aAppendFunc, void* aData)
{
  if (!ExpectSymbol('{', PR_TRUE)) {
    REPORT_UNEXPECTED_TOKEN(PEBadFontBlockStart);
    return PR_FALSE;
  }

  nsRefPtr<nsCSSFontFaceRule> rule(new nsCSSFontFaceRule());
  if (!rule) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }

  for (;;) {
    if (!GetToken(PR_TRUE)) {
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
      if (!SkipDeclaration(PR_TRUE))
        break;
    }
  }
  if (!ExpectSymbol('}', PR_TRUE)) {
    REPORT_UNEXPECTED_TOKEN(PEBadFontBlockEnd);
    return PR_FALSE;
  }
  (*aAppendFunc)(rule, aData);
  return PR_TRUE;
}













PRBool
CSSParserImpl::ParseFontDescriptor(nsCSSFontFaceRule* aRule)
{
  if (eCSSToken_Ident != mToken.mType) {
    REPORT_UNEXPECTED_TOKEN(PEFontDescExpected);
    return PR_FALSE;
  }

  nsString descName = mToken.mIdent;
  if (!ExpectSymbol(':', PR_TRUE)) {
    REPORT_UNEXPECTED_TOKEN(PEParseDeclarationNoColon);
    OUTPUT_ERROR();
    return PR_FALSE;
  }

  nsCSSFontDesc descID = nsCSSProps::LookupFontDesc(descName);
  nsCSSValue value;

  if (descID == eCSSFontDesc_UNKNOWN) {
    if (NonMozillaVendorIdentifier(descName)) {
      
      SkipDeclaration(PR_TRUE);
      return PR_TRUE;
    } else {
      const PRUnichar *params[] = {
        descName.get()
      };
      REPORT_UNEXPECTED_P(PEUnknownFontDesc, params);
      return PR_FALSE;
    }
  }

  if (!ParseFontDescriptorValue(descID, value)) {
    const PRUnichar *params[] = {
      descName.get()
    };
    REPORT_UNEXPECTED_P(PEValueParsingError, params);
    return PR_FALSE;
  }

  if (!ExpectEndProperty())
    return PR_FALSE;

  aRule->SetDesc(descID, value);
  return PR_TRUE;
}


PRBool
CSSParserImpl::ParsePageRule(RuleAppendFunc aAppendFunc, void* aData)
{
  
  return PR_FALSE;
}

void
CSSParserImpl::SkipUntil(PRUnichar aStopSymbol)
{
  nsCSSToken* tk = &mToken;
  nsAutoTArray<PRUnichar, 16> stack;
  stack.AppendElement(aStopSymbol);
  for (;;) {
    if (!GetToken(PR_TRUE)) {
      break;
    }
    if (eCSSToken_Symbol == tk->mType) {
      PRUnichar symbol = tk->mSymbol;
      PRUint32 stackTopIndex = stack.Length() - 1;
      if (symbol == stack.ElementAt(stackTopIndex)) {
        stack.RemoveElementAt(stackTopIndex);
        if (stackTopIndex == 0) {
          break;
        }
      } else if ('{' == symbol) {
        
        
        
        stack.AppendElement('}');
      } else if ('[' == symbol) {
        stack.AppendElement(']');
      } else if ('(' == symbol) {
        stack.AppendElement(')');
      }
    }
  }
}

void
CSSParserImpl::SkipUntilOneOf(const PRUnichar* aStopSymbolChars)
{
  nsCSSToken* tk = &mToken;
  nsDependentString stopSymbolChars(aStopSymbolChars);
  for (;;) {
    if (!GetToken(PR_TRUE)) {
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
    }
  }
}

PRBool
CSSParserImpl::GetNonCloseParenToken(PRBool aSkipWS)
{
  if (!GetToken(aSkipWS))
    return PR_FALSE;
  if (mToken.mType == eCSSToken_Symbol && mToken.mSymbol == ')') {
    UngetToken();
    return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
CSSParserImpl::SkipDeclaration(PRBool aCheckForBraces)
{
  nsCSSToken* tk = &mToken;
  for (;;) {
    if (!GetToken(PR_TRUE)) {
      if (aCheckForBraces) {
        REPORT_UNEXPECTED_EOF(PESkipDeclBraceEOF);
      }
      return PR_FALSE;
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
    }
  }
  return PR_TRUE;
}

void
CSSParserImpl::SkipRuleSet(PRBool aInsideBraces)
{
  nsCSSToken* tk = &mToken;
  for (;;) {
    if (!GetToken(PR_TRUE)) {
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
    }
  }
}

PRBool
CSSParserImpl::PushGroup(nsICSSGroupRule* aRule)
{
  if (mGroupStack.AppendObject(aRule))
    return PR_TRUE;

  return PR_FALSE;
}

void
CSSParserImpl::PopGroup(void)
{
  PRInt32 count = mGroupStack.Count();
  if (0 < count) {
    mGroupStack.RemoveObjectAt(count - 1);
  }
}

void
CSSParserImpl::AppendRule(nsICSSRule* aRule)
{
  PRInt32 count = mGroupStack.Count();
  if (0 < count) {
    mGroupStack[count - 1]->AppendStyleRule(aRule);
  }
  else {
    mSheet->AppendStyleRule(aRule);
  }
}

PRBool
CSSParserImpl::ParseRuleSet(RuleAppendFunc aAppendFunc, void* aData,
                            PRBool aInsideBraces)
{
  
  nsCSSSelectorList* slist = nsnull;
  PRUint32 linenum = mScanner.GetLineNumber();
  if (! ParseSelectorList(slist, PR_TRUE)) {
    REPORT_UNEXPECTED(PEBadSelectorRSIgnored);
    OUTPUT_ERROR();
    SkipRuleSet(aInsideBraces);
    return PR_FALSE;
  }
  NS_ASSERTION(nsnull != slist, "null selector list");
  CLEAR_ERROR();

  
  nsCSSDeclaration* declaration = ParseDeclarationBlock(PR_TRUE);
  if (nsnull == declaration) {
    
    delete slist;
    return PR_FALSE;
  }

#if 0
  slist->Dump();
  fputs("{\n", stdout);
  declaration->List();
  fputs("}\n", stdout);
#endif

  

  nsCOMPtr<nsICSSStyleRule> rule;
  NS_NewCSSStyleRule(getter_AddRefs(rule), slist, declaration);
  if (!rule) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    delete slist;
    return PR_FALSE;
  }
  rule->SetLineNumber(linenum);
  (*aAppendFunc)(rule, aData);

  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseSelectorList(nsCSSSelectorList*& aListHead,
                                 PRBool aTerminateAtBrace)
{
  nsCSSSelectorList* list = nsnull;
  if (! ParseSelectorGroup(list)) {
    
    aListHead = nsnull;
    return PR_FALSE;
  }
  NS_ASSERTION(nsnull != list, "no selector list");
  aListHead = list;

  
  
  nsCSSToken* tk = &mToken;
  for (;;) {
    if (! GetToken(PR_TRUE)) {
      if (!aTerminateAtBrace) {
        return PR_TRUE;
      }

      REPORT_UNEXPECTED_EOF(PESelectorListExtraEOF);
      break;
    }

    if (eCSSToken_Symbol == tk->mType) {
      if (',' == tk->mSymbol) {
        nsCSSSelectorList* newList = nsnull;
        
        if (! ParseSelectorGroup(newList)) {
          break;
        }
        
        list->mNext = newList;
        list = newList;
        continue;
      } else if ('{' == tk->mSymbol && aTerminateAtBrace) {
        UngetToken();
        return PR_TRUE;
      }
    }
    REPORT_UNEXPECTED_TOKEN(PESelectorListExtra);
    UngetToken();
    break;
  }

  delete aListHead;
  aListHead = nsnull;
  return PR_FALSE;
}

static PRBool IsSinglePseudoClass(const nsCSSSelector& aSelector)
{
  return PRBool((aSelector.mNameSpace == kNameSpaceID_Unknown) &&
                (aSelector.mLowercaseTag == nsnull) &&
                (aSelector.mIDList == nsnull) &&
                (aSelector.mClassList == nsnull) &&
                (aSelector.mAttrList == nsnull) &&
                (aSelector.mNegations == nsnull) &&
                (aSelector.mPseudoClassList != nsnull) &&
                (aSelector.mPseudoClassList->mNext == nsnull));
}

#ifdef MOZ_XUL
static PRBool IsTreePseudoElement(nsIAtom* aPseudo)
{
  const char* str;
  aPseudo->GetUTF8String(&str);
  static const char moz_tree[] = ":-moz-tree-";
  return nsCRT::strncmp(str, moz_tree, PRInt32(sizeof(moz_tree)-1)) == 0;
}
#endif

PRBool
CSSParserImpl::ParseSelectorGroup(nsCSSSelectorList*& aList)
{
  nsAutoPtr<nsCSSSelectorList> list;
  PRUnichar     combinator = PRUnichar(0);
  PRInt32       weight = 0;
  PRBool        havePseudoElement = PR_FALSE;
  PRBool        done = PR_FALSE;
  while (!done) {
    nsAutoPtr<nsCSSSelector> newSelector(new nsCSSSelector());
    if (!newSelector) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      return PR_FALSE;
    }
    nsSelectorParsingStatus parsingStatus =
      ParseSelector(*newSelector);
    if (parsingStatus == eSelectorParsingStatus_Empty) {
      if (!list) {
        REPORT_UNEXPECTED(PESelectorGroupNoSelector);
      }
      break;
    }
    if (parsingStatus == eSelectorParsingStatus_Error) {
      list = nsnull;
      break;
    }
    if (nsnull == list) {
      list = new nsCSSSelectorList();
      if (nsnull == list) {
        mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
        return PR_FALSE;
      }
    }
    list->AddSelector(newSelector);
    nsCSSSelector* listSel = list->mSelectors;

    
    nsPseudoClassList* prevList = nsnull;
    nsPseudoClassList* pseudoClassList = listSel->mPseudoClassList;
    while (nsnull != pseudoClassList) {
      if (! nsCSSPseudoClasses::IsPseudoClass(pseudoClassList->mAtom)) {
        havePseudoElement = PR_TRUE;
        if (IsSinglePseudoClass(*listSel)) {  
          nsIAtom* pseudoElement = pseudoClassList->mAtom;  
          pseudoClassList->mAtom = nsnull;
          listSel->Reset();
          if (listSel->mNext) {
            listSel->mOperator = PRUnichar('>');
            nsAutoPtr<nsCSSSelector> empty(new nsCSSSelector());
            if (!empty) {
              mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
              return PR_FALSE;
            }
            list->AddSelector(empty); 
            listSel = list->mSelectors; 
          }
          listSel->mLowercaseTag = pseudoElement;
        }
        else {  
          nsAutoPtr<nsCSSSelector> pseudoTagSelector(new nsCSSSelector());
          if (!pseudoTagSelector) {
            mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
            return PR_FALSE;
          }
          pseudoTagSelector->mLowercaseTag = pseudoClassList->mAtom; 
#ifdef MOZ_XUL
          if (IsTreePseudoElement(pseudoTagSelector->mLowercaseTag)) {
            
            
            
            
            pseudoTagSelector->mPseudoClassList = pseudoClassList->mNext;
            pseudoClassList->mNext = nsnull;
          }
#endif
          list->AddSelector(pseudoTagSelector);
          pseudoClassList->mAtom = nsnull;
          listSel->mOperator = PRUnichar('>');
          if (nsnull == prevList) { 
            listSel->mPseudoClassList = pseudoClassList->mNext;
          }
          else {
            prevList->mNext = pseudoClassList->mNext;
          }
          pseudoClassList->mNext = nsnull;
          delete pseudoClassList;
          weight += listSel->CalcWeight(); 
        }
        break;  
      }
      prevList = pseudoClassList;
      pseudoClassList = pseudoClassList->mNext;
    }

    combinator = PRUnichar(0);
    if (!GetToken(PR_FALSE)) {
      break;
    }

    
    done = PR_TRUE;
    if (eCSSToken_WhiteSpace == mToken.mType) {
      if (!GetToken(PR_TRUE)) {
        break;
      }
      done = PR_FALSE;
    }

    if (eCSSToken_Symbol == mToken.mType &&
        ('+' == mToken.mSymbol ||
         '>' == mToken.mSymbol ||
         '~' == mToken.mSymbol)) {
      done = PR_FALSE;
      combinator = mToken.mSymbol;
      list->mSelectors->SetOperator(combinator);
    }
    else {
      if (eCSSToken_Symbol == mToken.mType &&
          ('{' == mToken.mSymbol ||
           ',' == mToken.mSymbol)) {
        
        done = PR_TRUE;
      }
      UngetToken(); 
                    
    }

    if (havePseudoElement) {
      break;
    }
    else {
      weight += listSel->CalcWeight();
    }
  }

  if (PRUnichar(0) != combinator) { 
    list = nsnull;
    
    REPORT_UNEXPECTED(PESelectorGroupExtraCombinator);
  }
  aList = list.forget();
  if (aList) {
    aList->mWeight = weight;
  }
  return PRBool(nsnull != aList);
}

#define SEL_MASK_NSPACE   0x01
#define SEL_MASK_ELEM     0x02
#define SEL_MASK_ID       0x04
#define SEL_MASK_CLASS    0x08
#define SEL_MASK_ATTRIB   0x10
#define SEL_MASK_PCLASS   0x20
#define SEL_MASK_PELEM    0x40




CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParseIDSelector(PRInt32&       aDataMask,
                               nsCSSSelector& aSelector)
{
  NS_ASSERTION(!mToken.mIdent.IsEmpty(),
               "Empty mIdent in eCSSToken_ID token?");
  aDataMask |= SEL_MASK_ID;
  aSelector.AddID(mToken.mIdent);
  return eSelectorParsingStatus_Continue;
}




CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParseClassSelector(PRInt32&       aDataMask,
                                  nsCSSSelector& aSelector)
{
  if (! GetToken(PR_FALSE)) { 
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
CSSParserImpl::ParseTypeOrUniversalSelector(PRInt32&       aDataMask,
                                            nsCSSSelector& aSelector,
                                            PRBool         aIsNegated)
{
  nsAutoString buffer;
  if (mToken.IsSymbol('*')) {  
    if (ExpectSymbol('|', PR_FALSE)) {  
      aDataMask |= SEL_MASK_NSPACE;
      aSelector.SetNameSpace(kNameSpaceID_Unknown); 

      if (! GetToken(PR_FALSE)) {
        REPORT_UNEXPECTED_EOF(PETypeSelEOF);
        return eSelectorParsingStatus_Error;
      }
      if (eCSSToken_Ident == mToken.mType) {  
        aDataMask |= SEL_MASK_ELEM;

        aSelector.SetTag(mToken.mIdent, mCaseSensitive);
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
    if (! GetToken(PR_FALSE)) {   
      return eSelectorParsingStatus_Done;
    }
  }
  else if (eCSSToken_Ident == mToken.mType) {    
    buffer = mToken.mIdent; 

    if (ExpectSymbol('|', PR_FALSE)) {  
      aDataMask |= SEL_MASK_NSPACE;
      PRInt32 nameSpaceID;
      if (!GetNamespaceIdForPrefix(buffer, &nameSpaceID)) {
        return eSelectorParsingStatus_Error;
      }
      aSelector.SetNameSpace(nameSpaceID);

      if (! GetToken(PR_FALSE)) {
        REPORT_UNEXPECTED_EOF(PETypeSelEOF);
        return eSelectorParsingStatus_Error;
      }
      if (eCSSToken_Ident == mToken.mType) {  
        aDataMask |= SEL_MASK_ELEM;
       
        aSelector.SetTag(mToken.mIdent, mCaseSensitive);
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
      aSelector.SetTag(buffer, mCaseSensitive);

      aDataMask |= SEL_MASK_ELEM;
    }
    if (! GetToken(PR_FALSE)) {   
      return eSelectorParsingStatus_Done;
    }
  }
  else if (mToken.IsSymbol('|')) {  
    aDataMask |= SEL_MASK_NSPACE;
    aSelector.SetNameSpace(kNameSpaceID_None);  

    
    if (! GetToken(PR_FALSE)) {
      REPORT_UNEXPECTED_EOF(PETypeSelEOF);
      return eSelectorParsingStatus_Error;
    }
    if (eCSSToken_Ident == mToken.mType) {  
      aDataMask |= SEL_MASK_ELEM;
      aSelector.SetTag(mToken.mIdent, mCaseSensitive);
    }
    else if (mToken.IsSymbol('*')) {  
      aDataMask |= SEL_MASK_ELEM;
      
    }
    else {
      REPORT_UNEXPECTED_TOKEN(PETypeSelNotType);
      UngetToken();
      return eSelectorParsingStatus_Error;
    }
    if (! GetToken(PR_FALSE)) {   
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
CSSParserImpl::ParseAttributeSelector(PRInt32&       aDataMask,
                                      nsCSSSelector& aSelector)
{
  if (! GetToken(PR_TRUE)) { 
    REPORT_UNEXPECTED_EOF(PEAttributeNameEOF);
    return eSelectorParsingStatus_Error;
  }

  PRInt32 nameSpaceID = kNameSpaceID_None;
  nsAutoString  attr;
  if (mToken.IsSymbol('*')) { 
    nameSpaceID = kNameSpaceID_Unknown;
    if (ExpectSymbol('|', PR_FALSE)) {
      if (! GetToken(PR_FALSE)) { 
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
    if (! GetToken(PR_FALSE)) { 
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
    if (ExpectSymbol('|', PR_FALSE)) {  
      if (!GetNamespaceIdForPrefix(attr, &nameSpaceID)) {
        return eSelectorParsingStatus_Error;
      }
      if (! GetToken(PR_FALSE)) { 
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

  if (! mCaseSensitive) {
    ToLowerCase(attr);
  }
  if (! GetToken(PR_TRUE)) { 
    REPORT_UNEXPECTED_EOF(PEAttSelInnerEOF);
    return eSelectorParsingStatus_Error;
  }
  if ((eCSSToken_Symbol == mToken.mType) ||
      (eCSSToken_Includes == mToken.mType) ||
      (eCSSToken_Dashmatch == mToken.mType) ||
      (eCSSToken_Beginsmatch == mToken.mType) ||
      (eCSSToken_Endsmatch == mToken.mType) ||
      (eCSSToken_Containsmatch == mToken.mType)) {
    PRUint8 func;
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
      if (! GetToken(PR_TRUE)) { 
        REPORT_UNEXPECTED_EOF(PEAttSelValueEOF);
        return eSelectorParsingStatus_Error;
      }
      if ((eCSSToken_Ident == mToken.mType) || (eCSSToken_String == mToken.mType)) {
        nsAutoString  value(mToken.mIdent);
        if (! GetToken(PR_TRUE)) { 
          REPORT_UNEXPECTED_EOF(PEAttSelCloseEOF);
          return eSelectorParsingStatus_Error;
        }
        if (mToken.IsSymbol(']')) {
          PRBool isCaseSensitive = PR_TRUE;

          
          
          
          
          if (!mCaseSensitive && nameSpaceID == kNameSpaceID_None) {
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
              nsnull
            };
            short i = 0;
            const char* htmlAttr;
            while ((htmlAttr = caseInsensitiveHTMLAttribute[i++])) {
              if (attr.EqualsIgnoreCase(htmlAttr)) {
                isCaseSensitive = PR_FALSE;
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
CSSParserImpl::ParsePseudoSelector(PRInt32&       aDataMask,
                                   nsCSSSelector& aSelector,
                                   PRBool         aIsNegated)
{
  if (! GetToken(PR_FALSE)) { 
    REPORT_UNEXPECTED_EOF(PEPseudoSelEOF);
    return eSelectorParsingStatus_Error;
  }

  
  PRBool parsingPseudoElement = PR_FALSE;
  if (mToken.IsSymbol(':')) {
    parsingPseudoElement = PR_TRUE;
    if (! GetToken(PR_FALSE)) { 
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
  ToLowerCase(buffer);
  nsCOMPtr<nsIAtom> pseudo = do_GetAtom(buffer);

  
  PRBool isTreePseudo = PR_FALSE;
#ifdef MOZ_XUL
  isTreePseudo = IsTreePseudoElement(pseudo);
  
  
  
  
  
  
  
  PRBool isTree = (eCSSToken_Function == mToken.mType) && isTreePseudo;
#endif
  PRBool isPseudoElement = nsCSSPseudoElements::IsPseudoElement(pseudo);
  
  
  PRBool isAnonBox = nsCSSAnonBoxes::IsAnonBox(pseudo) &&
    (mUnsafeRulesEnabled || isTreePseudo);
  PRBool isPseudoClass = nsCSSPseudoClasses::IsPseudoClass(pseudo);

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
       nsCSSPseudoClasses::notPseudo == pseudo ||
       nsCSSPseudoClasses::HasStringArg(pseudo) ||
       nsCSSPseudoClasses::HasNthPairArg(pseudo))) {
    
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

  if (!parsingPseudoElement && nsCSSPseudoClasses::notPseudo == pseudo) {
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
    if (nsCSSPseudoClasses::HasStringArg(pseudo)) {
      nsSelectorParsingStatus parsingStatus =
        ParsePseudoClassWithIdentArg(aSelector, pseudo);
      if (eSelectorParsingStatus_Continue != parsingStatus) {
        return parsingStatus;
      }
    }
    else if (nsCSSPseudoClasses::HasNthPairArg(pseudo)) {
      nsSelectorParsingStatus parsingStatus =
        ParsePseudoClassWithNthPairArg(aSelector, pseudo);
      if (eSelectorParsingStatus_Continue != parsingStatus) {
        return parsingStatus;
      }
    }
    else {
      aSelector.AddPseudoClass(pseudo);
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
      aSelector.AddPseudoClass(pseudo); 

#ifdef MOZ_XUL
      if (isTree) {
        
        
        
        
        if (!ParseTreePseudoElement(aSelector)) {
          return eSelectorParsingStatus_Error;
        }
      }
#endif

      
      if (GetToken(PR_FALSE)) { 
        if ((eCSSToken_WhiteSpace == mToken.mType) ||
            (mToken.IsSymbol('{') || mToken.IsSymbol(','))) {
          UngetToken();
          return eSelectorParsingStatus_Done;
        }
        REPORT_UNEXPECTED_TOKEN(PEPseudoSelTrailing);
        UngetToken();
        return eSelectorParsingStatus_Error;
      }
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
CSSParserImpl::ParseNegatedSimpleSelector(PRInt32&       aDataMask,
                                          nsCSSSelector& aSelector)
{
  
  if (!ExpectSymbol('(', PR_FALSE)) {
    REPORT_UNEXPECTED_TOKEN(PENegationBadArg);
    return eSelectorParsingStatus_Error;
  }

  if (! GetToken(PR_TRUE)) { 
    REPORT_UNEXPECTED_EOF(PENegationEOF);
    return eSelectorParsingStatus_Error;
  }

  
  
  
  
  
  
  
  nsCSSSelector *newSel = new nsCSSSelector();
  if (!newSel) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return eSelectorParsingStatus_Error;
  }
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
    parsingStatus = ParsePseudoSelector(aDataMask, *newSel, PR_TRUE);
  }
  else if (mToken.IsSymbol('[')) {    
    parsingStatus = ParseAttributeSelector(aDataMask, *newSel);
  }
  else {
    
    parsingStatus = ParseTypeOrUniversalSelector(aDataMask, *newSel, PR_TRUE);
  }
  if (eSelectorParsingStatus_Error == parsingStatus) {
    REPORT_UNEXPECTED_TOKEN(PENegationBadInner);
    return parsingStatus;
  }
  
  if (!ExpectSymbol(')', PR_TRUE)) {
    REPORT_UNEXPECTED_TOKEN(PENegationNoClose);
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
                                            nsIAtom*       aPseudo)
{
  
  if (!ExpectSymbol('(', PR_FALSE)) {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassNoArg);
    return eSelectorParsingStatus_Error;
  }

  if (! GetToken(PR_TRUE)) { 
    REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
    return eSelectorParsingStatus_Error;
  }
  
  if (eCSSToken_Ident != mToken.mType) {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotIdent);
    UngetToken();
    
    return eSelectorParsingStatus_Error;
  }

  
  if (aPseudo == nsCSSPseudoClasses::mozLocaleDir) {
    if (!mToken.mIdent.EqualsLiteral("ltr") &&
        !mToken.mIdent.EqualsLiteral("rtl")) {
      return eSelectorParsingStatus_Error;
    }
  }

  
  aSelector.AddPseudoClass(aPseudo, mToken.mIdent.get());

  
  if (!ExpectSymbol(')', PR_TRUE)) {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassNoClose);
    
    return eSelectorParsingStatus_Error;
  }

  return eSelectorParsingStatus_Continue;
}

CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParsePseudoClassWithNthPairArg(nsCSSSelector& aSelector,
                                              nsIAtom*       aPseudo)
{
  PRInt32 numbers[2] = { 0, 0 };
  PRBool lookForB = PR_TRUE;

  
  if (!ExpectSymbol('(', PR_FALSE)) {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassNoArg);
    return eSelectorParsingStatus_Error;
  }

  
  

  if (! GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
    return eSelectorParsingStatus_Error;
  }

  if (eCSSToken_Ident == mToken.mType || eCSSToken_Dimension == mToken.mType) {
    
    
    
    
    
    PRUint32 truncAt = 0;
    if (StringBeginsWith(mToken.mIdent, NS_LITERAL_STRING("n-"))) {
      truncAt = 1;
    } else if (StringBeginsWith(mToken.mIdent, NS_LITERAL_STRING("-n-"))) {
      truncAt = 2;
    }
    if (truncAt != 0) {
      for (PRUint32 i = mToken.mIdent.Length() - 1; i >= truncAt; --i) {
        mScanner.Pushback(mToken.mIdent[i]);
      }
      mToken.mIdent.Truncate(truncAt);
    }
  }

  if (eCSSToken_Ident == mToken.mType) {
    if (mToken.mIdent.EqualsIgnoreCase("odd")) {
      numbers[0] = 2;
      numbers[1] = 1;
      lookForB = PR_FALSE;
    }
    else if (mToken.mIdent.EqualsIgnoreCase("even")) {
      numbers[0] = 2;
      numbers[1] = 0;
      lookForB = PR_FALSE;
    }
    else if (mToken.mIdent.EqualsIgnoreCase("n")) {
      numbers[0] = 1;
    }
    else if (mToken.mIdent.EqualsIgnoreCase("-n")) {
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
    lookForB = PR_FALSE;
  }
  else if (eCSSToken_Dimension == mToken.mType) {
    if (!mToken.mIntegerValid || !mToken.mIdent.EqualsIgnoreCase("n")) {
      REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
      
      return eSelectorParsingStatus_Error;
    }
    numbers[0] = mToken.mInteger;
  }
  
  else {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassArgNotNth);
    
    return eSelectorParsingStatus_Error;
  }

  if (! GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
    return eSelectorParsingStatus_Error;
  }
  if (lookForB && !mToken.IsSymbol(')')) {
    
    
    
    PRBool haveSign = PR_FALSE;
    PRInt32 sign = 1;
    if (mToken.IsSymbol('+') || mToken.IsSymbol('-')) {
      haveSign = PR_TRUE;
      if (mToken.IsSymbol('-')) {
        sign = -1;
      }
      if (! GetToken(PR_TRUE)) {
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
    if (! GetToken(PR_TRUE)) {
      REPORT_UNEXPECTED_EOF(PEPseudoClassArgEOF);
      return eSelectorParsingStatus_Error;
    }
  }
  if (!mToken.IsSymbol(')')) {
    REPORT_UNEXPECTED_TOKEN(PEPseudoClassNoClose);
    
    return eSelectorParsingStatus_Error;
  }
  aSelector.AddPseudoClass(aPseudo, numbers);
  return eSelectorParsingStatus_Continue;
}






CSSParserImpl::nsSelectorParsingStatus
CSSParserImpl::ParseSelector(nsCSSSelector& aSelector)
{
  if (! GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PESelectorEOF);
    return eSelectorParsingStatus_Error;
  }

  PRInt32 dataMask = 0;
  nsSelectorParsingStatus parsingStatus =
    ParseTypeOrUniversalSelector(dataMask, aSelector, PR_FALSE);
  if (parsingStatus != eSelectorParsingStatus_Continue) {
    return parsingStatus;
  }

  for (;;) {
    if (eCSSToken_ID == mToken.mType) { 
      parsingStatus = ParseIDSelector(dataMask, aSelector);
    }
    else if (mToken.IsSymbol('.')) {    
      parsingStatus = ParseClassSelector(dataMask, aSelector);
    }
    else if (mToken.IsSymbol(':')) {    
      parsingStatus = ParsePseudoSelector(dataMask, aSelector, PR_FALSE);
    }
    else if (mToken.IsSymbol('[')) {    
      parsingStatus = ParseAttributeSelector(dataMask, aSelector);
    }
    else {  
      parsingStatus = eSelectorParsingStatus_Done;
      break;
    }

    if (parsingStatus != eSelectorParsingStatus_Continue) {
      return parsingStatus;
    }

    if (! GetToken(PR_FALSE)) { 
      return eSelectorParsingStatus_Done;
    }
  }
  UngetToken();
  return dataMask ? parsingStatus : eSelectorParsingStatus_Empty;
}

nsCSSDeclaration*
CSSParserImpl::ParseDeclarationBlock(PRBool aCheckForBraces)
{
  if (aCheckForBraces) {
    if (!ExpectSymbol('{', PR_TRUE)) {
      REPORT_UNEXPECTED_TOKEN(PEBadDeclBlockStart);
      OUTPUT_ERROR();
      return nsnull;
    }
  }
  nsCSSDeclaration* declaration = new nsCSSDeclaration();
  mData.AssertInitialState();
  if (declaration) {
    for (;;) {
      PRBool changed;
      if (!ParseDeclaration(declaration, aCheckForBraces,
                            PR_TRUE, &changed)) {
        if (!SkipDeclaration(aCheckForBraces)) {
          break;
        }
        if (aCheckForBraces) {
          if (ExpectSymbol('}', PR_TRUE)) {
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

PRBool
CSSParserImpl::ParseColor(nsCSSValue& aValue)
{
  if (!GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PEColorEOF);
    return PR_FALSE;
  }

  nsCSSToken* tk = &mToken;
  nscolor rgba;
  switch (tk->mType) {
    case eCSSToken_ID:
    case eCSSToken_Ref:
      
      if (NS_HexToRGB(tk->mIdent, &rgba)) {
        aValue.SetColorValue(rgba);
        return PR_TRUE;
      }
      break;

    case eCSSToken_Ident:
      if (NS_ColorNameToRGB(tk->mIdent, &rgba)) {
        aValue.SetStringValue(tk->mIdent, eCSSUnit_Ident);
        return PR_TRUE;
      }
      else {
        nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(tk->mIdent);
        if (eCSSKeyword_UNKNOWN < keyword) { 
          PRInt32 value;
          if (nsCSSProps::FindKeyword(keyword, nsCSSProps::kColorKTable, value)) {
            aValue.SetIntValue(value, eCSSUnit_EnumColor);
            return PR_TRUE;
          }
        }
      }
      break;
    case eCSSToken_Function:
      if (mToken.mIdent.LowerCaseEqualsLiteral("rgb")) {
        
        PRUint8 r, g, b;
        PRInt32 type = COLOR_TYPE_UNKNOWN;
        if (ExpectSymbol('(', PR_FALSE) && 
            ParseColorComponent(r, type, ',') &&
            ParseColorComponent(g, type, ',') &&
            ParseColorComponent(b, type, ')')) {
          aValue.SetColorValue(NS_RGB(r,g,b));
          return PR_TRUE;
        }
        return PR_FALSE;  
      }
      else if (mToken.mIdent.LowerCaseEqualsLiteral("-moz-rgba") ||
               mToken.mIdent.LowerCaseEqualsLiteral("rgba")) {
        
        PRUint8 r, g, b, a;
        PRInt32 type = COLOR_TYPE_UNKNOWN;
        if (ExpectSymbol('(', PR_FALSE) && 
            ParseColorComponent(r, type, ',') &&
            ParseColorComponent(g, type, ',') &&
            ParseColorComponent(b, type, ',') &&
            ParseColorOpacity(a)) {
          aValue.SetColorValue(NS_RGBA(r, g, b, a));
          return PR_TRUE;
        }
        return PR_FALSE;  
      }
      else if (mToken.mIdent.LowerCaseEqualsLiteral("hsl")) {
        
        
        if (ParseHSLColor(rgba, ')')) {
          aValue.SetColorValue(rgba);
          return PR_TRUE;
        }
        return PR_FALSE;
      }
      else if (mToken.mIdent.LowerCaseEqualsLiteral("-moz-hsla") ||
               mToken.mIdent.LowerCaseEqualsLiteral("hsla")) {
        
        
        
        PRUint8 a;
        if (ParseHSLColor(rgba, ',') &&
            ParseColorOpacity(a)) {
          aValue.SetColorValue(NS_RGBA(NS_GET_R(rgba), NS_GET_G(rgba),
                                       NS_GET_B(rgba), a));
          return PR_TRUE;
        }
        return PR_FALSE;
      }
      break;
    default:
      break;
  }

  
  if (mNavQuirkMode && !IsParsingCompoundProperty()) {
    
    
    
    
    
    
    
    
    
    
    
    
    
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
      return PR_TRUE;
    }
  }

  
  REPORT_UNEXPECTED_TOKEN(PEColorNotColor);
  UngetToken();
  return PR_FALSE;
}



PRBool
CSSParserImpl::ParseColorComponent(PRUint8& aComponent,
                                   PRInt32& aType,
                                   char aStop)
{
  if (!GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PEColorComponentEOF);
    return PR_FALSE;
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
        return PR_FALSE;
      default:
        NS_NOTREACHED("Someone forgot to add the new color component type in here");
    }

    if (!mToken.mIntegerValid) {
      REPORT_UNEXPECTED_TOKEN(PEExpectedInt);
      UngetToken();
      return PR_FALSE;
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
        return PR_FALSE;
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
    return PR_FALSE;
  }
  if (ExpectSymbol(aStop, PR_TRUE)) {
    if (value < 0.0f) value = 0.0f;
    if (value > 255.0f) value = 255.0f;
    aComponent = NSToIntRound(value);
    return PR_TRUE;
  }
  const PRUnichar stopString[] = { PRUnichar(aStop), PRUnichar(0) };
  const PRUnichar *params[] = {
    nsnull,
    stopString
  };
  REPORT_UNEXPECTED_TOKEN_P(PEColorComponentBadTerm, params);
  return PR_FALSE;
}


PRBool
CSSParserImpl::ParseHSLColor(nscolor& aColor,
                             char aStop)
{
  float h, s, l;
  if (!ExpectSymbol('(', PR_FALSE)) {
    NS_ERROR("How did this get to be a function token?");
    return PR_FALSE;
  }

  
  if (!GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PEColorHueEOF);
    return PR_FALSE;
  }
  if (mToken.mType != eCSSToken_Number) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedNumber);
    UngetToken();
    return PR_FALSE;
  }
  h = mToken.mNumber;
  h /= 360.0f;
  
  h = h - floor(h);

  if (!ExpectSymbol(',', PR_TRUE)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedComma);
    return PR_FALSE;
  }

  
  if (!GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PEColorSaturationEOF);
    return PR_FALSE;
  }
  if (mToken.mType != eCSSToken_Percentage) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedPercent);
    UngetToken();
    return PR_FALSE;
  }
  s = mToken.mNumber;
  if (s < 0.0f) s = 0.0f;
  if (s > 1.0f) s = 1.0f;

  if (!ExpectSymbol(',', PR_TRUE)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedComma);
    return PR_FALSE;
  }

  
  if (!GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PEColorLightnessEOF);
    return PR_FALSE;
  }
  if (mToken.mType != eCSSToken_Percentage) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedPercent);
    UngetToken();
    return PR_FALSE;
  }
  l = mToken.mNumber;
  if (l < 0.0f) l = 0.0f;
  if (l > 1.0f) l = 1.0f;

  if (ExpectSymbol(aStop, PR_TRUE)) {
    aColor = NS_HSL2RGB(h, s, l);
    return PR_TRUE;
  }

  const PRUnichar stopString[] = { PRUnichar(aStop), PRUnichar(0) };
  const PRUnichar *params[] = {
    nsnull,
    stopString
  };
  REPORT_UNEXPECTED_TOKEN_P(PEColorComponentBadTerm, params);
  return PR_FALSE;
}


PRBool
CSSParserImpl::ParseColorOpacity(PRUint8& aOpacity)
{
  if (!GetToken(PR_TRUE)) {
    REPORT_UNEXPECTED_EOF(PEColorOpacityEOF);
    return PR_FALSE;
  }

  if (mToken.mType != eCSSToken_Number) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedNumber);
    UngetToken();
    return PR_FALSE;
  }

  if (mToken.mNumber < 0.0f) {
    mToken.mNumber = 0.0f;
  } else if (mToken.mNumber > 1.0f) {
    mToken.mNumber = 1.0f;
  }

  PRUint8 value = nsStyleUtil::FloatToColorComponent(mToken.mNumber);
  
  
  NS_ASSERTION(fabs(255.0f*mToken.mNumber - value) <= 0.51f,
               "FloatToColorComponent did something weird");

  if (!ExpectSymbol(')', PR_TRUE)) {
    REPORT_UNEXPECTED_TOKEN(PEExpectedCloseParen);
    return PR_FALSE;
  }

  aOpacity = value;

  return PR_TRUE;
}

#ifdef MOZ_XUL
PRBool
CSSParserImpl::ParseTreePseudoElement(nsCSSSelector& aSelector)
{
  if (ExpectSymbol('(', PR_FALSE)) {
    while (!ExpectSymbol(')', PR_TRUE)) {
      if (!GetToken(PR_TRUE)) {
        return PR_FALSE;
      }
      else if (eCSSToken_Ident == mToken.mType) {
        nsCOMPtr<nsIAtom> pseudo = do_GetAtom(mToken.mIdent);
        aSelector.AddPseudoClass(pseudo);
      }
      else if (eCSSToken_Symbol == mToken.mType) {
        if (!mToken.IsSymbol(','))
          return PR_FALSE;
      }
      else return PR_FALSE;
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}
#endif



PRBool
CSSParserImpl::ParseDeclaration(nsCSSDeclaration* aDeclaration,
                                PRBool aCheckForBraces,
                                PRBool aMustCallValueAppended,
                                PRBool* aChanged)
{
  mTempData.AssertInitialState();

  
  nsCSSToken* tk = &mToken;
  nsAutoString propertyName;
  for (;;) {
    if (!GetToken(PR_TRUE)) {
      if (aCheckForBraces) {
        REPORT_UNEXPECTED_EOF(PEDeclEndEOF);
      }
      return PR_FALSE;
    }
    if (eCSSToken_Ident == tk->mType) {
      propertyName = tk->mIdent;
      
      if (!ExpectSymbol(':', PR_TRUE)) {
        REPORT_UNEXPECTED_TOKEN(PEParseDeclarationNoColon);
        REPORT_UNEXPECTED(PEDeclDropped);
        OUTPUT_ERROR();
        return PR_FALSE;
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
    return PR_FALSE;
  }

  
  nsCSSProperty propID = nsCSSProps::LookupProperty(propertyName);
  if (eCSSProperty_UNKNOWN == propID) { 
    if (!NonMozillaVendorIdentifier(propertyName)) {
      const PRUnichar *params[] = {
        propertyName.get()
      };
      REPORT_UNEXPECTED_P(PEUnknownProperty, params);
      REPORT_UNEXPECTED(PEDeclDropped);
      OUTPUT_ERROR();
    }

    return PR_FALSE;
  }
  if (! ParseProperty(propID)) {
    
    const PRUnichar *params[] = {
      propertyName.get()
    };
    REPORT_UNEXPECTED_P(PEValueParsingError, params);
    REPORT_UNEXPECTED(PEDeclDropped);
    OUTPUT_ERROR();
    ClearTempData(propID);
    return PR_FALSE;
  }
  CLEAR_ERROR();

  
  PRBool isImportant = PR_FALSE;
  if (!GetToken(PR_TRUE)) {
    
    TransferTempData(aDeclaration, propID, isImportant,
                     aMustCallValueAppended, aChanged);
    return PR_TRUE;
  }

  if (eCSSToken_Symbol == tk->mType && '!' == tk->mSymbol) {
    
    if (!GetToken(PR_TRUE)) {
      
      REPORT_UNEXPECTED_EOF(PEImportantEOF);
      ClearTempData(propID);
      return PR_FALSE;
    }
    if ((eCSSToken_Ident != tk->mType) ||
        !tk->mIdent.LowerCaseEqualsLiteral("important")) {
      REPORT_UNEXPECTED_TOKEN(PEExpectedImportant);
      OUTPUT_ERROR();
      UngetToken();
      ClearTempData(propID);
      return PR_FALSE;
    }
    isImportant = PR_TRUE;
  }
  else {
    
    UngetToken();
  }

  
  
  
  if (!GetToken(PR_TRUE)) {
    
    TransferTempData(aDeclaration, propID, isImportant,
                     aMustCallValueAppended, aChanged);
    return PR_TRUE;
  }
  if (eCSSToken_Symbol == tk->mType) {
    if (';' == tk->mSymbol) {
      TransferTempData(aDeclaration, propID, isImportant,
                       aMustCallValueAppended, aChanged);
      return PR_TRUE;
    }
    if (aCheckForBraces && '}' == tk->mSymbol) {
      
      
      UngetToken();
      TransferTempData(aDeclaration, propID, isImportant,
                       aMustCallValueAppended, aChanged);
      return PR_TRUE;
    }
  }
  if (aCheckForBraces)
    REPORT_UNEXPECTED_TOKEN(PEBadDeclOrRuleEnd2);
  else
    REPORT_UNEXPECTED_TOKEN(PEBadDeclEnd);
  REPORT_UNEXPECTED(PEDeclDropped);
  OUTPUT_ERROR();
  ClearTempData(propID);
  return PR_FALSE;
}

void
CSSParserImpl::ClearTempData(nsCSSProperty aPropID)
{
  if (nsCSSProps::IsShorthand(aPropID)) {
    CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, aPropID) {
      mTempData.ClearProperty(*p);
    }
  } else {
    mTempData.ClearProperty(aPropID);
  }
  mTempData.AssertInitialState();
}

void
CSSParserImpl::TransferTempData(nsCSSDeclaration* aDeclaration,
                                nsCSSProperty aPropID, PRBool aIsImportant,
                                PRBool aMustCallValueAppended,
                                PRBool* aChanged)
{
  if (nsCSSProps::IsShorthand(aPropID)) {
    CSSPROPS_FOR_SHORTHAND_SUBPROPERTIES(p, aPropID) {
      DoTransferTempData(aDeclaration, *p, aIsImportant,
                         aMustCallValueAppended, aChanged);
    }
  } else {
    DoTransferTempData(aDeclaration, aPropID, aIsImportant,
                       aMustCallValueAppended, aChanged);
  }
  mTempData.AssertInitialState();
}




void
CSSParserImpl::DoTransferTempData(nsCSSDeclaration* aDeclaration,
                                  nsCSSProperty aPropID, PRBool aIsImportant,
                                  PRBool aMustCallValueAppended,
                                  PRBool* aChanged)
{
  NS_ASSERTION(mTempData.HasPropertyBit(aPropID), "oops");
  if (aIsImportant) {
    if (!mData.HasImportantBit(aPropID))
      *aChanged = PR_TRUE;
    mData.SetImportantBit(aPropID);
  } else {
    if (mData.HasImportantBit(aPropID)) {
      mTempData.ClearProperty(aPropID);
      return;
    }
  }

  if (aMustCallValueAppended || !mData.HasPropertyBit(aPropID)) {
    aDeclaration->ValueAppended(aPropID);
  }

  mData.SetPropertyBit(aPropID);
  mTempData.ClearPropertyBit(aPropID);

  




  void *v_source = mTempData.PropertyAt(aPropID);
  void *v_dest = mData.PropertyAt(aPropID);
  CopyValue(v_source, v_dest, aPropID, aChanged);
}

void
CSSParserImpl::CopyValue(void *aSource, void *aDest, nsCSSProperty aPropID,
                         PRBool* aChanged)
{
  switch (nsCSSProps::kTypeTable[aPropID]) {
    case eCSSType_Value: {
      nsCSSValue *source = static_cast<nsCSSValue*>(aSource);
      nsCSSValue *dest = static_cast<nsCSSValue*>(aDest);
      if (*source != *dest)
        *aChanged = PR_TRUE;
      dest->~nsCSSValue();
      memcpy(dest, source, sizeof(nsCSSValue));
      new (source) nsCSSValue();
    } break;

    case eCSSType_Rect: {
      nsCSSRect *source = static_cast<nsCSSRect*>(aSource);
      nsCSSRect *dest = static_cast<nsCSSRect*>(aDest);
      if (*source != *dest)
        *aChanged = PR_TRUE;
      dest->~nsCSSRect();
      memcpy(dest, source, sizeof(nsCSSRect));
      new (source) nsCSSRect();
    } break;

    case eCSSType_ValuePair: {
      nsCSSValuePair *source = static_cast<nsCSSValuePair*>(aSource);
      nsCSSValuePair *dest = static_cast<nsCSSValuePair*>(aDest);
      if (*source != *dest)
        *aChanged = PR_TRUE;
      dest->~nsCSSValuePair();
      memcpy(dest, source, sizeof(nsCSSValuePair));
      new (source) nsCSSValuePair();
    } break;

    case eCSSType_ValueList: {
      nsCSSValueList **source = static_cast<nsCSSValueList**>(aSource);
      nsCSSValueList **dest = static_cast<nsCSSValueList**>(aDest);
      if (!nsCSSValueList::Equal(*source, *dest))
        *aChanged = PR_TRUE;
      delete *dest;
      *dest = *source;
      *source = nsnull;
    } break;

    case eCSSType_ValuePairList: {
      nsCSSValuePairList **source =
        static_cast<nsCSSValuePairList**>(aSource);
      nsCSSValuePairList **dest =
        static_cast<nsCSSValuePairList**>(aDest);
      if (!nsCSSValuePairList::Equal(*source, *dest))
        *aChanged = PR_TRUE;
      delete *dest;
      *dest = *source;
      *source = nsnull;
    } break;
  }
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

PRBool
CSSParserImpl::ParseEnum(nsCSSValue& aValue,
                         const PRInt32 aKeywordTable[])
{
  nsSubstring* ident = NextIdent();
  if (nsnull == ident) {
    return PR_FALSE;
  }
  nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(*ident);
  if (eCSSKeyword_UNKNOWN < keyword) {
    PRInt32 value;
    if (nsCSSProps::FindKeyword(keyword, aKeywordTable, value)) {
      aValue.SetIntValue(value, eCSSUnit_Enumerated);
      return PR_TRUE;
    }
  }

  
  UngetToken();
  return PR_FALSE;
}


struct UnitInfo {
  char name[5];  
                 
  PRUint32 length;
  nsCSSUnit unit;
  PRInt32 type;
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
  { STR_WITH_LEN("pc"), eCSSUnit_Pica, VARIANT_LENGTH },
  { STR_WITH_LEN("deg"), eCSSUnit_Degree, VARIANT_ANGLE },
  { STR_WITH_LEN("grad"), eCSSUnit_Grad, VARIANT_ANGLE },
  { STR_WITH_LEN("rad"), eCSSUnit_Radian, VARIANT_ANGLE },
  { STR_WITH_LEN("hz"), eCSSUnit_Hertz, VARIANT_FREQUENCY },
  { STR_WITH_LEN("khz"), eCSSUnit_Kilohertz, VARIANT_FREQUENCY },
  { STR_WITH_LEN("s"), eCSSUnit_Seconds, VARIANT_TIME },
  { STR_WITH_LEN("ms"), eCSSUnit_Milliseconds, VARIANT_TIME }
};

#undef STR_WITH_LEN

PRBool
CSSParserImpl::TranslateDimension(nsCSSValue& aValue,
                                  PRInt32 aVariantMask,
                                  float aNumber,
                                  const nsString& aUnit)
{
  nsCSSUnit units;
  PRInt32   type = 0;
  if (!aUnit.IsEmpty()) {
    PRUint32 i;
    for (i = 0; i < NS_ARRAY_LENGTH(UnitData); ++i) {
      if (aUnit.LowerCaseEqualsASCII(UnitData[i].name,
                                     UnitData[i].length)) {
        units = UnitData[i].unit;
        type = UnitData[i].type;
        break;
      }
    }

    if (i == NS_ARRAY_LENGTH(UnitData)) {
      
      return PR_FALSE;
    }
  } else {
    
    NS_ASSERTION(0 == aNumber, "numbers without units must be 0");
    if ((VARIANT_LENGTH & aVariantMask) != 0) {
      units = eCSSUnit_Point;
      type = VARIANT_LENGTH;
    }
    else if ((VARIANT_ANGLE & aVariantMask) != 0) {
      units = eCSSUnit_Degree;
      type = VARIANT_ANGLE;
    }
    else if ((VARIANT_FREQUENCY & aVariantMask) != 0) {
      units = eCSSUnit_Hertz;
      type = VARIANT_FREQUENCY;
    }
    else if ((VARIANT_TIME & aVariantMask) != 0) {
      units = eCSSUnit_Seconds;
      type = VARIANT_TIME;
    }
    else {
      NS_ERROR("Variant mask does not include dimension; why were we called?");
      return PR_FALSE;
    }
  }
  if ((type & aVariantMask) != 0) {
    aValue.SetFloatValue(aNumber, units);
    return PR_TRUE;
  }
  return PR_FALSE;
}

#define VARIANT_ALL_NONNUMERIC \
  VARIANT_KEYWORD | \
  VARIANT_COLOR | \
  VARIANT_URL | \
  VARIANT_STRING | \
  VARIANT_COUNTER | \
  VARIANT_ATTR | \
  VARIANT_IDENTIFIER | \
  VARIANT_AUTO | \
  VARIANT_INHERIT | \
  VARIANT_NONE | \
  VARIANT_NORMAL | \
  VARIANT_SYSFONT

PRBool
CSSParserImpl::ParseNonNegativeVariant(nsCSSValue& aValue,
                                       PRInt32 aVariantMask,
                                       const PRInt32 aKeywordTable[])
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
        return PR_FALSE;
      }
    }
    else if (aValue.GetUnit() == eCSSUnit_Percent) {
      if (aValue.GetPercentValue() < 0) {
        UngetToken();
        return PR_FALSE;
      }
    } else if (aValue.GetUnit() == eCSSUnit_Integer) {
      if (aValue.GetIntValue() < 0) {
        UngetToken();
        return PR_FALSE;
      }
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParsePositiveNonZeroVariant(nsCSSValue& aValue,
                                           PRInt32 aVariantMask,
                                           const PRInt32 aKeywordTable[])
{
  
  
  NS_ABORT_IF_FALSE((aVariantMask & ~(VARIANT_ALL_NONNUMERIC |
                                      VARIANT_INTEGER)) == 0,
                    "need to update code below to handle additional variants");

  if (ParseVariant(aValue, aVariantMask, aKeywordTable)) {
    if (aValue.GetUnit() == eCSSUnit_Integer) {
      if (aValue.GetIntValue() <= 0) {
        UngetToken();
        return PR_FALSE;
      }
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}


PRBool
CSSParserImpl::ParseVariant(nsCSSValue& aValue,
                            PRInt32 aVariantMask,
                            const PRInt32 aKeywordTable[])
{
  NS_ASSERTION(IsParsingCompoundProperty() ||
               ((~aVariantMask) & (VARIANT_LENGTH|VARIANT_COLOR)),
               "cannot distinguish lengths and colors in quirks mode");

  if (!GetToken(PR_TRUE)) {
    return PR_FALSE;
  }
  nsCSSToken* tk = &mToken;
  if (((aVariantMask & (VARIANT_AHK | VARIANT_NORMAL | VARIANT_NONE)) != 0) &&
      (eCSSToken_Ident == tk->mType)) {
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(tk->mIdent);
    if (eCSSKeyword_UNKNOWN < keyword) { 
      if ((aVariantMask & VARIANT_AUTO) != 0) {
        if (eCSSKeyword_auto == keyword) {
          aValue.SetAutoValue();
          return PR_TRUE;
        }
      }
      if ((aVariantMask & VARIANT_INHERIT) != 0) {
        
        
        
        if (eCSSKeyword_inherit == keyword) {
          aValue.SetInheritValue();
          return PR_TRUE;
        }
        else if (eCSSKeyword__moz_initial == keyword) { 
          aValue.SetInitialValue();
          return PR_TRUE;
        }
      }
      if ((aVariantMask & VARIANT_NONE) != 0) {
        if (eCSSKeyword_none == keyword) {
          aValue.SetNoneValue();
          return PR_TRUE;
        }
      }
      if ((aVariantMask & VARIANT_NORMAL) != 0) {
        if (eCSSKeyword_normal == keyword) {
          aValue.SetNormalValue();
          return PR_TRUE;
        }
      }
      if ((aVariantMask & VARIANT_SYSFONT) != 0) {
        if (eCSSKeyword__moz_use_system_font == keyword &&
            !IsParsingCompoundProperty()) {
          aValue.SetSystemFontValue();
          return PR_TRUE;
        }
      }
      if ((aVariantMask & VARIANT_KEYWORD) != 0) {
        PRInt32 value;
        if (nsCSSProps::FindKeyword(keyword, aKeywordTable, value)) {
          aValue.SetIntValue(value, eCSSUnit_Enumerated);
          return PR_TRUE;
        }
      }
    }
  }
  if (((aVariantMask & (VARIANT_LENGTH | VARIANT_ANGLE | VARIANT_FREQUENCY | VARIANT_TIME)) != 0) &&
      tk->IsDimension()) {
    if (TranslateDimension(aValue, aVariantMask, tk->mNumber, tk->mIdent)) {
      return PR_TRUE;
    }
    
    UngetToken();
    return PR_FALSE;
  }
  if (((aVariantMask & VARIANT_PERCENT) != 0) &&
      (eCSSToken_Percentage == tk->mType)) {
    aValue.SetPercentValue(tk->mNumber);
    return PR_TRUE;
  }
  if (((aVariantMask & VARIANT_NUMBER) != 0) &&
      (eCSSToken_Number == tk->mType)) {
    aValue.SetFloatValue(tk->mNumber, eCSSUnit_Number);
    return PR_TRUE;
  }
  if (((aVariantMask & VARIANT_INTEGER) != 0) &&
      (eCSSToken_Number == tk->mType) && tk->mIntegerValid) {
    aValue.SetIntValue(tk->mInteger, eCSSUnit_Integer);
    return PR_TRUE;
  }
  if (mNavQuirkMode && !IsParsingCompoundProperty()) { 
    if (((aVariantMask & VARIANT_LENGTH) != 0) &&
        (eCSSToken_Number == tk->mType)) {
      aValue.SetFloatValue(tk->mNumber, eCSSUnit_Pixel);
      return PR_TRUE;
    }
  }

#ifdef  MOZ_SVG
  if (IsSVGMode() && !IsParsingCompoundProperty()) {
    
    
    if (((aVariantMask & VARIANT_LENGTH) != 0) &&
        (eCSSToken_Number == tk->mType)) {
      aValue.SetFloatValue(tk->mNumber, eCSSUnit_Pixel);
      return PR_TRUE;
    }
  }
#endif

  if (((aVariantMask & VARIANT_URL) != 0) &&
      (eCSSToken_Function == tk->mType) &&
      tk->mIdent.LowerCaseEqualsLiteral("url")) {
    if (ParseURL(aValue)) {
      return PR_TRUE;
    }
    return PR_FALSE;
  }
  if ((aVariantMask & VARIANT_GRADIENT) != 0 &&
      eCSSToken_Function == tk->mType) {
    
    if (tk->mIdent.LowerCaseEqualsLiteral("-moz-linear-gradient"))
      return ParseGradient(aValue, PR_FALSE);

    if (tk->mIdent.LowerCaseEqualsLiteral("-moz-radial-gradient"))
      return ParseGradient(aValue, PR_TRUE);
  }
  if ((aVariantMask & VARIANT_COLOR) != 0) {
    if ((mNavQuirkMode && !IsParsingCompoundProperty()) || 
        (eCSSToken_ID == tk->mType) ||
        (eCSSToken_Ref == tk->mType) ||
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
        return PR_TRUE;
      }
      return PR_FALSE;
    }
  }
  if (((aVariantMask & VARIANT_STRING) != 0) &&
      (eCSSToken_String == tk->mType)) {
    nsAutoString  buffer;
    buffer.Append(tk->mIdent);
    aValue.SetStringValue(buffer, eCSSUnit_String);
    return PR_TRUE;
  }
  if (((aVariantMask & VARIANT_IDENTIFIER) != 0) &&
      (eCSSToken_Ident == tk->mType)) {
    aValue.SetStringValue(tk->mIdent, eCSSUnit_Ident);
    return PR_TRUE;
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
    return ParseAttr(aValue);
  }

  UngetToken();
  return PR_FALSE;
}


PRBool
CSSParserImpl::ParseCounter(nsCSSValue& aValue)
{
  nsCSSUnit unit = (mToken.mIdent.LowerCaseEqualsLiteral("counter") ?
                    eCSSUnit_Counter : eCSSUnit_Counters);

  if (!ExpectSymbol('(', PR_FALSE))
    return PR_FALSE;

  if (!GetNonCloseParenToken(PR_TRUE) ||
      eCSSToken_Ident != mToken.mType) {
    SkipUntil(')');
    return PR_FALSE;
  }

  nsRefPtr<nsCSSValue::Array> val =
    nsCSSValue::Array::Create(unit == eCSSUnit_Counter ? 2 : 3);
  if (!val) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }

  val->Item(0).SetStringValue(mToken.mIdent, eCSSUnit_Ident);

  if (eCSSUnit_Counters == unit) {
    
    if (!ExpectSymbol(',', PR_TRUE) ||
        !(GetNonCloseParenToken(PR_TRUE) &&
          eCSSToken_String == mToken.mType)) {
      SkipUntil(')');
      return PR_FALSE;
    }
    val->Item(1).SetStringValue(mToken.mIdent, eCSSUnit_String);
  }

  
  PRInt32 type = NS_STYLE_LIST_STYLE_DECIMAL;
  if (ExpectSymbol(',', PR_TRUE)) {
    nsCSSKeyword keyword;
    PRBool success = GetNonCloseParenToken(PR_TRUE) &&
                     eCSSToken_Ident == mToken.mType &&
                     (keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent)) !=
                      eCSSKeyword_UNKNOWN;
    if (success) {
      if (keyword == eCSSKeyword_none) {
        type = NS_STYLE_LIST_STYLE_NONE;
      } else {
        success = nsCSSProps::FindKeyword(keyword,
                                          nsCSSProps::kListStyleKTable, type);
      }
    }
    if (!success) {
      SkipUntil(')');
      return PR_FALSE;
    }
  }
  PRInt32 typeItem = eCSSUnit_Counters == unit ? 2 : 1;
  if (type == NS_STYLE_LIST_STYLE_NONE) {
    val->Item(typeItem).SetNoneValue();
  } else {
    val->Item(typeItem).SetIntValue(type, eCSSUnit_Enumerated);
  }

  if (!ExpectSymbol(')', PR_TRUE)) {
    SkipUntil(')');
    return PR_FALSE;
  }

  aValue.SetArrayValue(val, unit);
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseAttr(nsCSSValue& aValue)
{
  if (ExpectSymbol('(', PR_FALSE)) {
    if (GetToken(PR_TRUE)) {
      nsAutoString attr;
      if (eCSSToken_Ident == mToken.mType) {  
        nsAutoString  holdIdent(mToken.mIdent);
        if (ExpectSymbol('|', PR_FALSE)) {  
          PRInt32 nameSpaceID;
          if (!GetNamespaceIdForPrefix(holdIdent, &nameSpaceID)) {
            return PR_FALSE;
          }
          attr.AppendInt(nameSpaceID, 10);
          attr.Append(PRUnichar('|'));
          if (! GetToken(PR_FALSE)) {
            REPORT_UNEXPECTED_EOF(PEAttributeNameEOF);
            return PR_FALSE;
          }
          if (eCSSToken_Ident == mToken.mType) {
            if (mCaseSensitive) {
              attr.Append(mToken.mIdent);
            } else {
              nsAutoString buffer;
              ToLowerCase(mToken.mIdent, buffer);
              attr.Append(buffer);
            }
          }
          else {
            REPORT_UNEXPECTED_TOKEN(PEAttributeNameExpected);
            UngetToken();
            return PR_FALSE;
          }
        }
        else {  
          if (mCaseSensitive) {
            attr = holdIdent;
          }
          else {
            ToLowerCase(holdIdent, attr);
          }
        }
      }
      else if (mToken.IsSymbol('*')) {  
        
        REPORT_UNEXPECTED_TOKEN(PEAttributeNameExpected);
        UngetToken();
        return PR_FALSE;
      }
      else if (mToken.IsSymbol('|')) {  
        if (! GetToken(PR_FALSE)) {
          REPORT_UNEXPECTED_EOF(PEAttributeNameEOF);
          return PR_FALSE;
        }
        if (eCSSToken_Ident == mToken.mType) {
          if (mCaseSensitive) {
            attr.Append(mToken.mIdent);
          } else {
            nsAutoString buffer;
            ToLowerCase(mToken.mIdent, buffer);
            attr.Append(buffer);
          }
        }
        else {
          REPORT_UNEXPECTED_TOKEN(PEAttributeNameExpected);
          UngetToken();
          return PR_FALSE;
        }
      }
      else {
        REPORT_UNEXPECTED_TOKEN(PEAttributeNameOrNamespaceExpected);
        UngetToken();
        return PR_FALSE;
      }
      if (ExpectSymbol(')', PR_TRUE)) {
        aValue.SetStringValue(attr, eCSSUnit_Attr);
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseURL(nsCSSValue& aValue)
{
  if (!mSheetPrincipal) {
    NS_NOTREACHED("Codepaths that expect to parse URLs MUST pass in an "
                  "origin principal");
    return PR_FALSE;
  }

  nsString url;
  if (!GetURLInParens(url))
    return PR_FALSE;

  
  
  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), url, nsnull, mBaseURL);

  nsStringBuffer* buffer = nsCSSValue::BufferFromString(url);
  if (NS_UNLIKELY(!buffer)) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }
  nsCSSValue::URL *urlVal =
    new nsCSSValue::URL(uri, buffer, mSheetURL, mSheetPrincipal);

  buffer->Release();
  if (NS_UNLIKELY(!urlVal)) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }
  aValue.SetURLValue(urlVal);
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseGradientStop(nsCSSValueGradient* aGradient)
{
  if (!GetToken(PR_TRUE))
    return PR_FALSE;

  if (eCSSToken_Function != mToken.mType) {
    UngetToken();
    return PR_FALSE;
  }

  if (mToken.mIdent.LowerCaseEqualsLiteral("from")) {
    
    if (!ExpectSymbol('(', PR_FALSE)) {
      NS_ABORT_IF_FALSE(PR_FALSE, "function token without (");
    }

    nsCSSValue fromFloat(0.0f, eCSSUnit_Percent);
    nsCSSValue fromColor;
    if (!ParseVariant(fromColor, VARIANT_COLOR, nsnull)) {
      SkipUntil(')');
      return PR_FALSE;
    }

    if (!ExpectSymbol(')', PR_TRUE)) {
      SkipUntil(')');
      return PR_FALSE;
    }

    aGradient->mStops.AppendElement(nsCSSValueGradientStop(fromFloat, fromColor));
    return PR_TRUE;
  }

  if (mToken.mIdent.LowerCaseEqualsLiteral("to")) {
    
    if (!ExpectSymbol('(', PR_FALSE)) {
      NS_ABORT_IF_FALSE(PR_FALSE, "function token without (");
    }

    nsCSSValue toFloat(1.0f, eCSSUnit_Percent);
    nsCSSValue toColor;
    if (!ParseVariant(toColor, VARIANT_COLOR, nsnull)) {
      SkipUntil(')');
      return PR_FALSE;
    }

    if (!ExpectSymbol(')', PR_TRUE)) {
      SkipUntil(')');
      return PR_FALSE;
    }

    aGradient->mStops.AppendElement(nsCSSValueGradientStop(toFloat, toColor));
    return PR_TRUE;
  }

  if (mToken.mIdent.LowerCaseEqualsLiteral("color-stop")) {
    
    if (!ExpectSymbol('(', PR_FALSE)) {
      NS_ABORT_IF_FALSE(PR_FALSE, "function token without (");
    }

    nsCSSValue stopFloat;
    if (!ParseVariant(stopFloat, VARIANT_PERCENT | VARIANT_NUMBER, nsnull)) {
      SkipUntil(')');
      return PR_FALSE;
    }

    
    if (stopFloat.GetUnit() == eCSSUnit_Percent) {
      if (stopFloat.GetPercentValue() > 1.0)
        stopFloat.SetPercentValue(1.0);
      else if (stopFloat.GetPercentValue() < 0.0)
        stopFloat.SetPercentValue(0.0);
    } else {
      if (stopFloat.GetFloatValue() > 1.0)
        stopFloat.SetFloatValue(1.0, eCSSUnit_Number);
      else if (stopFloat.GetFloatValue() < 0.0)
        stopFloat.SetFloatValue(0.0, eCSSUnit_Number);
    }

    if (!ExpectSymbol(',', PR_TRUE)) {
      SkipUntil(')');
      return PR_FALSE;
    }

    nsCSSValue stopColor;
    if (!ParseVariant(stopColor, VARIANT_COLOR, nsnull)) {
      SkipUntil(')');
      return PR_FALSE;
    }

    if (!ExpectSymbol(')', PR_TRUE)) {
      SkipUntil(')');
      return PR_FALSE;
    }

    aGradient->mStops.AppendElement(nsCSSValueGradientStop(stopFloat, stopColor));
    return PR_TRUE;
  }

  
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseGradient(nsCSSValue& aValue,
                             PRBool aIsRadial)
{
  if (!ExpectSymbol('(', PR_FALSE)) {
    NS_ABORT_IF_FALSE(PR_FALSE, "function token without (");
  }

  nsCSSValuePair startPos;
  if (!ParseBoxPositionValues(startPos, PR_FALSE))
    return PR_FALSE;

  if (!ExpectSymbol(',', PR_TRUE)) {
    SkipUntil(')');
    return PR_FALSE;
  }

  nsCSSValue startRadius;
  if (aIsRadial) {
    if (!ParseNonNegativeVariant(startRadius, VARIANT_LENGTH, nsnull)) {
      SkipUntil(')');
      return PR_FALSE;
    }

    if (!ExpectSymbol(',', PR_TRUE)) {
      SkipUntil(')');
      return PR_FALSE;
    }
  }

  nsCSSValuePair endPos;
  if (!ParseBoxPositionValues(endPos, PR_FALSE)) {
    SkipUntil(')');
    return PR_FALSE;
  }

  nsCSSValue endRadius;
  if (aIsRadial) {
    if (!ExpectSymbol(',', PR_TRUE)) {
      SkipUntil(')');
      return PR_FALSE;
    }

    if (!ParseNonNegativeVariant(endRadius, VARIANT_LENGTH, nsnull)) {
      SkipUntil(')');
      return PR_FALSE;
    }
  }

  nsRefPtr<nsCSSValueGradient> cssGradient =
    new nsCSSValueGradient(aIsRadial, startPos.mXValue, startPos.mYValue,
                           startRadius, endPos.mXValue, endPos.mYValue,
                           endRadius);

  
  while (ExpectSymbol(',', PR_TRUE)) {
    if (!ParseGradientStop(cssGradient)) {
      SkipUntil(')');
      return PR_FALSE;
    }
  }

  if (!ExpectSymbol(')', PR_TRUE)) {
    SkipUntil(')');
    return PR_FALSE;
  }

  aValue.SetGradientValue(cssGradient);
  return PR_TRUE;
}

PRInt32
CSSParserImpl::ParseChoice(nsCSSValue aValues[],
                           const nsCSSProperty aPropIDs[], PRInt32 aNumIDs)
{
  PRInt32 found = 0;
  nsAutoParseCompoundProperty compound(this);

  PRInt32 loop;
  for (loop = 0; loop < aNumIDs; loop++) {
    
    PRInt32 hadFound = found;
    PRInt32 index;
    for (index = 0; index < aNumIDs; index++) {
      PRInt32 bit = 1 << index;
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
  NS_ASSERTION(0 <= aPropID && aPropID < eCSSProperty_COUNT_no_shorthands,
               "property out of range");
  NS_ASSERTION(nsCSSProps::kTypeTable[aPropID] == eCSSType_Value,
               nsPrintfCString(64, "type error (property=\'%s\')",
                             nsCSSProps::GetStringValue(aPropID).get()).get());
  nsCSSValue& storage =
      *static_cast<nsCSSValue*>(mTempData.PropertyAt(aPropID));
  storage = aValue;
  mTempData.SetPropertyBit(aPropID);
}






PRBool
CSSParserImpl::ParseBoxProperties(nsCSSRect& aResult,
                                  const nsCSSProperty aPropIDs[])
{
  
  PRInt32 count = 0;
  nsCSSRect result;
  NS_FOR_CSS_SIDES (index) {
    if (! ParseSingleValueProperty(result.*(nsCSSRect::sides[index]),
                                   aPropIDs[index])) {
      break;
    }
    count++;
  }
  if ((count == 0) || (PR_FALSE == ExpectEndProperty())) {
    return PR_FALSE;
  }

  if (1 < count) { 
    NS_FOR_CSS_SIDES (index) {
      nsCSSUnit unit = (result.*(nsCSSRect::sides[index])).GetUnit();
      if (eCSSUnit_Inherit == unit || eCSSUnit_Initial == unit) {
        return PR_FALSE;
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
    mTempData.SetPropertyBit(aPropIDs[index]);
  }
  aResult = result;
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseDirectionalBoxProperty(nsCSSProperty aProperty,
                                           PRInt32 aSourceType)
{
  const nsCSSProperty* subprops = nsCSSProps::SubpropertyEntryFor(aProperty);
  NS_ASSERTION(subprops[3] == eCSSProperty_UNKNOWN,
               "not box property with physical vs. logical cascading");
  nsCSSValue value;
  if (!ParseSingleValueProperty(value, subprops[0]) ||
      !ExpectEndProperty())
    return PR_FALSE;

  AppendValue(subprops[0], value);
  nsCSSValue typeVal(aSourceType, eCSSUnit_Enumerated);
  AppendValue(subprops[1], typeVal);
  AppendValue(subprops[2], typeVal);
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseBoxCornerRadius(nsCSSProperty aPropID)
{
  nsCSSValue dimenX, dimenY;
  
  if (! ParseNonNegativeVariant(dimenX, VARIANT_HLP, nsnull))
    return PR_FALSE;
  
  if (dimenX.GetUnit() == eCSSUnit_Inherit ||
      dimenX.GetUnit() == eCSSUnit_Initial ||
      ! ParseNonNegativeVariant(dimenY, VARIANT_LP, nsnull))
    dimenY = dimenX;

  NS_ASSERTION(nsCSSProps::kTypeTable[aPropID] == eCSSType_ValuePair,
               nsPrintfCString(64, "type error (property='%s')",
                               nsCSSProps::GetStringValue(aPropID).get())
               .get());
  nsCSSValuePair& storage =
    *static_cast<nsCSSValuePair*>(mTempData.PropertyAt(aPropID));
  storage.mXValue = dimenX;
  storage.mYValue = dimenY;
  mTempData.SetPropertyBit(aPropID);
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseBoxCornerRadii(nsCSSCornerSizes& aRadii,
                                   const nsCSSProperty aPropIDs[])
{
  
  
  
  nsCSSRect dimenX, dimenY;
  PRInt32 countX = 0, countY = 0;

  NS_FOR_CSS_SIDES (side) {
    if (! ParseNonNegativeVariant(dimenX.*nsCSSRect::sides[side],
                                  side > 0 ? VARIANT_LP : VARIANT_HLP, nsnull))
      break;
    countX++;
  }
  if (countX == 0)
    return PR_FALSE;

  if (ExpectSymbol('/', PR_TRUE)) {
    NS_FOR_CSS_SIDES (side) {
      if (! ParseNonNegativeVariant(dimenY.*nsCSSRect::sides[side],
                                    VARIANT_LP, nsnull))
        break;
      countY++;
    }
    if (countY == 0)
      return PR_FALSE;
  }
  if (!ExpectEndProperty())
    return PR_FALSE;

  
  if (countX > 1 || countY > 0) {
    nsCSSUnit unit = dimenX.mTop.GetUnit();
    if (eCSSUnit_Inherit == unit || eCSSUnit_Initial == unit)
      return PR_FALSE;
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
    nsCSSValuePair& corner =
      aRadii.GetFullCorner(NS_SIDE_TO_FULL_CORNER(side, PR_FALSE));
    corner.mXValue = dimenX.*nsCSSRect::sides[side];
    corner.mYValue = dimenY.*nsCSSRect::sides[side];
    mTempData.SetPropertyBit(aPropIDs[side]);
  }
  return PR_TRUE;
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
  eCSSProperty__moz_border_radius_topLeft,
  eCSSProperty__moz_border_radius_topRight,
  eCSSProperty__moz_border_radius_bottomRight,
  eCSSProperty__moz_border_radius_bottomLeft
};
static const nsCSSProperty kOutlineRadiusIDs[] = {
  eCSSProperty__moz_outline_radius_topLeft,
  eCSSProperty__moz_outline_radius_topRight,
  eCSSProperty__moz_outline_radius_bottomRight,
  eCSSProperty__moz_outline_radius_bottomLeft
};

PRBool
CSSParserImpl::ParseProperty(nsCSSProperty aPropID)
{
  NS_ASSERTION(aPropID < eCSSProperty_COUNT, "index out of range");

  switch (aPropID) {  
  case eCSSProperty_background:
    return ParseBackground();
  case eCSSProperty_background_position:
    return ParseBackgroundPosition();
  case eCSSProperty_background_attachment:
  case eCSSProperty__moz_background_clip:
  case eCSSProperty_background_image:
  case eCSSProperty__moz_background_origin:
  case eCSSProperty_background_repeat:
    return ParseBackgroundList(aPropID);
  case eCSSProperty__moz_background_size:
    return ParseBackgroundSize();
  case eCSSProperty_border:
    return ParseBorderSide(kBorderTopIDs, PR_TRUE);
  case eCSSProperty_border_color:
    return ParseBorderColor();
  case eCSSProperty_border_spacing:
    return ParseBorderSpacing();
  case eCSSProperty_border_style:
    return ParseBorderStyle();
  case eCSSProperty_border_bottom:
    return ParseBorderSide(kBorderBottomIDs, PR_FALSE);
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
    return ParseBorderSide(kBorderTopIDs, PR_FALSE);
  case eCSSProperty_border_bottom_colors:
    return ParseBorderColors(&mTempData.mMargin.mBorderColors.mBottom,
                             aPropID);
  case eCSSProperty_border_left_colors:
    return ParseBorderColors(&mTempData.mMargin.mBorderColors.mLeft,
                             aPropID);
  case eCSSProperty_border_right_colors:
    return ParseBorderColors(&mTempData.mMargin.mBorderColors.mRight,
                             aPropID);
  case eCSSProperty_border_top_colors:
    return ParseBorderColors(&mTempData.mMargin.mBorderColors.mTop,
                             aPropID);
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
  case eCSSProperty__moz_border_radius:
    return ParseBoxCornerRadii(mTempData.mMargin.mBorderRadius,
                               kBorderRadiusIDs);
  case eCSSProperty__moz_outline_radius:
    return ParseBoxCornerRadii(mTempData.mMargin.mOutlineRadius,
                               kOutlineRadiusIDs);

  case eCSSProperty__moz_border_radius_topLeft:
  case eCSSProperty__moz_border_radius_topRight:
  case eCSSProperty__moz_border_radius_bottomRight:
  case eCSSProperty__moz_border_radius_bottomLeft:
  case eCSSProperty__moz_outline_radius_topLeft:
  case eCSSProperty__moz_outline_radius_topRight:
  case eCSSProperty__moz_outline_radius_bottomRight:
  case eCSSProperty__moz_outline_radius_bottomLeft:
    return ParseBoxCornerRadius(aPropID);

  case eCSSProperty_box_shadow:
    return ParseBoxShadow();
  case eCSSProperty_clip:
    return ParseRect(mTempData.mDisplay.mClip, eCSSProperty_clip);
  case eCSSProperty__moz_column_rule:
    return ParseBorderSide(kColumnRuleIDs, PR_FALSE);
  case eCSSProperty_content:
    return ParseContent();
  case eCSSProperty_counter_increment:
    return ParseCounterData(&mTempData.mContent.mCounterIncrement,
                            aPropID);
  case eCSSProperty_counter_reset:
    return ParseCounterData(&mTempData.mContent.mCounterReset,
                            aPropID);
  case eCSSProperty_cue:
    return ParseCue();
  case eCSSProperty_cursor:
    return ParseCursor();
  case eCSSProperty_font:
    return ParseFont();
  case eCSSProperty_image_region:
    return ParseRect(mTempData.mList.mImageRegion,
                     eCSSProperty_image_region);
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
  case eCSSProperty_pause:
    return ParsePause();
  case eCSSProperty_quotes:
    return ParseQuotes();
  case eCSSProperty_size:
    return ParseSize();
  case eCSSProperty_text_shadow:
    return ParseTextShadow();
  case eCSSProperty__moz_transform:
    return ParseMozTransform();
  case eCSSProperty__moz_transform_origin:
    return ParseMozTransformOrigin();

#ifdef MOZ_SVG
  case eCSSProperty_fill:
    return ParsePaint(&mTempData.mSVG.mFill, eCSSProperty_fill);
  case eCSSProperty_stroke:
    return ParsePaint(&mTempData.mSVG.mStroke, eCSSProperty_stroke);
  case eCSSProperty_stroke_dasharray:
    return ParseDasharray();
  case eCSSProperty_marker:
    return ParseMarker();
#endif

  
  case eCSSProperty__x_system_font:
  case eCSSProperty_margin_end_value:
  case eCSSProperty_margin_left_value:
  case eCSSProperty_margin_right_value:
  case eCSSProperty_margin_start_value:
  case eCSSProperty_margin_left_ltr_source:
  case eCSSProperty_margin_left_rtl_source:
  case eCSSProperty_margin_right_ltr_source:
  case eCSSProperty_margin_right_rtl_source:
  case eCSSProperty_padding_end_value:
  case eCSSProperty_padding_left_value:
  case eCSSProperty_padding_right_value:
  case eCSSProperty_padding_start_value:
  case eCSSProperty_padding_left_ltr_source:
  case eCSSProperty_padding_left_rtl_source:
  case eCSSProperty_padding_right_ltr_source:
  case eCSSProperty_padding_right_rtl_source:
  case eCSSProperty_border_end_color_value:
  case eCSSProperty_border_left_color_value:
  case eCSSProperty_border_right_color_value:
  case eCSSProperty_border_start_color_value:
  case eCSSProperty_border_left_color_ltr_source:
  case eCSSProperty_border_left_color_rtl_source:
  case eCSSProperty_border_right_color_ltr_source:
  case eCSSProperty_border_right_color_rtl_source:
  case eCSSProperty_border_end_style_value:
  case eCSSProperty_border_left_style_value:
  case eCSSProperty_border_right_style_value:
  case eCSSProperty_border_start_style_value:
  case eCSSProperty_border_left_style_ltr_source:
  case eCSSProperty_border_left_style_rtl_source:
  case eCSSProperty_border_right_style_ltr_source:
  case eCSSProperty_border_right_style_rtl_source:
  case eCSSProperty_border_end_width_value:
  case eCSSProperty_border_left_width_value:
  case eCSSProperty_border_right_width_value:
  case eCSSProperty_border_start_width_value:
  case eCSSProperty_border_left_width_ltr_source:
  case eCSSProperty_border_left_width_rtl_source:
  case eCSSProperty_border_right_width_ltr_source:
  case eCSSProperty_border_right_width_rtl_source:
    
    REPORT_UNEXPECTED(PEInaccessibleProperty2);
    return PR_FALSE;
  default:  
    {
      nsCSSValue value;
      if (ParseSingleValueProperty(value, aPropID)) {
        if (ExpectEndProperty()) {
          AppendValue(aPropID, value);
          return PR_TRUE;
        }
        
      }
      
    }
  }
  return PR_FALSE;
}


#define BG_CENTER  NS_STYLE_BG_POSITION_CENTER
#define BG_TOP     NS_STYLE_BG_POSITION_TOP
#define BG_BOTTOM  NS_STYLE_BG_POSITION_BOTTOM
#define BG_LEFT    NS_STYLE_BG_POSITION_LEFT
#define BG_RIGHT   NS_STYLE_BG_POSITION_RIGHT
#define BG_CTB    (BG_CENTER | BG_TOP | BG_BOTTOM)
#define BG_CLR    (BG_CENTER | BG_LEFT | BG_RIGHT)

PRBool
CSSParserImpl::ParseSingleValueProperty(nsCSSValue& aValue,
                                        nsCSSProperty aPropID)
{
  switch (aPropID) {
  case eCSSProperty_UNKNOWN:
  case eCSSProperty_background:
  case eCSSProperty_background_position:
  case eCSSProperty_border:
  case eCSSProperty_border_color:
  case eCSSProperty_border_bottom_colors:
  case eCSSProperty_border_image:
  case eCSSProperty_border_left_colors:
  case eCSSProperty_border_right_colors:
  case eCSSProperty_border_end_color:
  case eCSSProperty_border_left_color:
  case eCSSProperty_border_right_color:
  case eCSSProperty_border_start_color:
  case eCSSProperty_border_end_style:
  case eCSSProperty_border_left_style:
  case eCSSProperty_border_right_style:
  case eCSSProperty_border_start_style:
  case eCSSProperty_border_end_width:
  case eCSSProperty_border_left_width:
  case eCSSProperty_border_right_width:
  case eCSSProperty_border_start_width:
  case eCSSProperty_border_top_colors:
  case eCSSProperty_border_spacing:
  case eCSSProperty_border_style:
  case eCSSProperty_border_bottom:
  case eCSSProperty_border_end:
  case eCSSProperty_border_left:
  case eCSSProperty_border_right:
  case eCSSProperty_border_start:
  case eCSSProperty_border_top:
  case eCSSProperty_border_width:
  case eCSSProperty__moz_background_size:
  case eCSSProperty__moz_border_radius:
  case eCSSProperty__moz_border_radius_topLeft:
  case eCSSProperty__moz_border_radius_topRight:
  case eCSSProperty__moz_border_radius_bottomRight:
  case eCSSProperty__moz_border_radius_bottomLeft:
  case eCSSProperty_box_shadow:
  case eCSSProperty_clip:
  case eCSSProperty__moz_column_rule:
  case eCSSProperty_content:
  case eCSSProperty_counter_increment:
  case eCSSProperty_counter_reset:
  case eCSSProperty_cue:
  case eCSSProperty_cursor:
  case eCSSProperty_font:
  case eCSSProperty_image_region:
  case eCSSProperty_list_style:
  case eCSSProperty_margin:
  case eCSSProperty_margin_end:
  case eCSSProperty_margin_left:
  case eCSSProperty_margin_right:
  case eCSSProperty_margin_start:
  case eCSSProperty_outline:
  case eCSSProperty__moz_outline_radius:
  case eCSSProperty__moz_outline_radius_topLeft:
  case eCSSProperty__moz_outline_radius_topRight:
  case eCSSProperty__moz_outline_radius_bottomRight:
  case eCSSProperty__moz_outline_radius_bottomLeft:
  case eCSSProperty_overflow:
  case eCSSProperty_padding:
  case eCSSProperty_padding_end:
  case eCSSProperty_padding_left:
  case eCSSProperty_padding_right:
  case eCSSProperty_padding_start:
  case eCSSProperty_pause:
  case eCSSProperty_quotes:
  case eCSSProperty_size:
  case eCSSProperty_text_shadow:
  case eCSSProperty__moz_transform:
  case eCSSProperty__moz_transform_origin:
  case eCSSProperty_COUNT:
#ifdef MOZ_SVG
  case eCSSProperty_fill:
  case eCSSProperty_stroke:
  case eCSSProperty_stroke_dasharray:
  case eCSSProperty_marker:
#endif
    NS_ERROR("not a single value property");
    return PR_FALSE;

  case eCSSProperty__x_system_font:
  case eCSSProperty_margin_left_ltr_source:
  case eCSSProperty_margin_left_rtl_source:
  case eCSSProperty_margin_right_ltr_source:
  case eCSSProperty_margin_right_rtl_source:
  case eCSSProperty_padding_left_ltr_source:
  case eCSSProperty_padding_left_rtl_source:
  case eCSSProperty_padding_right_ltr_source:
  case eCSSProperty_padding_right_rtl_source:
  case eCSSProperty_border_left_color_ltr_source:
  case eCSSProperty_border_left_color_rtl_source:
  case eCSSProperty_border_right_color_ltr_source:
  case eCSSProperty_border_right_color_rtl_source:
  case eCSSProperty_border_left_style_ltr_source:
  case eCSSProperty_border_left_style_rtl_source:
  case eCSSProperty_border_right_style_ltr_source:
  case eCSSProperty_border_right_style_rtl_source:
  case eCSSProperty_border_left_width_ltr_source:
  case eCSSProperty_border_left_width_rtl_source:
  case eCSSProperty_border_right_width_ltr_source:
  case eCSSProperty_border_right_width_rtl_source:
#ifdef MOZ_MATHML
  case eCSSProperty_script_size_multiplier:
  case eCSSProperty_script_min_size:
#endif
    NS_ERROR("not currently parsed here");
    return PR_FALSE;

  case eCSSProperty_appearance:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kAppearanceKTable);
  case eCSSProperty_azimuth:
    return ParseAzimuth(aValue);
  case eCSSProperty_background_attachment:
    
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBackgroundAttachmentKTable);
  case eCSSProperty__moz_background_clip:
    
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBackgroundClipKTable);
  case eCSSProperty_background_color:
    return ParseVariant(aValue, VARIANT_HC, nsnull);
  case eCSSProperty_background_image:
    
    return ParseVariant(aValue, VARIANT_HUO | VARIANT_GRADIENT, nsnull);
  case eCSSProperty__moz_background_inline_policy:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBackgroundInlinePolicyKTable);
  case eCSSProperty__moz_background_origin:
    
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBackgroundOriginKTable);
  case eCSSProperty_background_repeat:
    
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBackgroundRepeatKTable);
  case eCSSProperty_binding:
    return ParseVariant(aValue, VARIANT_HUO, nsnull);
  case eCSSProperty_border_collapse:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBorderCollapseKTable);
  case eCSSProperty_border_bottom_color:
  case eCSSProperty_border_end_color_value: 
  case eCSSProperty_border_left_color_value: 
  case eCSSProperty_border_right_color_value: 
  case eCSSProperty_border_start_color_value: 
  case eCSSProperty_border_top_color:
  case eCSSProperty__moz_column_rule_color:
    return ParseVariant(aValue, VARIANT_HCK,
                        nsCSSProps::kBorderColorKTable);
  case eCSSProperty_border_bottom_style:
  case eCSSProperty_border_end_style_value: 
  case eCSSProperty_border_left_style_value: 
  case eCSSProperty_border_right_style_value: 
  case eCSSProperty_border_start_style_value: 
  case eCSSProperty_border_top_style:
  case eCSSProperty__moz_column_rule_style:
    return ParseVariant(aValue, VARIANT_HOK,
                        nsCSSProps::kBorderStyleKTable);
  case eCSSProperty_border_bottom_width:
  case eCSSProperty_border_end_width_value: 
  case eCSSProperty_border_left_width_value: 
  case eCSSProperty_border_right_width_value: 
  case eCSSProperty_border_start_width_value: 
  case eCSSProperty_border_top_width:
  case eCSSProperty__moz_column_rule_width:
    return ParseNonNegativeVariant(aValue, VARIANT_HKL,
                                   nsCSSProps::kBorderWidthKTable);
  case eCSSProperty__moz_column_count:
    
    
    return ParsePositiveNonZeroVariant(aValue, VARIANT_AHI, nsnull);
  case eCSSProperty__moz_column_width:
    return ParseNonNegativeVariant(aValue, VARIANT_AHL, nsnull);
  case eCSSProperty__moz_column_gap:
    return ParseNonNegativeVariant(aValue, VARIANT_HL | VARIANT_NORMAL, nsnull);
  case eCSSProperty_bottom:
  case eCSSProperty_top:
  case eCSSProperty_left:
  case eCSSProperty_right:
    return ParseVariant(aValue, VARIANT_AHLP, nsnull);
  case eCSSProperty_box_align:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBoxAlignKTable);
  case eCSSProperty_box_direction:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBoxDirectionKTable);
  case eCSSProperty_box_flex:
    return ParseNonNegativeVariant(aValue, VARIANT_HN, nsnull);
  case eCSSProperty_box_orient:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBoxOrientKTable);
  case eCSSProperty_box_pack:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBoxPackKTable);
  case eCSSProperty_box_ordinal_group:
    return ParseNonNegativeVariant(aValue, VARIANT_HI, nsnull);
#ifdef MOZ_SVG
  case eCSSProperty_clip_path:
    return ParseVariant(aValue, VARIANT_HUO, nsnull);
  case eCSSProperty_clip_rule:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kFillRuleKTable);
  case eCSSProperty_color_interpolation:
  case eCSSProperty_color_interpolation_filters:
    return ParseVariant(aValue, VARIANT_AHK,
                        nsCSSProps::kColorInterpolationKTable);
  case eCSSProperty_dominant_baseline:
    return ParseVariant(aValue, VARIANT_AHK,
                        nsCSSProps::kDominantBaselineKTable);
  case eCSSProperty_fill_opacity:
    return ParseVariant(aValue, VARIANT_HN,
                        nsnull);
  case eCSSProperty_fill_rule:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kFillRuleKTable);
  case eCSSProperty_filter:
    return ParseVariant(aValue, VARIANT_HUO, nsnull);
  case eCSSProperty_flood_color:
    return ParseVariant(aValue, VARIANT_HC, nsnull);
  case eCSSProperty_flood_opacity:
    return ParseVariant(aValue, VARIANT_HN, nsnull);
  case eCSSProperty_image_rendering:
    return ParseVariant(aValue, VARIANT_AHK,
                        nsCSSProps::kImageRenderingKTable);
  case eCSSProperty_lighting_color:
    return ParseVariant(aValue, VARIANT_HC, nsnull);
  case eCSSProperty_marker_end:
  case eCSSProperty_marker_mid:
  case eCSSProperty_marker_start:
    return ParseVariant(aValue, VARIANT_HUO, nsnull);
  case eCSSProperty_mask:
    return ParseVariant(aValue, VARIANT_HUO, nsnull);
  case eCSSProperty_pointer_events:
    return ParseVariant(aValue, VARIANT_HOK,
                        nsCSSProps::kPointerEventsKTable);
  case eCSSProperty_shape_rendering:
    return ParseVariant(aValue, VARIANT_AHK,
                        nsCSSProps::kShapeRenderingKTable);
  case eCSSProperty_stop_color:
    return ParseVariant(aValue, VARIANT_HC,
                        nsnull);
  case eCSSProperty_stop_opacity:
    return ParseVariant(aValue, VARIANT_HN,
                        nsnull);
  case eCSSProperty_stroke_dashoffset:
    return ParseVariant(aValue, VARIANT_HLPN,
                        nsnull);
  case eCSSProperty_stroke_linecap:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kStrokeLinecapKTable);
  case eCSSProperty_stroke_linejoin:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kStrokeLinejoinKTable);
  case eCSSProperty_stroke_miterlimit:
    return ParseVariant(aValue, VARIANT_HN, nsnull) &&
           
           (aValue.GetUnit() != eCSSUnit_Number || 
            aValue.GetFloatValue() >= 1.0f);
  case eCSSProperty_stroke_opacity:
    return ParseVariant(aValue, VARIANT_HN,
                        nsnull);
  case eCSSProperty_stroke_width:
    return ParseNonNegativeVariant(aValue, VARIANT_HLPN, nsnull);
  case eCSSProperty_text_anchor:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kTextAnchorKTable);
  case eCSSProperty_text_rendering:
    return ParseVariant(aValue, VARIANT_AHK,
                        nsCSSProps::kTextRenderingKTable);
#endif
  case eCSSProperty_box_sizing:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kBoxSizingKTable);
  case eCSSProperty_height:
    return ParseNonNegativeVariant(aValue, VARIANT_AHLP, nsnull);
  case eCSSProperty_width:
    return ParseNonNegativeVariant(aValue, VARIANT_AHKLP,
                                   nsCSSProps::kWidthKTable);
  case eCSSProperty_force_broken_image_icon:
    return ParseNonNegativeVariant(aValue, VARIANT_HI, nsnull);
  case eCSSProperty_caption_side:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kCaptionSideKTable);
  case eCSSProperty_clear:
    return ParseVariant(aValue, VARIANT_HOK,
                        nsCSSProps::kClearKTable);
  case eCSSProperty_color:
    return ParseVariant(aValue, VARIANT_HC, nsnull);
  case eCSSProperty_cue_after:
  case eCSSProperty_cue_before:
    return ParseVariant(aValue, VARIANT_HUO, nsnull);
  case eCSSProperty_direction:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kDirectionKTable);
  case eCSSProperty_display:
    if (ParseVariant(aValue, VARIANT_HOK, nsCSSProps::kDisplayKTable)) {
      if (aValue.GetUnit() == eCSSUnit_Enumerated) {
        switch (aValue.GetIntValue()) {
          case NS_STYLE_DISPLAY_MARKER:        
          case NS_STYLE_DISPLAY_RUN_IN:        
          case NS_STYLE_DISPLAY_COMPACT:       
            return PR_FALSE;
        }
      }
      return PR_TRUE;
    }
    return PR_FALSE;
  case eCSSProperty_elevation:
    return ParseVariant(aValue, VARIANT_HK | VARIANT_ANGLE,
                        nsCSSProps::kElevationKTable);
  case eCSSProperty_empty_cells:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kEmptyCellsKTable);
  case eCSSProperty_float:
    return ParseVariant(aValue, VARIANT_HOK,
                        nsCSSProps::kFloatKTable);
  case eCSSProperty_float_edge:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kFloatEdgeKTable);
  case eCSSProperty_font_family:
    return ParseFamily(aValue);
  case eCSSProperty_font_size:
    return ParseNonNegativeVariant(aValue,
                                   VARIANT_HKLP | VARIANT_SYSFONT,
                                   nsCSSProps::kFontSizeKTable);
  case eCSSProperty_font_size_adjust:
    return ParseVariant(aValue, VARIANT_HON | VARIANT_SYSFONT,
                        nsnull);
  case eCSSProperty_font_stretch:
    return ParseVariant(aValue, VARIANT_HMK | VARIANT_SYSFONT,
                        nsCSSProps::kFontStretchKTable);
  case eCSSProperty_font_style:
    return ParseVariant(aValue, VARIANT_HMK | VARIANT_SYSFONT,
                        nsCSSProps::kFontStyleKTable);
  case eCSSProperty_font_variant:
    return ParseVariant(aValue, VARIANT_HMK | VARIANT_SYSFONT,
                        nsCSSProps::kFontVariantKTable);
  case eCSSProperty_font_weight:
    return ParseFontWeight(aValue);
  case eCSSProperty_ime_mode:
    return ParseVariant(aValue, VARIANT_AHK | VARIANT_NORMAL,
                        nsCSSProps::kIMEModeKTable);
  case eCSSProperty_letter_spacing:
  case eCSSProperty_word_spacing:
    return ParseVariant(aValue, VARIANT_HL | VARIANT_NORMAL, nsnull);
  case eCSSProperty_line_height:
    return ParseNonNegativeVariant(aValue, VARIANT_HLPN | VARIANT_KEYWORD | VARIANT_NORMAL | VARIANT_SYSFONT, nsCSSProps::kLineHeightKTable);
  case eCSSProperty_list_style_image:
    return ParseVariant(aValue, VARIANT_HUO, nsnull);
  case eCSSProperty_list_style_position:
    return ParseVariant(aValue, VARIANT_HK, nsCSSProps::kListStylePositionKTable);
  case eCSSProperty_list_style_type:
    return ParseVariant(aValue, VARIANT_HOK, nsCSSProps::kListStyleKTable);
  case eCSSProperty_margin_bottom:
  case eCSSProperty_margin_end_value: 
  case eCSSProperty_margin_left_value: 
  case eCSSProperty_margin_right_value: 
  case eCSSProperty_margin_start_value: 
  case eCSSProperty_margin_top:
    return ParseVariant(aValue, VARIANT_AHLP, nsnull);
  case eCSSProperty_marker_offset:
    return ParseVariant(aValue, VARIANT_AHL, nsnull);
  case eCSSProperty_marks:
    return ParseMarks(aValue);
  case eCSSProperty_max_height:
    return ParseNonNegativeVariant(aValue, VARIANT_HLPO, nsnull);
  case eCSSProperty_max_width:
    return ParseNonNegativeVariant(aValue, VARIANT_HKLPO,
                                   nsCSSProps::kWidthKTable);
  case eCSSProperty_min_height:
    return ParseNonNegativeVariant(aValue, VARIANT_HLP, nsnull);
  case eCSSProperty_min_width:
    return ParseNonNegativeVariant(aValue, VARIANT_HKLP,
                                   nsCSSProps::kWidthKTable);
  case eCSSProperty_opacity:
    return ParseVariant(aValue, VARIANT_HN, nsnull);
  case eCSSProperty_orphans:
  case eCSSProperty_widows:
    return ParsePositiveNonZeroVariant(aValue, VARIANT_HI, nsnull);
  case eCSSProperty_outline_color:
    return ParseVariant(aValue, VARIANT_HCK,
                        nsCSSProps::kOutlineColorKTable);
  case eCSSProperty_outline_style:
    return ParseVariant(aValue, VARIANT_HOK | VARIANT_AUTO,
                        nsCSSProps::kOutlineStyleKTable);
  case eCSSProperty_outline_width:
    return ParseNonNegativeVariant(aValue, VARIANT_HKL,
                                   nsCSSProps::kBorderWidthKTable);
  case eCSSProperty_outline_offset:
    return ParseVariant(aValue, VARIANT_HL, nsnull);
  case eCSSProperty_overflow_x:
  case eCSSProperty_overflow_y:
    return ParseVariant(aValue, VARIANT_AHK,
                        nsCSSProps::kOverflowSubKTable);
  case eCSSProperty_padding_bottom:
  case eCSSProperty_padding_end_value: 
  case eCSSProperty_padding_left_value: 
  case eCSSProperty_padding_right_value: 
  case eCSSProperty_padding_start_value: 
  case eCSSProperty_padding_top:
    return ParseNonNegativeVariant(aValue, VARIANT_HLP, nsnull);
  case eCSSProperty_page:
    return ParseVariant(aValue, VARIANT_AUTO | VARIANT_IDENTIFIER, nsnull);
  case eCSSProperty_page_break_after:
  case eCSSProperty_page_break_before:
    return ParseVariant(aValue, VARIANT_AHK,
                        nsCSSProps::kPageBreakKTable);
  case eCSSProperty_page_break_inside:
    return ParseVariant(aValue, VARIANT_AHK,
                        nsCSSProps::kPageBreakInsideKTable);
  case eCSSProperty_pause_after:
  case eCSSProperty_pause_before:
    return ParseVariant(aValue, VARIANT_HTP, nsnull);
  case eCSSProperty_pitch:
    return ParseVariant(aValue, VARIANT_HKF, nsCSSProps::kPitchKTable);
  case eCSSProperty_pitch_range:
    return ParseVariant(aValue, VARIANT_HN, nsnull);
  case eCSSProperty_position:
    return ParseVariant(aValue, VARIANT_HK, nsCSSProps::kPositionKTable);
  case eCSSProperty_richness:
    return ParseVariant(aValue, VARIANT_HN, nsnull);
#ifdef MOZ_MATHML
  
  
  
  
  case eCSSProperty_script_level:
    if (!mUnsafeRulesEnabled)
      return PR_FALSE;
    return ParseVariant(aValue, VARIANT_HI, nsnull);
#endif
  case eCSSProperty_speak:
    return ParseVariant(aValue, VARIANT_HMK | VARIANT_NONE,
                        nsCSSProps::kSpeakKTable);
  case eCSSProperty_speak_header:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kSpeakHeaderKTable);
  case eCSSProperty_speak_numeral:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kSpeakNumeralKTable);
  case eCSSProperty_speak_punctuation:
    return ParseVariant(aValue, VARIANT_HOK,
                        nsCSSProps::kSpeakPunctuationKTable);
  case eCSSProperty_speech_rate:
    return ParseVariant(aValue, VARIANT_HN | VARIANT_KEYWORD,
                        nsCSSProps::kSpeechRateKTable);
  case eCSSProperty_stack_sizing:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kStackSizingKTable);
  case eCSSProperty_stress:
    return ParseVariant(aValue, VARIANT_HN, nsnull);
  case eCSSProperty_table_layout:
    return ParseVariant(aValue, VARIANT_AHK,
                        nsCSSProps::kTableLayoutKTable);
  case eCSSProperty_text_align:
    
    
    return ParseVariant(aValue, VARIANT_HK ,
                        nsCSSProps::kTextAlignKTable);
  case eCSSProperty_text_decoration:
    return ParseTextDecoration(aValue);
  case eCSSProperty_text_indent:
    return ParseVariant(aValue, VARIANT_HLP, nsnull);
  case eCSSProperty_text_transform:
    return ParseVariant(aValue, VARIANT_HOK,
                        nsCSSProps::kTextTransformKTable);
  case eCSSProperty_unicode_bidi:
    return ParseVariant(aValue, VARIANT_HMK,
                        nsCSSProps::kUnicodeBidiKTable);
  case eCSSProperty_user_focus:
    return ParseVariant(aValue, VARIANT_HMK | VARIANT_NONE,
                        nsCSSProps::kUserFocusKTable);
  case eCSSProperty_user_input:
    return ParseVariant(aValue, VARIANT_AHK | VARIANT_NONE,
                        nsCSSProps::kUserInputKTable);
  case eCSSProperty_user_modify:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kUserModifyKTable);
  case eCSSProperty_user_select:
    return ParseVariant(aValue, VARIANT_AHK | VARIANT_NONE,
                        nsCSSProps::kUserSelectKTable);
  case eCSSProperty_vertical_align:
    return ParseVariant(aValue, VARIANT_HKLP,
                        nsCSSProps::kVerticalAlignKTable);
  case eCSSProperty_visibility:
    return ParseVariant(aValue, VARIANT_HK,
                        nsCSSProps::kVisibilityKTable);
  case eCSSProperty_voice_family:
    return ParseFamily(aValue);
  case eCSSProperty_volume:
    return ParseVariant(aValue, VARIANT_HPN | VARIANT_KEYWORD,
                        nsCSSProps::kVolumeKTable);
  case eCSSProperty_white_space:
    return ParseVariant(aValue, VARIANT_HMK,
                        nsCSSProps::kWhitespaceKTable);
  case eCSSProperty__moz_window_shadow:
    return ParseVariant(aValue, VARIANT_HOK,
                        nsCSSProps::kWindowShadowKTable);
  case eCSSProperty_word_wrap:
    return ParseVariant(aValue, VARIANT_HMK,
                        nsCSSProps::kWordwrapKTable);
  case eCSSProperty_z_index:
    return ParseVariant(aValue, VARIANT_AHI, nsnull);
  case eCSSPropertyExtra_x_none_value:
    return ParseVariant(aValue, VARIANT_NONE | VARIANT_INHERIT, nsnull);
  }
  
  
  return PR_FALSE;
}


struct NS_STACK_CLASS ExtractFirstFamilyData {
  nsAutoString mFamilyName;
  PRBool mGood;
  ExtractFirstFamilyData() : mFamilyName(), mGood(PR_FALSE) {}
};

static PRBool
ExtractFirstFamily(const nsString& aFamily,
                   PRBool aGeneric,
                   void* aData)
{
  ExtractFirstFamilyData* realData = (ExtractFirstFamilyData*) aData;
  if (aGeneric || realData->mFamilyName.Length() > 0) {
    realData->mGood = PR_FALSE;
    return PR_FALSE;
  }
  realData->mFamilyName.Assign(aFamily);
  realData->mGood = PR_TRUE;
  return PR_TRUE;
}



PRBool
CSSParserImpl::ParseFontDescriptorValue(nsCSSFontDesc aDescID,
                                        nsCSSValue& aValue)
{
  switch (aDescID) {
    
    
  case eCSSFontDesc_Family: {
    if (!ParseFamily(aValue) ||
        aValue.GetUnit() != eCSSUnit_Families)
      return PR_FALSE;

    
    
    nsAutoString valueStr;
    aValue.GetStringValue(valueStr);
    nsFont font(valueStr, 0, 0, 0, 0, 0, 0);
    ExtractFirstFamilyData dat;

    font.EnumerateFamilies(ExtractFirstFamily, (void*) &dat);
    if (!dat.mGood)
      return PR_FALSE;

    aValue.SetStringValue(dat.mFamilyName, eCSSUnit_String);
    return PR_TRUE;
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
    
    return (ParseVariant(aValue, VARIANT_KEYWORD | VARIANT_NORMAL,
                         nsCSSProps::kFontStretchKTable) &&
            (aValue.GetUnit() != eCSSUnit_Enumerated ||
             (aValue.GetIntValue() != NS_STYLE_FONT_STRETCH_WIDER &&
              aValue.GetIntValue() != NS_STYLE_FONT_STRETCH_NARROWER)));

    
  case eCSSFontDesc_Src:
    return ParseFontSrc(aValue);

  case eCSSFontDesc_UnicodeRange:
    return ParseFontRanges(aValue);

  case eCSSFontDesc_UNKNOWN:
  case eCSSFontDesc_COUNT:
    NS_NOTREACHED("bad nsCSSFontDesc code");
  }
  
  
  return PR_FALSE;
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

PRBool
CSSParserImpl::ParseAzimuth(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_HK | VARIANT_ANGLE,
                   nsCSSProps::kAzimuthKTable)) {
    if (eCSSUnit_Enumerated == aValue.GetUnit()) {
      PRInt32 intValue = aValue.GetIntValue();
      if ((NS_STYLE_AZIMUTH_LEFT_SIDE <= intValue) &&
          (intValue <= NS_STYLE_AZIMUTH_BEHIND)) {  
        nsCSSValue  modifier;
        if (ParseEnum(modifier, nsCSSProps::kAzimuthKTable)) {
          PRInt32 enumValue = modifier.GetIntValue();
          if (((intValue == NS_STYLE_AZIMUTH_BEHIND) &&
               (NS_STYLE_AZIMUTH_LEFT_SIDE <= enumValue) && (enumValue <= NS_STYLE_AZIMUTH_RIGHT_SIDE)) ||
              ((enumValue == NS_STYLE_AZIMUTH_BEHIND) &&
               (NS_STYLE_AZIMUTH_LEFT_SIDE <= intValue) && (intValue <= NS_STYLE_AZIMUTH_RIGHT_SIDE))) {
            aValue.SetIntValue(intValue | enumValue, eCSSUnit_Enumerated);
            return PR_TRUE;
          }
          
          UngetToken();
          return PR_FALSE;
        }
      }
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

static nsCSSValue
BoxPositionMaskToCSSValue(PRInt32 aMask, PRBool isX)
{
  PRInt32 val = NS_STYLE_BG_POSITION_CENTER;
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

PRBool
CSSParserImpl::ParseBackground()
{
  nsAutoParseCompoundProperty compound(this);

  
  mTempData.SetPropertyBit(eCSSProperty_background_color);
  mTempData.mColor.mBackColor.SetColorValue(NS_RGBA(0, 0, 0, 0));

  BackgroundItem bgitem;
  nsCSSValuePairList *positionHead = nsnull, **positionTail = &positionHead;
  nsCSSValuePairList *sizeHead = nsnull, **sizeTail = &sizeHead;
  static const BackgroundItemSimpleValueInfo simpleValues[] = {
    { &BackgroundItem::mImage,      eCSSProperty_background_image },
    { &BackgroundItem::mRepeat,     eCSSProperty_background_repeat },
    { &BackgroundItem::mAttachment, eCSSProperty_background_attachment },
    { &BackgroundItem::mClip,       eCSSProperty__moz_background_clip },
    { &BackgroundItem::mOrigin,     eCSSProperty__moz_background_origin }
  };
  nsCSSValueList *simpleHeads[NS_ARRAY_LENGTH(simpleValues)];
  nsCSSValueList **simpleTails[NS_ARRAY_LENGTH(simpleValues)];
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(simpleValues); ++i) {
    simpleHeads[i] = nsnull;
    simpleTails[i] = &simpleHeads[i];
  }
  for (;;) {
    if (!ParseBackgroundItem(bgitem, !positionHead)) {
      break;
    }

    nsCSSValuePairList *positionItem = new nsCSSValuePairList;
    if (!positionItem) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      break;
    }
    positionItem->mXValue = bgitem.mPosition.mXValue;
    positionItem->mYValue = bgitem.mPosition.mYValue;
    *positionTail = positionItem;
    positionTail = &positionItem->mNext;

    nsCSSValuePairList *sizeItem = new nsCSSValuePairList;
    if (!sizeItem) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      break;
    }
    sizeItem->mXValue = bgitem.mSize.mXValue;
    sizeItem->mYValue = bgitem.mSize.mYValue;
    *sizeTail = sizeItem;
    sizeTail = &sizeItem->mNext;

    PRBool fail = PR_FALSE;
    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(simpleValues); ++i) {
      nsCSSValueList *item = new nsCSSValueList;
      if (!item) {
        mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
        fail = PR_TRUE;
        break;
      }
      item->mValue = bgitem.*(simpleValues[i].member);
      *simpleTails[i] = item;
      simpleTails[i] = &item->mNext;
    }
    if (fail) {
      break;
    }

    if (!bgitem.mLastItem && ExpectSymbol(',', PR_TRUE)) {
      continue;
    }
    if (!ExpectEndProperty()) {
      break;
    }

    mTempData.mColor.mBackPosition = positionHead;
    mTempData.mColor.mBackSize = sizeHead;
    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(simpleValues); ++i) {
      nsCSSValueList **source = static_cast<nsCSSValueList**>(
        mTempData.PropertyAt(simpleValues[i].propID));
      *source = simpleHeads[i];
    }

    mTempData.SetPropertyBit(eCSSProperty_background_image);
    mTempData.SetPropertyBit(eCSSProperty_background_repeat);
    mTempData.SetPropertyBit(eCSSProperty_background_attachment);
    mTempData.SetPropertyBit(eCSSProperty_background_position);
    mTempData.SetPropertyBit(eCSSProperty__moz_background_clip);
    mTempData.SetPropertyBit(eCSSProperty__moz_background_origin);
    mTempData.SetPropertyBit(eCSSProperty__moz_background_size);
    return PR_TRUE;
  }
  delete positionHead;
  delete sizeHead;
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(simpleValues); ++i) {
    delete simpleHeads[i];
  }
  return PR_FALSE;
}


PRBool
CSSParserImpl::ParseBackgroundItem(CSSParserImpl::BackgroundItem& aItem,
                                   PRBool aFirstItem)
{
  
  
  aItem.mImage.SetNoneValue();
  aItem.mRepeat.SetIntValue(NS_STYLE_BG_REPEAT_XY, eCSSUnit_Enumerated);
  aItem.mAttachment.SetIntValue(NS_STYLE_BG_ATTACHMENT_SCROLL,
                                eCSSUnit_Enumerated);
  aItem.mPosition.mXValue.SetPercentValue(0.0f);
  aItem.mPosition.mYValue.SetPercentValue(0.0f);
  aItem.mClip.SetIntValue(NS_STYLE_BG_CLIP_BORDER, eCSSUnit_Enumerated);
  aItem.mOrigin.SetIntValue(NS_STYLE_BG_ORIGIN_PADDING, eCSSUnit_Enumerated);
  aItem.mSize.mXValue.SetAutoValue();
  aItem.mSize.mYValue.SetAutoValue();
  aItem.mLastItem = PR_FALSE;

  PRBool haveColor = PR_FALSE,
         haveImage = PR_FALSE,
         haveRepeat = PR_FALSE,
         haveAttach = PR_FALSE,
         havePosition = PR_FALSE,
         haveSomething = PR_FALSE;
  while (GetToken(PR_TRUE)) {
    nsCSSTokenType tt = mToken.mType;
    UngetToken(); 
    if (tt == eCSSToken_Symbol) {
      
      
      break;
    }

    if (tt == eCSSToken_Ident) {
      nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);
      PRInt32 dummy;
      if (keyword == eCSSKeyword_inherit ||
          keyword == eCSSKeyword__moz_initial) {
        if (haveSomething || !aFirstItem)
          return PR_FALSE;
        haveColor = haveImage = haveRepeat = haveAttach = havePosition =
          PR_TRUE;
        GetToken(PR_TRUE); 
        nsCSSValue val;
        if (keyword == eCSSKeyword_inherit) {
          val.SetInheritValue();
        } else {
          val.SetInitialValue();
        }
        mTempData.mColor.mBackColor = val;
        aItem.mImage = val;
        aItem.mRepeat = val;
        aItem.mAttachment = val;
        aItem.mPosition.SetBothValuesTo(val);
        aItem.mClip = val;
        aItem.mOrigin = val;
        aItem.mSize.mXValue = val;
        aItem.mSize.mYValue = val;
        aItem.mLastItem = PR_TRUE;
        haveSomething = PR_TRUE;
        break;
      } else if (keyword == eCSSKeyword_none) {
        if (haveImage)
          return PR_FALSE;
        haveImage = PR_TRUE;
        if (!ParseSingleValueProperty(aItem.mImage,
                                      eCSSProperty_background_image)) {
          NS_NOTREACHED("should be able to parse");
          return PR_FALSE;
        }
      } else if (nsCSSProps::FindKeyword(keyword,
                   nsCSSProps::kBackgroundAttachmentKTable, dummy)) {
        if (haveAttach)
          return PR_FALSE;
        haveAttach = PR_TRUE;
        if (!ParseSingleValueProperty(aItem.mAttachment,
                                      eCSSProperty_background_attachment)) {
          NS_NOTREACHED("should be able to parse");
          return PR_FALSE;
        }
      } else if (nsCSSProps::FindKeyword(keyword,
                   nsCSSProps::kBackgroundRepeatKTable, dummy)) {
        if (haveRepeat)
          return PR_FALSE;
        haveRepeat = PR_TRUE;
        if (!ParseSingleValueProperty(aItem.mRepeat,
                                      eCSSProperty_background_repeat)) {
          NS_NOTREACHED("should be able to parse");
          return PR_FALSE;
        }
      } else if (nsCSSProps::FindKeyword(keyword,
                   nsCSSProps::kBackgroundPositionKTable, dummy)) {
        if (havePosition)
          return PR_FALSE;
        havePosition = PR_TRUE;
        if (!ParseBoxPositionValues(aItem.mPosition, PR_FALSE)) {
          return PR_FALSE;
        }
#if 0
    
    
    
    
      } else if (nsCSSProps::FindKeyword(keyword,
                   nsCSSProps::kBackgroundClipKTable, dummy)) {
        
        
        
        NS_ASSERTION(
          nsCSSProps::kBackgroundClipKTable[0] == eCSSKeyword_border &&
          nsCSSProps::kBackgroundClipKTable[2] == eCSSKeyword_padding &&
          nsCSSProps::kBackgroundClipKTable[4] == eCSSKeyword_UNKNOWN,
          "need to rewrite this code");
        if (haveOrigin)
          return PR_FALSE;
        haveOrigin = PR_TRUE;
        if (!ParseSingleValueProperty(aItem.mOrigin,
                                      eCSSProperty__moz_background_origin)) {
          NS_NOTREACHED("should be able to parse");
          return PR_FALSE;
        }
        PR_STATIC_ASSERT(NS_STYLE_BG_CLIP_BORDER ==
                         NS_STYLE_BG_ORIGIN_BORDER);
        PR_STATIC_ASSERT(NS_STYLE_BG_CLIP_PADDING ==
                         NS_STYLE_BG_ORIGIN_PADDING);
        
        
        
        aItem.mClip = aItem.mOrigin;
      
#endif
      } else {
        if (haveColor)
          return PR_FALSE;
        haveColor = PR_TRUE;
        if (!ParseSingleValueProperty(mTempData.mColor.mBackColor,
                                      eCSSProperty_background_color)) {
          return PR_FALSE;
        }
        aItem.mLastItem = PR_TRUE;
      }
    } else if (eCSSToken_Function == tt &&
               (mToken.mIdent.LowerCaseEqualsLiteral("url") ||
                mToken.mIdent.LowerCaseEqualsLiteral("-moz-linear-gradient") ||
                mToken.mIdent.LowerCaseEqualsLiteral("-moz-radial-gradient"))) {
      if (haveImage)
        return PR_FALSE;
      haveImage = PR_TRUE;
      if (!ParseSingleValueProperty(aItem.mImage,
                                    eCSSProperty_background_image)) {
        return PR_FALSE;
      }
    } else if (mToken.IsDimension() || tt == eCSSToken_Percentage) {
      if (havePosition)
        return PR_FALSE;
      havePosition = PR_TRUE;
      if (!ParseBoxPositionValues(aItem.mPosition, PR_FALSE)) {
        return PR_FALSE;
      }
    } else {
      if (haveColor)
        return PR_FALSE;
      haveColor = PR_TRUE;
      
      
      if (!ParseSingleValueProperty(mTempData.mColor.mBackColor,
                                    eCSSProperty_background_color)) {
        return PR_FALSE;
      }
      aItem.mLastItem = PR_TRUE;
    }
    haveSomething = PR_TRUE;
  }

  return haveSomething;
}



PRBool
CSSParserImpl::ParseBackgroundList(nsCSSProperty aPropID)
{
  
  nsCSSValue value;
  nsCSSValueList *head = nsnull, **tail = &head;
  for (;;) {
    if (!ParseSingleValueProperty(value, aPropID)) {
      break;
    }
    PRBool inheritOrInitial = value.GetUnit() == eCSSUnit_Inherit ||
                              value.GetUnit() == eCSSUnit_Initial;
    if (inheritOrInitial && head) {
      
      break;
    }
    nsCSSValueList *item = new nsCSSValueList;
    if (!item) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      break;
    }
    item->mValue = value;
    *tail = item;
    tail = &item->mNext;
    if (!inheritOrInitial && ExpectSymbol(',', PR_TRUE)) {
      continue;
    }
    if (!ExpectEndProperty()) {
      break;
    }
    nsCSSValueList **source =
      static_cast<nsCSSValueList**>(mTempData.PropertyAt(aPropID));
    *source = head;
    mTempData.SetPropertyBit(aPropID);
    return PR_TRUE;
  }
  delete head;
  return PR_FALSE;
}


PRBool
CSSParserImpl::ParseBackgroundPosition()
{
  
  nsCSSValuePair valuePair;
  nsCSSValuePairList *head = nsnull, **tail = &head;
  for (;;) {
    if (!ParseBoxPositionValues(valuePair, !head)) {
      break;
    }
    PRBool inheritOrInitial = valuePair.mXValue.GetUnit() == eCSSUnit_Inherit ||
                              valuePair.mXValue.GetUnit() == eCSSUnit_Initial;
    nsCSSValuePairList *item = new nsCSSValuePairList;
    if (!item) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      break;
    }
    item->mXValue = valuePair.mXValue;
    item->mYValue = valuePair.mYValue;
    *tail = item;
    tail = &item->mNext;
    if (!inheritOrInitial && ExpectSymbol(',', PR_TRUE)) {
      continue;
    }
    if (!ExpectEndProperty()) {
      break;
    }
    mTempData.mColor.mBackPosition = head;
    mTempData.SetPropertyBit(eCSSProperty_background_position);
    return PR_TRUE;
  }
  delete head;
  return PR_FALSE;
}










PRBool CSSParserImpl::ParseBoxPositionValues(nsCSSValuePair &aOut,
                                             PRBool aAcceptsInherit)
{
  
  nsCSSValue &xValue = aOut.mXValue,
             &yValue = aOut.mYValue;
  PRInt32 variantMask = aAcceptsInherit ? VARIANT_HLP : VARIANT_LP;
  if (ParseVariant(xValue, variantMask, nsnull)) {
    if (eCSSUnit_Inherit == xValue.GetUnit() ||
        eCSSUnit_Initial == xValue.GetUnit()) {  
      yValue = xValue;
      return PR_TRUE;
    }
    
    
    if (ParseVariant(yValue, VARIANT_LP, nsnull)) {
      
      return PR_TRUE;
    }

    if (ParseEnum(yValue, nsCSSProps::kBackgroundPositionKTable)) {
      PRInt32 yVal = yValue.GetIntValue();
      if (!(yVal & BG_CTB)) {
        
        return PR_FALSE;
      }
      yValue = BoxPositionMaskToCSSValue(yVal, PR_FALSE);
      return PR_TRUE;
    }

    
    
    yValue.SetPercentValue(0.5f);
    return PR_TRUE;
  }

  
  
  
  
  
  
  PRInt32 mask = 0;
  if (ParseEnum(xValue, nsCSSProps::kBackgroundPositionKTable)) {
    PRInt32 bit = xValue.GetIntValue();
    mask |= bit;
    if (ParseEnum(xValue, nsCSSProps::kBackgroundPositionKTable)) {
      bit = xValue.GetIntValue();
      if (mask & (bit & ~BG_CENTER)) {
        
        return PR_FALSE;
      }
      mask |= bit;
    }
    else {
      
      if (ParseVariant(yValue, VARIANT_LP, nsnull)) {
        if (!(mask & BG_CLR)) {
          
          return PR_FALSE;
        }

        xValue = BoxPositionMaskToCSSValue(mask, PR_TRUE);
        return PR_TRUE;
      }
    }
  }

  
  
  if ((mask == 0) || (mask == (BG_TOP | BG_BOTTOM)) ||
      (mask == (BG_LEFT | BG_RIGHT))) {
    return PR_FALSE;
  }

  
  xValue = BoxPositionMaskToCSSValue(mask, PR_TRUE);
  yValue = BoxPositionMaskToCSSValue(mask, PR_FALSE);
  return PR_TRUE;
}



PRBool
CSSParserImpl::ParseBackgroundSize()
{
  nsCSSValuePair valuePair;
  nsCSSValuePairList *head = nsnull, **tail = &head;
  if (ParseVariant(valuePair.mXValue, VARIANT_INHERIT, nsnull)) {
    
    head = new nsCSSValuePairList;
    if (!head) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      return PR_FALSE;
    }
    head->mXValue = valuePair.mXValue;
    head->mYValue.Reset();
    mTempData.mColor.mBackSize = head;
    mTempData.SetPropertyBit(eCSSProperty__moz_background_size);
    return ExpectEndProperty();
  }

  for (;;) {
    if (!ParseBackgroundSizeValues(valuePair)) {
      break;
    }
    nsCSSValuePairList *item = new nsCSSValuePairList;
    if (!item) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      break;
    }
    item->mXValue = valuePair.mXValue;
    item->mYValue = valuePair.mYValue;
    *tail = item;
    tail = &item->mNext;
    if (ExpectSymbol(',', PR_TRUE)) {
      continue;
    }
    if (!ExpectEndProperty()) {
      break;
    }
    mTempData.mColor.mBackSize = head;
    mTempData.SetPropertyBit(eCSSProperty__moz_background_size);
    return PR_TRUE;
  }
  delete head;
  return PR_FALSE;
}











PRBool CSSParserImpl::ParseBackgroundSizeValues(nsCSSValuePair &aOut)
{
  
  nsCSSValue &xValue = aOut.mXValue,
             &yValue = aOut.mYValue;
  if (ParseNonNegativeVariant(xValue, VARIANT_LP | VARIANT_AUTO, nsnull)) {
    
    
    if (ParseNonNegativeVariant(yValue, VARIANT_LP | VARIANT_AUTO, nsnull)) {
      
      return PR_TRUE;
    }

    
    
    yValue.SetAutoValue();
    return PR_TRUE;
  }

  
  if (!ParseEnum(xValue, nsCSSProps::kBackgroundSizeKTable))
    return PR_FALSE;
  yValue.Reset();
  return PR_TRUE;
}

PRBool
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
  return ParseBoxProperties(mTempData.mMargin.mBorderColor,
                            kBorderColorIDs);
}

PRBool
CSSParserImpl::ParseBorderImage()
{
  if (ParseVariant(mTempData.mMargin.mBorderImage,
                   VARIANT_INHERIT | VARIANT_NONE, nsnull)) {
    mTempData.SetPropertyBit(eCSSProperty_border_image);
    return PR_TRUE;
  }

  
  nsRefPtr<nsCSSValue::Array> arr = nsCSSValue::Array::Create(11);
  if (!arr) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }

  nsCSSValue& url = arr->Item(0);
  nsCSSValue& splitTop = arr->Item(1);
  nsCSSValue& splitRight = arr->Item(2);
  nsCSSValue& splitBottom = arr->Item(3);
  nsCSSValue& splitLeft = arr->Item(4);
  nsCSSValue& borderWidthTop = arr->Item(5);
  nsCSSValue& borderWidthRight = arr->Item(6);
  nsCSSValue& borderWidthBottom = arr->Item(7);
  nsCSSValue& borderWidthLeft = arr->Item(8);
  nsCSSValue& horizontalKeyword = arr->Item(9);
  nsCSSValue& verticalKeyword = arr->Item(10);

  
  if (!ParseVariant(url, VARIANT_URL, nsnull)) {
    return PR_FALSE;
  }

  
  if (!ParseNonNegativeVariant(splitTop,
                               VARIANT_NUMBER | VARIANT_PERCENT, nsnull)) {
    return PR_FALSE;
  }
  if (!ParseNonNegativeVariant(splitRight,
                               VARIANT_NUMBER | VARIANT_PERCENT, nsnull)) {
    splitRight = splitTop;
  }
  if (!ParseNonNegativeVariant(splitBottom,
                               VARIANT_NUMBER | VARIANT_PERCENT, nsnull)) {
    splitBottom = splitTop;
  }
  if (!ParseNonNegativeVariant(splitLeft,
                               VARIANT_NUMBER | VARIANT_PERCENT, nsnull)) {
    splitLeft = splitRight;
  }

  
  if (ExpectSymbol('/', PR_TRUE)) {
    
    if (!ParseNonNegativeVariant(borderWidthTop, VARIANT_LENGTH, nsnull)) {
      return PR_FALSE;
    }
    if (!ParseNonNegativeVariant(borderWidthRight, VARIANT_LENGTH, nsnull)) {
      borderWidthRight = borderWidthTop;
    }
    if (!ParseNonNegativeVariant(borderWidthBottom, VARIANT_LENGTH, nsnull)) {
      borderWidthBottom = borderWidthTop;
    }
    if (!ParseNonNegativeVariant(borderWidthLeft, VARIANT_LENGTH, nsnull)) {
      borderWidthLeft = borderWidthRight;
    }
  }

  
  
  if (ParseEnum(horizontalKeyword, nsCSSProps::kBorderImageKTable)) {
    ParseEnum(verticalKeyword, nsCSSProps::kBorderImageKTable);
  }

  if (!ExpectEndProperty()) {
    return PR_FALSE;
  }

  mTempData.mMargin.mBorderImage.SetArrayValue(arr, eCSSUnit_Array);
  mTempData.SetPropertyBit(eCSSProperty_border_image);

  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseBorderSpacing()
{
  nsCSSValue  xValue;
  if (ParseNonNegativeVariant(xValue, VARIANT_HL, nsnull)) {
    if (xValue.IsLengthUnit()) {
      
      nsCSSValue yValue;
      if (ParseNonNegativeVariant(yValue, VARIANT_LENGTH, nsnull)) {
        
        if (ExpectEndProperty()) {
          mTempData.mTable.mBorderSpacing.mXValue = xValue;
          mTempData.mTable.mBorderSpacing.mYValue = yValue;
          mTempData.SetPropertyBit(eCSSProperty_border_spacing);
          return PR_TRUE;
        }
        return PR_FALSE;
      }
    }

    
    
    if (ExpectEndProperty()) {
      mTempData.mTable.mBorderSpacing.SetBothValuesTo(xValue);
      mTempData.SetPropertyBit(eCSSProperty_border_spacing);
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseBorderSide(const nsCSSProperty aPropIDs[],
                               PRBool aSetAllSides)
{
  const PRInt32 numProps = 3;
  nsCSSValue  values[numProps];

  PRInt32 found = ParseChoice(values, aPropIDs, numProps);
  if ((found < 1) || (PR_FALSE == ExpectEndProperty())) {
    return PR_FALSE;
  }

  if ((found & 1) == 0) { 
    values[0].SetIntValue(NS_STYLE_BORDER_WIDTH_MEDIUM, eCSSUnit_Enumerated);
  }
  if ((found & 2) == 0) { 
    values[1].SetNoneValue();
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

    
    for (PRInt32 index = 0; index < 4; index++) {
      NS_ASSERTION(numProps == 3, "This code needs updating");
      AppendValue(kBorderWidthIDs[index], values[0]);
      AppendValue(kBorderStyleIDs[index], values[1]);
      AppendValue(kBorderColorIDs[index], values[2]);
    }
  }
  else {
    
    for (PRInt32 index = 0; index < numProps; index++) {
      AppendValue(aPropIDs[index], values[index]);
    }
  }
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseDirectionalBorderSide(const nsCSSProperty aPropIDs[],
                                          PRInt32 aSourceType)
{
  const PRInt32 numProps = 3;
  nsCSSValue  values[numProps];

  PRInt32 found = ParseChoice(values, aPropIDs, numProps);
  if ((found < 1) || (PR_FALSE == ExpectEndProperty())) {
    return PR_FALSE;
  }

  if ((found & 1) == 0) { 
    values[0].SetIntValue(NS_STYLE_BORDER_WIDTH_MEDIUM, eCSSUnit_Enumerated);
  }
  if ((found & 2) == 0) { 
    values[1].SetNoneValue();
  }
  if ((found & 4) == 0) { 
    values[2].SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR, eCSSUnit_Enumerated);
  }
  for (PRInt32 index = 0; index < numProps; index++) {
    const nsCSSProperty* subprops =
      nsCSSProps::SubpropertyEntryFor(aPropIDs[index + numProps]);
    NS_ASSERTION(subprops[3] == eCSSProperty_UNKNOWN,
                 "not box property with physical vs. logical cascading");
    AppendValue(subprops[0], values[index]);
    nsCSSValue typeVal(aSourceType, eCSSUnit_Enumerated);
    AppendValue(subprops[1], typeVal);
    AppendValue(subprops[2], typeVal);
  }
  return PR_TRUE;
}

PRBool
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
  return ParseBoxProperties(mTempData.mMargin.mBorderStyle,
                            kBorderStyleIDs);
}

PRBool
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
  return ParseBoxProperties(mTempData.mMargin.mBorderWidth,
                            kBorderWidthIDs);
}

PRBool
CSSParserImpl::ParseBorderColors(nsCSSValueList** aResult,
                                 nsCSSProperty aProperty)
{
  nsCSSValueList *list = nsnull;
  for (nsCSSValueList **curp = &list, *cur; ; curp = &cur->mNext) {
    cur = *curp = new nsCSSValueList();
    if (!cur) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      break;
    }
    if (!ParseVariant(cur->mValue,
                      (cur == list)
                        ? (VARIANT_HCK | VARIANT_NONE)
                        : (VARIANT_COLOR | VARIANT_KEYWORD),
                      nsCSSProps::kBorderColorKTable)) {
      break;
    }
    if (ExpectEndProperty()) {
      
      
      mTempData.SetPropertyBit(aProperty);
      *aResult = list;
      return PR_TRUE;
    }
    if (cur->mValue.GetUnit() == eCSSUnit_Inherit ||
        cur->mValue.GetUnit() == eCSSUnit_Initial ||
        cur->mValue.GetUnit() == eCSSUnit_None) {
      
      break;
    }
  }
  
  delete list;
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseRect(nsCSSRect& aRect, nsCSSProperty aPropID)
{
  nsCSSRect rect;
  PRBool result;
  if ((result = DoParseRect(rect)) &&
      rect != aRect) {
    aRect = rect;
    mTempData.SetPropertyBit(aPropID);
  }
  return result;
}

PRBool
CSSParserImpl::DoParseRect(nsCSSRect& aRect)
{
  if (! GetToken(PR_TRUE)) {
    return PR_FALSE;
  }
  if (eCSSToken_Ident == mToken.mType) {
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);
    switch (keyword) {
      case eCSSKeyword_auto:
        if (ExpectEndProperty()) {
          aRect.SetAllSidesTo(nsCSSValue(eCSSUnit_RectIsAuto));
          return PR_TRUE;
        }
        break;
      case eCSSKeyword_inherit:
        if (ExpectEndProperty()) {
          aRect.SetAllSidesTo(nsCSSValue(eCSSUnit_Inherit));
          return PR_TRUE;
        }
        break;
      case eCSSKeyword__moz_initial:
        if (ExpectEndProperty()) {
          aRect.SetAllSidesTo(nsCSSValue(eCSSUnit_Initial));
          return PR_TRUE;
        }
        break;
      default:
        UngetToken();
        break;
    }
  } else if ((eCSSToken_Function == mToken.mType) &&
             mToken.mIdent.LowerCaseEqualsLiteral("rect")) {
    if (!ExpectSymbol('(', PR_TRUE)) {
      return PR_FALSE;
    }
    NS_FOR_CSS_SIDES(side) {
      if (! ParseVariant(aRect.*(nsCSSRect::sides[side]),
                         VARIANT_AL, nsnull)) {
        return PR_FALSE;
      }
      if (3 != side) {
        
        ExpectSymbol(',', PR_TRUE);
      }
    }
    if (!ExpectSymbol(')', PR_TRUE)) {
      return PR_FALSE;
    }
    if (ExpectEndProperty()) {
      return PR_TRUE;
    }
  } else {
    UngetToken();
  }
  return PR_FALSE;
}

#define VARIANT_CONTENT (VARIANT_STRING | VARIANT_URL | VARIANT_COUNTER | VARIANT_ATTR | \
                         VARIANT_KEYWORD)
PRBool
CSSParserImpl::ParseContent()
{
  
  nsCSSValue  value;
  if (ParseVariant(value,
                   VARIANT_CONTENT | VARIANT_INHERIT | VARIANT_NORMAL |
                     VARIANT_NONE,
                   nsCSSProps::kContentKTable)) {
    nsCSSValueList* listHead = new nsCSSValueList();
    nsCSSValueList* list = listHead;
    if (nsnull == list) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      return PR_FALSE;
    }
    list->mValue = value;

    while (nsnull != list) {
      if (ExpectEndProperty()) {
        mTempData.SetPropertyBit(eCSSProperty_content);
        mTempData.mContent.mContent = listHead;
        return PR_TRUE;
      }
      if (eCSSUnit_Inherit == value.GetUnit() ||
          eCSSUnit_Initial == value.GetUnit() ||
          eCSSUnit_Normal == value.GetUnit() ||
          eCSSUnit_None == value.GetUnit() ||
          (eCSSUnit_Enumerated == value.GetUnit() &&
           NS_STYLE_CONTENT_ALT_CONTENT == value.GetIntValue())) {
        
        delete listHead;
        return PR_FALSE;
      }
      if (ParseVariant(value, VARIANT_CONTENT, nsCSSProps::kContentKTable) &&
          
          (value.GetUnit() != eCSSUnit_Enumerated ||
           value.GetIntValue() != NS_STYLE_CONTENT_ALT_CONTENT)) {
        list->mNext = new nsCSSValueList();
        list = list->mNext;
        if (nsnull != list) {
          list->mValue = value;
        }
        else {
          mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
        }
      }
      else {
        break;
      }
    }
    delete listHead;
  }
  return PR_FALSE;
}

struct SingleCounterPropValue {
  char str[13];
  nsCSSUnit unit;
};

PRBool
CSSParserImpl::ParseCounterData(nsCSSValuePairList** aResult,
                                nsCSSProperty aPropID)
{
  nsSubstring* ident = NextIdent();
  if (nsnull == ident) {
    return PR_FALSE;
  }
  static const SingleCounterPropValue singleValues[] = {
    { "none", eCSSUnit_None },
    { "inherit", eCSSUnit_Inherit },
    { "-moz-initial", eCSSUnit_Initial }
  };
  for (const SingleCounterPropValue *sv = singleValues,
           *sv_end = singleValues + NS_ARRAY_LENGTH(singleValues);
       sv != sv_end; ++sv) {
    if (ident->LowerCaseEqualsASCII(sv->str)) {
      if (CheckEndProperty()) {
        nsCSSValuePairList* dataHead = new nsCSSValuePairList();
        if (!dataHead) {
          mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
          return PR_FALSE;
        }
        dataHead->mXValue = nsCSSValue(sv->unit);
        *aResult = dataHead;
        mTempData.SetPropertyBit(aPropID);
        return PR_TRUE;
      }
      return PR_FALSE;
    }
  }
  UngetToken(); 

  nsCSSValuePairList* dataHead = nsnull;
  nsCSSValuePairList **next = &dataHead;
  for (;;) {
    if (!GetToken(PR_TRUE) || mToken.mType != eCSSToken_Ident) {
      break;
    }
    nsCSSValuePairList *data = *next = new nsCSSValuePairList();
    if (!data) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      break;
    }
    next = &data->mNext;
    data->mXValue.SetStringValue(mToken.mIdent, eCSSUnit_Ident);
    if (GetToken(PR_TRUE)) {
      if (eCSSToken_Number == mToken.mType && mToken.mIntegerValid) {
        data->mYValue.SetIntValue(mToken.mInteger, eCSSUnit_Integer);
      } else {
        UngetToken();
      }
    }
    if (ExpectEndProperty()) {
      mTempData.SetPropertyBit(aPropID);
      *aResult = dataHead;
      return PR_TRUE;
    }
  }
  delete dataHead;
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseCue()
{
  nsCSSValue before;
  if (ParseSingleValueProperty(before, eCSSProperty_cue_before)) {
    if (eCSSUnit_Inherit != before.GetUnit() &&
        eCSSUnit_Initial != before.GetUnit()) {
      nsCSSValue after;
      if (ParseSingleValueProperty(after, eCSSProperty_cue_after)) {
        if (ExpectEndProperty()) {
          AppendValue(eCSSProperty_cue_before, before);
          AppendValue(eCSSProperty_cue_after, after);
          return PR_TRUE;
        }
        return PR_FALSE;
      }
    }
    if (ExpectEndProperty()) {
      AppendValue(eCSSProperty_cue_before, before);
      AppendValue(eCSSProperty_cue_after, before);
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseCursor()
{
  nsCSSValueList *list = nsnull;
  for (nsCSSValueList **curp = &list, *cur; ; curp = &cur->mNext) {
    cur = *curp = new nsCSSValueList();
    if (!cur) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      break;
    }
    if (!ParseVariant(cur->mValue,
                      (cur == list) ? VARIANT_AHUK : VARIANT_AUK,
                      nsCSSProps::kCursorKTable)) {
      break;
    }
    if (cur->mValue.GetUnit() != eCSSUnit_URL) {
      if (!ExpectEndProperty()) {
        break;
      }
      
      
      mTempData.SetPropertyBit(eCSSProperty_cursor);
      mTempData.mUserInterface.mCursor = list;
      return PR_TRUE;
    }
    
    nsRefPtr<nsCSSValue::Array> val = nsCSSValue::Array::Create(3);
    if (!val) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      break;
    }
    val->Item(0) = cur->mValue;
    cur->mValue.SetArrayValue(val, eCSSUnit_Array);

    
    if (ParseVariant(val->Item(1), VARIANT_NUMBER, nsnull)) {
      
      if (!ParseVariant(val->Item(2), VARIANT_NUMBER, nsnull)) {
        break;
      }
    }

    if (!ExpectSymbol(',', PR_TRUE)) {
      break;
    }
  }
  
  delete list;
  return PR_FALSE;
}


PRBool
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
      }
      return PR_TRUE;
    }
    return PR_FALSE;
  }

  
  const PRInt32 numProps = 3;
  nsCSSValue  values[numProps];
  PRInt32 found = ParseChoice(values, fontIDs, numProps);
  if ((found < 0) || (eCSSUnit_Inherit == values[0].GetUnit()) ||
      (eCSSUnit_Initial == values[0].GetUnit())) { 
    return PR_FALSE;
  }
  if ((found & 1) == 0) {
    
    values[0].SetNormalValue();
  }
  if ((found & 2) == 0) {
    
    values[1].SetNormalValue();
  }
  if ((found & 4) == 0) {
    
    values[2].SetNormalValue();
  }

  
  nsCSSValue  size;
  if (! ParseVariant(size, VARIANT_KEYWORD | VARIANT_LP, nsCSSProps::kFontSizeKTable)) {
    return PR_FALSE;
  }

  
  nsCSSValue  lineHeight;
  if (ExpectSymbol('/', PR_TRUE)) {
    if (! ParseNonNegativeVariant(lineHeight,
                                  VARIANT_NUMBER | VARIANT_LP | VARIANT_NORMAL,
                                  nsnull)) {
      return PR_FALSE;
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
      AppendValue(eCSSProperty_font_stretch, nsCSSValue(eCSSUnit_Normal));
      AppendValue(eCSSProperty_font_size_adjust, nsCSSValue(eCSSUnit_None));
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseFontWeight(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_HMKI | VARIANT_SYSFONT, nsCSSProps::kFontWeightKTable)) {
    if (eCSSUnit_Integer == aValue.GetUnit()) { 
      PRInt32 intValue = aValue.GetIntValue();
      if ((100 <= intValue) &&
          (intValue <= 900) &&
          (0 == (intValue % 100))) {
        return PR_TRUE;
      } else {
        UngetToken();
        return PR_FALSE;
      }
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseOneFamily(nsAString& aFamily)
{
  if (!GetToken(PR_TRUE))
    return PR_FALSE;

  nsCSSToken* tk = &mToken;

  if (eCSSToken_Ident == tk->mType) {
    aFamily.Append(tk->mIdent);
    for (;;) {
      if (!GetToken(PR_FALSE))
        break;

      if (eCSSToken_Ident == tk->mType) {
        aFamily.Append(tk->mIdent);
      } else if (eCSSToken_WhiteSpace == tk->mType) {
        
        
        if (!GetToken(PR_TRUE))
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
    return PR_TRUE;

  } else if (eCSSToken_String == tk->mType) {
    aFamily.Append(tk->mSymbol); 
    aFamily.Append(tk->mIdent); 
    aFamily.Append(tk->mSymbol);
    return PR_TRUE;

  } else {
    UngetToken();
    return PR_FALSE;
  }
}







PRBool
CSSParserImpl::ParseFunctionInternals(const PRInt32 aVariantMask[],
                                      PRUint16 aMinElems,
                                      PRUint16 aMaxElems,
                                      nsTArray<nsCSSValue> &aOutput)
{
  for (PRUint16 index = 0; index < aMaxElems; ++index) {
    nsCSSValue newValue;
    if (!ParseVariant(newValue, aVariantMask[index], nsnull))
      return PR_FALSE;

    if (!aOutput.AppendElement(newValue)) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      return PR_FALSE;
    }
    
    
    if (!ExpectSymbol(',', PR_TRUE)) {
      
      
      return ExpectSymbol(')', PR_TRUE) && (index + 1) >= aMinElems;
    }
  }

  
  
  return PR_FALSE;
}



















PRBool
CSSParserImpl::ParseFunction(const nsString &aFunction,
                             const PRInt32 aAllowedTypes[],
                             PRUint16 aMinElems, PRUint16 aMaxElems,
                             nsCSSValue &aValue)
{
  typedef nsTArray<nsCSSValue>::size_type arrlen_t;

  


  static const arrlen_t MAX_ALLOWED_ELEMS = 0xFFFE;

  



  nsString functionName(aFunction);

  
  if (!ExpectSymbol('(', PR_TRUE))
    return PR_FALSE;
  
  


  nsTArray<nsCSSValue> foundValues;
  if (!ParseFunctionInternals(aAllowedTypes, aMinElems, aMaxElems,
                              foundValues))
    return PR_FALSE;
  
  




  PRUint16 numElements = (foundValues.Length() <= MAX_ALLOWED_ELEMS ?
                          foundValues.Length() + 1 : MAX_ALLOWED_ELEMS);
  nsRefPtr<nsCSSValue::Array> convertedArray =
    nsCSSValue::Array::Create(numElements);
  if (!convertedArray) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }
  
  
  convertedArray->Item(0).SetStringValue(functionName, eCSSUnit_Ident);
  for (PRUint16 index = 0; index + 1 < numElements; ++index)
    convertedArray->Item(index + 1) = foundValues[static_cast<arrlen_t>(index)];
  
  
  aValue.SetArrayValue(convertedArray, eCSSUnit_Function);
  
  
  return PR_TRUE;
}













static PRBool GetFunctionParseInformation(nsCSSKeyword aToken,
                                          PRUint16 &aMinElems,
                                          PRUint16 &aMaxElems,
                                          const PRInt32 *& aVariantMask)
{




  enum { eLengthPercent,
         eTwoLengthPercents,
         eAngle,
         eTwoAngles,
         eNumber,
         eTwoNumbers,
         eMatrix,
         eNumVariantMasks };
  static const PRInt32 kMaxElemsPerFunction = 6;
  static const PRInt32 kVariantMasks[eNumVariantMasks][kMaxElemsPerFunction] = {
    {VARIANT_LENGTH | VARIANT_PERCENT},
    {VARIANT_LENGTH | VARIANT_PERCENT, VARIANT_LENGTH | VARIANT_PERCENT},
    {VARIANT_ANGLE},
    {VARIANT_ANGLE, VARIANT_ANGLE},
    {VARIANT_NUMBER},
    {VARIANT_NUMBER, VARIANT_NUMBER},
    {VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER, VARIANT_NUMBER,
     VARIANT_LENGTH | VARIANT_PERCENT, VARIANT_LENGTH | VARIANT_PERCENT}};

#ifdef DEBUG
  static const PRUint8 kVariantMaskLengths[eNumVariantMasks] =
    {1, 2, 1, 2, 1, 2, 6};
#endif

  PRInt32 variantIndex = eNumVariantMasks;

  switch (aToken) {
  case eCSSKeyword_translatex:
    
    variantIndex = eLengthPercent;
    aMinElems = 1U;
    aMaxElems = 1U;
    break;
  case eCSSKeyword_translatey:
    
    variantIndex = eLengthPercent;
    aMinElems = 1U;
    aMaxElems = 1U;
    break;
  case eCSSKeyword_scalex:
    
    variantIndex = eNumber;
    aMinElems = 1U;
    aMaxElems = 1U;
    break;
  case eCSSKeyword_scaley:
    
    variantIndex = eNumber;
    aMinElems = 1U;
    aMaxElems = 1U;
    break;
  case eCSSKeyword_rotate:
    
    variantIndex = eAngle;
    aMinElems = 1U;
    aMaxElems = 1U;
    break;
  case eCSSKeyword_translate:
    
    variantIndex = eTwoLengthPercents;
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
    
    variantIndex = eMatrix;
    aMinElems = 6U;
    aMaxElems = 6U;
    break;    
  default:
    
    return PR_FALSE;
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

  return PR_TRUE;
}
                                          




PRBool CSSParserImpl::ReadSingleTransform(nsCSSValueList **& aTail)
{
  typedef nsTArray<nsCSSValue>::size_type arrlen_t;

  if (!GetToken(PR_TRUE))
    return PR_FALSE;
  
  
  if (mToken.mType != eCSSToken_Function) {
    UngetToken();
    return PR_FALSE;
  }

  


  const PRInt32* variantMask;
  PRUint16 minElems, maxElems;
  if (!GetFunctionParseInformation(nsCSSKeywords::LookupKeyword(mToken.mIdent),
                                   minElems, maxElems, variantMask))
    return PR_FALSE;

  
  nsAutoPtr<nsCSSValue> newCell(new nsCSSValue);
  if (!newCell) {
    mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
    return PR_FALSE;
  }

  
  if (!ParseFunction(mToken.mIdent, variantMask, minElems, maxElems, *newCell))
    return PR_FALSE;

  
  nsAutoPtr<nsCSSValueList> toAppend(new nsCSSValueList);
  if (!toAppend)
    return PR_FALSE;

  toAppend->mValue = *newCell;
  
  


  *aTail = toAppend.forget();
  aTail = &(*aTail)->mNext;
  
  
  return PR_TRUE;
}




PRBool CSSParserImpl::ParseMozTransform()
{
  mTempData.mDisplay.mTransform = nsnull;
 
  


  nsCSSValue keywordValue;
  if (ParseVariant(keywordValue, VARIANT_INHERIT | VARIANT_NONE, nsnull)) {
    


    mTempData.mDisplay.mTransform = new nsCSSValueList;
    if (!mTempData.mDisplay.mTransform) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      return PR_FALSE;
    }

    
    mTempData.mDisplay.mTransform->mValue = keywordValue;
    mTempData.SetPropertyBit(eCSSProperty__moz_transform);
    return PR_TRUE;
  }
  
  



  nsCSSValueList *transformList = nsnull;
  nsCSSValueList **tail = &transformList;
  do {
    
    if (!ReadSingleTransform(tail)) {
      delete transformList;
      return PR_FALSE;
    }
  }
  while (!CheckEndProperty());

  


  if (!ExpectEndProperty()) {
    delete transformList;
    return PR_FALSE;
  }
  
  
  NS_ASSERTION(transformList, "Didn't read any transforms!");
  
  mTempData.SetPropertyBit(eCSSProperty__moz_transform);
  mTempData.mDisplay.mTransform = transformList;

  return PR_TRUE;
}

PRBool CSSParserImpl::ParseMozTransformOrigin()
{
  
  if (!ParseBoxPositionValues(mTempData.mDisplay.mTransformOrigin, PR_TRUE) ||
      !ExpectEndProperty())
    return PR_FALSE;

  
  mTempData.SetPropertyBit(eCSSProperty__moz_transform_origin);
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseFamily(nsCSSValue& aValue)
{
  if (!GetToken(PR_TRUE))
    return PR_FALSE;

  if (eCSSToken_Ident == mToken.mType) {
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(mToken.mIdent);
    if (keyword == eCSSKeyword_inherit) {
      aValue.SetInheritValue();
      return PR_TRUE;
    }
    if (keyword == eCSSKeyword__moz_initial) {
      aValue.SetInitialValue();
      return PR_TRUE;
    }
    if (keyword == eCSSKeyword__moz_use_system_font &&
        !IsParsingCompoundProperty()) {
      aValue.SetSystemFontValue();
      return PR_TRUE;
    }
  }

  UngetToken();

  nsAutoString family;
  for (;;) {
    if (!ParseOneFamily(family))
      return PR_FALSE;

    if (!ExpectSymbol(',', PR_TRUE))
      break;

    family.Append(PRUnichar(','));
  }

  if (family.IsEmpty()) {
    return PR_FALSE;
  }
  aValue.SetStringValue(family, eCSSUnit_Families);
  return PR_TRUE;
}





PRBool
CSSParserImpl::ParseFontSrc(nsCSSValue& aValue)
{
  
  nsTArray<nsCSSValue> values;
  nsCSSValue cur;
  for (;;) {
    if (!GetToken(PR_TRUE))
      break;

    if (mToken.mType == eCSSToken_Function &&
        mToken.mIdent.LowerCaseEqualsLiteral("url")) {
      if (!ParseURL(cur))
        return PR_FALSE;
      values.AppendElement(cur);
      if (!ParseFontSrcFormat(values))
        return PR_FALSE;

    } else if (mToken.mType == eCSSToken_Function &&
               mToken.mIdent.LowerCaseEqualsLiteral("local")) {
      
      
      
      
      

      nsAutoString family;
      if (!ExpectSymbol('(', PR_FALSE))
        return PR_FALSE;
      if (!ParseOneFamily(family))
        return PR_FALSE;
      if (!ExpectSymbol(')', PR_TRUE))
        return PR_FALSE;

      
      
      nsFont font(family, 0, 0, 0, 0, 0, 0);
      ExtractFirstFamilyData dat;

      font.EnumerateFamilies(ExtractFirstFamily, (void*) &dat);
      if (!dat.mGood)
        return PR_FALSE;

      cur.SetStringValue(dat.mFamilyName, eCSSUnit_Local_Font);
      values.AppendElement(cur);
    } else {
      return PR_FALSE;
    }

    if (!ExpectSymbol(',', PR_TRUE))
      break;
  }

  nsRefPtr<nsCSSValue::Array> srcVals
    = nsCSSValue::Array::Create(values.Length());
  if (!srcVals)
    return PR_FALSE;

  PRUint32 i;
  for (i = 0; i < values.Length(); i++)
    srcVals->Item(i) = values[i];
  aValue.SetArrayValue(srcVals, eCSSUnit_Array);
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseFontSrcFormat(nsTArray<nsCSSValue> & values)
{
  if (!GetToken(PR_TRUE))
    return PR_TRUE; 
  if (mToken.mType != eCSSToken_Function ||
      !mToken.mIdent.LowerCaseEqualsLiteral("format")) {
    UngetToken();
    return PR_TRUE;
  }
  if (!ExpectSymbol('(', PR_FALSE))
    return PR_FALSE;

  do {
    if (!GetToken(PR_TRUE))
      return PR_FALSE;

    if (mToken.mType != eCSSToken_String)
      return PR_FALSE;

    nsCSSValue cur(mToken.mIdent, eCSSUnit_Font_Format);
    values.AppendElement(cur);
  } while (ExpectSymbol(',', PR_TRUE));

  return ExpectSymbol(')', PR_TRUE);
}


PRBool
CSSParserImpl::ParseFontRanges(nsCSSValue& aValue)
{
  
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseListStyle()
{
  
  
  
  
  
  static const nsCSSProperty listStyleIDs[] = {
    eCSSPropertyExtra_x_none_value,
    eCSSProperty_list_style_type,
    eCSSProperty_list_style_position,
    eCSSProperty_list_style_image
  };

  nsCSSValue values[NS_ARRAY_LENGTH(listStyleIDs)];
  PRInt32 found =
    ParseChoice(values, listStyleIDs, NS_ARRAY_LENGTH(listStyleIDs));
  if (found < 1 || !ExpectEndProperty()) {
    return PR_FALSE;
  }

  if ((found & (1|2|8)) == (1|2|8)) {
    if (values[0].GetUnit() == eCSSUnit_None) {
      
      
      
      return PR_FALSE;
    } else {
      NS_ASSERTION(found == (1|2|4|8) && values[0] == values[1] &&
                   values[0] == values[2] && values[0] == values[3],
                   "should be a special value");
    }
  }

  
  if ((found & 2) == 0) {
    if (found & 1) {
      values[1].SetNoneValue();
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

  
  for (PRUint32 index = 1; index < NS_ARRAY_LENGTH(listStyleIDs); ++index) {
    AppendValue(listStyleIDs[index], values[index]);
  }
  return PR_TRUE;
}

PRBool
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
  return ParseBoxProperties(mTempData.mMargin.mMargin,
                            kMarginSideIDs);
}

PRBool
CSSParserImpl::ParseMarks(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_HOK, nsCSSProps::kPageMarksKTable)) {
    if (eCSSUnit_Enumerated == aValue.GetUnit()) {
      if (PR_FALSE == CheckEndProperty()) {
        nsCSSValue  second;
        if (ParseEnum(second, nsCSSProps::kPageMarksKTable)) {
          aValue.SetIntValue(aValue.GetIntValue() | second.GetIntValue(), eCSSUnit_Enumerated);
          return PR_TRUE;
        }
        return PR_FALSE;
      }
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseOutline()
{
  const PRInt32 numProps = 3;
  static const nsCSSProperty kOutlineIDs[] = {
    eCSSProperty_outline_color,
    eCSSProperty_outline_style,
    eCSSProperty_outline_width
  };

  nsCSSValue  values[numProps];
  PRInt32 found = ParseChoice(values, kOutlineIDs, numProps);
  if ((found < 1) || (PR_FALSE == ExpectEndProperty())) {
    return PR_FALSE;
  }

  
  if ((found & 1) == 0) {
#ifdef GFX_HAS_INVERT
    values[0].SetIntValue(NS_STYLE_COLOR_INVERT, eCSSUnit_Enumerated);
#else
    values[0].SetIntValue(NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR, eCSSUnit_Enumerated);
#endif
  }
  if ((found & 2) == 0) {
    values[1].SetNoneValue();
  }
  if ((found & 4) == 0) {
    values[2].SetIntValue(NS_STYLE_BORDER_WIDTH_MEDIUM, eCSSUnit_Enumerated);
  }

  PRInt32 index;
  for (index = 0; index < numProps; index++) {
    AppendValue(kOutlineIDs[index], values[index]);
  }
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseOverflow()
{
  nsCSSValue overflow;
  if (!ParseVariant(overflow, VARIANT_AHK,
                   nsCSSProps::kOverflowKTable) ||
      !ExpectEndProperty())
    return PR_FALSE;

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
  return PR_TRUE;
}

PRBool
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
  return ParseBoxProperties(mTempData.mMargin.mPadding,
                            kPaddingSideIDs);
}

PRBool
CSSParserImpl::ParsePause()
{
  nsCSSValue  before;
  if (ParseSingleValueProperty(before, eCSSProperty_pause_before)) {
    if (eCSSUnit_Inherit != before.GetUnit() && eCSSUnit_Initial != before.GetUnit()) {
      nsCSSValue after;
      if (ParseSingleValueProperty(after, eCSSProperty_pause_after)) {
        if (ExpectEndProperty()) {
          AppendValue(eCSSProperty_pause_before, before);
          AppendValue(eCSSProperty_pause_after, after);
          return PR_TRUE;
        }
        return PR_FALSE;
      }
    }
    if (ExpectEndProperty()) {
      AppendValue(eCSSProperty_pause_before, before);
      AppendValue(eCSSProperty_pause_after, before);
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseQuotes()
{
  nsCSSValue  open;
  if (ParseVariant(open, VARIANT_HOS, nsnull)) {
    if (eCSSUnit_String == open.GetUnit()) {
      nsCSSValuePairList* quotesHead = new nsCSSValuePairList();
      nsCSSValuePairList* quotes = quotesHead;
      if (nsnull == quotes) {
        mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
        return PR_FALSE;
      }
      quotes->mXValue = open;
      while (nsnull != quotes) {
        
        if (ParseVariant(quotes->mYValue, VARIANT_STRING,
                         nsnull)) {
          if (CheckEndProperty()) {
            mTempData.SetPropertyBit(eCSSProperty_quotes);
            mTempData.mContent.mQuotes = quotesHead;
            return PR_TRUE;
          }
          
          if (ParseVariant(open, VARIANT_STRING, nsnull)) {
            quotes->mNext = new nsCSSValuePairList();
            quotes = quotes->mNext;
            if (nsnull != quotes) {
              quotes->mXValue = open;
              continue;
            }
            mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
          }
        }
        break;
      }
      delete quotesHead;
      return PR_FALSE;
    }
    if (ExpectEndProperty()) {
      nsCSSValuePairList* quotesHead = new nsCSSValuePairList();
      quotesHead->mXValue = open;
      mTempData.mContent.mQuotes = quotesHead;
      mTempData.SetPropertyBit(eCSSProperty_quotes);
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseSize()
{
  nsCSSValue width;
  if (ParseVariant(width, VARIANT_AHKL, nsCSSProps::kPageSizeKTable)) {
    if (width.IsLengthUnit()) {
      nsCSSValue  height;
      if (ParseVariant(height, VARIANT_LENGTH, nsnull)) {
        if (ExpectEndProperty()) {
          mTempData.mPage.mSize.mXValue = width;
          mTempData.mPage.mSize.mYValue = height;
          mTempData.SetPropertyBit(eCSSProperty_size);
          return PR_TRUE;
        }
        return PR_FALSE;
      }
    }
    if (ExpectEndProperty()) {
      mTempData.mPage.mSize.SetBothValuesTo(width);
      mTempData.SetPropertyBit(eCSSProperty_size);
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseTextDecoration(nsCSSValue& aValue)
{
  if (ParseVariant(aValue, VARIANT_HOK, nsCSSProps::kTextDecorationKTable)) {
    if (eCSSUnit_Enumerated == aValue.GetUnit()) {  
      PRInt32 intValue = aValue.GetIntValue();
      nsCSSValue  keyword;
      PRInt32 index;
      for (index = 0; index < 3; index++) {
        if (ParseEnum(keyword, nsCSSProps::kTextDecorationKTable)) {
          PRInt32 newValue = keyword.GetIntValue();
          if (newValue & intValue) {
            
            return PR_FALSE;
          }
          intValue |= newValue;
        }
        else {
          break;
        }
      }
      aValue.SetIntValue(intValue, eCSSUnit_Enumerated);
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

nsCSSValueList*
CSSParserImpl::ParseCSSShadowList(PRBool aIsBoxShadow)
{
  nsAutoParseCompoundProperty compound(this);

  
  
  
  
  
  enum {
    IndexX,
    IndexY,
    IndexRadius,
    IndexSpread,
    IndexColor,
    IndexInset
  };

  nsCSSValueList *list = nsnull;
  for (nsCSSValueList **curp = &list, *cur; ; curp = &cur->mNext) {
    cur = *curp = new nsCSSValueList();
    if (!cur) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      break;
    }

    nsCSSValue isInset;
    if (aIsBoxShadow) {
      
      ParseVariant(isInset, VARIANT_KEYWORD,
                   nsCSSProps::kBoxShadowTypeKTable);
    }

    PRBool isFirstToken = (cur == list && isInset.GetUnit() == eCSSUnit_Null);
    if (!ParseVariant(cur->mValue,
                      isFirstToken ? VARIANT_HC | VARIANT_LENGTH | VARIANT_NONE
                                   : VARIANT_COLOR | VARIANT_LENGTH,
                      nsnull)) {
      break;
    }

    nsCSSUnit unit = cur->mValue.GetUnit();
    if (unit != eCSSUnit_None && unit != eCSSUnit_Inherit &&
        unit != eCSSUnit_Initial) {
      nsRefPtr<nsCSSValue::Array> val = nsCSSValue::Array::Create(6);
      if (!val) {
        mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
        break;
      }
      PRBool haveColor = PR_FALSE;
      if (cur->mValue.IsLengthUnit()) {
        val->Item(IndexX) = cur->mValue;
      } else {
        
        NS_ASSERTION(unit == eCSSUnit_Ident || unit == eCSSUnit_Color ||
                     unit == eCSSUnit_EnumColor,
                     "Must be a color value (named color, numeric color, "
                     "or system color)");
        haveColor = PR_TRUE;
        val->Item(IndexColor) = cur->mValue;

        
        if (!ParseVariant(val->Item(IndexX), VARIANT_LENGTH,
                          nsnull)) {
          break;
        }
      }
      cur->mValue.SetArrayValue(val, eCSSUnit_Array);

      
      if (!ParseVariant(val->Item(IndexY), VARIANT_LENGTH, nsnull)) {
        break;
      }

      
      
      
      
      if (ParseVariant(val->Item(IndexRadius), VARIANT_LENGTH, nsnull) &&
          val->Item(IndexRadius).GetFloatValue() < 0) {
        break;
      }

      if (aIsBoxShadow) {
        
        ParseVariant(val->Item(IndexSpread), VARIANT_LENGTH,
                     nsnull);
      }

      if (!haveColor) {
        
        ParseVariant(val->Item(IndexColor), VARIANT_COLOR,
                     nsnull);
      }

      if (aIsBoxShadow && isInset.GetUnit() == eCSSUnit_Null) {
        
        ParseVariant(val->Item(IndexInset), VARIANT_KEYWORD,
                     nsCSSProps::kBoxShadowTypeKTable);
      } else if (isInset.GetUnit() == eCSSUnit_Enumerated) {
        val->Item(IndexInset) = isInset;
      }

      
      if (ExpectSymbol(',', PR_TRUE)) {
        
        continue;
      }
    }

    if (!ExpectEndProperty()) {
      
      
      
      break;
    }

    
    
    return list;
  }
  
  delete list;
  return nsnull;
}

PRBool
CSSParserImpl::ParseTextShadow()
{
  nsCSSValueList* list = ParseCSSShadowList(PR_FALSE);
  if (!list)
    return PR_FALSE;

  mTempData.SetPropertyBit(eCSSProperty_text_shadow);
  mTempData.mText.mTextShadow = list;
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseBoxShadow()
{
  nsCSSValueList* list = ParseCSSShadowList(PR_TRUE);
  if (!list)
    return PR_FALSE;

  mTempData.SetPropertyBit(eCSSProperty_box_shadow);
  mTempData.mMargin.mBoxShadow = list;
  return PR_TRUE;
}

PRBool
CSSParserImpl::GetNamespaceIdForPrefix(const nsString& aPrefix,
                                       PRInt32* aNameSpaceID)
{
  NS_PRECONDITION(!aPrefix.IsEmpty(), "Must have a prefix here");

  PRInt32 nameSpaceID = kNameSpaceID_Unknown;
  if (mNameSpaceMap) {
    
    nsCOMPtr<nsIAtom> prefix = do_GetAtom(aPrefix);
    nameSpaceID = mNameSpaceMap->FindNameSpaceID(prefix);
  }
  

  if (kNameSpaceID_Unknown == nameSpaceID) {   
    const PRUnichar *params[] = {
      aPrefix.get()
    };
    REPORT_UNEXPECTED_P(PEUnknownNamespacePrefix, params);
    if (mUnresolvablePrefixException)
      mScanner.SetLowLevelError(NS_ERROR_DOM_NAMESPACE_ERR);
    return PR_FALSE;
  }

  *aNameSpaceID = nameSpaceID;
  return PR_TRUE;
}

void
CSSParserImpl::SetDefaultNamespaceOnSelector(nsCSSSelector& aSelector)
{
  if (mNameSpaceMap) {
    aSelector.SetNameSpace(mNameSpaceMap->FindNameSpaceID(nsnull));
  } else {
    aSelector.SetNameSpace(kNameSpaceID_Unknown); 
  }
}

#ifdef MOZ_SVG
PRBool
CSSParserImpl::ParsePaint(nsCSSValuePair* aResult,
                          nsCSSProperty aPropID)
{
  if (!ParseVariant(aResult->mXValue,
                    VARIANT_HC | VARIANT_NONE | VARIANT_URL,
                    nsnull))
    return PR_FALSE;

  if (aResult->mXValue.GetUnit() == eCSSUnit_URL) {
    if (!ParseVariant(aResult->mYValue, VARIANT_COLOR | VARIANT_NONE,
                     nsnull))
      aResult->mYValue.SetColorValue(NS_RGB(0, 0, 0));
  } else {
    aResult->mYValue = aResult->mXValue;
  }

  if (!ExpectEndProperty())
    return PR_FALSE;

  mTempData.SetPropertyBit(aPropID);
  return PR_TRUE;
}

PRBool
CSSParserImpl::ParseDasharray()
{
  nsCSSValue value;
  if (ParseVariant(value, VARIANT_HLPN | VARIANT_NONE, nsnull)) {
    nsCSSValueList *listHead = new nsCSSValueList;
    nsCSSValueList *list = listHead;
    if (!list) {
      mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
      return PR_FALSE;
    }

    list->mValue = value;

    for (;;) {
      if (CheckEndProperty()) {
        mTempData.SetPropertyBit(eCSSProperty_stroke_dasharray);
        mTempData.mSVG.mStrokeDasharray = listHead;
        return PR_TRUE;
      }

      if (eCSSUnit_Inherit == value.GetUnit() ||
          eCSSUnit_Initial == value.GetUnit() ||
          eCSSUnit_None    == value.GetUnit())
        break;

      if (!ExpectSymbol(',', PR_TRUE))
        break;

      if (!ParseVariant(value,
                        VARIANT_LENGTH | VARIANT_PERCENT | VARIANT_NUMBER,
                        nsnull))
        break;

      list->mNext = new nsCSSValueList;
      list = list->mNext;
      if (list)
        list->mValue = value;
      else {
        mScanner.SetLowLevelError(NS_ERROR_OUT_OF_MEMORY);
        break;
      }
    }
    delete listHead;
  }
  return PR_FALSE;
}

PRBool
CSSParserImpl::ParseMarker()
{
  nsCSSValue marker;
  if (ParseSingleValueProperty(marker, eCSSProperty_marker_end)) {
    if (ExpectEndProperty()) {
      AppendValue(eCSSProperty_marker_end, marker);
      AppendValue(eCSSProperty_marker_mid, marker);
      AppendValue(eCSSProperty_marker_start, marker);
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}
#endif
