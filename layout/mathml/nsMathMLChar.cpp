




#include "nsMathMLChar.h"

#include "gfxTextRun.h"
#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/MathAlgorithms.h"

#include "nsCOMPtr.h"
#include "nsDeviceContext.h"
#include "nsFontMetrics.h"
#include "nsIFrame.h"
#include "nsLayoutUtils.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsUnicharUtils.h"
#include "nsRenderingContext.h"

#include "mozilla/Preferences.h"
#include "nsIPersistentProperties2.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"

#include "mozilla/LookAndFeel.h"
#include "nsCSSRendering.h"
#include "prprf.h"         

#include "nsDisplayList.h"

#include "nsMathMLOperators.h"
#include <algorithm>

#include "gfxMathTable.h"
#include "nsUnicodeScriptCodes.h"

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::image;






static const float kMaxScaleFactor = 20.0;
static const float kLargeOpFactor = float(M_SQRT2);
static const float kIntegralFactor = 2.0;

static void
NormalizeDefaultFont(nsFont& aFont, float aFontSizeInflation)
{
  if (aFont.fontlist.GetDefaultFontType() != eFamily_none) {
    aFont.fontlist.Append(FontFamilyName(aFont.fontlist.GetDefaultFontType()));
    aFont.fontlist.SetDefaultFontType(eFamily_none);
  }
  aFont.size = NSToCoordRound(aFont.size * aFontSizeInflation);
}


static const nsGlyphCode kNullGlyph = {{{0, 0}}, 0};














class nsGlyphTable {
public:
  virtual ~nsGlyphTable() {}

  virtual const FontFamilyName&
  FontNameFor(const nsGlyphCode& aGlyphCode) const = 0;

  
  virtual nsGlyphCode ElementAt(gfxContext*   aThebesContext,
                                int32_t       aAppUnitsPerDevPixel,
                                gfxFontGroup* aFontGroup,
                                char16_t      aChar,
                                bool          aVertical,
                                uint32_t      aPosition) = 0;
  virtual nsGlyphCode BigOf(gfxContext*   aThebesContext,
                            int32_t       aAppUnitsPerDevPixel,
                            gfxFontGroup* aFontGroup,
                            char16_t      aChar,
                            bool          aVertical,
                            uint32_t      aSize) = 0;

  
  virtual bool HasPartsOf(gfxContext*   aThebesContext,
                          int32_t       aAppUnitsPerDevPixel,
                          gfxFontGroup* aFontGroup,
                          char16_t      aChar,
                          bool          aVertical) = 0;

  virtual gfxTextRun* MakeTextRun(gfxContext*        aThebesContext,
                                  int32_t            aAppUnitsPerDevPixel,
                                  gfxFontGroup*      aFontGroup,
                                  const nsGlyphCode& aGlyph) = 0;
protected:
  nsGlyphTable() : mCharCache(0) {}
  
  
  
  char16_t mCharCache;
};





















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
                                                NS_ConvertUTF16toUTF8(uriStr),
                                                nsContentUtils::GetSystemPrincipal(),
                                                nsIContentPolicy::TYPE_OTHER);
}

class nsPropertiesTable final : public nsGlyphTable {
public:
  explicit nsPropertiesTable(const nsString& aPrimaryFontName)
    : mState(NS_TABLE_STATE_EMPTY)
  {
    MOZ_COUNT_CTOR(nsPropertiesTable);
    mGlyphCodeFonts.AppendElement(FontFamilyName(aPrimaryFontName, eUnquotedName));
  }

  ~nsPropertiesTable()
  {
    MOZ_COUNT_DTOR(nsPropertiesTable);
  }

  const FontFamilyName& PrimaryFontName() const
  {
    return mGlyphCodeFonts[0];
  }

  const FontFamilyName&
  FontNameFor(const nsGlyphCode& aGlyphCode) const override
  {
    NS_ASSERTION(!aGlyphCode.IsGlyphID(),
                 "nsPropertiesTable can only access glyphs by code point");
    return mGlyphCodeFonts[aGlyphCode.font];
  }

  virtual nsGlyphCode ElementAt(gfxContext*   aThebesContext,
                                int32_t       aAppUnitsPerDevPixel,
                                gfxFontGroup* aFontGroup,
                                char16_t      aChar,
                                bool          aVertical,
                                uint32_t      aPosition) override;

  virtual nsGlyphCode BigOf(gfxContext*   aThebesContext,
                            int32_t       aAppUnitsPerDevPixel,
                            gfxFontGroup* aFontGroup,
                            char16_t      aChar,
                            bool          aVertical,
                            uint32_t      aSize) override
  {
    return ElementAt(aThebesContext, aAppUnitsPerDevPixel, aFontGroup,
                     aChar, aVertical, 4 + aSize);
  }

  virtual bool HasPartsOf(gfxContext*   aThebesContext,
                          int32_t       aAppUnitsPerDevPixel,
                          gfxFontGroup* aFontGroup,
                          char16_t      aChar,
                          bool          aVertical) override
  {
    return (ElementAt(aThebesContext, aAppUnitsPerDevPixel, aFontGroup,
                      aChar, aVertical, 0).Exists() ||
            ElementAt(aThebesContext, aAppUnitsPerDevPixel, aFontGroup,
                      aChar, aVertical, 1).Exists() ||
            ElementAt(aThebesContext, aAppUnitsPerDevPixel, aFontGroup,
                      aChar, aVertical, 2).Exists() ||
            ElementAt(aThebesContext, aAppUnitsPerDevPixel, aFontGroup,
                      aChar, aVertical, 3).Exists());
  }

  virtual gfxTextRun* MakeTextRun(gfxContext*        aThebesContext,
                                  int32_t            aAppUnitsPerDevPixel,
                                  gfxFontGroup*      aFontGroup,
                                  const nsGlyphCode& aGlyph) override;
private:

  
  
  
  nsTArray<FontFamilyName> mGlyphCodeFonts;

  
  int32_t mState;

  
  
  nsCOMPtr<nsIPersistentProperties> mGlyphProperties;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsString  mGlyphCache;
};


nsGlyphCode
nsPropertiesTable::ElementAt(gfxContext*   ,
                             int32_t       ,
                             gfxFontGroup* ,
                             char16_t      aChar,
                             bool          ,
                             uint32_t      aPosition)
{
  if (mState == NS_TABLE_STATE_ERROR) return kNullGlyph;
  
  if (mState == NS_TABLE_STATE_EMPTY) {
    nsAutoString primaryFontName;
    mGlyphCodeFonts[0].AppendToString(primaryFontName);
    nsresult rv = LoadProperties(primaryFontName, mGlyphProperties);
#ifdef DEBUG
    nsAutoCString uriStr;
    uriStr.AssignLiteral("resource://gre/res/fonts/mathfont");
    LossyAppendUTF16toASCII(primaryFontName, uriStr);
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
      mGlyphCodeFonts.AppendElement(FontFamilyName(value, eUnquotedName)); 
    }
  }

  
  if (mCharCache != aChar) {
    
    
    char key[10]; PR_snprintf(key, sizeof(key), "\\u%04X", aChar);
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
        if (font >= mGlyphCodeFonts.Length()) {
          NS_ERROR("Nonexistent font referenced in glyph table");
          return kNullGlyph;
        }
        
        if (!mGlyphCodeFonts[font].mName.Length()) {
          return kNullGlyph;
        }
      }
      buffer.Append(font);
    }
    
    mGlyphCache.Assign(buffer);
    mCharCache = aChar;
  }

  
  uint32_t index = 3*aPosition;
  if (index+2 >= mGlyphCache.Length()) return kNullGlyph;
  nsGlyphCode ch;
  ch.code[0] = mGlyphCache.CharAt(index);
  ch.code[1] = mGlyphCache.CharAt(index + 1);
  ch.font = mGlyphCache.CharAt(index + 2);
  return ch.code[0] == char16_t(0xFFFD) ? kNullGlyph : ch;
}


gfxTextRun*
nsPropertiesTable::MakeTextRun(gfxContext*        aThebesContext,
                               int32_t            aAppUnitsPerDevPixel,
                               gfxFontGroup*      aFontGroup,
                               const nsGlyphCode& aGlyph)
{
  NS_ASSERTION(!aGlyph.IsGlyphID(),
               "nsPropertiesTable can only access glyphs by code point");
  return aFontGroup->
    MakeTextRun(aGlyph.code, aGlyph.Length(), aThebesContext,
                aAppUnitsPerDevPixel, 0, nullptr);
}





class nsOpenTypeTable final : public nsGlyphTable {
public:
  ~nsOpenTypeTable()
  {
    MOZ_COUNT_DTOR(nsOpenTypeTable);
  }

  virtual nsGlyphCode ElementAt(gfxContext*   aThebesContext,
                                int32_t       aAppUnitsPerDevPixel,
                                gfxFontGroup* aFontGroup,
                                char16_t      aChar,
                                bool          aVertical,
                                uint32_t      aPosition) override;
  virtual nsGlyphCode BigOf(gfxContext*   aThebesContext,
                            int32_t       aAppUnitsPerDevPixel,
                            gfxFontGroup* aFontGroup,
                            char16_t      aChar,
                            bool          aVertical,
                            uint32_t      aSize) override;
  virtual bool HasPartsOf(gfxContext*   aThebesContext,
                          int32_t       aAppUnitsPerDevPixel,
                          gfxFontGroup* aFontGroup,
                          char16_t      aChar,
                          bool          aVertical) override;

  const FontFamilyName&
  FontNameFor(const nsGlyphCode& aGlyphCode) const override {
    NS_ASSERTION(aGlyphCode.IsGlyphID(),
                 "nsOpenTypeTable can only access glyphs by id");
    return mFontFamilyName;
  }

  virtual gfxTextRun* MakeTextRun(gfxContext*        aThebesContext,
                                  int32_t            aAppUnitsPerDevPixel,
                                  gfxFontGroup*      aFontGroup,
                                  const nsGlyphCode& aGlyph) override;

  
  
  
  static nsOpenTypeTable* Create(gfxFont* aFont)
  {
    if (!aFont->GetFontEntry()->TryGetMathTable()) {
      return nullptr;
    }
    return new nsOpenTypeTable(aFont->GetFontEntry());
  }

private:
  nsRefPtr<gfxFontEntry> mFontEntry;
  FontFamilyName mFontFamilyName;
  uint32_t mGlyphID;

  explicit nsOpenTypeTable(gfxFontEntry* aFontEntry)
    : mFontEntry(aFontEntry),
      mFontFamilyName(aFontEntry->FamilyName(), eUnquotedName) {
    MOZ_COUNT_CTOR(nsOpenTypeTable);
  }

  void UpdateCache(gfxContext*   aThebesContext,
                   int32_t       aAppUnitsPerDevPixel,
                   gfxFontGroup* aFontGroup,
                   char16_t      aChar);
};

void
nsOpenTypeTable::UpdateCache(gfxContext*   aThebesContext,
                             int32_t       aAppUnitsPerDevPixel,
                             gfxFontGroup* aFontGroup,
                             char16_t      aChar)
{
  if (mCharCache != aChar) {
    nsAutoPtr<gfxTextRun> textRun;
    textRun = aFontGroup->
      MakeTextRun(&aChar, 1, aThebesContext, aAppUnitsPerDevPixel, 0, nullptr);
    const gfxTextRun::CompressedGlyph& data = textRun->GetCharacterGlyphs()[0];
    if (data.IsSimpleGlyph()) {
      mGlyphID = data.GetSimpleGlyph();
    } else if (data.GetGlyphCount() == 1) {
      mGlyphID = textRun->GetDetailedGlyphs(0)->mGlyphID;
    } else {
      mGlyphID = 0;
    }
    mCharCache = aChar;
  }
}


nsGlyphCode
nsOpenTypeTable::ElementAt(gfxContext*   aThebesContext,
                           int32_t       aAppUnitsPerDevPixel,
                           gfxFontGroup* aFontGroup,
                           char16_t      aChar,
                           bool          aVertical,
                           uint32_t      aPosition)
{
  UpdateCache(aThebesContext, aAppUnitsPerDevPixel, aFontGroup, aChar);

  uint32_t parts[4];
  if (!mFontEntry->GetMathVariantsParts(mGlyphID, aVertical, parts)) {
    return kNullGlyph;
  }

  uint32_t glyphID = parts[aPosition];
  if (!glyphID) {
    return kNullGlyph;
  }
  nsGlyphCode glyph;
  glyph.glyphID = glyphID;
  glyph.font = -1;
  return glyph;
}


nsGlyphCode
nsOpenTypeTable::BigOf(gfxContext*   aThebesContext,
                       int32_t       aAppUnitsPerDevPixel,
                       gfxFontGroup* aFontGroup,
                       char16_t      aChar,
                       bool          aVertical,
                       uint32_t      aSize)
{
  UpdateCache(aThebesContext, aAppUnitsPerDevPixel, aFontGroup, aChar);

  uint32_t glyphID =
    mFontEntry->GetMathVariantsSize(mGlyphID, aVertical, aSize);
  if (!glyphID) {
    return kNullGlyph;
  }

  nsGlyphCode glyph;
  glyph.glyphID = glyphID;
  glyph.font = -1;
  return glyph;
}


bool
nsOpenTypeTable::HasPartsOf(gfxContext*   aThebesContext,
                            int32_t       aAppUnitsPerDevPixel,
                            gfxFontGroup* aFontGroup,
                            char16_t      aChar,
                            bool          aVertical)
{
  UpdateCache(aThebesContext, aAppUnitsPerDevPixel, aFontGroup, aChar);

  uint32_t parts[4];
  if (!mFontEntry->GetMathVariantsParts(mGlyphID, aVertical, parts)) {
    return false;
  }

  return parts[0] || parts[1] || parts[2] || parts[3];
}


gfxTextRun*
nsOpenTypeTable::MakeTextRun(gfxContext*        aThebesContext,
                             int32_t            aAppUnitsPerDevPixel,
                             gfxFontGroup*      aFontGroup,
                             const nsGlyphCode& aGlyph)
{
  NS_ASSERTION(aGlyph.IsGlyphID(),
               "nsOpenTypeTable can only access glyphs by id");

  gfxTextRunFactory::Parameters params = {
    aThebesContext, nullptr, nullptr, nullptr, 0, aAppUnitsPerDevPixel
  };
  gfxTextRun* textRun = gfxTextRun::Create(&params, 1, aFontGroup, 0);
  textRun->AddGlyphRun(aFontGroup->GetFirstValidFont(),
                       gfxTextRange::kFontGroup, 0,
                       false, gfxTextRunFactory::TEXT_ORIENT_HORIZONTAL);
                              
                              
  gfxTextRun::DetailedGlyph detailedGlyph;
  detailedGlyph.mGlyphID = aGlyph.glyphID;
  detailedGlyph.mAdvance =
    NSToCoordRound(aAppUnitsPerDevPixel *
                   aFontGroup->GetFirstValidFont()->
                   GetGlyphHAdvance(aThebesContext, aGlyph.glyphID));
  detailedGlyph.mXOffset = detailedGlyph.mYOffset = 0;
  gfxShapedText::CompressedGlyph g;
  g.SetComplex(true, true, 1);
  textRun->SetGlyphs(0, g, &detailedGlyph);

  return textRun;
}








class nsGlyphTableList final : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsPropertiesTable mUnicodeTable;

  nsGlyphTableList()
    : mUnicodeTable(NS_LITERAL_STRING("Unicode"))
  {
    MOZ_COUNT_CTOR(nsGlyphTableList);
  }

  nsresult Initialize();
  nsresult Finalize();

  
  nsGlyphTable*
  AddGlyphTable(const nsString& aPrimaryFontName);

  
  nsGlyphTable*
  GetGlyphTableFor(const nsAString& aFamily);

private:
  ~nsGlyphTableList()
  {
    MOZ_COUNT_DTOR(nsGlyphTableList);
  }

  nsPropertiesTable* PropertiesTableAt(int32_t aIndex) {
    return &mPropertiesTableList.ElementAt(aIndex);
  }
  int32_t PropertiesTableCount() {
    return mPropertiesTableList.Length();
  }
  
  nsTArray<nsPropertiesTable> mPropertiesTableList;
};

NS_IMPL_ISUPPORTS(nsGlyphTableList, nsIObserver)



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
  
  NS_IF_RELEASE(gGlyphTableList);
  return rv;
}

nsGlyphTable*
nsGlyphTableList::AddGlyphTable(const nsString& aPrimaryFontName)
{
  
  nsGlyphTable* glyphTable = GetGlyphTableFor(aPrimaryFontName);
  if (glyphTable != &mUnicodeTable)
    return glyphTable;

  
  glyphTable = mPropertiesTableList.AppendElement(aPrimaryFontName);
  return glyphTable;
}

nsGlyphTable*
nsGlyphTableList::GetGlyphTableFor(const nsAString& aFamily)
{
  for (int32_t i = 0; i < PropertiesTableCount(); i++) {
    nsPropertiesTable* glyphTable = PropertiesTableAt(i);
    const FontFamilyName& primaryFontName = glyphTable->PrimaryFontName();
    nsAutoString primaryFontNameStr;
    primaryFontName.AppendToString(primaryFontNameStr);
    
    if (primaryFontNameStr.Equals(aFamily, nsCaseInsensitiveStringComparator())) {
      return glyphTable;
    }
  }
  
  return &mUnicodeTable;
}



static nsresult
InitGlobals(nsPresContext* aPresContext)
{
  NS_ASSERTION(!gGlyphTableInitialized, "Error -- already initialized");
  gGlyphTableInitialized = true;

  
  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  nsRefPtr<nsGlyphTableList> glyphTableList = new nsGlyphTableList();
  if (glyphTableList) {
    rv = glyphTableList->Initialize();
  }
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  
  
  
  if (!glyphTableList->AddGlyphTable(NS_LITERAL_STRING("MathJax_Main")) ||
      !glyphTableList->AddGlyphTable(NS_LITERAL_STRING("STIXGeneral")) ||
      !glyphTableList->AddGlyphTable(NS_LITERAL_STRING("Standard Symbols L"))
#ifdef XP_WIN
      || !glyphTableList->AddGlyphTable(NS_LITERAL_STRING("Symbol"))
#endif
      ) {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }

  glyphTableList.forget(&gGlyphTableList);
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
  
  if (gGlyphTableList && (1 == mData.Length())) {
    mDirection = nsMathMLOperators::GetStretchyDirection(mData);
    
    
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


bool
nsMathMLChar::SetFontFamily(nsPresContext*          aPresContext,
                            const nsGlyphTable*     aGlyphTable,
                            const nsGlyphCode&      aGlyphCode,
                            const FontFamilyList&   aDefaultFamilyList,
                            nsFont&                 aFont,
                            nsRefPtr<gfxFontGroup>* aFontGroup)
{
  FontFamilyList glyphCodeFont;

  if (aGlyphCode.font) {
    glyphCodeFont.Append(aGlyphTable->FontNameFor(aGlyphCode));
  }

  const FontFamilyList& familyList =
    aGlyphCode.font ? glyphCodeFont : aDefaultFamilyList;

  if (!*aFontGroup || !(aFont.fontlist == familyList)) {
    nsFont font = aFont;
    font.fontlist = familyList;
    const nsStyleFont* styleFont = mStyleContext->StyleFont();
    nsRefPtr<nsFontMetrics> fm;
    aPresContext->DeviceContext()->
      GetMetricsFor(font,
                    styleFont->mLanguage,
                    styleFont->mExplicitLanguage,
                    gfxFont::eHorizontal,
                    aPresContext->GetUserFontSet(),
                    aPresContext->GetTextPerfMetrics(),
                    *getter_AddRefs(fm));
    
    
    gfxFont *firstFont = fm->GetThebesFontGroup()->GetFirstValidFont();
    FontFamilyList firstFontList;
    firstFontList.Append(
      FontFamilyName(firstFont->GetFontEntry()->FamilyName(), eUnquotedName));
    if (aGlyphTable == &gGlyphTableList->mUnicodeTable ||
        firstFontList == familyList) {
      aFont.fontlist = familyList;
      *aFontGroup = fm->GetThebesFontGroup();
    } else {
      return false; 
    }
  }
  return true;
}

static nsBoundingMetrics
MeasureTextRun(gfxContext* aThebesContext, gfxTextRun* aTextRun)
{
  gfxTextRun::Metrics metrics =
    aTextRun->MeasureText(0, aTextRun->GetLength(),
                          gfxFont::TIGHT_HINTED_OUTLINE_EXTENTS,
                          aThebesContext, nullptr);

  nsBoundingMetrics bm;
  bm.leftBearing = NSToCoordFloor(metrics.mBoundingBox.X());
  bm.rightBearing = NSToCoordCeil(metrics.mBoundingBox.XMost());
  bm.ascent = NSToCoordCeil(-metrics.mBoundingBox.Y());
  bm.descent = NSToCoordCeil(metrics.mBoundingBox.YMost());
  bm.width = NSToCoordRound(metrics.mAdvanceWidth);

  return bm;
}

class nsMathMLChar::StretchEnumContext {
public:
  StretchEnumContext(nsMathMLChar*        aChar,
                     nsPresContext*       aPresContext,
                     gfxContext*          aThebesContext,
                     float                aFontSizeInflation,
                     nsStretchDirection   aStretchDirection,
                     nscoord              aTargetSize,
                     uint32_t             aStretchHint,
                     nsBoundingMetrics&   aStretchedMetrics,
                     const FontFamilyList&  aFamilyList,
                     bool&              aGlyphFound)
    : mChar(aChar),
      mPresContext(aPresContext),
      mThebesContext(aThebesContext),
      mFontSizeInflation(aFontSizeInflation),
      mDirection(aStretchDirection),
      mTargetSize(aTargetSize),
      mStretchHint(aStretchHint),
      mBoundingMetrics(aStretchedMetrics),
      mFamilyList(aFamilyList),
      mTryVariants(true),
      mTryParts(true),
      mGlyphFound(aGlyphFound) {}

  static bool
  EnumCallback(const FontFamilyName& aFamily, bool aGeneric, void *aData);

private:
  bool TryVariants(nsGlyphTable* aGlyphTable,
                   nsRefPtr<gfxFontGroup>* aFontGroup,
                   const FontFamilyList& aFamilyList);
  bool TryParts(nsGlyphTable* aGlyphTable,
                nsRefPtr<gfxFontGroup>* aFontGroup,
                const FontFamilyList& aFamilyList);

  nsMathMLChar* mChar;
  nsPresContext* mPresContext;
  gfxContext* mThebesContext;
  float mFontSizeInflation;
  const nsStretchDirection mDirection;
  const nscoord mTargetSize;
  const uint32_t mStretchHint;
  nsBoundingMetrics& mBoundingMetrics;
  
  const FontFamilyList& mFamilyList;

public:
  bool mTryVariants;
  bool mTryParts;

private:
  nsAutoTArray<nsGlyphTable*,16> mTablesTried;
  bool&       mGlyphFound;
};





bool
nsMathMLChar::
StretchEnumContext::TryVariants(nsGlyphTable* aGlyphTable,
                                nsRefPtr<gfxFontGroup>* aFontGroup,
                                const FontFamilyList& aFamilyList)
{
  
  nsStyleContext *sc = mChar->mStyleContext;
  nsFont font = sc->StyleFont()->mFont;
  NormalizeDefaultFont(font, mFontSizeInflation);

  bool isVertical = (mDirection == NS_STRETCH_DIRECTION_VERTICAL);
  nscoord oneDevPixel = mPresContext->AppUnitsPerDevPixel();
  char16_t uchar = mChar->mData[0];
  bool largeop = (NS_STRETCH_LARGEOP & mStretchHint) != 0;
  bool largeopOnly =
    largeop && (NS_STRETCH_VARIABLE_MASK & mStretchHint) == 0;
  bool maxWidth = (NS_STRETCH_MAXWIDTH & mStretchHint) != 0;

  nscoord bestSize =
    isVertical ? mBoundingMetrics.ascent + mBoundingMetrics.descent
               : mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing;
  bool haveBetter = false;

  
  int32_t size = 1;
  nsGlyphCode ch;
  nscoord displayOperatorMinHeight = 0;
  if (largeopOnly) {
    NS_ASSERTION(isVertical, "Stretching should be in the vertical direction");
    ch = aGlyphTable->BigOf(mThebesContext, oneDevPixel, *aFontGroup, uchar,
                            isVertical, 0);
    if (ch.IsGlyphID()) {
      gfxFont* mathFont = aFontGroup->get()->GetFirstMathFont();
      
      
      
      if (mathFont) {
        displayOperatorMinHeight =
          mathFont->GetMathConstant(gfxFontEntry::DisplayOperatorMinHeight,
                                    oneDevPixel);
        nsAutoPtr<gfxTextRun> textRun;
        textRun = aGlyphTable->MakeTextRun(mThebesContext, oneDevPixel,
                                           *aFontGroup, ch);
        nsBoundingMetrics bm = MeasureTextRun(mThebesContext, textRun);
        float largeopFactor = kLargeOpFactor;
        if (NS_STRETCH_INTEGRAL & mStretchHint) {
          
          largeopFactor = kIntegralFactor;
        }
        nscoord minHeight = largeopFactor * (bm.ascent + bm.descent);
        if (displayOperatorMinHeight < minHeight) {
          displayOperatorMinHeight = minHeight;
        }
      }
    }
  }
#ifdef NOISY_SEARCH
  printf("  searching in %s ...\n",
           NS_LossyConvertUTF16toASCII(aFamily).get());
#endif
  while ((ch = aGlyphTable->BigOf(mThebesContext, oneDevPixel, *aFontGroup,
                                  uchar, isVertical, size)).Exists()) {

    if (!mChar->SetFontFamily(mPresContext, aGlyphTable, ch, aFamilyList, font,
                              aFontGroup)) {
      
      if (largeopOnly) break;
      ++size;
      continue;
    }

    nsAutoPtr<gfxTextRun> textRun;
    textRun = aGlyphTable->MakeTextRun(mThebesContext, oneDevPixel,
                                       *aFontGroup, ch);
    nsBoundingMetrics bm = MeasureTextRun(mThebesContext, textRun);
    if (ch.IsGlyphID()) {
      gfxFont* mathFont = aFontGroup->get()->GetFirstMathFont();
      if (mathFont) {
        
        
        
        
        
        
        
        gfxFloat italicCorrection;
        if (mathFont->GetFontEntry()->
            GetMathItalicsCorrection(ch.glyphID, &italicCorrection)) {
          bm.width -=
            NSToCoordRound(italicCorrection *
                           mathFont->GetAdjustedSize() * oneDevPixel);
          if (bm.width < 0) {
            bm.width = 0;
          }
        }
      }
    }

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
        mChar->mGlyphs[0] = textRun;
        mChar->mDraw = DRAW_VARIANT;
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

    
    if (largeopOnly && (bm.ascent + bm.descent) >= displayOperatorMinHeight) {
      break;
    }
    ++size;
  }

  return haveBetter &&
    (largeopOnly ||
     IsSizeOK(mPresContext, bestSize, mTargetSize, mStretchHint));
}




bool
nsMathMLChar::StretchEnumContext::TryParts(nsGlyphTable* aGlyphTable,
                                           nsRefPtr<gfxFontGroup>* aFontGroup,
                                           const FontFamilyList& aFamilyList)
{
  
  nsFont font = mChar->mStyleContext->StyleFont()->mFont;
  NormalizeDefaultFont(font, mFontSizeInflation);

  
  nsAutoPtr<gfxTextRun> textRun[4];
  nsGlyphCode chdata[4];
  nsBoundingMetrics bmdata[4];
  nscoord sizedata[4];

  bool isVertical = (mDirection == NS_STRETCH_DIRECTION_VERTICAL);
  nscoord oneDevPixel = mPresContext->AppUnitsPerDevPixel();
  char16_t uchar = mChar->mData[0];
  bool maxWidth = (NS_STRETCH_MAXWIDTH & mStretchHint) != 0;
  if (!aGlyphTable->HasPartsOf(mThebesContext, oneDevPixel, *aFontGroup,
                               uchar, isVertical))
    return false; 

  for (int32_t i = 0; i < 4; i++) {
    nsGlyphCode ch = aGlyphTable->ElementAt(mThebesContext, oneDevPixel,
                                            *aFontGroup, uchar, isVertical, i);
    chdata[i] = ch;
    if (ch.Exists()) {
      if (!mChar->SetFontFamily(mPresContext, aGlyphTable, ch, aFamilyList, font,
                                aFontGroup))
        return false;

      textRun[i] = aGlyphTable->MakeTextRun(mThebesContext, oneDevPixel,
                                            *aFontGroup, ch);
      nsBoundingMetrics bm = MeasureTextRun(mThebesContext, textRun[i]);
      bmdata[i] = bm;
      sizedata[i] = isVertical ? bm.ascent + bm.descent
                               : bm.rightBearing - bm.leftBearing;
    } else {
      
      
      textRun[i] = nullptr;
      bmdata[i] = nsBoundingMetrics();
      sizedata[i] = i == 3 ? mTargetSize : 0;
    }
  }

  
  
  if (aGlyphTable == &gGlyphTableList->mUnicodeTable) {
    gfxFont* unicodeFont = nullptr;
    for (int32_t i = 0; i < 4; i++) {
      if (!textRun[i]) {
        continue;
      }
      if (textRun[i]->GetLength() != 1 ||
          textRun[i]->GetCharacterGlyphs()[0].IsMissing()) {
        return false;
      }
      uint32_t numGlyphRuns;
      const gfxTextRun::GlyphRun* glyphRuns =
        textRun[i]->GetGlyphRuns(&numGlyphRuns);
      if (numGlyphRuns != 1) {
        return false;
      }
      if (!unicodeFont) {
        unicodeFont = glyphRuns[0].mFont;
      } else if (unicodeFont != glyphRuns[0].mFont) {
        return false;
      }
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
    
    
    for (i = 0; i <= 3 && !textRun[i]; i++);
    if (i == 4) {
      NS_ERROR("Cannot stretch - All parts missing");
      return false;
    }
    nscoord lbearing = bmdata[i].leftBearing;
    nscoord rbearing = bmdata[i].rightBearing;
    nscoord width = bmdata[i].width;
    i++;
    for (; i <= 3; i++) {
      if (!textRun[i]) continue;
      lbearing = std::min(lbearing, bmdata[i].leftBearing);
      rbearing = std::max(rbearing, bmdata[i].rightBearing);
      width = std::max(width, bmdata[i].width);
    }
    if (maxWidth) {
      lbearing = std::min(lbearing, mBoundingMetrics.leftBearing);
      rbearing = std::max(rbearing, mBoundingMetrics.rightBearing);
      width = std::max(width, mBoundingMetrics.width);
    }
    mBoundingMetrics.width = width;
    
    
    
    mBoundingMetrics.ascent = bmdata[0].ascent; 
                                                
    mBoundingMetrics.descent = computedSize - mBoundingMetrics.ascent;
    mBoundingMetrics.leftBearing = lbearing;
    mBoundingMetrics.rightBearing = rbearing;
  }
  else {
    int32_t i;
    
    
    for (i = 0; i <= 3 && !textRun[i]; i++);
    if (i == 4) {
      NS_ERROR("Cannot stretch - All parts missing");
      return false;
    }
    nscoord ascent = bmdata[i].ascent;
    nscoord descent = bmdata[i].descent;
    i++;
    for (; i <= 3; i++) {
      if (!textRun[i]) continue;
      ascent = std::max(ascent, bmdata[i].ascent);
      descent = std::max(descent, bmdata[i].descent);
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

  
  mChar->mDraw = DRAW_PARTS;
  for (int32_t i = 0; i < 4; i++) {
    mChar->mGlyphs[i] = textRun[i];
    mChar->mBmData[i] = bmdata[i];
  }

  return IsSizeOK(mPresContext, computedSize, mTargetSize, mStretchHint);
}


bool
nsMathMLChar::StretchEnumContext::EnumCallback(const FontFamilyName& aFamily,
                                               bool aGeneric, void *aData)
{
  StretchEnumContext* context = static_cast<StretchEnumContext*>(aData);

  
  FontFamilyName unquotedFamilyName(aFamily);
  if (unquotedFamilyName.mType == eFamily_named_quoted) {
    unquotedFamilyName.mType = eFamily_named;
  }

  
  
  nsStyleContext *sc = context->mChar->mStyleContext;
  nsFont font = sc->StyleFont()->mFont;
  NormalizeDefaultFont(font, context->mFontSizeInflation);
  nsRefPtr<gfxFontGroup> fontGroup;
  FontFamilyList family;
  family.Append(unquotedFamilyName);
  if (!aGeneric && !context->mChar->SetFontFamily(context->mPresContext,
                                                  nullptr, kNullGlyph, family,
                                                  font, &fontGroup))
     return true; 

  
  nsAutoPtr<nsOpenTypeTable> openTypeTable;
  nsGlyphTable* glyphTable;
  if (aGeneric) {
    
    glyphTable = &gGlyphTableList->mUnicodeTable;
  } else {
    
    openTypeTable = nsOpenTypeTable::Create(fontGroup->GetFirstValidFont());
    if (openTypeTable) {
      glyphTable = openTypeTable;
    } else {
      
      
      nsAutoString familyName;
      unquotedFamilyName.AppendToString(familyName);
      glyphTable = gGlyphTableList->GetGlyphTableFor(familyName);
    }
  }

  if (!openTypeTable) {
    if (context->mTablesTried.Contains(glyphTable))
      return true; 
    
    
    context->mTablesTried.AppendElement(glyphTable);
  }

  
  
  
  const FontFamilyList& familyList = glyphTable == &gGlyphTableList->mUnicodeTable ?
    context->mFamilyList : family;

  if((context->mTryVariants &&
      context->TryVariants(glyphTable, &fontGroup, familyList)) ||
     (context->mTryParts && context->TryParts(glyphTable,
                                              &fontGroup,
                                              familyList)))
    return false; 

  return true; 
}



static void
InsertMathFallbacks(FontFamilyList& aFamilyList,
                    nsTArray<nsString>& aFallbacks)
{
  FontFamilyList aMergedList;

  bool inserted = false;
  const nsTArray<FontFamilyName>& fontlist = aFamilyList.GetFontlist();
  uint32_t i, num = fontlist.Length();
  for (i = 0; i < num; i++) {
    const FontFamilyName& name = fontlist[i];
    if (!inserted && name.IsGeneric()) {
      inserted = true;
      aMergedList.Append(aFallbacks);
    }
    aMergedList.Append(name);
  }

  if (!inserted) {
    aMergedList.Append(aFallbacks);
  }
  aFamilyList = aMergedList;
}

nsresult
nsMathMLChar::StretchInternal(nsPresContext*           aPresContext,
                              gfxContext*              aThebesContext,
                              float                    aFontSizeInflation,
                              nsStretchDirection&      aStretchDirection,
                              const nsBoundingMetrics& aContainerSize,
                              nsBoundingMetrics&       aDesiredStretchSize,
                              uint32_t                 aStretchHint,
                              
                              
                              float                    aMaxSize,
                              bool                     aMaxSizeIsAbsolute)
{
  
  
  
  nsStretchDirection direction = nsMathMLOperators::GetStretchyDirection(mData);

  
  
  
  nsFont font = mStyleContext->GetParent()->StyleFont()->mFont;
  NormalizeDefaultFont(font, aFontSizeInflation);

  const nsStyleFont* styleFont = mStyleContext->StyleFont();
  nsRefPtr<nsFontMetrics> fm;
  aPresContext->DeviceContext()->
    GetMetricsFor(font,
                  styleFont->mLanguage,
                  styleFont->mExplicitLanguage,
                  gfxFont::eHorizontal,
                  aPresContext->GetUserFontSet(),
                  aPresContext->GetTextPerfMetrics(),
                  *getter_AddRefs(fm));
  uint32_t len = uint32_t(mData.Length());
  nsAutoPtr<gfxTextRun> textRun;
  textRun = fm->GetThebesFontGroup()->
    MakeTextRun(static_cast<const char16_t*>(mData.get()), len, aThebesContext,
                aPresContext->AppUnitsPerDevPixel(), 0,
                aPresContext->MissingFontRecorder());
  aDesiredStretchSize = MeasureTextRun(aThebesContext, textRun);
  mGlyphs[0] = textRun;

  bool maxWidth = (NS_STRETCH_MAXWIDTH & aStretchHint) != 0;
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

  bool done = false;

  if (!maxWidth && !largeop) {
    
    
    if ((targetSize <= 0) || 
        ((isVertical && charSize >= targetSize) ||
         IsSizeOK(aPresContext, charSize, targetSize, aStretchHint)))
      done = true;
  }

  
  
  

  bool glyphFound = false;

  if (!done) { 
    
    font = mStyleContext->StyleFont()->mFont;
    NormalizeDefaultFont(font, aFontSizeInflation);

    
    
    nsAutoTArray<nsString, 10> mathFallbacks;
    gfxFontUtils::GetPrefsFontList("font.mathfont-family", mathFallbacks);
    InsertMathFallbacks(font.fontlist, mathFallbacks);


#ifdef NOISY_SEARCH
    nsAutoString fontlistStr;
    font.fontlist.ToString(fontlistStr, false, true);
    printf("Searching in "%s" for a glyph of appropriate size for: 0x%04X:%c\n",
           NS_ConvertUTF16toUTF8(fontlistStr).get(), mData[0], mData[0]&0x00FF);
#endif
    StretchEnumContext enumData(this, aPresContext, aThebesContext,
                                aFontSizeInflation,
                                aStretchDirection, targetSize, aStretchHint,
                                aDesiredStretchSize, font.fontlist, glyphFound);
    enumData.mTryParts = !largeopOnly;

    const nsTArray<FontFamilyName>& fontlist = font.fontlist.GetFontlist();
    uint32_t i, num = fontlist.Length();
    bool next = true;
    for (i = 0; i < num && next; i++) {
      const FontFamilyName& name = fontlist[i];
      next = StretchEnumContext::EnumCallback(name, name.IsGeneric(), &enumData);
    }
  }

  if (!maxWidth) {
    
    
    mUnscaledAscent = aDesiredStretchSize.ascent;
  }
    
  if (glyphFound) {
    return NS_OK;
  }

  
  
  
  gfxMissingFontRecorder* MFR = aPresContext->MissingFontRecorder();
  if (MFR && !fm->GetThebesFontGroup()->GetFirstMathFont()) {
    MFR->RecordScript(MOZ_SCRIPT_MATHEMATICAL_NOTATION);
  }

  
  if (!Preferences::GetBool("mathml.scale_stretchy_operators.enabled", true)) {
    return NS_OK;
  }
  
  
  if (stretchy) {
    if (isVertical) {
      float scale =
        std::min(kMaxScaleFactor, float(aContainerSize.ascent + aContainerSize.descent) /
        (aDesiredStretchSize.ascent + aDesiredStretchSize.descent));
      if (!largeop || scale > 1.0) {
        
        if (!maxWidth) {
          mScaleY *= scale;
        }
        aDesiredStretchSize.ascent *= scale;
        aDesiredStretchSize.descent *= scale;
      }
    } else {
      float scale =
        std::min(kMaxScaleFactor, float(aContainerSize.rightBearing - aContainerSize.leftBearing) /
        (aDesiredStretchSize.rightBearing - aDesiredStretchSize.leftBearing));
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

  
  
  if (largeop) {
    float scale;
    float largeopFactor = kLargeOpFactor;

    
    
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
      
      largeopFactor = kIntegralFactor;
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
                      float                    aFontSizeInflation,
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

  mDraw = DRAW_NORMAL;
  mMirrored = aRTL && nsMathMLOperators::IsMirrorableOperator(mData);
  mScaleY = mScaleX = 1.0;
  mDirection = aStretchDirection;
  nsresult rv =
    StretchInternal(aPresContext, aRenderingContext.ThebesContext(),
                    aFontSizeInflation, mDirection,
                    aContainerSize, aDesiredStretchSize, aStretchHint);

  
  mBoundingMetrics = aDesiredStretchSize;

  return rv;
}













nscoord
nsMathMLChar::GetMaxWidth(nsPresContext* aPresContext,
                          nsRenderingContext& aRenderingContext,
                          float aFontSizeInflation,
                          uint32_t aStretchHint)
{
  nsBoundingMetrics bm;
  nsStretchDirection direction = NS_STRETCH_DIRECTION_VERTICAL;
  const nsBoundingMetrics container; 

  StretchInternal(aPresContext, aRenderingContext.ThebesContext(),
                  aFontSizeInflation, direction,
                  container, bm, aStretchHint | NS_STRETCH_MAXWIDTH);

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
                     nsRenderingContext* aCtx) override;
  NS_DISPLAY_DECL_NAME("MathMLSelectionRect", TYPE_MATHML_SELECTION_RECT)
private:
  nsRect    mRect;
};

void nsDisplayMathMLSelectionRect::Paint(nsDisplayListBuilder* aBuilder,
                                         nsRenderingContext* aCtx)
{
  DrawTarget* drawTarget = aCtx->GetDrawTarget();
  Rect rect = NSRectToSnappedRect(mRect + ToReferenceFrame(),
                                  mFrame->PresContext()->AppUnitsPerDevPixel(),
                                  *drawTarget);
  
  nscolor bgColor =
    LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectBackground,
                          NS_RGB(0, 0, 0));
  drawTarget->FillRect(rect, ColorPattern(ToDeviceColor(bgColor)));
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

  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) override;
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion *aInvalidRegion) override;
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) override;
  NS_DISPLAY_DECL_NAME("MathMLCharBackground", TYPE_MATHML_CHAR_BACKGROUND)
private:
  nsStyleContext* mStyleContext;
  nsRect          mRect;
};

nsDisplayItemGeometry*
nsDisplayMathMLCharBackground::AllocateGeometry(nsDisplayListBuilder* aBuilder)
{
  return new nsDisplayItemGenericImageGeometry(this, aBuilder);
}

void
nsDisplayMathMLCharBackground::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                                         const nsDisplayItemGeometry* aGeometry,
                                                         nsRegion *aInvalidRegion)
{
  auto geometry =
    static_cast<const nsDisplayItemGenericImageGeometry*>(aGeometry);

  if (aBuilder->ShouldSyncDecodeImages() &&
      geometry->ShouldInvalidateToSyncDecodeImages()) {
    bool snap;
    aInvalidRegion->Or(*aInvalidRegion, GetBounds(aBuilder, &snap));
  }

  nsDisplayItem::ComputeInvalidationRegion(aBuilder, aGeometry, aInvalidRegion);
}

void nsDisplayMathMLCharBackground::Paint(nsDisplayListBuilder* aBuilder,
                                          nsRenderingContext* aCtx)
{
  const nsStyleBorder* border = mStyleContext->StyleBorder();
  nsRect rect(mRect + ToReferenceFrame());

  DrawResult result =
    nsCSSRendering::PaintBackgroundWithSC(mFrame->PresContext(), *aCtx, mFrame,
                                          mVisibleRect, rect,
                                          mStyleContext, *border,
                                          aBuilder->GetBackgroundPaintFlags());

  nsDisplayItemGenericImageGeometry::UpdateDrawResult(this, result);
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

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) override {
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
                     nsRenderingContext* aCtx) override
  {
    mChar->PaintForeground(mFrame->PresContext(), *aCtx,
                           ToReferenceFrame(), mIsSelected);
  }

  NS_DISPLAY_DECL_NAME("MathMLCharForeground", TYPE_MATHML_CHAR_FOREGROUND)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) override
  {
    bool snap;
    return GetBounds(aBuilder, &snap);
  }
  
  virtual uint32_t GetPerFrameKey() override {
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
                     nsRenderingContext* aCtx) override;
  NS_DISPLAY_DECL_NAME("MathMLCharDebug", TYPE_MATHML_CHAR_DEBUG)

private:
  nsRect mRect;
};

void nsDisplayMathMLCharDebug::Paint(nsDisplayListBuilder* aBuilder,
                                     nsRenderingContext* aCtx)
{
  
  Sides skipSides;
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

  if (mDraw == DRAW_NORMAL) {
    
    
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
nsMathMLChar::ApplyTransforms(gfxContext* aThebesContext,
                              int32_t aAppUnitsPerGfxUnit,
                              nsRect &r)
{
  
  if (mMirrored) {
    nsPoint pt = r.TopRight();
    gfxPoint devPixelOffset(NSAppUnitsToFloatPixels(pt.x, aAppUnitsPerGfxUnit),
                            NSAppUnitsToFloatPixels(pt.y, aAppUnitsPerGfxUnit));
    aThebesContext->SetMatrix(
      aThebesContext->CurrentMatrix().Translate(devPixelOffset).
                                      Scale(-mScaleX, mScaleY));
  } else {
    nsPoint pt = r.TopLeft();
    gfxPoint devPixelOffset(NSAppUnitsToFloatPixels(pt.x, aAppUnitsPerGfxUnit),
                            NSAppUnitsToFloatPixels(pt.y, aAppUnitsPerGfxUnit));
    aThebesContext->SetMatrix(
      aThebesContext->CurrentMatrix().Translate(devPixelOffset).
                                      Scale(mScaleX, mScaleY));
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

  if (mDraw == DRAW_NORMAL) {
    
    
    styleContext = parentContext;
  }

  nsRefPtr<gfxContext> thebesContext = aRenderingContext.ThebesContext();

  
  nscolor fgColor = styleContext->GetVisitedDependentColor(eCSSProperty_color);
  if (aIsSelected) {
    
    fgColor = LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectForeground,
                                    fgColor);
  }
  thebesContext->SetColor(fgColor);
  thebesContext->Save();
  nsRect r = mRect + aPt;
  ApplyTransforms(thebesContext, aPresContext->AppUnitsPerDevPixel(), r);

  switch(mDraw)
  {
    case DRAW_NORMAL:
    case DRAW_VARIANT:
      
      
      if (mGlyphs[0]) {
        mGlyphs[0]->Draw(thebesContext, gfxPoint(0.0, mUnscaledAscent),
                         DrawMode::GLYPH_FILL, 0, mGlyphs[0]->GetLength(),
                         nullptr, nullptr, nullptr);
      }
      break;
    case DRAW_PARTS: {
      
      if (NS_STRETCH_DIRECTION_VERTICAL == mDirection)
        PaintVertically(aPresContext, thebesContext, r, fgColor);
      else if (NS_STRETCH_DIRECTION_HORIZONTAL == mDirection)
        PaintHorizontally(aPresContext, thebesContext, r, fgColor);
      break;
    }
    default:
      NS_NOTREACHED("Unknown drawing method");
      break;
  }

  thebesContext->Restore();
}





class AutoPushClipRect {
  gfxContext* mThebesContext;
public:
  AutoPushClipRect(gfxContext* aThebesContext, int32_t aAppUnitsPerGfxUnit,
                   const nsRect& aRect)
    : mThebesContext(aThebesContext) {
    mThebesContext->Save();
    mThebesContext->NewPath();
    gfxRect clip = nsLayoutUtils::RectToGfxRect(aRect, aAppUnitsPerGfxUnit);
    mThebesContext->SnappedRectangle(clip);
    mThebesContext->Clip();
  }
  ~AutoPushClipRect() {
    mThebesContext->Restore();
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

static void
PaintRule(DrawTarget& aDrawTarget,
          int32_t     aAppUnitsPerGfxUnit,
          nsRect&     aRect,
          nscolor     aColor)
{
  Rect rect = NSRectToSnappedRect(aRect, aAppUnitsPerGfxUnit, aDrawTarget);
  ColorPattern color(ToDeviceColor(aColor));
  aDrawTarget.FillRect(rect, color);
}


nsresult
nsMathMLChar::PaintVertically(nsPresContext* aPresContext,
                              gfxContext*    aThebesContext,
                              nsRect&        aRect,
                              nscolor        aColor)
{
  DrawTarget& aDrawTarget = *aThebesContext->GetDrawTarget();

  
  
  nscoord oneDevPixel = aPresContext->AppUnitsPerDevPixel();

  
  int32_t i = 0;
  nscoord dx = aRect.x;
  nscoord offset[3], start[3], end[3];
  for (i = 0; i <= 2; ++i) {
    const nsBoundingMetrics& bm = mBmData[i];
    nscoord dy;
    if (0 == i) { 
      dy = aRect.y + bm.ascent;
    }
    else if (2 == i) { 
      dy = aRect.y + aRect.height - bm.descent;
    }
    else { 
      dy = aRect.y + bm.ascent + (aRect.height - (bm.ascent + bm.descent))/2;
    }
    
    
    
    dy = SnapToDevPixels(aThebesContext, oneDevPixel, nsPoint(dx, dy)).y;
    
    offset[i] = dy;
    
    
    
    if (bm.ascent + bm.descent >= 2 * oneDevPixel) {
      start[i] = dy - bm.ascent + oneDevPixel; 
      end[i] = dy + bm.descent - oneDevPixel; 
    } else {
      
      
      start[i] = dy - bm.ascent; 
      end[i] = dy + bm.descent; 
    }
  }

  
  for (i = 0; i < 2; ++i) {
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

  
  
  for (i = 0; i <= 2; ++i) {
    
    if (mGlyphs[i]) {
      nscoord dy = offset[i];
      
      
      nsRect clipRect = unionRect;
      
      
      
      nscoord height = mBmData[i].ascent + mBmData[i].descent;
      if (height * (1.0 - NS_MATHML_DELIMITER_FACTOR) > oneDevPixel) {
        if (0 == i) { 
          clipRect.height = end[i] - clipRect.y;
        }
        else if (2 == i) { 
          clipRect.height -= start[i] - clipRect.y;
          clipRect.y = start[i];
        }
        else { 
          clipRect.y = start[i];
          clipRect.height = end[i] - start[i];
        }
      }
      if (!clipRect.IsEmpty()) {
        AutoPushClipRect clip(aThebesContext, oneDevPixel, clipRect);
        mGlyphs[i]->Draw(aThebesContext, gfxPoint(dx, dy),
                         DrawMode::GLYPH_FILL, 0, mGlyphs[i]->GetLength(),
                         nullptr, nullptr, nullptr);
      }
    }
  }

  
  
  if (!mGlyphs[3]) { 
    
    
    
    
    
    
    
    nscoord lbearing, rbearing;
    int32_t first = 0, last = 1;
    while (last <= 2) {
      if (mGlyphs[last]) {
        lbearing = mBmData[last].leftBearing;
        rbearing = mBmData[last].rightBearing;
        if (mGlyphs[first]) {
          if (lbearing < mBmData[first].leftBearing)
            lbearing = mBmData[first].leftBearing;
          if (rbearing > mBmData[first].rightBearing)
            rbearing = mBmData[first].rightBearing;
        }
      }
      else if (mGlyphs[first]) {
        lbearing = mBmData[first].leftBearing;
        rbearing = mBmData[first].rightBearing;
      }
      else {
        NS_ERROR("Cannot stretch - All parts missing");
        return NS_ERROR_UNEXPECTED;
      }
      
      nsRect rule(aRect.x + lbearing, end[first],
                  rbearing - lbearing, start[last] - end[first]);
      PaintRule(aDrawTarget, oneDevPixel, rule, aColor);
      first = last;
      last++;
    }
  }
  else if (mBmData[3].ascent + mBmData[3].descent > 0) {
    
    nsBoundingMetrics& bm = mBmData[3];
    
    if (bm.ascent + bm.descent >= 3 * oneDevPixel) {
      
      
      bm.ascent -= oneDevPixel;
      bm.descent -= oneDevPixel;
    }

    nsRect clipRect = unionRect;

    for (i = 0; i < 2; ++i) {
      
      nscoord dy = std::max(end[i], aRect.y);
      nscoord fillEnd = std::min(start[i+1], aRect.YMost());
      while (dy < fillEnd) {
        clipRect.y = dy;
        clipRect.height = std::min(bm.ascent + bm.descent, fillEnd - dy);
        AutoPushClipRect clip(aThebesContext, oneDevPixel, clipRect);
        dy += bm.ascent;
        mGlyphs[3]->Draw(aThebesContext, gfxPoint(dx, dy),
                            DrawMode::GLYPH_FILL, 0, mGlyphs[3]->GetLength(),
                            nullptr, nullptr, nullptr);
        dy += bm.descent;
      }
    }
  }
#ifdef DEBUG
  else {
    for (i = 0; i < 2; ++i) {
      NS_ASSERTION(end[i] >= start[i+1],
                   "gap between parts with missing glue glyph");
    }
  }
#endif
  return NS_OK;
}


nsresult
nsMathMLChar::PaintHorizontally(nsPresContext* aPresContext,
                                gfxContext*    aThebesContext,
                                nsRect&        aRect,
                                nscolor        aColor)
{
  DrawTarget& aDrawTarget = *aThebesContext->GetDrawTarget();

  
  
  nscoord oneDevPixel = aPresContext->AppUnitsPerDevPixel();

  
  int32_t i = 0;
  nscoord dy = aRect.y + mBoundingMetrics.ascent;
  nscoord offset[3], start[3], end[3];
  for (i = 0; i <= 2; ++i) {
    const nsBoundingMetrics& bm = mBmData[i];
    nscoord dx;
    if (0 == i) { 
      dx = aRect.x - bm.leftBearing;
    }
    else if (2 == i) { 
      dx = aRect.x + aRect.width - bm.rightBearing;
    }
    else { 
      dx = aRect.x + (aRect.width - bm.width)/2;
    }
    
    
    
    dx = SnapToDevPixels(aThebesContext, oneDevPixel, nsPoint(dx, dy)).x;
    
    offset[i] = dx;
    
    
    
    if (bm.rightBearing - bm.leftBearing >= 2 * oneDevPixel) { 
      start[i] = dx + bm.leftBearing + oneDevPixel; 
      end[i] = dx + bm.rightBearing - oneDevPixel; 
    } else {
      
      
      start[i] = dx + bm.leftBearing; 
      end[i] = dx + bm.rightBearing; 
    }
  }

  
  for (i = 0; i < 2; ++i) {
    if (end[i] > start[i+1]) {
      end[i] = (end[i] + start[i+1]) / 2;
      start[i+1] = end[i];
    }
  }

  nsRect unionRect = aRect;
  unionRect.Inflate(oneDevPixel, oneDevPixel);

  
  
  for (i = 0; i <= 2; ++i) {
    
    if (mGlyphs[i]) {
      nscoord dx = offset[i];
      nsRect clipRect = unionRect;
      
      
      
      nscoord width = mBmData[i].rightBearing - mBmData[i].leftBearing;
      if (width * (1.0 - NS_MATHML_DELIMITER_FACTOR) > oneDevPixel) {
        if (0 == i) { 
          clipRect.width = end[i] - clipRect.x;
        }
        else if (2 == i) { 
          clipRect.width -= start[i] - clipRect.x;
          clipRect.x = start[i];
        }
        else { 
          clipRect.x = start[i];
          clipRect.width = end[i] - start[i];
        }
      }
      if (!clipRect.IsEmpty()) {
        AutoPushClipRect clip(aThebesContext, oneDevPixel, clipRect);
        mGlyphs[i]->Draw(aThebesContext, gfxPoint(dx, dy),
                         DrawMode::GLYPH_FILL, 0, mGlyphs[i]->GetLength(),
                         nullptr, nullptr, nullptr);
      }
    }
  }

  
  
  if (!mGlyphs[3]) { 
    
    
    
    
    
    nscoord ascent, descent;
    int32_t first = 0, last = 1;
    while (last <= 2) {
      if (mGlyphs[last]) {
        ascent = mBmData[last].ascent;
        descent = mBmData[last].descent;
        if (mGlyphs[first]) {
          if (ascent > mBmData[first].ascent)
            ascent = mBmData[first].ascent;
          if (descent > mBmData[first].descent)
            descent = mBmData[first].descent;
        }
      }
      else if (mGlyphs[first]) {
        ascent = mBmData[first].ascent;
        descent = mBmData[first].descent;
      }
      else {
        NS_ERROR("Cannot stretch - All parts missing");
        return NS_ERROR_UNEXPECTED;
      }
      
      nsRect rule(end[first], dy - ascent,
                  start[last] - end[first], ascent + descent);
      PaintRule(aDrawTarget, oneDevPixel, rule, aColor);
      first = last;
      last++;
    }
  }
  else if (mBmData[3].rightBearing - mBmData[3].leftBearing > 0) {
    
    nsBoundingMetrics& bm = mBmData[3];
    
    if (bm.rightBearing - bm.leftBearing >= 3 * oneDevPixel) {
      
      
      bm.leftBearing += oneDevPixel;
      bm.rightBearing -= oneDevPixel;
    }

    nsRect clipRect = unionRect;

    for (i = 0; i < 2; ++i) {
      
      nscoord dx = std::max(end[i], aRect.x);
      nscoord fillEnd = std::min(start[i+1], aRect.XMost());
      while (dx < fillEnd) {
        clipRect.x = dx;
        clipRect.width = std::min(bm.rightBearing - bm.leftBearing, fillEnd - dx);
        AutoPushClipRect clip(aThebesContext, oneDevPixel, clipRect);
        dx -= bm.leftBearing;
        mGlyphs[3]->Draw(aThebesContext, gfxPoint(dx, dy),
                            DrawMode::GLYPH_FILL, 0, mGlyphs[3]->GetLength(),
                            nullptr, nullptr, nullptr);
        dx += bm.rightBearing;
      }
    }
  }
#ifdef DEBUG
  else { 
    for (i = 0; i < 2; ++i) {
      NS_ASSERTION(end[i] >= start[i+1],
                   "gap between parts with missing glue glyph");
    }
  }
#endif
  return NS_OK;
}
