




#include "nsMathMLChar.h"
#include "mozilla/MathAlgorithms.h"

#include "nsCOMPtr.h"
#include "nsIFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsUnicharUtils.h"
#include "nsRenderingContext.h"

#include "mozilla/Preferences.h"
#include "nsIPersistentProperties2.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsNetUtil.h"

#include "mozilla/LookAndFeel.h"
#include "nsCSSRendering.h"
#include "prprf.h"         

#include "nsDisplayList.h"

#include "nsMathMLOperators.h"
#include <algorithm>

using namespace mozilla;




static const nsGlyphCode kNullGlyph = {{0, 0}, 0};
typedef enum {eExtension_base, eExtension_variants, eExtension_parts}
  nsMathfontPrefExtension;

































#define NS_TABLE_TYPE_UNICODE       0
#define NS_TABLE_TYPE_GLYPH_INDEX   1

#define NS_TABLE_STATE_ERROR       -1
#define NS_TABLE_STATE_EMPTY        0
#define NS_TABLE_STATE_READY        1


static void
Clean(nsString& aValue)
{
  
  int32_t comment = aValue.RFindChar('#');
  if (comment > 0) aValue.Truncate(comment);
  aValue.CompressWhitespace();
}


static nsresult
LoadProperties(const nsString& aName,
               nsCOMPtr<nsIPersistentProperties>& aProperties)
{
  nsAutoString uriStr;
  uriStr.AssignLiteral("resource://gre/res/fonts/mathfont");
  uriStr.Append(aName);
  uriStr.StripWhitespace(); 
  uriStr.AppendLiteral(".properties");
  return NS_LoadPersistentPropertiesFromURISpec(getter_AddRefs(aProperties), 
                                                NS_ConvertUTF16toUTF8(uriStr));
}



class nsGlyphTable {
public:
  explicit nsGlyphTable(const nsString& aPrimaryFontName)
    : mFontName(1), 
      mState(NS_TABLE_STATE_EMPTY),
      mCharCache(0)
  {
    MOZ_COUNT_CTOR(nsGlyphTable);
    mFontName.AppendElement(aPrimaryFontName);
  }

  
  ~nsGlyphTable()
  {
    MOZ_COUNT_DTOR(nsGlyphTable);
  }

  const nsAString& PrimaryFontName() const
  {
    return mFontName[0];
  }

  const nsAString& FontNameFor(const nsGlyphCode& aGlyphCode) const
  {
    return mFontName[aGlyphCode.font];
  }

  
  
  bool Has(nsPresContext* aPresContext, nsMathMLChar* aChar);

  
  bool HasVariantsOf(nsPresContext* aPresContext, nsMathMLChar* aChar);

  
  bool HasPartsOf(nsPresContext* aPresContext, nsMathMLChar* aChar);

  
  nsGlyphCode TopOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 0);
  }
  nsGlyphCode MiddleOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 1);
  }
  nsGlyphCode BottomOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 2);
  }
  nsGlyphCode GlueOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 3);
  }
  nsGlyphCode BigOf(nsPresContext* aPresContext, nsMathMLChar* aChar,
                    int32_t aSize) {
    return ElementAt(aPresContext, aChar, 4 + aSize);
  }
  nsGlyphCode LeftOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 0);
  }
  nsGlyphCode RightOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 2);
  }

private:
  nsGlyphCode ElementAt(nsPresContext* aPresContext, nsMathMLChar* aChar,
                        uint32_t aPosition);

  
  
  
  nsTArray<nsString> mFontName;

  
  int32_t mState;

  
  
  nsCOMPtr<nsIPersistentProperties> mGlyphProperties;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsString  mGlyphCache;
  char16_t mCharCache;
};

nsGlyphCode
nsGlyphTable::ElementAt(nsPresContext* aPresContext, nsMathMLChar* aChar,
                        uint32_t aPosition)
{
  if (mState == NS_TABLE_STATE_ERROR) return kNullGlyph;
  
  if (mState == NS_TABLE_STATE_EMPTY) {
    nsresult rv = LoadProperties(mFontName[0], mGlyphProperties);
#ifdef DEBUG
    nsAutoCString uriStr;
    uriStr.AssignLiteral("resource://gre/res/fonts/mathfont");
    LossyAppendUTF16toASCII(mFontName[0], uriStr);
    uriStr.StripWhitespace(); 
    uriStr.AppendLiteral(".properties");
    printf("Loading %s ... %s\n",
            uriStr.get(),
            (NS_FAILED(rv)) ? "Failed" : "Done");
#endif
    if (NS_FAILED(rv)) {
      mState = NS_TABLE_STATE_ERROR; 
      return kNullGlyph;
    }
    mState = NS_TABLE_STATE_READY;

    
    nsAutoCString key;
    nsAutoString value;
    for (int32_t i = 1; ; i++) {
      key.AssignLiteral("external.");
      key.AppendInt(i, 10);
      rv = mGlyphProperties->GetStringProperty(key, value);
      if (NS_FAILED(rv)) break;
      Clean(value);
      mFontName.AppendElement(value); 
    }
  }

  
  char16_t uchar = aChar->mData[0];
  if (mCharCache != uchar) {
    
    
    char key[10]; PR_snprintf(key, sizeof(key), "\\u%04X", uchar);
    nsAutoString value;
    nsresult rv = mGlyphProperties->GetStringProperty(nsDependentCString(key),
                                                      value);
    if (NS_FAILED(rv)) return kNullGlyph;
    Clean(value);
    
    
    
    
    
    
    
    
    
    nsAutoString buffer;
    int32_t length = value.Length();
    int32_t i = 0; 
    while (i < length) {
      char16_t code = value[i];
      ++i;
      buffer.Append(code);
      
      if (i < length && NS_IS_HIGH_SURROGATE(code)) {
        code = value[i];
        ++i;
      } else {
        code = char16_t('\0');
      }
      buffer.Append(code);

      
      
      char16_t font = 0;
      if (i+1 < length && value[i] == char16_t('@') &&
          value[i+1] >= char16_t('0') && value[i+1] <= char16_t('9')) {
        ++i;
        font = value[i] - '0';
        ++i;
        if (font >= mFontName.Length()) {
          NS_ERROR("Nonexistent font referenced in glyph table");
          return kNullGlyph;
        }
        
        if (!mFontName[font].Length()) {
          return kNullGlyph;
        }
      }
      buffer.Append(font);
    }
    
    mGlyphCache.Assign(buffer);
    mCharCache = uchar;
  }

  
  uint32_t index = 3*aPosition;
  if (index+2 >= mGlyphCache.Length()) return kNullGlyph;
  nsGlyphCode ch;
  ch.code[0] = mGlyphCache.CharAt(index);
  ch.code[1] = mGlyphCache.CharAt(index + 1);
  ch.font = mGlyphCache.CharAt(index + 2);
  return ch.code[0] == char16_t(0xFFFD) ? kNullGlyph : ch;
}

bool
nsGlyphTable::Has(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  return HasVariantsOf(aPresContext, aChar) || HasPartsOf(aPresContext, aChar);
}

bool
nsGlyphTable::HasVariantsOf(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  
  return BigOf(aPresContext, aChar, 1).Exists();
}

bool
nsGlyphTable::HasPartsOf(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  return GlueOf(aPresContext, aChar).Exists() ||
    TopOf(aPresContext, aChar).Exists() ||
    BottomOf(aPresContext, aChar).Exists() ||
    MiddleOf(aPresContext, aChar).Exists();
}








class nsGlyphTableList : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsGlyphTable mUnicodeTable;

  nsGlyphTableList()
    : mUnicodeTable(NS_LITERAL_STRING("Unicode"))
  {
    MOZ_COUNT_CTOR(nsGlyphTableList);
  }

  virtual ~nsGlyphTableList()
  {
    MOZ_COUNT_DTOR(nsGlyphTableList);
  }

  nsresult Initialize();
  nsresult Finalize();

  
  nsGlyphTable*
  AddGlyphTable(const nsString& aPrimaryFontName);

  
  nsGlyphTable*
  GetGlyphTableFor(nsPresContext* aPresContext,
                   nsMathMLChar*  aChar);

  
  nsGlyphTable*
  GetGlyphTableFor(const nsAString& aFamily);

private:
  nsGlyphTable* TableAt(int32_t aIndex) {
    return &mTableList.ElementAt(aIndex);
  }
  int32_t Count() {
    return mTableList.Length();
  }

  
  nsTArray<nsGlyphTable> mTableList;
};

NS_IMPL_ISUPPORTS1(nsGlyphTableList, nsIObserver)



static nsGlyphTableList* gGlyphTableList = nullptr;

static bool gGlyphTableInitialized = false;


NS_IMETHODIMP
nsGlyphTableList::Observe(nsISupports*     aSubject,
                          const char* aTopic,
                          const char16_t* someData)
{
  Finalize();
  return NS_OK;
}


nsresult
nsGlyphTableList::Initialize()
{
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs)
    return NS_ERROR_FAILURE;

  nsresult rv = obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsGlyphTableList::Finalize()
{
  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs)
    rv = obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
  else
    rv = NS_ERROR_FAILURE;

  gGlyphTableInitialized = false;
  
  return rv;
}

nsGlyphTable*
nsGlyphTableList::AddGlyphTable(const nsString& aPrimaryFontName)
{
  
  nsGlyphTable* glyphTable = GetGlyphTableFor(aPrimaryFontName);
  if (glyphTable != &mUnicodeTable)
    return glyphTable;

  
  glyphTable = mTableList.AppendElement(aPrimaryFontName);
  return glyphTable;
}

nsGlyphTable*
nsGlyphTableList::GetGlyphTableFor(nsPresContext* aPresContext, 
                                   nsMathMLChar*   aChar)
{
  if (mUnicodeTable.Has(aPresContext, aChar))
    return &mUnicodeTable;

  int32_t i;
  for (i = 0; i < Count(); i++) {
    nsGlyphTable* glyphTable = TableAt(i);
    if (glyphTable->Has(aPresContext, aChar)) {
      return glyphTable;
    }
  }
  return nullptr;
}

nsGlyphTable*
nsGlyphTableList::GetGlyphTableFor(const nsAString& aFamily)
{
  for (int32_t i = 0; i < Count(); i++) {
    nsGlyphTable* glyphTable = TableAt(i);
    const nsAString& fontName = glyphTable->PrimaryFontName();
    
    if (fontName.Equals(aFamily, nsCaseInsensitiveStringComparator())) {
      return glyphTable;
    }
  }
  
  return &mUnicodeTable;
}









static bool
GetFontExtensionPref(char16_t aChar,
                     nsMathfontPrefExtension aExtension, nsString& aValue)
{
  
  aValue.Truncate();

  
  
  
  
  
  
  
  
  

  static const char* kMathFontPrefix = "font.mathfont-family.";

  nsAutoCString extension;
  switch (aExtension)
  {
    case eExtension_base:
      extension.AssignLiteral(".base");
      break;
    case eExtension_variants:
      extension.AssignLiteral(".variants");
      break;
    case eExtension_parts:
      extension.AssignLiteral(".parts");
      break;
    default:
      return false;
  }

  
  nsAutoCString key;
  key.AssignASCII(kMathFontPrefix);
  char ustr[10];
  PR_snprintf(ustr, sizeof(ustr), "\\u%04X", aChar);
  key.Append(ustr);
  key.Append(extension);
  
  nsAutoCString alternateKey;
  alternateKey.AssignASCII(kMathFontPrefix);
  NS_ConvertUTF16toUTF8 tmp(&aChar, 1);
  alternateKey.Append(tmp);
  alternateKey.Append(extension);

  aValue = Preferences::GetString(key.get());
  if (aValue.IsEmpty()) {
    aValue = Preferences::GetString(alternateKey.get());
  }
  return !aValue.IsEmpty();
}


static bool
MathFontEnumCallback(const nsString& aFamily, bool aGeneric, void *aData)
{
  if (!gGlyphTableList->AddGlyphTable(aFamily))
    return false; 
  return true; 
}

static nsresult
InitGlobals(nsPresContext* aPresContext)
{
  NS_ASSERTION(!gGlyphTableInitialized, "Error -- already initialized");
  gGlyphTableInitialized = true;

  
  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  gGlyphTableList = new nsGlyphTableList();
  if (gGlyphTableList) {
    rv = gGlyphTableList->Initialize();
  }
  if (NS_FAILED(rv)) {
    delete gGlyphTableList;
    gGlyphTableList = nullptr;
    return rv;
  }
  





  nsAutoCString key;
  nsAutoString value;
  nsCOMPtr<nsIPersistentProperties> mathfontProp;

  
  
  

  
  value.Truncate();
  rv = LoadProperties(value, mathfontProp);
  if (NS_FAILED(rv)) return rv;

  
  
  
  
  nsFont font("", 0, 0, 0, 0, 0, 0);
  NS_NAMED_LITERAL_CSTRING(defaultKey, "font.mathfont-glyph-tables");
  rv = mathfontProp->GetStringProperty(defaultKey, font.name);
  if (NS_FAILED(rv)) return rv;

  
  nsAutoString missingFamilyList;

  font.EnumerateFamilies(MathFontEnumCallback, nullptr);
  return rv;
}




nsMathMLChar::~nsMathMLChar()
{
  MOZ_COUNT_DTOR(nsMathMLChar);
  mStyleContext->Release();
}

nsStyleContext*
nsMathMLChar::GetStyleContext() const
{
  NS_ASSERTION(mStyleContext, "chars should always have style context");
  return mStyleContext;
}

void
nsMathMLChar::SetStyleContext(nsStyleContext* aStyleContext)
{
  NS_PRECONDITION(aStyleContext, "null ptr");
  if (aStyleContext != mStyleContext) {
    if (mStyleContext)
      mStyleContext->Release();
    if (aStyleContext) {
      mStyleContext = aStyleContext;
      aStyleContext->AddRef();
    }
  }
}

void
nsMathMLChar::SetData(nsPresContext* aPresContext,
                      nsString&       aData)
{
  if (!gGlyphTableInitialized) {
    InitGlobals(aPresContext);
  }
  mData = aData;
  
  
  mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
  mBoundingMetrics = nsBoundingMetrics();
  mGlyphTable = nullptr;
  
  if (gGlyphTableList && (1 == mData.Length())) {
    mDirection = nsMathMLOperators::GetStretchyDirection(mData);
    
    
    mGlyphTable = gGlyphTableList->GetGlyphTableFor(aPresContext, this);
  }
}
















































































#define NS_MATHML_DELIMITER_FACTOR             0.901f
#define NS_MATHML_DELIMITER_SHORTFALL_POINTS   5.0f

static bool
IsSizeOK(nsPresContext* aPresContext, nscoord a, nscoord b, uint32_t aHint)
{
  
  
  
  
  bool isNormal =
    (aHint & NS_STRETCH_NORMAL) &&
    Abs<float>(a - b) < (1.0f - NS_MATHML_DELIMITER_FACTOR) * float(b);

  
  
  
  bool isNearer = false;
  if (aHint & (NS_STRETCH_NEARER | NS_STRETCH_LARGEOP)) {
    float c = std::max(float(b) * NS_MATHML_DELIMITER_FACTOR,
                     float(b) - nsPresContext::
                     CSSPointsToAppUnits(NS_MATHML_DELIMITER_SHORTFALL_POINTS));
    isNearer = Abs<float>(b - a) <= float(b) - c;
  }

  
  
  bool isSmaller =
    (aHint & NS_STRETCH_SMALLER) &&
    float(a) >= NS_MATHML_DELIMITER_FACTOR * float(b) &&
    a <= b;

  
  
  bool isLarger =
    (aHint & (NS_STRETCH_LARGER | NS_STRETCH_LARGEOP)) &&
    a >= b;

  return (isNormal || isSmaller || isNearer || isLarger);
}

static bool
IsSizeBetter(nscoord a, nscoord olda, nscoord b, uint32_t aHint)
{
  if (0 == olda)
    return true;
  if (aHint & (NS_STRETCH_LARGER | NS_STRETCH_LARGEOP))
    return (a >= olda) ? (olda < b) : (a >= b);
  if (aHint & NS_STRETCH_SMALLER)
    return (a <= olda) ? (olda > b) : (a <= b);

  
  return Abs(a - b) < Abs(olda - b);
}





static nscoord
ComputeSizeFromParts(nsPresContext* aPresContext,
                     nsGlyphCode* aGlyphs,
                     nscoord*     aSizes,
                     nscoord      aTargetSize)
{
  enum {first, middle, last, glue};
  
  nscoord sum = 0;
  for (int32_t i = first; i <= last; i++) {
    if (aGlyphs[i] != aGlyphs[glue]) {
      sum += aSizes[i];
    }
  }

  
  nscoord oneDevPixel = aPresContext->AppUnitsPerDevPixel();
  int32_t joins = aGlyphs[middle] == aGlyphs[glue] ? 1 : 2;

  
  
  const int32_t maxGlyphs = 1000;

  
  
  nscoord maxSize = sum - 2 * joins * oneDevPixel + maxGlyphs * aSizes[glue];
  if (maxSize < aTargetSize)
    return maxSize; 

  
  nscoord minSize = NSToCoordRound(NS_MATHML_DELIMITER_FACTOR * sum);

  if (minSize > aTargetSize)
    return minSize; 

  
  return aTargetSize;
}



static void
AddFallbackFonts(nsAString& aFontName, const nsAString& aFallbackFamilies)
{
  if (aFallbackFamilies.IsEmpty())
    return;

  if (aFontName.IsEmpty()) {
    return;
  }

  static const char16_t kSingleQuote  = char16_t('\'');
  static const char16_t kDoubleQuote  = char16_t('\"');
  static const char16_t kComma        = char16_t(',');

  const char16_t *p_begin, *p_end;
  aFontName.BeginReading(p_begin);
  aFontName.EndReading(p_end);

  const char16_t *p = p_begin;
  const char16_t *p_name = nullptr;
  while (p < p_end) {
    while (nsCRT::IsAsciiSpace(*p))
      if (++p == p_end)
        goto insert;

    p_name = p;
    if (*p == kSingleQuote || *p == kDoubleQuote) {
      
      char16_t quoteMark = *p;
      if (++p == p_end)
        goto insert;

      
      while (*p != quoteMark)
        if (++p == p_end)
          goto insert;

      while (++p != p_end && *p != kComma)
         ;

    } else {
      
      const char16_t *nameStart = p;
      while (++p != p_end && *p != kComma)
         ;

      nsAutoString family;
      family = Substring(nameStart, p);
      family.CompressWhitespace(false, true);

      uint8_t generic;
      nsFont::GetGenericID(family, &generic);
      if (generic != kGenericFont_NONE)
        goto insert;
    }

    ++p; 
  }

  aFontName.Append(NS_LITERAL_STRING(",") + aFallbackFamilies);
  return;

insert:
  if (p_name) {
    aFontName.Insert(aFallbackFamilies + NS_LITERAL_STRING(","),
                     p_name - p_begin);
  }
  else { 
    aFontName = aFallbackFamilies;
  }
}


static bool
SetFontFamily(nsStyleContext*      aStyleContext,
              nsRenderingContext&  aRenderingContext,
              nsFont&              aFont,
              const nsGlyphTable*  aGlyphTable,
              const nsGlyphCode&   aGlyphCode,
              const nsAString&     aDefaultFamily)
{
  const nsAString& family =
    aGlyphCode.font ? aGlyphTable->FontNameFor(aGlyphCode) : aDefaultFamily;
  if (! family.Equals(aFont.name)) {
    nsFont font = aFont;
    font.name = family;
    nsRefPtr<nsFontMetrics> fm;
    aRenderingContext.DeviceContext()->GetMetricsFor(font,
      aStyleContext->StyleFont()->mLanguage,
      aStyleContext->PresContext()->GetUserFontSet(),
      aStyleContext->PresContext()->GetTextPerfMetrics(),
      *getter_AddRefs(fm));
    
    
    if (aGlyphTable == &gGlyphTableList->mUnicodeTable ||
        fm->GetThebesFontGroup()->GetFontAt(0)->GetFontEntry()->
        FamilyName() == family) {
      aFont.name = family;
      aRenderingContext.SetFont(fm);
    } else {
      return false; 
    }
  }
  return true;
}

class nsMathMLChar::StretchEnumContext {
public:
  StretchEnumContext(nsMathMLChar*        aChar,
                     nsPresContext*       aPresContext,
                     nsRenderingContext& aRenderingContext,
                     nsStretchDirection   aStretchDirection,
                     nscoord              aTargetSize,
                     uint32_t             aStretchHint,
                     nsBoundingMetrics&   aStretchedMetrics,
                     const nsAString&     aFamilies,
                     bool&              aGlyphFound)
    : mChar(aChar),
      mPresContext(aPresContext),
      mRenderingContext(aRenderingContext),
      mDirection(aStretchDirection),
      mTargetSize(aTargetSize),
      mStretchHint(aStretchHint),
      mBoundingMetrics(aStretchedMetrics),
      mFamilies(aFamilies),
      mTryVariants(true),
      mTryParts(true),
      mGlyphFound(aGlyphFound) {}

  static bool
  EnumCallback(const nsString& aFamily, bool aGeneric, void *aData);

private:
  bool TryVariants(nsGlyphTable* aGlyphTable, const nsAString& aFamily);
  bool TryParts(nsGlyphTable* aGlyphTable, const nsAString& aFamily);

  nsMathMLChar* mChar;
  nsPresContext* mPresContext;
  nsRenderingContext& mRenderingContext;
  const nsStretchDirection mDirection;
  const nscoord mTargetSize;
  const uint32_t mStretchHint;
  nsBoundingMetrics& mBoundingMetrics;
  
  const nsAString& mFamilies;

public:
  bool mTryVariants;
  bool mTryParts;

private:
  nsAutoTArray<nsGlyphTable*,16> mTablesTried;
  nsGlyphTable* mGlyphTable; 
  bool&       mGlyphFound;
};





bool
nsMathMLChar::StretchEnumContext::TryVariants(nsGlyphTable*    aGlyphTable,
                                              const nsAString& aFamily)
{
  
  nsStyleContext *sc = mChar->mStyleContext;
  nsFont font = sc->StyleFont()->mFont;
  
  font.name.Truncate();

  bool isVertical = (mDirection == NS_STRETCH_DIRECTION_VERTICAL);
  bool largeop = (NS_STRETCH_LARGEOP & mStretchHint) != 0;
  bool largeopOnly =
    largeop && (NS_STRETCH_VARIABLE_MASK & mStretchHint) == 0;
  bool maxWidth = (NS_STRETCH_MAXWIDTH & mStretchHint) != 0;

  nscoord bestSize =
    isVertical ? mBoundingMetrics.ascent + mBoundingMetrics.descent
               : mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing;
  bool haveBetter = false;

  
  int32_t size = 1;
#ifdef NOISY_SEARCH
  printf("  searching in %s ...\n",
           NS_LossyConvertUTF16toASCII(aFamily).get());
#endif

  nsGlyphCode ch;
  while ((ch = aGlyphTable->BigOf(mPresContext, mChar, size)).Exists()) {

    if(!SetFontFamily(sc, mRenderingContext, font, aGlyphTable, ch, aFamily)) {
      
      if (largeopOnly) break;
      ++size;
      continue;
    }

    NS_ASSERTION(maxWidth || ch.code[0] != mChar->mGlyph.code[0] ||
                 ch.code[1] != mChar->mGlyph.code[1] ||
                 !font.name.Equals(mChar->mFamily),
                 "glyph table incorrectly set -- duplicate found");

    nsBoundingMetrics bm = mRenderingContext.GetBoundingMetrics(ch.code,
                                                                ch.Length());
    nscoord charSize =
      isVertical ? bm.ascent + bm.descent
      : bm.rightBearing - bm.leftBearing;

    if (largeopOnly ||
        IsSizeBetter(charSize, bestSize, mTargetSize, mStretchHint)) {
      mGlyphFound = true;
      if (maxWidth) {
        
        
        if (mBoundingMetrics.width < bm.width)
          mBoundingMetrics.width = bm.width;
        if (mBoundingMetrics.leftBearing > bm.leftBearing)
          mBoundingMetrics.leftBearing = bm.leftBearing;
        if (mBoundingMetrics.rightBearing < bm.rightBearing)
          mBoundingMetrics.rightBearing = bm.rightBearing;
        
        haveBetter = largeopOnly;
      }
      else {
        mBoundingMetrics = bm;
        haveBetter = true;
        bestSize = charSize;
        mChar->mGlyphTable = aGlyphTable;
        mChar->mGlyph = ch;
        mChar->mFamily = font.name;
      }
#ifdef NOISY_SEARCH
      printf("    size:%d Current best\n", size);
#endif
    }
    else {
#ifdef NOISY_SEARCH
      printf("    size:%d Rejected!\n", size);
#endif
      if (haveBetter)
        break; 
    }

    
    if (largeopOnly) break;
    ++size;
  }

  return haveBetter &&
    (largeopOnly ||
     IsSizeOK(mPresContext, bestSize, mTargetSize, mStretchHint));
}




bool
nsMathMLChar::StretchEnumContext::TryParts(nsGlyphTable*    aGlyphTable,
                                           const nsAString& aFamily)
{
  if (!aGlyphTable->HasPartsOf(mPresContext, mChar))
    return false; 

  

  
  nsFont font = mChar->mStyleContext->StyleFont()->mFont;
  
  font.name.Truncate();

  
  nsGlyphCode chdata[4];
  nsBoundingMetrics bmdata[4];
  nscoord sizedata[4];
  nsGlyphCode glue = aGlyphTable->GlueOf(mPresContext, mChar);

  bool isVertical = (mDirection == NS_STRETCH_DIRECTION_VERTICAL);
  bool maxWidth = (NS_STRETCH_MAXWIDTH & mStretchHint) != 0;

  for (int32_t i = 0; i < 4; i++) {
    nsGlyphCode ch;
    switch (i) {
    case 0: ch = aGlyphTable->TopOf(mPresContext, mChar);    break;
    case 1: ch = aGlyphTable->MiddleOf(mPresContext, mChar); break;
    case 2: ch = aGlyphTable->BottomOf(mPresContext, mChar); break;
    case 3: ch = glue;                                       break;
    }
    
    if (!ch.Exists()) ch = glue;
    chdata[i] = ch;
    if (!ch.Exists()) {
      
      
      sizedata[i] = mTargetSize;
    }
    else {
      if (!SetFontFamily(mChar->mStyleContext, mRenderingContext,
                         font, aGlyphTable, ch, aFamily))
        return false;

      nsBoundingMetrics bm = mRenderingContext.GetBoundingMetrics(ch.code,
                                                                  ch.Length());

      
      
      
      bmdata[i] = bm;
      sizedata[i] = isVertical ? bm.ascent + bm.descent
                               : bm.rightBearing - bm.leftBearing;
    }
  }

  
  
  nscoord computedSize = ComputeSizeFromParts(mPresContext, chdata, sizedata,
                                              mTargetSize);

  nscoord currentSize =
    isVertical ? mBoundingMetrics.ascent + mBoundingMetrics.descent
               : mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing;

  if (!IsSizeBetter(computedSize, currentSize, mTargetSize, mStretchHint)) {
#ifdef NOISY_SEARCH
    printf("    Font %s Rejected!\n",
           NS_LossyConvertUTF16toASCII(fontName).get());
#endif
    return false; 
  }

#ifdef NOISY_SEARCH
  printf("    Font %s Current best!\n",
         NS_LossyConvertUTF16toASCII(fontName).get());
#endif

  
  
  if (isVertical) {
    int32_t i;
    nscoord lbearing;
    nscoord rbearing;
    nscoord width;
    if (maxWidth) {
      lbearing = mBoundingMetrics.leftBearing;
      rbearing = mBoundingMetrics.rightBearing;
      width = mBoundingMetrics.width;
      i = 0;
    }
    else {
      lbearing = bmdata[0].leftBearing;
      rbearing = bmdata[0].rightBearing;
      width = bmdata[0].width;
      i = 1;
    }
    for (; i < 4; i++) {
      const nsBoundingMetrics& bm = bmdata[i];
      if (width < bm.width) width = bm.width;
      if (lbearing > bm.leftBearing) lbearing = bm.leftBearing;
      if (rbearing < bm.rightBearing) rbearing = bm.rightBearing;
    }
    mBoundingMetrics.width = width;
    
    
    
    mBoundingMetrics.ascent = bmdata[0].ascent; 
                                                
    mBoundingMetrics.descent = computedSize - mBoundingMetrics.ascent;
    mBoundingMetrics.leftBearing = lbearing;
    mBoundingMetrics.rightBearing = rbearing;
  }
  else {
    nscoord ascent = bmdata[0].ascent;
    nscoord descent = bmdata[0].descent;
    for (int32_t i = 1; i < 4; i++) {
      const nsBoundingMetrics& bm = bmdata[i];
      if (ascent < bm.ascent) ascent = bm.ascent;
      if (descent < bm.descent) descent = bm.descent;
    }
    mBoundingMetrics.width = computedSize;
    mBoundingMetrics.ascent = ascent;
    mBoundingMetrics.descent = descent;
    mBoundingMetrics.leftBearing = 0;
    mBoundingMetrics.rightBearing = computedSize;
  }
  mGlyphFound = true;
  if (maxWidth)
    return false; 

  
  mChar->mGlyph = kNullGlyph; 
  mChar->mGlyphTable = aGlyphTable;
  mChar->mFamily = aFamily;

  return IsSizeOK(mPresContext, computedSize, mTargetSize, mStretchHint);
}


bool
nsMathMLChar::StretchEnumContext::EnumCallback(const nsString& aFamily,
                                               bool aGeneric, void *aData)
{
  StretchEnumContext* context = static_cast<StretchEnumContext*>(aData);

  
  
  nsGlyphTable* glyphTable = aGeneric ?
    &gGlyphTableList->mUnicodeTable :
    gGlyphTableList->GetGlyphTableFor(aFamily);

  if (context->mTablesTried.Contains(glyphTable))
    return true; 

  
  
  nsStyleContext *sc = context->mChar->mStyleContext;
  nsFont font = sc->StyleFont()->mFont;
  if (!aGeneric && !SetFontFamily(sc, context->mRenderingContext,
                                  font, nullptr, kNullGlyph, aFamily))
     return true; 

  context->mGlyphTable = glyphTable;

  

  
  context->mTablesTried.AppendElement(glyphTable);

  
  
  
  const nsAString& family = glyphTable == &gGlyphTableList->mUnicodeTable ?
    context->mFamilies : aFamily;

  if((context->mTryVariants && context->TryVariants(glyphTable, family)) ||
     (context->mTryParts && context->TryParts(glyphTable, family)))
    return false; 

  return true; 
}

nsresult
nsMathMLChar::StretchInternal(nsPresContext*           aPresContext,
                              nsRenderingContext&     aRenderingContext,
                              nsStretchDirection&      aStretchDirection,
                              const nsBoundingMetrics& aContainerSize,
                              nsBoundingMetrics&       aDesiredStretchSize,
                              uint32_t                 aStretchHint,
                              
                              
                              float                    aMaxSize,
                              bool                     aMaxSizeIsAbsolute)
{
  
  
  
  nsStretchDirection direction = nsMathMLOperators::GetStretchyDirection(mData);

  
  
  
  nsFont font = mStyleContext->GetParent()->StyleFont()->mFont;

  
  nsAutoString families;
  if (GetFontExtensionPref(mData[0], eExtension_base, families)) {
    font.name = families;
  }

  
  bool maxWidth = (NS_STRETCH_MAXWIDTH & aStretchHint) != 0;
  if (!maxWidth) {
    
    
    mFamily = families;
  }

  nsRefPtr<nsFontMetrics> fm;
  aRenderingContext.DeviceContext()->GetMetricsFor(font,
    mStyleContext->StyleFont()->mLanguage,
    aPresContext->GetUserFontSet(),
    aPresContext->GetTextPerfMetrics(), *getter_AddRefs(fm));
  aRenderingContext.SetFont(fm);
  aDesiredStretchSize =
    aRenderingContext.GetBoundingMetrics(mData.get(), uint32_t(mData.Length()));

  if (!maxWidth) {
    mUnscaledAscent = aDesiredStretchSize.ascent;
  }

  
  
  

  
  if ((aStretchDirection != direction &&
       aStretchDirection != NS_STRETCH_DIRECTION_DEFAULT) ||
      (aStretchHint & ~NS_STRETCH_MAXWIDTH) == NS_STRETCH_NONE) {
    mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
    return NS_OK;
  }

  
  if (aStretchDirection == NS_STRETCH_DIRECTION_DEFAULT) {
    aStretchDirection = direction;
  }

  
  bool largeop = (NS_STRETCH_LARGEOP & aStretchHint) != 0;
  bool stretchy = (NS_STRETCH_VARIABLE_MASK & aStretchHint) != 0;
  bool largeopOnly = largeop && !stretchy;

  bool isVertical = (direction == NS_STRETCH_DIRECTION_VERTICAL);

  nscoord targetSize =
    isVertical ? aContainerSize.ascent + aContainerSize.descent
    : aContainerSize.rightBearing - aContainerSize.leftBearing;

  if (maxWidth) {
    
    
    
    
    if (stretchy) {
      
      aStretchHint =
        (aStretchHint & ~NS_STRETCH_VARIABLE_MASK) | NS_STRETCH_SMALLER;
    }

    
    
    if (aMaxSize == NS_MATHML_OPERATOR_SIZE_INFINITY) {
      aDesiredStretchSize.ascent = nscoord_MAX;
      aDesiredStretchSize.descent = 0;
    }
    else {
      nscoord height = aDesiredStretchSize.ascent + aDesiredStretchSize.descent;
      if (height == 0) {
        if (aMaxSizeIsAbsolute) {
          aDesiredStretchSize.ascent =
            NSToCoordRound(aMaxSize / NS_MATHML_DELIMITER_FACTOR);
          aDesiredStretchSize.descent = 0;
        }
        
      }
      else {
        float scale = aMaxSizeIsAbsolute ? aMaxSize / height : aMaxSize;
        scale /= NS_MATHML_DELIMITER_FACTOR;
        aDesiredStretchSize.ascent =
          NSToCoordRound(scale * aDesiredStretchSize.ascent);
        aDesiredStretchSize.descent =
          NSToCoordRound(scale * aDesiredStretchSize.descent);
      }
    }
  }

  nsBoundingMetrics initialSize = aDesiredStretchSize;
  nscoord charSize =
    isVertical ? initialSize.ascent + initialSize.descent
    : initialSize.rightBearing - initialSize.leftBearing;

  bool done = (mGlyphTable ? false : true);

  if (!done && !maxWidth && !largeop) {
    
    
    if ((targetSize <= 0) || 
        ((isVertical && charSize >= targetSize) ||
         IsSizeOK(aPresContext, charSize, targetSize, aStretchHint)))
      done = true;
  }

  
  
  

  bool glyphFound = false;
  nsAutoString cssFamilies;

  if (!done) {
    font = mStyleContext->StyleFont()->mFont;
    cssFamilies = font.name;
  }

  
  if (!done && GetFontExtensionPref(mData[0], eExtension_variants, families)) {
    font.name = families;

    StretchEnumContext enumData(this, aPresContext, aRenderingContext,
                                aStretchDirection, targetSize, aStretchHint,
                                aDesiredStretchSize, font.name, glyphFound);
    enumData.mTryParts = false;

    done = !font.EnumerateFamilies(StretchEnumContext::EnumCallback, &enumData);
  }

  
  if (!done && !largeopOnly
      && GetFontExtensionPref(mData[0], eExtension_parts, families)) {
    font.name = families;

    StretchEnumContext enumData(this, aPresContext, aRenderingContext,
                                aStretchDirection, targetSize, aStretchHint,
                                aDesiredStretchSize, font.name, glyphFound);
    enumData.mTryVariants = false;

    done = !font.EnumerateFamilies(StretchEnumContext::EnumCallback, &enumData);
  }

  if (!done) { 
    
    font.name = cssFamilies;
    NS_NAMED_LITERAL_CSTRING(defaultKey, "font.mathfont-family");
    nsAdoptingString fallbackFonts = Preferences::GetString(defaultKey.get());
    if (!fallbackFonts.IsEmpty()) {
      AddFallbackFonts(font.name, fallbackFonts);
    }

#ifdef NOISY_SEARCH
    printf("Searching in "%s" for a glyph of appropriate size for: 0x%04X:%c\n",
           font.name, mData[0], mData[0]&0x00FF);
#endif
    StretchEnumContext enumData(this, aPresContext, aRenderingContext,
                                aStretchDirection, targetSize, aStretchHint,
                                aDesiredStretchSize, font.name, glyphFound);
    enumData.mTryParts = !largeopOnly;

    font.EnumerateFamilies(StretchEnumContext::EnumCallback, &enumData);
  }

  if (!maxWidth) {
    
    
    mDrawNormal = !glyphFound;
    mUnscaledAscent = aDesiredStretchSize.ascent;
  }
    
  
  if (stretchy) {
    if (isVertical) {
      float scale =
        float(aContainerSize.ascent + aContainerSize.descent) /
        (aDesiredStretchSize.ascent + aDesiredStretchSize.descent);
      if (!largeop || scale > 1.0) {
        
        if (!maxWidth) {
          mScaleY *= scale;
        }
        aDesiredStretchSize.ascent *= scale;
        aDesiredStretchSize.descent *= scale;
      }
    } else {
      float scale =
        float(aContainerSize.rightBearing - aContainerSize.leftBearing) /
        (aDesiredStretchSize.rightBearing - aDesiredStretchSize.leftBearing);
      if (!largeop || scale > 1.0) {
        
        if (!maxWidth) {
          mScaleX *= scale;
        }
        aDesiredStretchSize.leftBearing *= scale;
        aDesiredStretchSize.rightBearing *= scale;
        aDesiredStretchSize.width *= scale;
      }
    }
  }

  
  
  if (!glyphFound && largeop) {
    float scale;
    float largeopFactor = float(M_SQRT2);

    
    
    if ((aDesiredStretchSize.rightBearing - aDesiredStretchSize.leftBearing) <
        largeopFactor * (initialSize.rightBearing - initialSize.leftBearing)) {
      scale = (largeopFactor *
               (initialSize.rightBearing - initialSize.leftBearing)) /
        (aDesiredStretchSize.rightBearing - aDesiredStretchSize.leftBearing);
      if (!maxWidth) {
        mScaleX *= scale;
      }
      aDesiredStretchSize.leftBearing *= scale;
      aDesiredStretchSize.rightBearing *= scale;
      aDesiredStretchSize.width *= scale;
    }

    
    
    if (NS_STRETCH_INTEGRAL & aStretchHint) {
      
      largeopFactor = 2.0;
    }
    if ((aDesiredStretchSize.ascent + aDesiredStretchSize.descent) <
        largeopFactor * (initialSize.ascent + initialSize.descent)) {
      scale = (largeopFactor *
               (initialSize.ascent + initialSize.descent)) /
        (aDesiredStretchSize.ascent + aDesiredStretchSize.descent);
      if (!maxWidth) {
        mScaleY *= scale;
      }
      aDesiredStretchSize.ascent *= scale;
      aDesiredStretchSize.descent *= scale;
    }
  }

  return NS_OK;
}

nsresult
nsMathMLChar::Stretch(nsPresContext*           aPresContext,
                      nsRenderingContext&     aRenderingContext,
                      nsStretchDirection       aStretchDirection,
                      const nsBoundingMetrics& aContainerSize,
                      nsBoundingMetrics&       aDesiredStretchSize,
                      uint32_t                 aStretchHint,
                      bool                     aRTL)
{
  NS_ASSERTION(!(aStretchHint &
                 ~(NS_STRETCH_VARIABLE_MASK | NS_STRETCH_LARGEOP |
                   NS_STRETCH_INTEGRAL)),
               "Unexpected stretch flags");

  mDrawNormal = true;
  mMirrored = aRTL && nsMathMLOperators::IsMirrorableOperator(mData);
  mScaleY = mScaleX = 1.0;
  mDirection = aStretchDirection;
  nsresult rv =
    StretchInternal(aPresContext, aRenderingContext, mDirection,
                    aContainerSize, aDesiredStretchSize, aStretchHint);

  
  mBoundingMetrics = aDesiredStretchSize;

  return rv;
}













nscoord
nsMathMLChar::GetMaxWidth(nsPresContext* aPresContext,
                          nsRenderingContext& aRenderingContext,
                          uint32_t aStretchHint,
                          float aMaxSize, bool aMaxSizeIsAbsolute)
{
  nsBoundingMetrics bm;
  nsStretchDirection direction = NS_STRETCH_DIRECTION_VERTICAL;
  const nsBoundingMetrics container; 

  StretchInternal(aPresContext, aRenderingContext, direction, container,
                  bm, aStretchHint | NS_STRETCH_MAXWIDTH);

  return std::max(bm.width, bm.rightBearing) - std::min(0, bm.leftBearing);
}

class nsDisplayMathMLSelectionRect : public nsDisplayItem {
public:
  nsDisplayMathMLSelectionRect(nsDisplayListBuilder* aBuilder,
                               nsIFrame* aFrame, const nsRect& aRect)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLSelectionRect);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLSelectionRect() {
    MOZ_COUNT_DTOR(nsDisplayMathMLSelectionRect);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLSelectionRect", TYPE_MATHML_SELECTION_RECT)
private:
  nsRect    mRect;
};

void nsDisplayMathMLSelectionRect::Paint(nsDisplayListBuilder* aBuilder,
                                         nsRenderingContext* aCtx)
{
  
  nscolor bgColor =
    LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectBackground,
                          NS_RGB(0, 0, 0));
  aCtx->SetColor(bgColor);
  aCtx->FillRect(mRect + ToReferenceFrame());
}

class nsDisplayMathMLCharBackground : public nsDisplayItem {
public:
  nsDisplayMathMLCharBackground(nsDisplayListBuilder* aBuilder,
                                nsIFrame* aFrame, const nsRect& aRect,
                                nsStyleContext* aStyleContext)
    : nsDisplayItem(aBuilder, aFrame), mStyleContext(aStyleContext),
      mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLCharBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLCharBackground() {
    MOZ_COUNT_DTOR(nsDisplayMathMLCharBackground);
  }
#endif

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion *aInvalidRegion) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLCharBackground", TYPE_MATHML_CHAR_BACKGROUND)
private:
  nsStyleContext* mStyleContext;
  nsRect          mRect;
};

void
nsDisplayMathMLCharBackground::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                                         const nsDisplayItemGeometry* aGeometry,
                                                         nsRegion *aInvalidRegion)
{
  AddInvalidRegionForSyncDecodeBackgroundImages(aBuilder, aGeometry, aInvalidRegion);

  nsDisplayItem::ComputeInvalidationRegion(aBuilder, aGeometry, aInvalidRegion);
}

void nsDisplayMathMLCharBackground::Paint(nsDisplayListBuilder* aBuilder,
                                          nsRenderingContext* aCtx)
{
  const nsStyleBorder* border = mStyleContext->StyleBorder();
  nsRect rect(mRect + ToReferenceFrame());
  nsCSSRendering::PaintBackgroundWithSC(mFrame->PresContext(), *aCtx, mFrame,
                                        mVisibleRect, rect,
                                        mStyleContext, *border,
                                        aBuilder->GetBackgroundPaintFlags());
}

class nsDisplayMathMLCharForeground : public nsDisplayItem {
public:
  nsDisplayMathMLCharForeground(nsDisplayListBuilder* aBuilder,
                                nsIFrame* aFrame, nsMathMLChar* aChar,
				                uint32_t aIndex, bool aIsSelected)
    : nsDisplayItem(aBuilder, aFrame), mChar(aChar), 
      mIndex(aIndex), mIsSelected(aIsSelected) {
    MOZ_COUNT_CTOR(nsDisplayMathMLCharForeground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLCharForeground() {
    MOZ_COUNT_DTOR(nsDisplayMathMLCharForeground);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
    *aSnap = false;
    nsRect rect;
    mChar->GetRect(rect);
    nsPoint offset = ToReferenceFrame() + rect.TopLeft();
    nsBoundingMetrics bm;
    mChar->GetBoundingMetrics(bm);
    nsRect temp(offset.x + bm.leftBearing, offset.y,
                bm.rightBearing - bm.leftBearing, bm.ascent + bm.descent);
    
    temp.Inflate(mFrame->PresContext()->AppUnitsPerDevPixel());
    return temp;
  }
  
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx)
  {
    mChar->PaintForeground(mFrame->PresContext(), *aCtx,
                           ToReferenceFrame(), mIsSelected);
  }

  NS_DISPLAY_DECL_NAME("MathMLCharForeground", TYPE_MATHML_CHAR_FOREGROUND)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder)
  {
    bool snap;
    return GetBounds(aBuilder, &snap);
  }
  
  virtual uint32_t GetPerFrameKey() {
    return (mIndex << nsDisplayItem::TYPE_BITS)
      | nsDisplayItem::GetPerFrameKey();
  }

private:
  nsMathMLChar* mChar;
  uint32_t      mIndex;
  bool          mIsSelected;
};

#ifdef DEBUG
class nsDisplayMathMLCharDebug : public nsDisplayItem {
public:
  nsDisplayMathMLCharDebug(nsDisplayListBuilder* aBuilder,
                           nsIFrame* aFrame, const nsRect& aRect)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLCharDebug);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLCharDebug() {
    MOZ_COUNT_DTOR(nsDisplayMathMLCharDebug);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLCharDebug", TYPE_MATHML_CHAR_DEBUG)

private:
  nsRect mRect;
};

void nsDisplayMathMLCharDebug::Paint(nsDisplayListBuilder* aBuilder,
                                     nsRenderingContext* aCtx)
{
  
  int skipSides = 0;
  nsPresContext* presContext = mFrame->PresContext();
  nsStyleContext* styleContext = mFrame->StyleContext();
  nsRect rect = mRect + ToReferenceFrame();
  nsCSSRendering::PaintBorder(presContext, *aCtx, mFrame,
                              mVisibleRect, rect, styleContext, skipSides);
  nsCSSRendering::PaintOutline(presContext, *aCtx, mFrame,
                               mVisibleRect, rect, styleContext);
}
#endif


void
nsMathMLChar::Display(nsDisplayListBuilder*   aBuilder,
                      nsIFrame*               aForFrame,
                      const nsDisplayListSet& aLists,
                      uint32_t                aIndex,
                      const nsRect*           aSelectedRect)
{
  nsStyleContext* parentContext = mStyleContext->GetParent();
  nsStyleContext* styleContext = mStyleContext;

  if (mDrawNormal) {
    
    
    styleContext = parentContext;
  }

  if (!styleContext->StyleVisibility()->IsVisible())
    return;

  
  
  
  
  if (aSelectedRect && !aSelectedRect->IsEmpty()) {
    aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayMathMLSelectionRect(aBuilder, aForFrame, *aSelectedRect));
  }
  else if (mRect.width && mRect.height) {
    const nsStyleBackground* backg = styleContext->StyleBackground();
    if (styleContext != parentContext &&
        NS_GET_A(backg->mBackgroundColor) > 0) {
      aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayMathMLCharBackground(aBuilder, aForFrame, mRect,
                                      styleContext));
    }
    
    

#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
    
    aLists.BorderBackground()->AppendToTop(new (aBuilder)
      nsDisplayMathMLCharDebug(aBuilder, aForFrame, mRect));
#endif
  }
  aLists.Content()->AppendNewToTop(new (aBuilder)
    nsDisplayMathMLCharForeground(aBuilder, aForFrame, this,
                                  aIndex,
                                  aSelectedRect &&
                                  !aSelectedRect->IsEmpty()));
}

void
nsMathMLChar::ApplyTransforms(nsRenderingContext& aRenderingContext, nsRect &r)
{
  
  if (mMirrored) {
    aRenderingContext.Translate(r.TopRight());
    aRenderingContext.Scale(-mScaleX, mScaleY);
  } else {
    aRenderingContext.Translate(r.TopLeft());
    aRenderingContext.Scale(mScaleX, mScaleY);
  }

  
  r.x = r.y = 0;
  r.width /= mScaleX;
  r.height /= mScaleY;
}

void
nsMathMLChar::PaintForeground(nsPresContext* aPresContext,
                              nsRenderingContext& aRenderingContext,
                              nsPoint aPt,
                              bool aIsSelected)
{
  nsStyleContext* parentContext = mStyleContext->GetParent();
  nsStyleContext* styleContext = mStyleContext;

  if (mDrawNormal) {
    
    
    styleContext = parentContext;
  }

  
  nscolor fgColor = styleContext->GetVisitedDependentColor(eCSSProperty_color);
  if (aIsSelected) {
    
    fgColor = LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectForeground,
                                    fgColor);
  }
  aRenderingContext.SetColor(fgColor);

  nsFont theFont(styleContext->StyleFont()->mFont);
  if (! mFamily.IsEmpty()) {
    theFont.name = mFamily;
  }
  nsRefPtr<nsFontMetrics> fm;
  aRenderingContext.DeviceContext()->GetMetricsFor(theFont,
    styleContext->StyleFont()->mLanguage,
    aPresContext->GetUserFontSet(), aPresContext->GetTextPerfMetrics(),
    *getter_AddRefs(fm));
  aRenderingContext.SetFont(fm);

  aRenderingContext.PushState();
  nsRect r = mRect + aPt;
  ApplyTransforms(aRenderingContext, r);

  if (mDrawNormal) {
    
    
    uint32_t len = uint32_t(mData.Length());
    aRenderingContext.DrawString(mData.get(), len, 0, mUnscaledAscent);
  }
  else {
    
    
    if (mGlyph.Exists()) {
      aRenderingContext.DrawString(mGlyph.code, mGlyph.Length(),
                                   0, mUnscaledAscent);
    }
    else { 
      if (NS_STRETCH_DIRECTION_VERTICAL == mDirection)
        PaintVertically(aPresContext, aRenderingContext, theFont, styleContext,
                        mGlyphTable, r);
      else if (NS_STRETCH_DIRECTION_HORIZONTAL == mDirection)
        PaintHorizontally(aPresContext, aRenderingContext, theFont,
                          styleContext, mGlyphTable, r);
    }
  }

  aRenderingContext.PopState();
}





class AutoPushClipRect {
  nsRenderingContext& mCtx;
public:
  AutoPushClipRect(nsRenderingContext& aCtx, const nsRect& aRect)
    : mCtx(aCtx) {
    mCtx.PushState();
    mCtx.IntersectClip(aRect);
  }
  ~AutoPushClipRect() {
    mCtx.PopState();
  }
};

static nsPoint
SnapToDevPixels(const gfxContext* aThebesContext, int32_t aAppUnitsPerGfxUnit,
                const nsPoint& aPt)
{
  gfxPoint pt(NSAppUnitsToFloatPixels(aPt.x, aAppUnitsPerGfxUnit),
              NSAppUnitsToFloatPixels(aPt.y, aAppUnitsPerGfxUnit));
  pt = aThebesContext->UserToDevice(pt);
  pt.Round();
  pt = aThebesContext->DeviceToUser(pt);
  return nsPoint(NSFloatPixelsToAppUnits(pt.x, aAppUnitsPerGfxUnit),
                 NSFloatPixelsToAppUnits(pt.y, aAppUnitsPerGfxUnit));
}


nsresult
nsMathMLChar::PaintVertically(nsPresContext*      aPresContext,
                              nsRenderingContext& aRenderingContext,
                              nsFont&              aFont,
                              nsStyleContext*      aStyleContext,
                              nsGlyphTable*        aGlyphTable,
                              nsRect&              aRect)
{
  
  
  nscoord oneDevPixel = aPresContext->AppUnitsPerDevPixel();

  
  int32_t i = 0;
  nsGlyphCode ch, chdata[4];
  nsBoundingMetrics bmdata[4];
  int32_t glue, bottom;
  nsGlyphCode chGlue = aGlyphTable->GlueOf(aPresContext, this);
  for (int32_t j = 0; j < 4; ++j) {
    switch (j) {
      case 0:
        ch = aGlyphTable->TopOf(aPresContext, this);
        break;
      case 1:
        ch = aGlyphTable->MiddleOf(aPresContext, this);
        if (!ch.Exists())
          continue; 
        break;
      case 2:
        ch = aGlyphTable->BottomOf(aPresContext, this);
        bottom = i;
        break;
      case 3:
        ch = chGlue;
        glue = i;
        break;
    }
    
    if (!ch.Exists()) ch = chGlue;
    
    if (ch.Exists()) {
      SetFontFamily(aStyleContext, aRenderingContext,
                    aFont, aGlyphTable, ch, mFamily);
      bmdata[i] = aRenderingContext.GetBoundingMetrics(ch.code, ch.Length());
    }
    chdata[i] = ch;
    ++i;
  }
  nscoord dx = aRect.x;
  nscoord offset[3], start[3], end[3];
  nsRefPtr<gfxContext> ctx = aRenderingContext.ThebesContext();
  for (i = 0; i <= bottom; ++i) {
    ch = chdata[i];
    const nsBoundingMetrics& bm = bmdata[i];
    nscoord dy;
    if (0 == i) { 
      dy = aRect.y + bm.ascent;
    }
    else if (bottom == i) { 
      dy = aRect.y + aRect.height - bm.descent;
    }
    else { 
      dy = aRect.y + bm.ascent + (aRect.height - (bm.ascent + bm.descent))/2;
    }
    
    
    
    dy = SnapToDevPixels(ctx, oneDevPixel, nsPoint(dx, dy)).y;
    
    offset[i] = dy;
    
    
    
    start[i] = dy - bm.ascent + oneDevPixel; 
    end[i] = dy + bm.descent - oneDevPixel; 
  }

  
  for (i = 0; i < bottom; ++i) {
    if (end[i] > start[i+1]) {
      end[i] = (end[i] + start[i+1]) / 2;
      start[i+1] = end[i];
    }
  }

  nsRect unionRect = aRect;
  unionRect.x += mBoundingMetrics.leftBearing;
  unionRect.width =
    mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing;
  unionRect.Inflate(oneDevPixel, oneDevPixel);

  
  
  for (i = 0; i <= bottom; ++i) {
    ch = chdata[i];
    
    if (ch.Exists()) {
      nscoord dy = offset[i];
      
      
      nsRect clipRect = unionRect;
      
      
      
      nscoord height = bmdata[i].ascent + bmdata[i].descent;
      if (ch == chGlue ||
          height * (1.0 - NS_MATHML_DELIMITER_FACTOR) > oneDevPixel) {
        if (0 == i) { 
          clipRect.height = end[i] - clipRect.y;
        }
        else if (bottom == i) { 
          clipRect.height -= start[i] - clipRect.y;
          clipRect.y = start[i];
        }
        else { 
          clipRect.y = start[i];
          clipRect.height = end[i] - start[i];
        }
      }
      if (!clipRect.IsEmpty()) {
        AutoPushClipRect clip(aRenderingContext, clipRect);
        SetFontFamily(aStyleContext, aRenderingContext,
                      aFont, aGlyphTable, ch, mFamily);
        aRenderingContext.DrawString(ch.code, ch.Length(), dx, dy);
      }
    }
  }

  
  
  if (!chGlue.Exists()) { 
    
    
    
    
    
    
    
    nscoord lbearing, rbearing;
    int32_t first = 0, last = 1;
    while (last <= bottom) {
      if (chdata[last].Exists()) {
        lbearing = bmdata[last].leftBearing;
        rbearing = bmdata[last].rightBearing;
        if (chdata[first].Exists()) {
          if (lbearing < bmdata[first].leftBearing)
            lbearing = bmdata[first].leftBearing;
          if (rbearing > bmdata[first].rightBearing)
            rbearing = bmdata[first].rightBearing;
        }
      }
      else if (chdata[first].Exists()) {
        lbearing = bmdata[first].leftBearing;
        rbearing = bmdata[first].rightBearing;
      }
      else {
        NS_ERROR("Cannot stretch - All parts missing");
        return NS_ERROR_UNEXPECTED;
      }
      
      nsRect rule(aRect.x + lbearing, end[first],
                  rbearing - lbearing, start[last] - end[first]);
      if (!rule.IsEmpty())
        aRenderingContext.FillRect(rule);
      first = last;
      last++;
    }
  }
  else if (bmdata[glue].ascent + bmdata[glue].descent > 0) {
    
    nsBoundingMetrics& bm = bmdata[glue];
    
    if (bm.ascent + bm.descent >= 3 * oneDevPixel) {
      
      
      bm.ascent -= oneDevPixel;
      bm.descent -= oneDevPixel;
    }

    SetFontFamily(aStyleContext, aRenderingContext,
                  aFont, aGlyphTable, chGlue, mFamily);
    nsRect clipRect = unionRect;

    for (i = 0; i < bottom; ++i) {
      
      nscoord dy = std::max(end[i], aRect.y);
      nscoord fillEnd = std::min(start[i+1], aRect.YMost());
      while (dy < fillEnd) {
        clipRect.y = dy;
        clipRect.height = std::min(bm.ascent + bm.descent, fillEnd - dy);
        AutoPushClipRect clip(aRenderingContext, clipRect);
        dy += bm.ascent;
        aRenderingContext.DrawString(chGlue.code, chGlue.Length(), dx, dy);
        dy += bm.descent;
      }
    }
  }
#ifdef DEBUG
  else {
    for (i = 0; i < bottom; ++i) {
      NS_ASSERTION(end[i] >= start[i+1],
                   "gap between parts with missing glue glyph");
    }
  }
#endif
  return NS_OK;
}


nsresult
nsMathMLChar::PaintHorizontally(nsPresContext*      aPresContext,
                                nsRenderingContext& aRenderingContext,
                                nsFont&              aFont,
                                nsStyleContext*      aStyleContext,
                                nsGlyphTable*        aGlyphTable,
                                nsRect&              aRect)
{
  
  
  nscoord oneDevPixel = aPresContext->AppUnitsPerDevPixel();

  
  int32_t i = 0;
  nsGlyphCode ch, chdata[4];
  nsBoundingMetrics bmdata[4];
  int32_t glue, right;
  nsGlyphCode chGlue = aGlyphTable->GlueOf(aPresContext, this);
  for (int32_t j = 0; j < 4; ++j) {
    switch (j) {
      case 0:
        ch = aGlyphTable->LeftOf(aPresContext, this);
        break;
      case 1:
        ch = aGlyphTable->MiddleOf(aPresContext, this);
        if (!ch.Exists())
          continue; 
        break;
      case 2:
        ch = aGlyphTable->RightOf(aPresContext, this);
        right = i;
        break;
      case 3:
        ch = chGlue;
        glue = i;
        break;
    }
    
    if (!ch.Exists()) ch = chGlue;
    
    if (ch.Exists()) {
      SetFontFamily(aStyleContext, aRenderingContext,
                    aFont, aGlyphTable, ch, mFamily);
      bmdata[i] = aRenderingContext.GetBoundingMetrics(ch.code, ch.Length());
    }
    chdata[i] = ch;
    ++i;
  }
  nscoord dy = aRect.y + mBoundingMetrics.ascent;
  nscoord offset[3], start[3], end[3];
  nsRefPtr<gfxContext> ctx = aRenderingContext.ThebesContext();
  for (i = 0; i <= right; ++i) {
    ch = chdata[i];
    const nsBoundingMetrics& bm = bmdata[i];
    nscoord dx;
    if (0 == i) { 
      dx = aRect.x - bm.leftBearing;
    }
    else if (right == i) { 
      dx = aRect.x + aRect.width - bm.rightBearing;
    }
    else { 
      dx = aRect.x + (aRect.width - bm.width)/2;
    }
    
    
    
    dx = SnapToDevPixels(ctx, oneDevPixel, nsPoint(dx, dy)).x;
    
    offset[i] = dx;
    
    
    
    start[i] = dx + bm.leftBearing + oneDevPixel; 
    end[i] = dx + bm.rightBearing - oneDevPixel; 
  }

  
  for (i = 0; i < right; ++i) {
    if (end[i] > start[i+1]) {
      end[i] = (end[i] + start[i+1]) / 2;
      start[i+1] = end[i];
    }
  }

  nsRect unionRect = aRect;
  unionRect.Inflate(oneDevPixel, oneDevPixel);

  
  
  for (i = 0; i <= right; ++i) {
    ch = chdata[i];
    
    if (ch.Exists()) {
      nscoord dx = offset[i];
      nsRect clipRect = unionRect;
      
      
      
      nscoord width = bmdata[i].rightBearing - bmdata[i].leftBearing;
      if (ch == chGlue ||
          width * (1.0 - NS_MATHML_DELIMITER_FACTOR) > oneDevPixel) {
        if (0 == i) { 
          clipRect.width = end[i] - clipRect.x;
        }
        else if (right == i) { 
          clipRect.width -= start[i] - clipRect.x;
          clipRect.x = start[i];
        }
        else { 
          clipRect.x = start[i];
          clipRect.width = end[i] - start[i];
        }
      }
      if (!clipRect.IsEmpty()) {
        AutoPushClipRect clip(aRenderingContext, clipRect);
        SetFontFamily(aStyleContext, aRenderingContext,
                      aFont, aGlyphTable, ch, mFamily);
        aRenderingContext.DrawString(ch.code, ch.Length(), dx, dy);
      }
    }
  }

  
  
  if (!chGlue.Exists()) { 
    
    
    
    
    
    nscoord ascent, descent;
    int32_t first = 0, last = 1;
    while (last <= right) {
      if (chdata[last].Exists()) {
        ascent = bmdata[last].ascent;
        descent = bmdata[last].descent;
        if (chdata[first].Exists()) {
          if (ascent > bmdata[first].ascent)
            ascent = bmdata[first].ascent;
          if (descent > bmdata[first].descent)
            descent = bmdata[first].descent;
        }
      }
      else if (chdata[first].Exists()) {
        ascent = bmdata[first].ascent;
        descent = bmdata[first].descent;
      }
      else {
        NS_ERROR("Cannot stretch - All parts missing");
        return NS_ERROR_UNEXPECTED;
      }
      
      nsRect rule(end[first], dy - ascent,
                  start[last] - end[first], ascent + descent);
      if (!rule.IsEmpty())
        aRenderingContext.FillRect(rule);
      first = last;
      last++;
    }
  }
  else if (bmdata[glue].rightBearing - bmdata[glue].leftBearing > 0) {
    
    nsBoundingMetrics& bm = bmdata[glue];
    
    if (bm.rightBearing - bm.leftBearing >= 3 * oneDevPixel) {
      
      
      bm.leftBearing += oneDevPixel;
      bm.rightBearing -= oneDevPixel;
    }

    SetFontFamily(aStyleContext, aRenderingContext,
                  aFont, aGlyphTable, chGlue, mFamily);
    nsRect clipRect = unionRect;

    for (i = 0; i < right; ++i) {
      
      nscoord dx = std::max(end[i], aRect.x);
      nscoord fillEnd = std::min(start[i+1], aRect.XMost());
      while (dx < fillEnd) {
        clipRect.x = dx;
        clipRect.width = std::min(bm.rightBearing - bm.leftBearing, fillEnd - dx);
        AutoPushClipRect clip(aRenderingContext, clipRect);
        dx -= bm.leftBearing;
        aRenderingContext.DrawString(chGlue.code, chGlue.Length(), dx, dy);
        dx += bm.rightBearing;
      }
    }
  }
#ifdef DEBUG
  else { 
    for (i = 0; i < right; ++i) {
      NS_ASSERTION(end[i] >= start[i+1],
                   "gap between parts with missing glue glyph");
    }
  }
#endif
  return NS_OK;
}
