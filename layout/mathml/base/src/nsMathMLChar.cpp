







































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsIRenderingContext.h"
#include "gfxPlatform.h"
#include "nsIFontMetrics.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsIPersistentProperties2.h"
#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsNetUtil.h"

#include "nsILookAndFeel.h"
#include "nsIDeviceContext.h"
#include "nsCSSRendering.h"
#include "prprf.h"         

#if ALERT_MISSING_FONTS
#include "nsIDOMWindow.h"
#include "nsINonBlockingAlertService.h"
#include "nsIWindowWatcher.h"
#include "nsIStringBundle.h"
#endif
#include "nsDisplayList.h"

#include "nsMathMLOperators.h"
#include "nsMathMLChar.h"





static const PRUnichar   kSpaceCh   = PRUnichar(' ');
static const nsGlyphCode kNullGlyph = {0, 0};
typedef enum {eExtension_base, eExtension_variants, eExtension_parts}
  nsMathfontPrefExtension;












































#define NS_TABLE_TYPE_UNICODE       0
#define NS_TABLE_TYPE_GLYPH_INDEX   1

#define NS_TABLE_STATE_ERROR       -1
#define NS_TABLE_STATE_EMPTY        0
#define NS_TABLE_STATE_READY        1


static PRBool
CheckFontExistence(nsPresContext* aPresContext, const nsString& aFontName)
{
  PRBool aliased;
  nsAutoString localName;
  nsIDeviceContext *deviceContext = aPresContext->DeviceContext();
  deviceContext->GetLocalFontName(aFontName, localName, aliased);
  
  PRBool rv = (aliased || (NS_OK == deviceContext->CheckFontExistence(localName)));
  
  return rv;
}

#if ALERT_MISSING_FONTS


static void
AlertMissingFonts(nsString& aMissingFonts)
{
  nsCOMPtr<nsIStringBundleService> sbs(do_GetService(NS_STRINGBUNDLE_CONTRACTID));
  if (!sbs)
    return;

  nsCOMPtr<nsIStringBundle> sb;
  sbs->CreateBundle("resource://gre/res/fonts/mathfont.properties", getter_AddRefs(sb));
  if (!sb)
    return;

  nsXPIDLString title, message;
  const PRUnichar* strings[] = { aMissingFonts.get() };
  sb->GetStringFromName(NS_LITERAL_STRING("mathfont_missing_dialog_title").get(), getter_Copies(title));
  sb->FormatStringFromName(NS_LITERAL_STRING("mathfont_missing_dialog_message").get(),
                           strings, 1, getter_Copies(message));

  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
  if (!wwatch)
    return;

  nsCOMPtr<nsIDOMWindow> parent;
  wwatch->GetActiveWindow(getter_AddRefs(parent));
  nsresult rv;
  nsCOMPtr<nsINonBlockingAlertService> prompter =
    do_GetService("@mozilla.org/embedcomp/nbalert-service;1", &rv);

  if (prompter && parent) {
    prompter->ShowNonBlockingAlert(parent, title.get(), message.get());
  }
}
#endif


static void
Clean(nsString& aValue)
{
  
  PRInt32 comment = aValue.RFindChar('#');
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
    : mType(NS_TABLE_TYPE_UNICODE),
      mFontName(1), 
      mState(NS_TABLE_STATE_EMPTY),
      mCharCache(0)
  {
    MOZ_COUNT_CTOR(nsGlyphTable);
    mFontName.AppendString(aPrimaryFontName);
  }

  ~nsGlyphTable() 
  {
    MOZ_COUNT_DTOR(nsGlyphTable);
  }

  const nsAString& PrimaryFontName() const
  {
    return *mFontName.StringAt(0);
  }

  const nsAString& FontNameFor(const nsGlyphCode& aGlyphCode) const
  {
    return *mFontName.StringAt(aGlyphCode.font);
  }

  
  
  PRBool Has(nsPresContext* aPresContext, nsMathMLChar* aChar);

  
  PRBool HasVariantsOf(nsPresContext* aPresContext, nsMathMLChar* aChar);

  
  PRBool HasPartsOf(nsPresContext* aPresContext, nsMathMLChar* aChar);

  
  PRBool IsComposite(nsPresContext* aPresContext, nsMathMLChar* aChar);

  
  PRInt32 ChildCountOf(nsPresContext* aPresContext, nsMathMLChar* aChar);

  
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
  nsGlyphCode BigOf(nsPresContext* aPresContext, nsMathMLChar* aChar, PRInt32 aSize) {
    return ElementAt(aPresContext, aChar, 4 + aSize);
  }
  nsGlyphCode LeftOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 0);
  }
  nsGlyphCode RightOf(nsPresContext* aPresContext, nsMathMLChar* aChar) {
    return ElementAt(aPresContext, aChar, 2);
  }

private:
  nsGlyphCode ElementAt(nsPresContext* aPresContext, nsMathMLChar* aChar, PRUint32 aPosition);

  
  PRInt32 mType;    
                           
  
  
  
  nsStringArray mFontName; 
                               
  
  PRInt32 mState;

  
  nsCOMPtr<nsIPersistentProperties> mGlyphProperties;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsString  mGlyphCache;
  PRUnichar mCharCache;
};

nsGlyphCode
nsGlyphTable::ElementAt(nsPresContext* aPresContext, nsMathMLChar* aChar, PRUint32 aPosition)
{
  if (mState == NS_TABLE_STATE_ERROR) return kNullGlyph;
  
  if (mState == NS_TABLE_STATE_EMPTY) {
    nsresult rv = LoadProperties(*mFontName[0], mGlyphProperties);
#ifdef NS_DEBUG
    nsCAutoString uriStr;
    uriStr.AssignLiteral("resource://gre/res/fonts/mathfont");
    LossyAppendUTF16toASCII(*mFontName[0], uriStr);
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

    
    nsCAutoString key;
    nsAutoString value;
    for (PRInt32 i = 1; ; i++) {
      key.AssignLiteral("external.");
      key.AppendInt(i, 10);
      rv = mGlyphProperties->GetStringProperty(key, value);
      if (NS_FAILED(rv)) break;
      Clean(value);
      mFontName.AppendString(value); 
    }
  }

  
  
  if (aChar->mParent && (aChar->mGlyphTable != this)) return kNullGlyph;

  
  PRUnichar uchar = aChar->mData[0];
  if (mCharCache != uchar) {
    
    
    char key[10]; PR_snprintf(key, sizeof(key), "\\u%04X", uchar);
    nsAutoString value;
    nsresult rv = mGlyphProperties->GetStringProperty(nsDependentCString(key), value);
    if (NS_FAILED(rv)) return kNullGlyph;
    Clean(value);
    
    
    
    
    
    
    nsAutoString buffer;
    PRInt32 length = value.Length();
    PRInt32 i = 0; 
    PRInt32 j = 0; 
    while (i < length) {
      PRUnichar code = value[i];
      ++i;
      PRUnichar font = 0;
      
      if (code == kSpaceCh) {
        
        j = -1;
      }
#if 0 
      
      
      
      else if (code == PRUnichar(0xF8FF) && gGlyphTableList &&
               this != &gGlyphTableList->mUnicodeTable) {
        code = gGlyphTableList->mUnicodeTable.
          ElementAt(aPresContext, aChar, aPosition).code;
      }
      
      
      else if ((i+1 < length) && (value[i] == PRUnichar('.'))) {
        ++i;
        
        
        if (1)
          return kNullGlyph;
        ++i;
      }
#endif
      
      
      if (i+1 < length && value[i] == PRUnichar('@') &&
          value[i+1] >= PRUnichar('0') && value[i+1] <= PRUnichar('9')) {
        ++i;
        font = value[i] - '0';
        ++i;
        if (font >= mFontName.Count()) {
          NS_ERROR("Non-existant font referenced in glyph table");
          return kNullGlyph;
        }
        
        nsAutoString fontName;
        mFontName.StringAt(font, fontName);
        if (!fontName.Length() || !CheckFontExistence(aPresContext, fontName)) {
          return kNullGlyph;
        }
      }
      buffer.Append(code);
      buffer.Append(font);
      ++j;
    }
    
    mGlyphCache.Assign(buffer);
    mCharCache = uchar;
  }

  
  
  
  
  if (!aChar->mParent && (kNotFound != mGlyphCache.FindChar(kSpaceCh))) {
    return kNullGlyph;
  }

  
  
  PRUint32 offset = 0;
  PRUint32 length = mGlyphCache.Length();
  if (aChar->mParent) {
    nsMathMLChar* child = aChar->mParent->mSibling;
    
    while (child && (child != aChar)) {
      offset += 5; 
      child = child->mSibling;
    }
    length = 2*(offset + 4); 
  }
  PRUint32 index = 2*(offset + aPosition); 
  if (index+1 >= length) return kNullGlyph;
  nsGlyphCode ch;
  ch.code = mGlyphCache.CharAt(index);
  ch.font = mGlyphCache.CharAt(index + 1);
  return (ch.code == PRUnichar(0xFFFD)) ? kNullGlyph : ch;
}

PRBool
nsGlyphTable::IsComposite(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  
  
  if (aChar->mParent) return PR_FALSE;
  
  mCharCache = 0; mGlyphCache.Truncate(); ElementAt(aPresContext, aChar, 0);
  
  if (8 >= mGlyphCache.Length()) return PR_FALSE;
  
  return (kSpaceCh == mGlyphCache.CharAt(8));
}

PRInt32
nsGlyphTable::ChildCountOf(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  
  if (!IsComposite(aPresContext, aChar)) return 0;
  
  return 1 + mGlyphCache.CountChar(kSpaceCh);
}

PRBool
nsGlyphTable::Has(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  return HasVariantsOf(aPresContext, aChar) || HasPartsOf(aPresContext, aChar);
}

PRBool
nsGlyphTable::HasVariantsOf(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  
  return BigOf(aPresContext, aChar, 1).Exists();
}

PRBool
nsGlyphTable::HasPartsOf(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  return GlueOf(aPresContext, aChar).Exists() ||
    TopOf(aPresContext, aChar).Exists() ||
    BottomOf(aPresContext, aChar).Exists() ||
    MiddleOf(aPresContext, aChar).Exists() ||
    IsComposite(aPresContext, aChar);
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
  nsGlyphTable* TableAt(PRInt32 aIndex) {
    return &mTableList.ElementAt(aIndex);
  }
  PRInt32 Count() {
    return mTableList.Length();
  }

  
  nsTArray<nsGlyphTable> mTableList;
};

NS_IMPL_ISUPPORTS1(nsGlyphTableList, nsIObserver)



static nsGlyphTableList* gGlyphTableList = nsnull;

static PRBool gInitialized = PR_FALSE;


NS_IMETHODIMP
nsGlyphTableList::Observe(nsISupports*     aSubject,
                          const char* aTopic,
                          const PRUnichar* someData)
{
  Finalize();
  return NS_OK;
}


nsresult
nsGlyphTableList::Initialize()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> obs = 
           do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsGlyphTableList::Finalize()
{
  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIObserverService> obs = 
           do_GetService("@mozilla.org/observer-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
  }
  gInitialized = PR_FALSE;
  
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

  PRInt32 i;
  for (i = 0; i < Count(); i++) {
    nsGlyphTable* glyphTable = TableAt(i);
    if (glyphTable->Has(aPresContext, aChar)) {
      return glyphTable;
    }
  }
  return nsnull;
}

nsGlyphTable*
nsGlyphTableList::GetGlyphTableFor(const nsAString& aFamily)
{
  for (PRInt32 i = 0; i < Count(); i++) {
    nsGlyphTable* glyphTable = TableAt(i);
    const nsAString& fontName = glyphTable->PrimaryFontName();
    
    if (fontName.Equals(aFamily, nsCaseInsensitiveStringComparator())) {
      return glyphTable;
    }
  }
  
  return &mUnicodeTable;
}




static PRBool
GetPrefValue(nsIPrefBranch* aPrefBranch, const char* aPrefKey, nsString& aPrefValue)
{
  aPrefValue.Truncate();
  if (aPrefBranch) {
    nsCOMPtr<nsISupportsString> prefString;
    aPrefBranch->GetComplexValue(aPrefKey,
                                 NS_GET_IID(nsISupportsString),
                                 getter_AddRefs(prefString));
    if (prefString) {
      prefString->GetData(aPrefValue);
    }
  }
  return !aPrefValue.IsEmpty();
}







static PRBool
GetFontExtensionPref(nsIPrefBranch* aPrefBranch, PRUnichar aChar,
                     nsMathfontPrefExtension aExtension, nsString& aValue)
{
  
  aValue.Truncate();

  
  
  
  
  
  
  
  

  static const char* kMathFontPrefix = "font.mathfont-family.";

  nsCAutoString extension;
  switch (aExtension)
  {
    case eExtension_base:
      extension.AssignLiteral(".base");
    case eExtension_variants:
      extension.AssignLiteral(".variants");
    case eExtension_parts:
      extension.AssignLiteral(".parts");
    default:
      return PR_FALSE;
  }

  
  nsCAutoString key;
  key.AssignASCII(kMathFontPrefix);
  char ustr[10];
  PR_snprintf(ustr, sizeof(ustr), "\\u%04X", aChar);
  key.Append(ustr);
  key.Append(extension);
  
  nsCAutoString alternateKey;
  alternateKey.AssignASCII(kMathFontPrefix);
  NS_ConvertUTF16toUTF8 tmp(&aChar, 1);
  key.Append(tmp);
  key.Append(extension);

  return GetPrefValue(aPrefBranch, key.get(), aValue) ||
    GetPrefValue(aPrefBranch, alternateKey.get(), aValue);
}

#if ALERT_MISSING_FONTS
struct MathFontEnumContext {
  nsPresContext* mPresContext;
  nsString*       mMissingFamilyList;
};
#endif

static PRBool
MathFontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
#if ALERT_MISSING_FONTS
  
  MathFontEnumContext* context = (MathFontEnumContext*)aData;
  nsPresContext* presContext = context->mPresContext;
  nsString* missingFamilyList = context->mMissingFamilyList;
  if (!CheckFontExistence(presContext, aFamily)) {

   
   
   
   if (aFamily.LowerCaseEqualsLiteral("mt extra"))
     return PR_TRUE; 

    if (!missingFamilyList->IsEmpty()) {
      missingFamilyList->AppendLiteral(", ");
    }
    missingFamilyList->Append(aFamily);
  }
#endif

  if (!gGlyphTableList->AddGlyphTable(aFamily))
    return PR_FALSE; 
  return PR_TRUE; 
}

static nsresult
InitGlobals(nsPresContext* aPresContext)
{
  NS_ASSERTION(!gInitialized, "Error -- already initialized");
  gInitialized = PR_TRUE;
  PRUint32 count = nsMathMLOperators::CountStretchyOperator();
  if (!count) {
    
    return NS_OK;
  }

  
  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  gGlyphTableList = new nsGlyphTableList();
  if (gGlyphTableList) {
    rv = gGlyphTableList->Initialize();
  }
  if (NS_FAILED(rv)) {
    delete gGlyphTableList;
    gGlyphTableList = nsnull;
    return rv;
  }
  





  nsCAutoString key;
  nsAutoString value;
  nsCOMPtr<nsIPersistentProperties> mathfontProp;
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));

  
  
  

  
  value.Truncate();
  rv = LoadProperties(value, mathfontProp);
  if (NS_FAILED(rv)) return rv;

  
  
  
  
  nsFont font("", 0, 0, 0, 0, 0);
  NS_NAMED_LITERAL_CSTRING(defaultKey, "font.mathfont-glyph-tables");
  rv = mathfontProp->GetStringProperty(defaultKey, font.name);
  if (NS_FAILED(rv)) return rv;

  
  nsAutoString missingFamilyList;

#if ALERT_MISSING_FONTS
  
  
  
  
  
  MathFontEnumContext context = {aPresContext, &missingFamilyList};
  font.EnumerateFamilies(MathFontEnumCallback, &context);
  
  if (!missingFamilyList.IsEmpty()) {
    AlertMissingFonts(missingFamilyList);
  }
#else
  font.EnumerateFamilies(MathFontEnumCallback, nsnull);
#endif
  return rv;
}




nsStyleContext*
nsMathMLChar::GetStyleContext() const
{
  NS_ASSERTION(!mParent, "invalid call - not allowed for child chars");
  NS_ASSERTION(mStyleContext, "chars shoud always have style context");
  return mStyleContext;
  return NS_OK;
}

void
nsMathMLChar::SetStyleContext(nsStyleContext* aStyleContext)
{
  NS_ASSERTION(!mParent, "invalid call - not allowed for child chars");
  NS_PRECONDITION(aStyleContext, "null ptr");
  if (aStyleContext != mStyleContext) {
    if (mStyleContext)
      mStyleContext->Release();
    if (aStyleContext) {
      mStyleContext = aStyleContext;
      aStyleContext->AddRef();

      
      nsMathMLChar* child = mSibling;
      while (child) {
        child->mStyleContext = mStyleContext;
        child = child->mSibling;
      }
    }
  }
}

void
nsMathMLChar::SetData(nsPresContext* aPresContext,
                      nsString&       aData)
{
  NS_ASSERTION(!mParent, "invalid call - not allowed for child chars");
  if (!gInitialized) {
    InitGlobals(aPresContext);
  }
  mData = aData;
  
  
  mOperator = -1;
  mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
  mBoundingMetrics.Clear();
  mGlyphTable = nsnull;
  
  if (gGlyphTableList && (1 == mData.Length())) {
    mOperator = nsMathMLOperators::FindStretchyOperator(mData[0]);
    if (mOperator >= 0) {
      mDirection = nsMathMLOperators::GetStretchyDirectionAt(mOperator);
      
      mGlyphTable = gGlyphTableList->GetGlyphTableFor(aPresContext, this);
      
      
      if (!mGlyphTable) { 
        
        nsMathMLOperators::DisableStretchyOperatorAt(mOperator);
        mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
        mOperator = -1;
      }
    }
  }
}
























































































#define NS_MATHML_DELIMITER_FACTOR             0.901f
#define NS_MATHML_DELIMITER_SHORTFALL_POINTS   5.0f

static PRBool
IsSizeOK(nsPresContext* aPresContext, nscoord a, nscoord b, PRUint32 aHint)
{
  
  
  
  
  PRBool isNormal =
    (aHint & NS_STRETCH_NORMAL)
    && PRBool(float(PR_ABS(a - b))
              < (1.0f - NS_MATHML_DELIMITER_FACTOR) * float(b));
  
  
  
  PRBool isNearer = PR_FALSE;
  if (aHint & (NS_STRETCH_NEARER | NS_STRETCH_LARGEOP)) {
    float c = PR_MAX(float(b) * NS_MATHML_DELIMITER_FACTOR,
                     float(b) - aPresContext->PointsToAppUnits(NS_MATHML_DELIMITER_SHORTFALL_POINTS));
    isNearer = PRBool(float(PR_ABS(b - a)) <= (float(b) - c));
  }
  
  
  PRBool isSmaller =
    (aHint & NS_STRETCH_SMALLER)
    && PRBool((float(a) >= (NS_MATHML_DELIMITER_FACTOR * float(b)))
              && (a <= b));
  
  
  PRBool isLarger =
    (aHint & (NS_STRETCH_LARGER | NS_STRETCH_LARGEOP))
    && PRBool(a >= b);
  return (isNormal || isSmaller || isNearer || isLarger);
}

static PRBool
IsSizeBetter(nscoord a, nscoord olda, nscoord b, PRUint32 aHint)
{
  if (0 == olda)
    return PR_TRUE;
  if (aHint & (NS_STRETCH_LARGER | NS_STRETCH_LARGEOP))
    return (a >= olda) ? (olda < b) : (a >= b);
  if (aHint & NS_STRETCH_SMALLER)
    return (a <= olda) ? (olda > b) : (a <= b);

  
  return PR_ABS(a - b) < PR_ABS(olda - b);
}





static nscoord
ComputeSizeFromParts(nsPresContext* aPresContext,
                     nsGlyphCode* aGlyphs,
                     nscoord*     aSizes,
                     nscoord      aTargetSize,
                     PRUint32     aHint)
{
  enum {first, middle, last, glue};
  
  nscoord sum = 0;
  for (PRInt32 i = first; i <= last; i++) {
    if (aGlyphs[i] != aGlyphs[glue]) {
      sum += aSizes[i];
    }
  }

  
  const float flex = 0.901f;
  nscoord minSize = NSToCoordRound(flex * sum);

  if (minSize > aTargetSize)
    return minSize; 

  
  
  const PRInt32 maxGlyphs = 1000;
  
  
  nscoord maxSize = sum + maxGlyphs * aSizes[glue];
  if (maxSize < aTargetSize)
    return maxSize; 

  
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

  static const PRUnichar kSingleQuote  = PRUnichar('\'');
  static const PRUnichar kDoubleQuote  = PRUnichar('\"');
  static const PRUnichar kComma        = PRUnichar(',');

  const PRUnichar *p_begin, *p_end;
  aFontName.BeginReading(p_begin);
  aFontName.EndReading(p_end);

  const PRUnichar *p = p_begin;
  const PRUnichar *p_name = nsnull;
  while (p < p_end) {
    while (nsCRT::IsAsciiSpace(*p))
      if (++p == p_end)
        goto insert;

    p_name = p;
    if (*p == kSingleQuote || *p == kDoubleQuote) {
      
      PRUnichar quoteMark = *p;
      if (++p == p_end)
        goto insert;

      
      while (*p != quoteMark)
        if (++p == p_end)
          goto insert;

      while (++p != p_end && *p != kComma)
         ;

    } else {
      
      const PRUnichar *nameStart = p;
      while (++p != p_end && *p != kComma)
         ;

      nsAutoString family;
      family = Substring(nameStart, p);
      family.CompressWhitespace(PR_FALSE, PR_TRUE);

      PRUint8 generic;
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


static void
SetFontFamily(nsIRenderingContext& aRenderingContext,
              nsFont&              aFont,
              const nsGlyphTable*  aGlyphTable,
              const nsGlyphCode&   aGlyphCode,
              const nsAString&     aDefaultFamily)
{
  const nsAString& family =
    aGlyphCode.font ? aGlyphTable->FontNameFor(aGlyphCode) : aDefaultFamily;
  if (! family.Equals(aFont.name)) {
    aFont.name = family;
    aRenderingContext.SetFont(aFont, nsnull);
  }
}




PRBool
nsMathMLChar::TryVariants(nsPresContext*       aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          nsGlyphTable*        aGlyphTable,
                          nscoord              aTargetSize,
                          PRUint32             aStretchHint,
                          const nsAString&     aFamily)
{
  
  nsFont font = mStyleContext->GetStyleFont()->mFont;
  
  font.name.Truncate();

  PRBool isVertical = (mDirection == NS_STRETCH_DIRECTION_VERTICAL);
  PRBool largeop = (NS_STRETCH_LARGEOP & aStretchHint) != 0;
  PRBool largeopOnly = (NS_STRETCH_LARGEOP == aStretchHint); 

  nscoord bestSize =
    isVertical ? mBoundingMetrics.ascent + mBoundingMetrics.descent
               : mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing;
  PRBool haveBetter = PR_FALSE;

  
  PRInt32 size = 1; 
  if (largeop && aGlyphTable->BigOf(aPresContext, this, 2).Exists()) {
    size = 2;
  }
#ifdef NOISY_SEARCH
  printf("  searching in %s ...\n",
           NS_LossyConvertUTF16toASCII(aFamily).get());
#endif

  nsGlyphCode ch;
  while ((ch = aGlyphTable->BigOf(aPresContext, this, size)).Exists()) {

    SetFontFamily(aRenderingContext, font, aGlyphTable, ch, aFamily);

    NS_ASSERTION(ch.code != mGlyph.code || ! font.name.Equals(mFamily),
                 "glyph table incorrectly set -- duplicate found");

    nsBoundingMetrics bm;
    nsresult rv = aRenderingContext.GetBoundingMetrics(&ch.code, 1, bm);
    if (NS_SUCCEEDED(rv)) {
      nscoord charSize =
        isVertical ? bm.ascent + bm.descent
                   : bm.rightBearing - bm.leftBearing;

      if (largeopOnly ||
          IsSizeBetter(charSize, bestSize, aTargetSize, aStretchHint)) {
        haveBetter = PR_TRUE;
        bestSize = charSize;
        mGlyphTable = aGlyphTable;
        mGlyph = ch;
        mBoundingMetrics = bm;
        mFamily = font.name;
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
    }

    
    if (largeopOnly) break;
    ++size;
  }

  return haveBetter &&
    (largeopOnly || IsSizeOK(aPresContext, bestSize, aTargetSize, aStretchHint));
}




PRBool
nsMathMLChar::TryParts(nsPresContext*       aPresContext,
                       nsIRenderingContext& aRenderingContext,
                       nsGlyphTable*        aGlyphTable,
                       nscoord              aTargetSize,
                       PRUint32             aStretchHint,
                       const nsAString&     aFamily)
{
  if (!aGlyphTable->HasPartsOf(aPresContext, this))
    return PR_FALSE; 

  
  if (aGlyphTable->IsComposite(aPresContext, this)) {
    
    nsBoundingMetrics compositeSize;
    nsresult rv = ComposeChildren(aPresContext, aRenderingContext, aGlyphTable,
                                  aTargetSize, compositeSize, aStretchHint);
#ifdef NOISY_SEARCH
    printf("    Composing %d chars in font %s %s!\n",
           aGlyphTable->ChildCountOf(aPresContext, this),
           NS_LossyConvertUTF16toASCII(fontName).get(),
           NS_SUCCEEDED(rv)? "OK" : "Rejected");
#endif
    if (NS_FAILED(rv))
      return PR_FALSE; 

    
    mGlyph = kNullGlyph; 
    mGlyphTable = aGlyphTable;
    mBoundingMetrics = compositeSize;
    return PR_TRUE; 
  }

  

  
  nsFont font = mStyleContext->GetStyleFont()->mFont;
  
  font.name.Truncate();

  
  PRInt32 i;
  nsGlyphCode chdata[4];
  nsBoundingMetrics bmdata[4];
  nscoord sizedata[4];
  nsGlyphCode glue = aGlyphTable->GlueOf(aPresContext, this);

  PRBool isVertical = (mDirection == NS_STRETCH_DIRECTION_VERTICAL);

  for (i = 0; i < 4; i++) {
    nsGlyphCode ch;
    switch (i) {
    case 0: ch = aGlyphTable->TopOf(aPresContext, this);    break;
    case 1: ch = aGlyphTable->MiddleOf(aPresContext, this); break;
    case 2: ch = aGlyphTable->BottomOf(aPresContext, this); break;
    case 3: ch = glue;                                     break;
    }
    
    if (!ch.Exists()) ch = glue;
    nsBoundingMetrics bm;
    chdata[i] = ch;
    if (!ch.Exists()) {
      
      
      sizedata[i] = aTargetSize;
    }
    else {
      SetFontFamily(aRenderingContext, font, aGlyphTable, ch, aFamily);
      nsresult rv = aRenderingContext.GetBoundingMetrics(&ch.code, 1, bm);
      if (NS_FAILED(rv)) {
        
        NS_WARNING("GetBoundingMetrics failed");
        return PR_FALSE; 
      }

      
      
      
      bmdata[i] = bm;
      sizedata[i] = isVertical ? bm.ascent + bm.descent
                               : bm.rightBearing - bm.leftBearing;
    }
  }

  
  
  nscoord computedSize = ComputeSizeFromParts(aPresContext, chdata, sizedata,
                                              aTargetSize, aStretchHint);

  nscoord currentSize =
    isVertical ? mBoundingMetrics.ascent + mBoundingMetrics.descent
               : mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing;

  if (!IsSizeBetter(computedSize, currentSize, aTargetSize, aStretchHint)) {
#ifdef NOISY_SEARCH
    printf("    Font %s Rejected!\n",
           NS_LossyConvertUTF16toASCII(fontName).get());
#endif
    return PR_FALSE; 
  }

#ifdef NOISY_SEARCH
  printf("    Font %s Current best!\n",
         NS_LossyConvertUTF16toASCII(fontName).get());
#endif

  
  
  if (isVertical) {
    nscoord lbearing = bmdata[0].leftBearing;
    nscoord rbearing = bmdata[0].rightBearing;
    nscoord width = bmdata[0].width;
    for (i = 1; i < 4; i++) {
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
    for (i = 1; i < 4; i++) {
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
  
  mGlyph = kNullGlyph; 
  mGlyphTable = aGlyphTable;
  mFamily = aFamily;

  return IsSizeOK(aPresContext, computedSize, aTargetSize, aStretchHint);
}

struct StretchEnumContext {
  StretchEnumContext(nsMathMLChar*        aChar,
                     nsPresContext*       aPresContext,
                     nsIRenderingContext& aRenderingContext,
                     nscoord              aTargetSize,
                     PRUint32             aStretchHint,
                     const nsAString&     aFamilies) :
    mChar(aChar),
    mPresContext(aPresContext),
    mRenderingContext(aRenderingContext),
    mTargetSize(aTargetSize),
    mStretchHint(aStretchHint),
    mFamilies(aFamilies),
    mTryVariants(PR_TRUE),
    mTryParts(PR_TRUE)
  {
  }

  nsMathMLChar*        mChar;
  nsPresContext*       mPresContext;
  nsIRenderingContext& mRenderingContext;
  const nscoord        mTargetSize;
  const PRUint32       mStretchHint;
  
  const nsAString& mFamilies;

  PRBool         mTryVariants;
  PRBool         mTryParts;

  nsAutoTArray<nsGlyphTable*,16> mTablesTried;
  nsGlyphTable*  mGlyphTable; 
};




PRBool
nsMathMLChar::StretchResolverCallback (const nsAString& aFamily, void *aData)
{
  StretchEnumContext* context = static_cast<StretchEnumContext*>(aData);
  nsGlyphTable* glyphTable = context->mGlyphTable;

  
  context->mTablesTried.AppendElement(glyphTable);

  
  
  
  const nsAString& family = glyphTable == &gGlyphTableList->mUnicodeTable ?
    context->mFamilies : aFamily;

  if(context->mTryVariants) {
    PRBool isOK = context->mChar->
      TryVariants(context->mPresContext, context->mRenderingContext,
                  glyphTable, context->mTargetSize, context->mStretchHint,
                  family);
    if (isOK)
      return PR_FALSE; 
  }

  if(context->mTryParts) {
    PRBool isOK = context->mChar->
      TryParts(context->mPresContext, context->mRenderingContext,
               glyphTable, context->mTargetSize, context->mStretchHint,
               family);
    if (isOK)
      return PR_FALSE; 
  }
  return PR_TRUE;
}


PRBool
nsMathMLChar::StretchEnumCallback(const nsString& aFamily, PRBool aGeneric,
                                  void *aData)
{
  StretchEnumContext* context = static_cast<StretchEnumContext*>(aData);

  
  
  nsGlyphTable* glyphTable = aGeneric ?
    &gGlyphTableList->mUnicodeTable : gGlyphTableList->GetGlyphTableFor(aFamily);

  if (context->mTablesTried.IndexOf(glyphTable) != context->mTablesTried.NoIndex)
    return PR_TRUE; 

  context->mGlyphTable = glyphTable;

  if (aGeneric)
    return StretchResolverCallback(aFamily, aData);

  PRBool aborted;
  gfxPlatform *pf = gfxPlatform::GetPlatform();
  nsresult rv =
    pf->ResolveFontName(aFamily, StretchResolverCallback, aData, aborted);
  return NS_SUCCEEDED(rv) && !aborted; 
}

nsresult
nsMathMLChar::Stretch(nsPresContext*      aPresContext,
                      nsIRenderingContext& aRenderingContext,
                      nsStretchDirection   aStretchDirection,
                      nsBoundingMetrics&   aContainerSize,
                      nsBoundingMetrics&   aDesiredStretchSize,
                      PRUint32             aStretchHint)
{
  nsresult rv = NS_OK;
  nsStretchDirection direction = aStretchDirection;

  
  
  
  if (mOperator >= 0) {
    
    mDirection = nsMathMLOperators::GetStretchyDirectionAt(mOperator);
  }

  
  if (direction == NS_STRETCH_DIRECTION_DEFAULT) {
    direction = mDirection;
  }

  
  
  
  nsAutoString fontName;
  nsFont font = mStyleContext->GetParent()->GetStyleFont()->mFont;

  
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
  nsAutoString families;
  if (GetFontExtensionPref(prefBranch, mData[0], eExtension_base,
                           families)) {
    font.name = families;
    mFamily = families;
  }
  else {
    
    
    mFamily.Truncate();
  }

  aRenderingContext.SetFont(font, nsnull);
  rv = aRenderingContext.GetBoundingMetrics(mData.get(),
                                            PRUint32(mData.Length()),
                                            mBoundingMetrics);
  if (NS_FAILED(rv)) {
    NS_WARNING("GetBoundingMetrics failed");
    
    
    mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
    return rv;
  }

  
  aDesiredStretchSize = mBoundingMetrics;

  
  if (!mGlyphTable || (mDirection != direction)) {
    
    
    mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
    return NS_OK;
  }

  
  PRBool largeop = (NS_STRETCH_LARGEOP & aStretchHint) != 0;
  PRBool largeopOnly = (NS_STRETCH_LARGEOP == aStretchHint); 

  
  
  

  nscoord targetSize, charSize;
  PRBool isVertical = (direction == NS_STRETCH_DIRECTION_VERTICAL);
  if (isVertical) {
    charSize = aDesiredStretchSize.ascent + aDesiredStretchSize.descent;
    targetSize = aContainerSize.ascent + aContainerSize.descent;
  }
  else {
    charSize = aDesiredStretchSize.width;
    targetSize = aContainerSize.width;
  }
  
  if ((targetSize <= 0) || 
      (!largeop && ((isVertical && charSize >= targetSize) ||
                     IsSizeOK(aPresContext, charSize, targetSize, aStretchHint)))) {
    
    
    mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
    return NS_OK;
  }

  
  
  

  font = mStyleContext->GetStyleFont()->mFont;
  nsAutoString cssFamilies;
  cssFamilies = font.name;

  mGlyph.font = -1; 
  PRBool done = PR_FALSE;

  
  if (GetFontExtensionPref(prefBranch, mData[0], eExtension_variants,
                           families)) {
    font.name = families;

    StretchEnumContext enumData(this, aPresContext, aRenderingContext,
                                targetSize, aStretchHint, font.name);
    enumData.mTryParts = PR_FALSE;

    done = ! font.EnumerateFamilies(StretchEnumCallback, &enumData);
  }

  
  if (!done && !largeopOnly
      && GetFontExtensionPref(prefBranch, mData[0], eExtension_parts,
                              families)) {
    font.name = families;

    StretchEnumContext enumData(this, aPresContext, aRenderingContext,
                                targetSize, aStretchHint, font.name);
    enumData.mTryVariants = PR_FALSE;

    done = ! font.EnumerateFamilies(StretchEnumCallback, &enumData);
  }

  if (!done) { 
    
    font.name = cssFamilies;
    NS_NAMED_LITERAL_CSTRING(defaultKey, "font.mathfont-family");
    nsAutoString fallbackFonts;
    if (GetPrefValue(prefBranch, defaultKey.get(), fallbackFonts)) {
      AddFallbackFonts(font.name, fallbackFonts);
    }

#ifdef NOISY_SEARCH
    printf("Searching in "%s" for a glyph of appropriate size for: 0x%04X:%c\n",
           font.name, mData[0], mData[0]&0x00FF);
#endif
    StretchEnumContext enumData(this, aPresContext, aRenderingContext,
                                targetSize, aStretchHint, font.name);
    enumData.mTryParts = ! largeopOnly;

    font.EnumerateFamilies(StretchEnumCallback, &enumData);
  }

  if (mGlyph.font == -1) { 
    
    
    mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
  }
  else {
    
    aDesiredStretchSize = mBoundingMetrics;
  }
  return NS_OK;
}

nsresult
nsMathMLChar::ComposeChildren(nsPresContext*      aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsGlyphTable*        aGlyphTable,
                              nscoord              aTargetSize,
                              nsBoundingMetrics&   aCompositeSize,
                              PRUint32             aStretchHint)
{
  PRInt32 i = 0;
  nsMathMLChar* child;
  PRInt32 count = aGlyphTable->ChildCountOf(aPresContext, this);
  NS_ASSERTION(count, "something is wrong somewhere");
  if (!count) return NS_ERROR_FAILURE;
  
  
  nsMathMLChar* last = this;
  while ((i < count) && last->mSibling) {
    i++;
    last = last->mSibling;
  }
  while (i < count) {
    child = new nsMathMLChar(this);
    if (!child) {
      if (mSibling) delete mSibling; 
      mSibling = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
    last->mSibling = child;
    last = child;
    i++;
  }
  if (last->mSibling) {
    delete last->mSibling;
    last->mSibling = nsnull;
  }
  
  nsBoundingMetrics splitSize;
  if (NS_STRETCH_DIRECTION_HORIZONTAL == mDirection)
    splitSize.width = aTargetSize / count;
  else {
    splitSize.ascent = aTargetSize / (count * 2);
    splitSize.descent = splitSize.ascent;
  }
  nscoord dx = 0, dy = 0;
  for (i = 0, child = mSibling; child; child = child->mSibling, i++) {
    
    child->mData = mData;
    child->mOperator = mOperator;
    child->mDirection = mDirection;
    child->mStyleContext = mStyleContext;
    child->mGlyphTable = aGlyphTable; 
    
    nsBoundingMetrics childSize;
    nsresult rv = child->Stretch(aPresContext, aRenderingContext, mDirection,
                                 splitSize, childSize, aStretchHint);
    
    if (NS_FAILED(rv) || (NS_STRETCH_DIRECTION_UNSUPPORTED == child->mDirection)) {
      delete mSibling; 
      mSibling = nsnull;
      return NS_ERROR_FAILURE;
    }
    child->SetRect(nsRect(dx, dy, childSize.width, childSize.ascent+childSize.descent));
    if (0 == i)
      aCompositeSize = childSize;
    else {
      if (NS_STRETCH_DIRECTION_HORIZONTAL == mDirection)
        aCompositeSize += childSize;
      else {
        aCompositeSize.descent += childSize.ascent + childSize.descent;
        if (aCompositeSize.leftBearing > childSize.leftBearing)
          aCompositeSize.leftBearing = childSize.leftBearing;
        if (aCompositeSize.rightBearing < childSize.rightBearing)
          aCompositeSize.rightBearing = childSize.rightBearing;
      }
    }
    if (NS_STRETCH_DIRECTION_HORIZONTAL == mDirection)
      dx += childSize.width;
    else
      dy += childSize.ascent + childSize.descent;
  }
  return NS_OK;
}

class nsDisplayMathMLSelectionRect : public nsDisplayItem {
public:
  nsDisplayMathMLSelectionRect(nsIFrame* aFrame, const nsRect& aRect)
    : nsDisplayItem(aFrame), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLSelectionRect);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLSelectionRect() {
    MOZ_COUNT_DTOR(nsDisplayMathMLSelectionRect);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("MathMLSelectionRect")
private:
  nsRect    mRect;
};

void nsDisplayMathMLSelectionRect::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  
  nscolor bgColor = NS_RGB(0, 0, 0);
  mFrame->PresContext()->LookAndFeel()->
      GetColor(nsILookAndFeel::eColor_TextSelectBackground, bgColor);
  aCtx->SetColor(bgColor);
  aCtx->FillRect(mRect + aBuilder->ToReferenceFrame(mFrame));
}

class nsDisplayMathMLCharBackground : public nsDisplayItem {
public:
  nsDisplayMathMLCharBackground(nsIFrame* aFrame, const nsRect& aRect,
      nsStyleContext* aStyleContext)
    : nsDisplayItem(aFrame), mStyleContext(aStyleContext), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLCharBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLCharBackground() {
    MOZ_COUNT_DTOR(nsDisplayMathMLCharBackground);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("MathMLCharBackground")
private:
  nsStyleContext* mStyleContext;
  nsRect          mRect;
};

void nsDisplayMathMLCharBackground::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  const nsStyleBorder* border = mStyleContext->GetStyleBorder();
  const nsStylePadding* padding = mStyleContext->GetStylePadding();
  const nsStyleBackground* backg = mStyleContext->GetStyleBackground();
  nsCSSRendering::PaintBackgroundWithSC(mFrame->PresContext(), *aCtx, mFrame,
                                        aDirtyRect,
                                        mRect + aBuilder->ToReferenceFrame(mFrame),
                                        *backg, *border, *padding,
                                        PR_TRUE);
}

class nsDisplayMathMLCharForeground : public nsDisplayItem {
public:
  nsDisplayMathMLCharForeground(nsIFrame* aFrame, nsMathMLChar* aChar,
				PRBool aIsSelected)
    : nsDisplayItem(aFrame), mChar(aChar), mIsSelected(aIsSelected) {
    MOZ_COUNT_CTOR(nsDisplayMathMLCharForeground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLCharForeground() {
    MOZ_COUNT_DTOR(nsDisplayMathMLCharForeground);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder) {
    nsRect rect;
    mChar->GetRect(rect);
    nsPoint offset =
      aBuilder->ToReferenceFrame(mFrame) + rect.TopLeft();
    nsBoundingMetrics bm;
    mChar->GetBoundingMetrics(bm);
    return nsRect(offset.x + bm.leftBearing, offset.y,
                  bm.rightBearing - bm.leftBearing, bm.ascent + bm.descent);
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect)
  {
    mChar->PaintForeground(mFrame->PresContext(), *aCtx,
                         aBuilder->ToReferenceFrame(mFrame), mIsSelected);
  }

  NS_DISPLAY_DECL_NAME("MathMLCharForeground")

private:
  nsMathMLChar* mChar;
  PRPackedBool  mIsSelected;
};

#ifdef NS_DEBUG
class nsDisplayMathMLCharDebug : public nsDisplayItem {
public:
  nsDisplayMathMLCharDebug(nsIFrame* aFrame, const nsRect& aRect)
    : nsDisplayItem(aFrame), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLCharDebug);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLCharDebug() {
    MOZ_COUNT_DTOR(nsDisplayMathMLCharDebug);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("MathMLCharDebug")
private:
  nsRect    mRect;
};

void nsDisplayMathMLCharDebug::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  
  PRIntn skipSides = 0;
  nsPresContext* presContext = mFrame->PresContext();
  const nsStyleBorder* border = mFrame->GetStyleBorder();
  nsStyleContext* styleContext = mFrame->GetStyleContext();
  nsRect rect = mRect + aBuilder->ToReferenceFrame(mFrame);
  nsCSSRendering::PaintBorder(presContext, *aCtx, mFrame,
                              aDirtyRect, rect, *border, styleContext, skipSides);
  nsCSSRendering::PaintOutline(presContext, *aCtx, mFrame,
                               aDirtyRect, rect, *border,
                               *mFrame->GetStyleOutline(), styleContext, 0);
}
#endif


nsresult
nsMathMLChar::Display(nsDisplayListBuilder*   aBuilder,
                      nsIFrame*               aForFrame,
                      const nsDisplayListSet& aLists,
                      const nsRect*           aSelectedRect)
{
  nsresult rv = NS_OK;
  nsStyleContext* parentContext = mStyleContext->GetParent();
  nsStyleContext* styleContext = mStyleContext;

  if (NS_STRETCH_DIRECTION_UNSUPPORTED == mDirection) {
    
    
    styleContext = parentContext;
  }

  if (!styleContext->GetStyleVisibility()->IsVisible())
    return NS_OK;

  
  
  
  
  if (aSelectedRect && !aSelectedRect->IsEmpty()) {
    rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayMathMLSelectionRect(aForFrame, *aSelectedRect));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (mRect.width && mRect.height) {
    const nsStyleBackground* backg = styleContext->GetStyleBackground();
    if (styleContext != parentContext &&
        0 == (backg->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT)) {
      rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
          nsDisplayMathMLCharBackground(aForFrame, mRect, styleContext));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    
    

#if defined(NS_DEBUG) && defined(SHOW_BOUNDING_BOX)
    
    rv = aLists.BorderBackground()->AppendToTop(new (aBuilder)
        nsDisplayMathMLCharDebug(aForFrame, mRect));
    NS_ENSURE_SUCCESS(rv, rv);
#endif
  }
  return aLists.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayMathMLCharForeground(aForFrame, this,
                                      aSelectedRect && !aSelectedRect->IsEmpty()));
}

void
nsMathMLChar::PaintForeground(nsPresContext* aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsPoint aPt,
                              PRBool aIsSelected)
{
  nsStyleContext* parentContext = mStyleContext->GetParent();
  nsStyleContext* styleContext = mStyleContext;

  if (NS_STRETCH_DIRECTION_UNSUPPORTED == mDirection) {
    
    
    styleContext = parentContext;
  }

  
  nscolor fgColor = styleContext->GetStyleColor()->mColor;
  if (aIsSelected) {
    
    aPresContext->LookAndFeel()->
      GetColor(nsILookAndFeel::eColor_TextSelectForeground, fgColor);
  }
  aRenderingContext.SetColor(fgColor);

  nsAutoString fontName;
  nsFont theFont(styleContext->GetStyleFont()->mFont);
  if (! mFamily.IsEmpty()) {
    theFont.name = mFamily;
  }
  aRenderingContext.SetFont(theFont, nsnull);

  if (NS_STRETCH_DIRECTION_UNSUPPORTED == mDirection) {
    
    
    PRUint32 len = PRUint32(mData.Length());


    aRenderingContext.DrawString(mData.get(), len, mRect.x + aPt.x,
                                 mRect.y + aPt.y + mBoundingMetrics.ascent);
  }
  else {
    
    
    if (mGlyph.Exists()) {


      aRenderingContext.DrawString(&mGlyph.code, 1, mRect.x + aPt.x,
                                   mRect.y + aPt.y + mBoundingMetrics.ascent);
    }
    else { 
      
      if (!mParent && mSibling) { 
        for (nsMathMLChar* child = mSibling; child; child = child->mSibling) {


          child->PaintForeground(aPresContext, aRenderingContext, aPt,
                                 aIsSelected);
        }
        return; 
       }

      nsRect r = mRect + aPt;
      if (NS_STRETCH_DIRECTION_VERTICAL == mDirection)
        PaintVertically(aPresContext, aRenderingContext, theFont, styleContext,
                        mGlyphTable, r);
      else if (NS_STRETCH_DIRECTION_HORIZONTAL == mDirection)
        PaintHorizontally(aPresContext, aRenderingContext, theFont, styleContext,
                          mGlyphTable, r);
    }
  }
}





class AutoPushClipRect {
  nsIRenderingContext& mCtx;
public:
  AutoPushClipRect(nsIRenderingContext& aCtx, const nsRect& aRect)
    : mCtx(aCtx) {
    mCtx.PushState();
    mCtx.SetClipRect(aRect, nsClipCombine_kIntersect);
  }
  ~AutoPushClipRect() {
    mCtx.PopState();
  }
};


nsresult
nsMathMLChar::PaintVertically(nsPresContext*      aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsFont&              aFont,
                              nsStyleContext*      aStyleContext,
                              nsGlyphTable*        aGlyphTable,
                              nsRect&              aRect)
{
  nsresult rv = NS_OK;
  nsRect clipRect;
  nscoord dx, dy;

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1); 

  
  PRInt32 i;
  nsGlyphCode ch, chdata[4];
  nsBoundingMetrics bmdata[4];
  nscoord stride = 0, offset[3], start[3], end[3];
  nscoord width = aRect.width;
  nsGlyphCode glue = aGlyphTable->GlueOf(aPresContext, this);
  for (i = 0; i < 4; i++) {
    switch (i) {
      case 0: ch = aGlyphTable->TopOf(aPresContext, this);    break;
      case 1: ch = aGlyphTable->MiddleOf(aPresContext, this); break;
      case 2: ch = aGlyphTable->BottomOf(aPresContext, this); break;
      case 3: ch = glue;                                       break;
    }
    
    if (!ch.Exists()) ch = glue;
    nsBoundingMetrics bm;
    
    if (ch.Exists()) {
      SetFontFamily(aRenderingContext, aFont, aGlyphTable, ch, mFamily);
      rv = aRenderingContext.GetBoundingMetrics(&ch.code, 1, bm);
      if (NS_FAILED(rv)) {
        NS_WARNING("GetBoundingMetrics failed");
        return rv;
      }
      if (width < bm.rightBearing) width =  bm.rightBearing;
    }
    chdata[i] = ch;
    bmdata[i] = bm;
  }
  dx = aRect.x;
  for (i = 0; i < 3; i++) {
    ch = chdata[i];
    const nsBoundingMetrics &bm = bmdata[i];
    if (0 == i) { 
      dy = aRect.y + bm.ascent;
    }
    else if (1 == i) { 
      dy = aRect.y + bm.ascent + (aRect.height - (bm.ascent + bm.descent))/2;
    }
    else { 
      dy = aRect.y + aRect.height - bm.descent;
    }
    
    offset[i] = dy;
    
    start[i] = dy - bm.ascent;
    
    end[i] = dy + bm.descent; 
  }

  
  
  for (i = 0; i < 3; i++) {
    ch = chdata[i];
    
    if (ch.Exists()) {
#ifdef SHOW_BORDERS
      
      aRenderingContext.SetColor(NS_RGB(0,0,0));
      aRenderingContext.DrawRect(nsRect(dx,start[i],width+30*(i+1),end[i]-start[i]));
#endif
      dy = offset[i];
      
      if (0 == i) { 
        clipRect.SetRect(dx, aRect.y, width, aRect.height);
      }
      else if (1 == i) { 
        clipRect.SetRect(dx, end[0], width, start[2]-end[0]);
      }
      else { 
        clipRect.SetRect(dx, start[2], width, end[2]-start[2]);
      }
      if (!clipRect.IsEmpty()) {
        clipRect.Inflate(onePixel, onePixel);
        AutoPushClipRect clip(aRenderingContext, clipRect);
        SetFontFamily(aRenderingContext, aFont, aGlyphTable, ch, mFamily);
        aRenderingContext.DrawString(&ch.code, 1, dx, dy);
      }
    }
  }

  
  
  if (!glue.Exists()) { 
    
    
    
    
    
    
    nscoord lbearing, rbearing;
    PRInt32 first = 0, last = 2;
    if (chdata[1].Exists()) { 
      last = 1;
    }
    while (last <= 2) {
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
      
      nsRect rule(aRect.x + lbearing, end[first] - onePixel,
                  rbearing - lbearing, start[last] - end[first] + 2*onePixel);
      if (!rule.IsEmpty())
        aRenderingContext.FillRect(rule);
      first = last;
      last++;
    }
  }
  else if (bmdata[3].ascent + bmdata[3].descent > 0) {
    
    nscoord overlap;
    nsCOMPtr<nsIFontMetrics> fm;
    aRenderingContext.GetFontMetrics(*getter_AddRefs(fm));
    nsMathMLFrame::GetRuleThickness(fm, overlap);
    overlap = 2 * PR_MAX(overlap, onePixel);
    
    while (overlap > 0 && bmdata[3].ascent + bmdata[3].descent <= 2*overlap + onePixel)
      overlap -= onePixel;

    if (overlap > 0) {
      
      
      
      
      
      
      
      bmdata[3].ascent -= overlap;
      bmdata[3].descent -= overlap;
    }
    nscoord edge = PR_MAX(overlap, onePixel);
    SetFontFamily(aRenderingContext, aFont, aGlyphTable, glue, mFamily);

    for (i = 0; i < 2; i++) {
      PRInt32 count = 0;
      dy = offset[i];
      clipRect.SetRect(dx, end[i], width, start[i+1]-end[i]);
      clipRect.Inflate(edge, edge);
      nsBoundingMetrics bm = bmdata[i];
#ifdef SHOW_BORDERS
      
      aRenderingContext.SetColor(NS_RGB(255,0,0));
      aRenderingContext.DrawRect(clipRect);
      {
#endif
      AutoPushClipRect clip(aRenderingContext, clipRect);
      while (dy + bm.descent < start[i+1]) {
        if (count++ < 2) {
          stride = bm.descent;
          bm = bmdata[3]; 
          stride += bm.ascent;
        }
        dy += stride;
        aRenderingContext.DrawString(&glue.code, 1, dx, dy);
      }
#ifdef SHOW_BORDERS
      }
      
      nscoord height = bm.ascent + bm.descent;
      aRenderingContext.SetColor(NS_RGB(0,255,0));
      aRenderingContext.DrawRect(nsRect(dx, dy-bm.ascent, width, height));
#endif
    }
  }
#ifdef DEBUG
  else { 
    NS_ASSERTION(end[0] >= start[1] && end[1] >= start[2],
                 "gap between parts with no glue");
  }
#endif
  return NS_OK;
}


nsresult
nsMathMLChar::PaintHorizontally(nsPresContext*      aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                nsFont&              aFont,
                                nsStyleContext*      aStyleContext,
                                nsGlyphTable*        aGlyphTable,
                                nsRect&              aRect)
{
  nsresult rv = NS_OK;
  nsRect clipRect;
  nscoord dx, dy;

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  
  PRInt32 i;
  nsGlyphCode ch, chdata[4];
  nsBoundingMetrics bmdata[4];
  nscoord stride = 0, offset[3], start[3], end[3];
  dy = aRect.y;
  nsGlyphCode glue = aGlyphTable->GlueOf(aPresContext, this);
  for (i = 0; i < 4; i++) {
    switch (i) {
      case 0: ch = aGlyphTable->LeftOf(aPresContext, this);   break;
      case 1: ch = aGlyphTable->MiddleOf(aPresContext, this); break;
      case 2: ch = aGlyphTable->RightOf(aPresContext, this);  break;
      case 3: ch = glue;                                       break;
    }
    
    if (!ch.Exists()) ch = glue;
    nsBoundingMetrics bm;
    
    if (ch.Exists()) {
      SetFontFamily(aRenderingContext, aFont, aGlyphTable, ch, mFamily);
      rv = aRenderingContext.GetBoundingMetrics(&ch.code, 1, bm);
      if (NS_FAILED(rv)) {
        NS_WARNING("GetBoundingMetrics failed");
        return rv;
      }
      if (dy < aRect.y + bm.ascent) {
        dy = aRect.y + bm.ascent;
      }
    }
    chdata[i] = ch;
    bmdata[i] = bm;
  }
  for (i = 0; i < 3; i++) {
    ch = chdata[i];
    nsBoundingMetrics &bm = bmdata[i];
    if (0 == i) { 
      dx = aRect.x - bm.leftBearing;
    }
    else if (1 == i) { 
      dx = aRect.x + (aRect.width - bm.width)/2;
    }
    else { 
      dx = aRect.x + aRect.width - bm.rightBearing;
    }
    
    offset[i] = dx;
    
    start[i] = dx + bm.leftBearing;
    
    end[i] = dx + bm.rightBearing; 
  }

  
  
  for (i = 0; i < 3; i++) {
    ch = chdata[i];
    
    if (ch.Exists()) {
#ifdef SHOW_BORDERS
      aRenderingContext.SetColor(NS_RGB(255,0,0));
      aRenderingContext.DrawRect(nsRect(start[i], dy - bmdata[i].ascent,
                                 end[i] - start[i], bmdata[i].ascent + bmdata[i].descent));
#endif
      dx = offset[i];
      if (0 == i) { 
        clipRect.SetRect(dx, aRect.y, aRect.width, aRect.height);
      }
      else if (1 == i) { 
        clipRect.SetRect(end[0], aRect.y, start[2]-end[0], aRect.height);
      }
      else { 
        clipRect.SetRect(start[2], aRect.y, end[2]-start[2], aRect.height);
      }
      if (!clipRect.IsEmpty()) {
        clipRect.Inflate(onePixel, onePixel);
        AutoPushClipRect clip(aRenderingContext, clipRect);
        SetFontFamily(aRenderingContext, aFont, aGlyphTable, ch, mFamily);
        aRenderingContext.DrawString(&ch.code, 1, dx, dy);
      }
    }
  }

  
  
  if (!glue.Exists()) { 
    
    
    
    
    
    nscoord ascent, descent;
    PRInt32 first = 0, last = 2;
    if (chdata[1].Exists()) { 
      last = 1;
    }
    while (last <= 2) {
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
      
      nsRect rule(end[first] - onePixel, dy - ascent,
                  start[last] - end[first] + 2*onePixel, ascent + descent);
      if (!rule.IsEmpty())
        aRenderingContext.FillRect(rule);
      first = last;
      last++;
    }
  }
  else if (bmdata[3].rightBearing - bmdata[3].leftBearing > 0) {
    
    nscoord overlap;
    nsCOMPtr<nsIFontMetrics> fm;
    aRenderingContext.GetFontMetrics(*getter_AddRefs(fm));
    nsMathMLFrame::GetRuleThickness(fm, overlap);
    overlap = 2 * PR_MAX(overlap, onePixel);
    
    while (overlap > 0 && bmdata[3].rightBearing - bmdata[3].leftBearing <= 2*overlap + onePixel)
      overlap -= onePixel;

    if (overlap > 0) {
      
      
      bmdata[3].leftBearing += overlap;
      bmdata[3].rightBearing -= overlap;
    }
    nscoord edge = PR_MAX(overlap, onePixel);
    SetFontFamily(aRenderingContext, aFont, aGlyphTable, glue, mFamily);

    for (i = 0; i < 2; i++) {
      PRInt32 count = 0;
      dx = offset[i];
      clipRect.SetRect(end[i], aRect.y, start[i+1]-end[i], aRect.height);
      clipRect.Inflate(edge, edge);
      nsBoundingMetrics bm = bmdata[i];
#ifdef SHOW_BORDERS
      
      aRenderingContext.SetColor(NS_RGB(255,0,0));
      aRenderingContext.DrawRect(clipRect);
      {
#endif
      AutoPushClipRect clip(aRenderingContext, clipRect);
      while (dx + bm.rightBearing < start[i+1]) {
        if (count++ < 2) {
          stride = bm.rightBearing;
          bm = bmdata[3]; 
          stride -= bm.leftBearing;
        }
        dx += stride;
        aRenderingContext.DrawString(&glue.code, 1, dx, dy);
      }
#ifdef SHOW_BORDERS
      }
      
      nscoord width = bm.rightBearing - bm.leftBearing;
      aRenderingContext.SetColor(NS_RGB(0,255,0));
      aRenderingContext.DrawRect(nsRect(dx + bm.leftBearing, aRect.y, width, aRect.height));
#endif
    }
  }
#ifdef DEBUG
  else { 
    NS_ASSERTION(end[0] >= start[1] && end[1] >= start[2],
                 "gap between parts with no glue");
  }
#endif
  return NS_OK;
}
