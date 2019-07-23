





































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsUnitConversion.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsIRenderingContext.h"
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

#include "nsIDOMWindow.h"
#include "nsINonBlockingAlertService.h"
#include "nsIWindowWatcher.h"
#include "nsIStringBundle.h"
#include "nsDoubleHashtable.h"
#include "nsDisplayList.h"

#include "nsMathMLOperators.h"
#include "nsMathMLChar.h"





static const PRUnichar   kSpaceCh   = PRUnichar(' ');
static const nsGlyphCode kNullGlyph = {0, 0};
enum {eExtension_base, eExtension_variants, eExtension_parts};












































#define NS_TABLE_TYPE_UNICODE       0
#define NS_TABLE_TYPE_GLYPH_INDEX   1

#define NS_TABLE_STATE_ERROR       -1
#define NS_TABLE_STATE_EMPTY        0
#define NS_TABLE_STATE_READY        1


static nsCOMPtr<nsIPersistentProperties> gPUAProperties; 


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


static nsStretchDirection
GetStretchyDirection(PRUnichar aChar)
{
  PRInt32 k = nsMathMLOperators::FindStretchyOperator(aChar);
  return (k == kNotFound)
    ? NS_STRETCH_DIRECTION_UNSUPPORTED
    : nsMathMLOperators::GetStretchyDirectionAt(k);
}



class nsGlyphTable {
public:
  nsGlyphTable(const nsString& aPrimaryFontName)
  {
    MOZ_COUNT_CTOR(nsGlyphTable);
    mFontName.AppendString(aPrimaryFontName);
    mType = NS_TABLE_TYPE_UNICODE;
    mState = NS_TABLE_STATE_EMPTY;
    mCharCache = 0;
  }

  ~nsGlyphTable() 
  {
    MOZ_COUNT_DTOR(nsGlyphTable);
  }

  void GetPrimaryFontName(nsString& aPrimaryFontName) {
    mFontName.StringAt(0, aPrimaryFontName);
  }

  
  
  PRBool Has(nsPresContext* aPresContext, nsMathMLChar* aChar);
  PRBool Has(nsPresContext* aPresContext, PRUnichar aChar);

  
  PRBool HasVariantsOf(nsPresContext* aPresContext, nsMathMLChar* aChar);
  PRBool HasVariantsOf(nsPresContext* aPresContext, PRUnichar aChar);

  
  PRBool HasPartsOf(nsPresContext* aPresContext, nsMathMLChar* aChar);
  PRBool HasPartsOf(nsPresContext* aPresContext, PRUnichar aChar);

  
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

  
  nsresult
  GetBoundingMetrics(nsIRenderingContext& aRenderingContext,
                     nsFont&              aFont,
                     nsGlyphCode&         aGlyphCode,
                     nsBoundingMetrics&   aBoundingMetrics);
  void
  DrawGlyph(nsIRenderingContext& aRenderingContext,
            nsFont&              aFont,
            nsGlyphCode&         aGlyphCode,
            nscoord              aX,
            nscoord              aY,
            nsRect*              aClipRect = nsnull);

private:
  char   GetAnnotation(nsMathMLChar* aChar, PRInt32 aPosition);
  nsGlyphCode ElementAt(nsPresContext* aPresContext, nsMathMLChar* aChar, PRUint32 aPosition);

  
  PRInt32 mType;    
                           
  
  
  
  nsStringArray mFontName; 
                               
  
  PRInt32 mState;

  
  nsCOMPtr<nsIPersistentProperties> mGlyphProperties;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsString  mGlyphCache;
  PRUnichar mCharCache;
};

char
nsGlyphTable::GetAnnotation(nsMathMLChar* aChar, PRInt32 aPosition)
{
  NS_ASSERTION(aChar->mDirection == NS_STRETCH_DIRECTION_VERTICAL ||
               aChar->mDirection == NS_STRETCH_DIRECTION_HORIZONTAL,
               "invalid call");
  static const char kVertical[]   = "TMBG";
  static const char kHorizontal[] = "LMRG";
  if (aPosition >= 4) {
    
    return '0' + aPosition - 4;
  }
  return (aChar->mDirection == NS_STRETCH_DIRECTION_VERTICAL) ?
      kVertical[aPosition] :
      kHorizontal[aPosition];
}

nsGlyphCode
nsGlyphTable::ElementAt(nsPresContext* aPresContext, nsMathMLChar* aChar, PRUint32 aPosition)
{
  if (mState == NS_TABLE_STATE_ERROR) return kNullGlyph;
  
  if (mState == NS_TABLE_STATE_EMPTY) {
    if (!CheckFontExistence(aPresContext, *mFontName[0])) {
      mState = NS_TABLE_STATE_ERROR;
      return kNullGlyph;
    }
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
    
    
    
    
    
    
    nsAutoString buffer, puaValue;
    char puaKey[10];
    PRInt32 length = value.Length();
    for (PRInt32 i = 0, j = 0; i < length; i++, j++) {
      PRUnichar code = value[i];
      PRUnichar font = PRUnichar('0');
      
      if (code == kSpaceCh) {
        
        j = -1;
      }
      
      
      else if (code == PRUnichar(0xF8FF)) {
        PR_snprintf(puaKey, sizeof(puaKey), "%s.%c", key, GetAnnotation(aChar, j));
        rv = gPUAProperties->GetStringProperty(nsDependentCString(puaKey), puaValue);
        if (NS_FAILED(rv) || puaValue.IsEmpty()) return kNullGlyph;
        code = puaValue[0];
      }
      
      
      else if ((i+2 < length) && (value[i+1] == PRUnichar('.'))) {
        i += 2;
        
        PR_snprintf(puaKey, sizeof(puaKey), "\\u%04X.%c", code, char(value[i]));
        rv = gPUAProperties->GetStringProperty(nsDependentCString(puaKey), puaValue);
        if (NS_FAILED(rv) || puaValue.IsEmpty()) return kNullGlyph;
        code = puaValue[0];
      }
      
      if ((i+2 < length) && (value[i+1] == PRUnichar('@')) &&
          (value[i+2] >= PRUnichar('0')) && (value[i+2] <= PRUnichar('9'))) {
        i += 2;
        font = value[i];
        
        nsAutoString fontName;
        mFontName.StringAt(font-'0', fontName);
        if (!fontName.Length() || !CheckFontExistence(aPresContext, fontName)) {
          return kNullGlyph;
        }
      }
      buffer.Append(code);
      buffer.Append(font);
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
  ch.font = mGlyphCache.CharAt(index + 1) - '0'; 
  return (ch == PRUnichar(0xFFFD)) ? kNullGlyph : ch;
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
nsGlyphTable::Has(nsPresContext* aPresContext, PRUnichar aChar)
{
  nsMathMLChar tmp;
  tmp.mData = aChar;
  tmp.mDirection = GetStretchyDirection(aChar);
  return (tmp.mDirection == NS_STRETCH_DIRECTION_UNSUPPORTED)
    ? PR_FALSE
    : Has(aPresContext, &tmp);
}

PRBool
nsGlyphTable::HasVariantsOf(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  return BigOf(aPresContext, aChar, 1) != 0;
}

PRBool
nsGlyphTable::HasVariantsOf(nsPresContext* aPresContext, PRUnichar aChar)
{
  nsMathMLChar tmp;
  tmp.mData = aChar;
  tmp.mDirection = GetStretchyDirection(aChar);
  return (tmp.mDirection == NS_STRETCH_DIRECTION_UNSUPPORTED)
    ? PR_FALSE
    : HasVariantsOf(aPresContext, &tmp);
}

PRBool
nsGlyphTable::HasPartsOf(nsPresContext* aPresContext, nsMathMLChar* aChar)
{
  return  GlueOf(aPresContext, aChar) || TopOf(aPresContext, aChar) ||
          BottomOf(aPresContext, aChar) || MiddleOf(aPresContext, aChar) ||
          IsComposite(aPresContext, aChar);
}

PRBool
nsGlyphTable::HasPartsOf(nsPresContext* aPresContext, PRUnichar aChar)
{
  nsMathMLChar tmp;
  tmp.mData = aChar;
  tmp.mDirection = GetStretchyDirection(aChar);
  return (tmp.mDirection == NS_STRETCH_DIRECTION_UNSUPPORTED)
    ? PR_FALSE
    : HasPartsOf(aPresContext, &tmp);
}



nsresult
nsGlyphTable::GetBoundingMetrics(nsIRenderingContext& aRenderingContext,
                                 nsFont&              aFont,
                                 nsGlyphCode&         aGlyphCode,
                                 nsBoundingMetrics&   aBoundingMetrics)
{
  nsresult rv;
  if (aGlyphCode.font) {
    
    mFontName.StringAt(aGlyphCode.font, aFont.name);
    aRenderingContext.SetFont(aFont, nsnull);
  }

  
    rv = aRenderingContext.GetBoundingMetrics((PRUnichar*)&aGlyphCode.code, PRUint32(1), aBoundingMetrics);
  
  
  

  if (aGlyphCode.font) {
    
    mFontName.StringAt(0, aFont.name);
    aRenderingContext.SetFont(aFont, nsnull);
  }
  return rv;
}



void
nsGlyphTable::DrawGlyph(nsIRenderingContext& aRenderingContext,
                        nsFont&              aFont,
                        nsGlyphCode&         aGlyphCode,
                        nscoord              aX,
                        nscoord              aY,
                        nsRect*              aClipRect)
{
  if (aClipRect) {
    aRenderingContext.PushState();
    aRenderingContext.SetClipRect(*aClipRect, nsClipCombine_kIntersect);
  }
  if (aGlyphCode.font) {
    
    mFontName.StringAt(aGlyphCode.font, aFont.name);
    aRenderingContext.SetFont(aFont, nsnull);
  }

  
    aRenderingContext.DrawString((PRUnichar*)&aGlyphCode.code, PRUint32(1), aX, aY);
  
  
  

  if (aGlyphCode.font) {
    
    mFontName.StringAt(0, aFont.name);
    aRenderingContext.SetFont(aFont, nsnull);
  }
  if (aClipRect)
    aRenderingContext.PopState();
}




class nsBaseFontEntry : public PLDHashInt32Entry
{
public:
  nsBaseFontEntry(const void* aKey) : PLDHashInt32Entry(aKey) { }
  ~nsBaseFontEntry() { }

  nsString mFontFamily; 
};

DECL_DHASH_WRAPPER(nsBaseFontHashtable, nsBaseFontEntry, PRInt32)
DHASH_WRAPPER(nsBaseFontHashtable, nsBaseFontEntry, PRInt32)








class nsGlyphTableList : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  static nsBaseFontHashtable* gBaseFonts;

  
  
  
  
  
  
  
  
  
  
  
  static PRInt32* gParts;
  static PRInt32* gVariants;

  nsGlyphTableList()
  {
    MOZ_COUNT_CTOR(nsGlyphTableList);
    mDefaultCount = 0;
  }

  virtual ~nsGlyphTableList()
  {
    MOZ_COUNT_DTOR(nsGlyphTableList);
  }

  nsresult Initialize();
  nsresult Finalize();

  nsGlyphTable* TableAt(PRInt32 aIndex) {
    return NS_STATIC_CAST(nsGlyphTable*, mTableList.ElementAt(aIndex));
  }
  PRInt32 Count(PRBool aEverything = PR_FALSE) {
    return (aEverything) ? mTableList.Count() : mDefaultCount;
  }

  nsGlyphTable* AdditionalTableAt(PRInt32 aIndex) {
    return NS_STATIC_CAST(nsGlyphTable*, mAdditionalTableList.ElementAt(aIndex));
  }
  PRInt32 AdditionalCount() {
    return mAdditionalTableList.Count();
  }

  PRBool AppendTable(nsGlyphTable* aGlyphTable) {
    return mTableList.AppendElement(aGlyphTable);
  }

  
  nsGlyphTable*
  AddGlyphTable(const nsString& aPrimaryFontName);
  nsGlyphTable*
  AddAdditionalGlyphTable(const nsString& aPrimaryFontName);

  
  nsGlyphTable*
  GetGlyphTableFor(nsPresContext* aPresContext,
                   nsMathMLChar*   aChar);

  
  
  nsresult
  GetListFor(nsPresContext* aPresContext,
             nsMathMLChar*   aChar,
             nsFont*         aFont,
             nsVoidArray*    aGlyphTableList);

  
  
  
  
  
  
  
  nsresult
  GetPreferredListAt(nsPresContext* aPresContext,
                     PRInt32         aStartingIndex, 
                     nsVoidArray*    aGlyphTableList,
                     PRInt32*        aCount);

private:
  
  
  
  
  
  PRInt32     mDefaultCount;
  nsVoidArray mTableList;
  
  
  
  
  
  nsVoidArray mAdditionalTableList; 
};

NS_IMPL_ISUPPORTS1(nsGlyphTableList, nsIObserver)



static nsGlyphTableList* gGlyphTableList = nsnull;
nsBaseFontHashtable* nsGlyphTableList::gBaseFonts = nsnull;
PRInt32* nsGlyphTableList::gParts = nsnull;
PRInt32* nsGlyphTableList::gVariants = nsnull;

static PRBool gInitialized = PR_FALSE;


NS_IMETHODIMP
nsGlyphTableList::Observe(nsISupports*     aSubject,
                          const char* aTopic,
                          const PRUnichar* someData)
{
  Finalize();
  
  gPUAProperties = nsnull;
  return NS_OK;
}


nsresult
nsGlyphTableList::Initialize()
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIObserverService> obs = 
           do_GetService("@mozilla.org/observer-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  }
  return rv;
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
  
  PRInt32 i;
  for (i = Count() - 1; i >= 0; i--) {
    nsGlyphTable* glyphTable = TableAt(i);
    delete glyphTable;
  }
  for (i = AdditionalCount() - 1; i >= 0; i--) {
    nsGlyphTable* glyphTable = AdditionalTableAt(i);
    delete glyphTable;
  }
  
  delete gBaseFonts;
  delete [] gParts;
  delete [] gVariants;
  gParts = gVariants = nsnull;
  gInitialized = PR_FALSE;
  
  return rv;
}

nsGlyphTable*
nsGlyphTableList::AddGlyphTable(const nsString& aPrimaryFontName)
{
  
  nsGlyphTable* glyphTable = new nsGlyphTable(aPrimaryFontName);
  if (!glyphTable) return nsnull;
  mTableList.AppendElement(glyphTable);
  mDefaultCount++;
  return glyphTable;
}

nsGlyphTable*
nsGlyphTableList::AddAdditionalGlyphTable(const nsString& aPrimaryFontName)
{
  
  nsGlyphTable* glyphTable = new nsGlyphTable(aPrimaryFontName);
  if (!glyphTable) return nsnull;
  mAdditionalTableList.AppendElement(glyphTable);
  return glyphTable;
}

nsGlyphTable*
nsGlyphTableList::GetGlyphTableFor(nsPresContext* aPresContext, 
                                   nsMathMLChar*   aChar)
{
  PRInt32 i;
  for (i = 0; i < Count(); i++) {
    nsGlyphTable* glyphTable = TableAt(i);
    if (glyphTable->Has(aPresContext, aChar)) {
      return glyphTable;
    }
  }
  for (i = 0; i < AdditionalCount(); i++) {
    nsGlyphTable* glyphTable = AdditionalTableAt(i);
    if (glyphTable->Has(aPresContext, aChar)) {
      return glyphTable;
    }
  }
  return nsnull;
}

struct StretchyFontEnumContext {
  nsPresContext* mPresContext;
  nsMathMLChar*   mChar;
  nsVoidArray*    mGlyphTableList;
};



static PRBool
StretchyFontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  if (aGeneric) return PR_FALSE; 
  StretchyFontEnumContext* context = (StretchyFontEnumContext*)aData;
  nsPresContext* currPresContext = context->mPresContext;
  nsMathMLChar* currChar = context->mChar;
  nsVoidArray* currList = context->mGlyphTableList;
  
  for (PRInt32 i = 0; i < gGlyphTableList->Count(); i++) {
    nsGlyphTable* glyphTable = gGlyphTableList->TableAt(i);
    nsAutoString fontName;
    glyphTable->GetPrimaryFontName(fontName);
    if (fontName.Equals(aFamily, nsCaseInsensitiveStringComparator()) &&
        glyphTable->Has(currPresContext, currChar)) {
      currList->AppendElement(glyphTable); 
      return PR_TRUE; 
    }
  }
  return PR_TRUE; 
}

nsresult
nsGlyphTableList::GetListFor(nsPresContext* aPresContext,
                             nsMathMLChar*   aChar,
                             nsFont*         aFont,
                             nsVoidArray*    aGlyphTableList)
{
  
  
  aGlyphTableList->Clear();
  PRBool useDocumentFonts =
    aPresContext->GetCachedBoolPref(kPresContext_UseDocumentFonts);

  
  
  if (useDocumentFonts) {
    
    
    StretchyFontEnumContext context = {aPresContext, aChar, aGlyphTableList};
    aFont->EnumerateFamilies(StretchyFontEnumCallback, &context);
  }
  if (!aGlyphTableList->Count()) {
    
    PRInt32 count = Count();
    for (PRInt32 i = 0; i < count; i++) {
      nsGlyphTable* glyphTable = TableAt(i);
      if (glyphTable->Has(aPresContext, aChar)) {
        aGlyphTableList->AppendElement(glyphTable);
      }
    }
  }
  return NS_OK;
}

nsresult
nsGlyphTableList::GetPreferredListAt(nsPresContext* aPresContext,
                                     PRInt32         aStartingIndex, 
                                     nsVoidArray*    aGlyphTableList,
                                     PRInt32*        aCount)
{
  *aCount = 0;
  if (aStartingIndex == kNotFound) {
    return NS_OK;
  }
  nsAutoString fontName;
  PRInt32 index = aStartingIndex;
  NS_ASSERTION(index < Count(PR_TRUE), "invalid call");
  nsGlyphTable* glyphTable = TableAt(index);
  while (glyphTable) {
    glyphTable->GetPrimaryFontName(fontName);
    if (CheckFontExistence(aPresContext, fontName)) {
#ifdef NOISY_SEARCH
      printf("Found preferreed font %s\n",
             NS_LossyConvertUTF16toASCII(fontName).get());
#endif
      if (index == aStartingIndex) {
        
        aGlyphTableList->Clear();
      }
      aGlyphTableList->AppendElement(glyphTable);
      ++*aCount;
    }
    glyphTable = TableAt(++index);
  } 
  
  return NS_OK;
}



struct PreferredFontEnumContext {
  PRInt32   mCharIndex;
  PRBool    mIsFontForParts;
  PRInt32   mFontCount;
};


static PRBool
PreferredFontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  PRInt32 i;
  nsAutoString fontName;
  nsGlyphTable* glyphTable = nsnull;
  PreferredFontEnumContext* context = (PreferredFontEnumContext*)aData;
  
  PRBool found = PR_FALSE;
  PRInt32 count = gGlyphTableList->Count();
  for (i = 0; i < count; i++) {
    glyphTable = gGlyphTableList->TableAt(i);
    glyphTable->GetPrimaryFontName(fontName);
    if (fontName.Equals(aFamily, nsCaseInsensitiveStringComparator())) {
      found = PR_TRUE;
      break;
    }
  }
  if (!found) {
    
    
    count = gGlyphTableList->AdditionalCount();
    for (i = 0; i < count; i++) {
      glyphTable = gGlyphTableList->AdditionalTableAt(i);
      glyphTable->GetPrimaryFontName(fontName);
      if (fontName.Equals(aFamily, nsCaseInsensitiveStringComparator())) {
        found = PR_TRUE;
        break;
      }
    }
    if (!found) {
      
      glyphTable = gGlyphTableList->AddAdditionalGlyphTable(aFamily);
      if (!glyphTable)
        return PR_FALSE; 
    }
  }

  
  if (!context->mFontCount) {
    
    
    PRInt32 startingIndex = gGlyphTableList->Count(PR_TRUE);
    if (context->mIsFontForParts) {
      NS_ASSERTION(nsGlyphTableList::gParts[context->mCharIndex] == -1,
                   "remove duplicate property in mathfont.properties");
      nsGlyphTableList::gParts[context->mCharIndex] = startingIndex;
    }
    else {
      NS_ASSERTION(nsGlyphTableList::gVariants[context->mCharIndex] == -1,
                   "remove duplicate property in mathfont.properties");
      nsGlyphTableList::gVariants[context->mCharIndex] = startingIndex;
    }
  }

  gGlyphTableList->AppendTable(glyphTable);
  ++context->mFontCount;

  return PR_TRUE; 
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
GetFontExtensionPref(nsIPrefBranch* aPrefBranch, const char* aKey, 
                     PRUnichar& aChar, PRInt32& aExtension, nsString& aValue)
{
  
  aChar = 0;
  aExtension = -1;
  aValue.Truncate();

  
  
  
  
  
  
  
  

  static const char* kMathFontPrefix = "font.mathfont-family.";
  nsCAutoString alternateKey;
  alternateKey.AssignASCII(kMathFontPrefix);
  PRInt32 ucharOffset = alternateKey.Length();
  PRInt32 ucharLength = nsDependentCString(aKey + ucharOffset).FindChar('.');
  if (ucharLength < 1 || ucharLength > 6) 
    return PR_FALSE;

  PRUnichar uchar;
  if (*(aKey + ucharOffset) == '\\') { 
    PRInt32 error = 0;
    uchar = nsDependentCString(aKey + ucharOffset + 2).ToInteger(&error, 16);
    if (error) return PR_FALSE;
    NS_ConvertUTF16toUTF8 tmp(&uchar, 1);
    alternateKey.Append(tmp.get());
  }
  else { 
    NS_ConvertUTF8toUTF16 tmp(aKey + ucharOffset, ucharLength);
    uchar = tmp[0];
    char ustr[10];
    PR_snprintf(ustr, sizeof(ustr), "\\u%04X", uchar);
    alternateKey.Append(ustr);
  }

  const char* extension = aKey + ucharOffset + ucharLength;
  if (!strcmp(extension, ".base"))
    aExtension = eExtension_base;
  else if (!strcmp(extension, ".variants"))
    aExtension = eExtension_variants;
  else if (!strcmp(extension, ".parts"))
    aExtension = eExtension_parts;
  else
    return PR_FALSE;

  aChar = uchar;
  alternateKey.AppendASCII(extension);
  return GetPrefValue(aPrefBranch, aKey, aValue) ||
         GetPrefValue(aPrefBranch, alternateKey.get(), aValue);
}


static void
SetPreferredFonts(PRUnichar aChar, PRInt32 aExtension, nsString& aFamilyList)
{
  if (aChar == 0 || aExtension < 0 || aFamilyList.IsEmpty())
    return;
#ifdef DEBUG_rbs
  static const char* const kExtension[] = {".base", ".variants", ".parts"};
  printf("Setting preferred fonts for \\u%04X%s: %s\n", aChar, kExtension[aExtension],
         NS_LossyConvertUTF16toASCII(aFamilyList).get());
#endif

  if (aExtension == eExtension_base) {
    
    nsBaseFontEntry* entry = nsGlyphTableList::gBaseFonts->AddEntry(aChar);
    if (entry) {
      entry->mFontFamily = aFamilyList;
    }
    return;
  }

  
  PRInt32 k = nsMathMLOperators::FindStretchyOperator(aChar);
  if (k != kNotFound) {
    
    
    nsFont font(aFamilyList, 0, 0, 0, 0, 0);
    PreferredFontEnumContext context = {k, aExtension == eExtension_parts, 0};
    font.EnumerateFamilies(PreferredFontEnumCallback, &context);
    if (context.mFontCount) { 
      
      gGlyphTableList->AppendTable(nsnull);
    }
  }
}

struct MathFontEnumContext {
  nsPresContext* mPresContext;
  nsString*       mMissingFamilyList;
};

static PRBool
MathFontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  
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
  nsGlyphTableList::gBaseFonts = new nsBaseFontHashtable();
  if (gGlyphTableList && nsGlyphTableList::gBaseFonts) {
    nsGlyphTableList::gParts = new PRInt32[count];
    nsGlyphTableList::gVariants = new PRInt32[count];
    if (nsGlyphTableList::gParts && nsGlyphTableList::gVariants) {
      rv = gGlyphTableList->Initialize();
    }
  }
  if (NS_FAILED(rv)) {
    delete gGlyphTableList;
    delete nsGlyphTableList::gBaseFonts;
    delete [] nsGlyphTableList::gParts;
    delete [] nsGlyphTableList::gVariants;
    gGlyphTableList = nsnull;
    nsGlyphTableList::gBaseFonts = nsnull;
    nsGlyphTableList::gParts = nsnull;
    nsGlyphTableList::gVariants = nsnull;
    return rv;
  }
  





  PRUint32 i;
  for (i = 0; i < count; i++) {
    nsGlyphTableList::gParts[i] = kNotFound; 
    nsGlyphTableList::gVariants[i] = kNotFound; 
  }
  nsGlyphTableList::gBaseFonts->Init(5);

  nsCAutoString key;
  nsAutoString value;
  nsCOMPtr<nsIPersistentProperties> mathfontProp;
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));

  
  
  

  
  value.Truncate();
  rv = LoadProperties(value, mathfontProp);
  if (NS_FAILED(rv)) return rv;

  
  value.AssignLiteral("PUA");
  rv = LoadProperties(value, gPUAProperties);
  if (NS_FAILED(rv)) return rv;

  
  nsFont font("", 0, 0, 0, 0, 0);
  NS_NAMED_LITERAL_CSTRING(defaultKey, "font.mathfont-family");
  if (!GetPrefValue(prefBranch, defaultKey.get(), font.name)) {
    
    rv = mathfontProp->GetStringProperty(defaultKey, font.name);
    if (NS_FAILED(rv)) return rv;
  }

  
  nsAutoString missingFamilyList;
  MathFontEnumContext context = {aPresContext, &missingFamilyList};
  font.EnumerateFamilies(MathFontEnumCallback, &context);
  
  gGlyphTableList->AppendTable(nsnull);

  
  if (!missingFamilyList.IsEmpty()) {
    AlertMissingFonts(missingFamilyList);
  }

  

  
  PRUnichar uchar;
  PRInt32 ext;
  char **allKey = nsnull;
  prefBranch->GetChildList("font.mathfont-family.", &count, &allKey);    
  for (i = 0; i < count; ++i) {
#ifdef DEBUG_rbs
    GetPrefValue(prefBranch, allKey[i], value);
    printf("Found user pref %s: %s\n", allKey[i],
           NS_LossyConvertUTF16toASCII(value).get());
#endif
    if (GetFontExtensionPref(prefBranch, allKey[i], uchar, ext, value)) {
      SetPreferredFonts(uchar, ext, value);
    }
  }
  NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(count, allKey);

  
  nsCOMPtr<nsISimpleEnumerator> iterator;
  if (NS_SUCCEEDED(mathfontProp->Enumerate(getter_AddRefs(iterator)))) {
    PRBool more;
    while ((NS_SUCCEEDED(iterator->HasMoreElements(&more))) && more) {
      nsCOMPtr<nsIPropertyElement> element;
      if (NS_SUCCEEDED(iterator->GetNext(getter_AddRefs(element)))) {
        if (NS_SUCCEEDED(element->GetKey(key))) {
          if ((30 < key.Length()) && 
              (0 == key.Find("font.mathfont-family.\\u")) &&
               !GetFontExtensionPref(prefBranch, key.get(), uchar, ext, value) && 
              NS_SUCCEEDED(element->GetValue(value))) {
            Clean(value);
            SetPreferredFonts(uchar, ext, value);
          }
        }
      }
    }
  }
  return rv;
}




static void
SetBaseFamily(PRUnichar aChar, nsFont& aFont)
{
  if (!nsGlyphTableList::gBaseFonts) return;
  nsBaseFontEntry* entry = nsGlyphTableList::gBaseFonts->GetEntry(aChar);
  if (entry) {
    aFont.name.Assign(entry->mFontFamily);
  }
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
  if (0 == olda) return PR_TRUE;
  if (PR_ABS(a - b) < PR_ABS(olda - b)) {
    if (aHint & (NS_STRETCH_NORMAL | NS_STRETCH_NEARER))
      return PR_TRUE;
    if (aHint & NS_STRETCH_SMALLER)
      return PRBool(a < olda);
    if (aHint & (NS_STRETCH_LARGER | NS_STRETCH_LARGEOP))
      return PRBool(a > olda);
  }
  return PR_FALSE;
}





static nscoord
ComputeSizeFromParts(nsPresContext* aPresContext,
                     nsGlyphCode* aGlyphs,
                     nscoord*     aSizes,
                     nscoord      aTargetSize,
                     PRUint32     aHint)
{
  enum {first, middle, last, glue};
  float flex[] = {0.901f, 0.901f, 0.901f};
  
  if (aGlyphs[glue] == aGlyphs[middle]) flex[middle] = 0.0f;
  if (aGlyphs[glue] == aGlyphs[first]) flex[first] = 0.0f;
  if (aGlyphs[glue] == aGlyphs[last]) flex[last] = 0.0f;

  
  nscoord computedSize = nscoord(flex[first] * aSizes[first] +
                                 flex[middle] * aSizes[middle] +
                                 flex[last] * aSizes[last]);

  if (computedSize <= aTargetSize) {
    
    return aTargetSize;
  }
  if (IsSizeOK(aPresContext, computedSize, aTargetSize, aHint)) {
    
    return computedSize;
  }
  
  return 0;
}



inline void
SetFirstFamily(nsFont& aFont, const nsString& aFamily)
{
  
  aFont.name.Assign(aFamily);
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
  nsFont theFont(mStyleContext->GetParent()->GetStyleFont()->mFont);

  
  PRUnichar uchar = mData[0];
  SetBaseFamily(uchar, theFont);
  aRenderingContext.SetFont(theFont, nsnull);
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

  
  
  

  PRInt32 size;
  nsGlyphTable* glyphTable;
  nsBoundingMetrics bm;
  nsGlyphCode startingGlyph = {uchar, 0}; 
  nsGlyphCode ch;

  
  nsGlyphCode bestGlyph = startingGlyph;
  nsGlyphTable* bestGlyphTable = mGlyphTable;
  nsBoundingMetrics bestbm = mBoundingMetrics;

  
  theFont = mStyleContext->GetStyleFont()->mFont;

  
  PRBool alreadyCSS = PR_FALSE;
  nsAutoVoidArray tableList;
  
  PRInt32 count, t = nsGlyphTableList::gVariants[mOperator];
  gGlyphTableList->GetPreferredListAt(aPresContext, t, &tableList, &count);
  if (!count) {
    
    gGlyphTableList->GetListFor(aPresContext, this, &theFont, &tableList);
    alreadyCSS = PR_TRUE;
  }

#ifdef NOISY_SEARCH
  printf("Searching in %d fonts for a glyph of appropriate size for: 0x%04X:%c\n",
          tableList.Count(), uchar, uchar&0x00FF);
#endif

  count = tableList.Count();
  for (t = 0; t < count; t++) {
    
    glyphTable = NS_STATIC_CAST(nsGlyphTable*, tableList.ElementAt(t));
    
    size = 1; 
    if (largeop && glyphTable->BigOf(aPresContext, this, 2)) {
      size = 2;
    }
    glyphTable->GetPrimaryFontName(fontName);
    SetFirstFamily(theFont, fontName);
    aRenderingContext.SetFont(theFont, nsnull);
#ifdef NOISY_SEARCH
    printf("  searching in %s ...\n",
           NS_LossyConvertUTF16toASCII(fontName).get());
#endif
    ch = glyphTable->BigOf(aPresContext, this, size++);
    while (ch) {
      NS_ASSERTION(ch != uchar, "glyph table incorrectly set -- duplicate found");
      rv = glyphTable->GetBoundingMetrics(aRenderingContext, theFont, ch, bm);
      if (NS_SUCCEEDED(rv)) {
        charSize = (isVertical)
                 ? bm.ascent + bm.descent
                 : bm.rightBearing - bm.leftBearing;
        
        if (largeopOnly || IsSizeOK(aPresContext, charSize, targetSize, aStretchHint)) {
#ifdef NOISY_SEARCH
          printf("    size:%d OK!\n", size-1);
#endif
          bestbm = bm;
          bestGlyphTable = glyphTable;
          bestGlyph = ch;
          goto done; 
        }
        nscoord oldSize = (isVertical)
                        ? bestbm.ascent + bestbm.descent
                        : bestbm.rightBearing - bestbm.leftBearing;
        if (IsSizeBetter(charSize, oldSize, targetSize, aStretchHint)) {
          bestGlyphTable = glyphTable;
          bestGlyph = ch;
          bestbm = bm;
#ifdef NOISY_SEARCH
          printf("    size:%d Current best\n", size-1);
        }
        else {
          printf("    size:%d Rejected!\n", size-1);
#endif
        }
      }
      
      if (largeopOnly) break;
      ch = glyphTable->BigOf(aPresContext, this, size++);
    }
  }
  if (largeopOnly) goto done; 

  
  
  
  
  

  
  t = nsGlyphTableList::gParts[mOperator];
  gGlyphTableList->GetPreferredListAt(aPresContext, t, &tableList, &count);
  if (!count && !alreadyCSS) {
    
    
    gGlyphTableList->GetListFor(aPresContext, this, &theFont, &tableList);
  }

#ifdef NOISY_SEARCH
  printf("Searching in %d fonts for the first font with suitable parts for: 0x%04X:%c\n",
          tableList.Count(), uchar, uchar&0x00FF);
#endif

  count = tableList.Count();
  for (t = 0; t < count; t++) {
    glyphTable = NS_STATIC_CAST(nsGlyphTable*, tableList.ElementAt(t));
    if (!glyphTable->HasPartsOf(aPresContext, this)) continue; 

    
    if (glyphTable->IsComposite(aPresContext, this)) {
      
      nsBoundingMetrics compositeSize;
      rv = ComposeChildren(aPresContext, aRenderingContext, glyphTable,
                           aContainerSize, compositeSize, aStretchHint);
#ifdef NOISY_SEARCH
      printf("    Composing %d chars in font %s %s!\n",
             glyphTable->ChildCountOf(aPresContext, this),
             NS_LossyConvertUTF16toASCII(fontName).get(),
             NS_SUCCEEDED(rv)? "OK" : "Rejected");
#endif
      if (NS_FAILED(rv)) continue; 

      
      mGlyph = kNullGlyph; 
      mGlyphTable = glyphTable;
      mBoundingMetrics = compositeSize;
      aDesiredStretchSize = compositeSize;
      return NS_OK; 
    }

    
    glyphTable->GetPrimaryFontName(fontName);
    SetFirstFamily(theFont, fontName);
    aRenderingContext.SetFont(theFont, nsnull);
    
    PRInt32 i;
    nsGlyphCode chdata[4];
    nsBoundingMetrics bmdata[4];
    nscoord computedSize, sizedata[4];
    nsGlyphCode glue = glyphTable->GlueOf(aPresContext, this);
    for (i = 0; i < 4; i++) {
      switch (i) {
        case 0: ch = glyphTable->TopOf(aPresContext, this);    break;
        case 1: ch = glyphTable->MiddleOf(aPresContext, this); break;
        case 2: ch = glyphTable->BottomOf(aPresContext, this); break;
        case 3: ch = glue;                                     break;
      }
      
      if (!ch) ch = glue;
      if (!ch) { 
        bm.Clear();
      }
      else {
        rv = glyphTable->GetBoundingMetrics(aRenderingContext, theFont, ch, bm);
        if (NS_FAILED(rv)) {
          
          NS_WARNING("GetBoundingMetrics failed");
          break;
        }
      }
      chdata[i] = ch;
      bmdata[i] = bm;
      sizedata[i] = (isVertical)
                  ? bm.ascent + bm.descent
                  : bm.rightBearing - bm.leftBearing;
    }
    if (NS_FAILED(rv)) continue; 

    
    
    computedSize = ComputeSizeFromParts(aPresContext, chdata, sizedata, targetSize, aStretchHint);
#ifdef NOISY_SEARCH
    printf("    Font %s %s!\n",
           NS_LossyConvertUTF16toASCII(fontName).get(),
           (computedSize) ? "OK" : "Rejected");
#endif
    if (!computedSize) continue; 

    
    
    if (isVertical) {
      nscoord lbearing = bmdata[0].leftBearing;
      nscoord rbearing = bmdata[0].rightBearing;
      nscoord width = bmdata[0].width;
      for (i = 1; i < 4; i++) {
        bm = bmdata[i];
        if (width < bm.width) width = bm.width;
        if (lbearing > bm.leftBearing) lbearing = bm.leftBearing;
        if (rbearing < bm.rightBearing) rbearing = bm.rightBearing;
      }
      bestbm.width = width;
      bestbm.ascent = bmdata[0].ascent; 
      bestbm.descent = computedSize - bestbm.ascent;
      bestbm.leftBearing = lbearing;
      bestbm.rightBearing = rbearing;
    }
    else {
      nscoord ascent = bmdata[0].ascent;
      nscoord descent = bmdata[0].descent;
      for (i = 1; i < 4; i++) {
        bm = bmdata[i];
        if (ascent < bm.ascent) ascent = bm.ascent;
        if (descent < bm.descent) descent = bm.descent;
      }
      bestbm.width = computedSize;
      bestbm.ascent = ascent;
      bestbm.descent = descent;
      bestbm.leftBearing = 0;
      bestbm.rightBearing = computedSize;
    }
    
    bestGlyph = kNullGlyph; 
    bestGlyphTable = glyphTable;
    goto done; 
  }
#ifdef NOISY_SEARCH
  printf("    No font with suitable parts found\n");
#endif
  
  

done:
  if (bestGlyph == startingGlyph) { 
    
    
    mDirection = NS_STRETCH_DIRECTION_UNSUPPORTED;
  }
  else {
    
    mGlyph = bestGlyph; 
    mGlyphTable = bestGlyphTable;
    mBoundingMetrics = bestbm;
    aDesiredStretchSize = bestbm;
  }
  return NS_OK;
}

nsresult
nsMathMLChar::ComposeChildren(nsPresContext*      aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsGlyphTable*        aGlyphTable,
                              nsBoundingMetrics&   aContainerSize,
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
  
  nsBoundingMetrics splitSize(aContainerSize);
  if (NS_STRETCH_DIRECTION_HORIZONTAL == mDirection)
    splitSize.width /= count;
  else {
    splitSize.ascent = ((splitSize.ascent + splitSize.descent) / count) / 2;
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

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("MathMLCharForeground")
private:
  nsMathMLChar* mChar;
  PRPackedBool  mIsSelected;
};

void nsDisplayMathMLCharForeground::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  mChar->PaintForeground(mFrame->PresContext(), *aCtx,
                         aBuilder->ToReferenceFrame(mFrame), mIsSelected);
}

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

  if (NS_STRETCH_DIRECTION_UNSUPPORTED == mDirection) {
    
    
    PRUint32 len = PRUint32(mData.Length());
    if (1 == len) {
      SetBaseFamily(mData[0], theFont);
    }
    aRenderingContext.SetFont(theFont, nsnull);


    aRenderingContext.DrawString(mData.get(), len, mRect.x + aPt.x,
                                 mRect.y + aPt.y + mBoundingMetrics.ascent);
  }
  else {
    
    mGlyphTable->GetPrimaryFontName(fontName);
    SetFirstFamily(theFont, fontName);
    aRenderingContext.SetFont(theFont, nsnull);
    
    if (mGlyph) {


      mGlyphTable->DrawGlyph(aRenderingContext, theFont, mGlyph,
                             mRect.x + aPt.x,
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
                        mGlyphTable, this, r);
      else if (NS_STRETCH_DIRECTION_HORIZONTAL == mDirection)
        PaintHorizontally(aPresContext, aRenderingContext, theFont, styleContext,
                          mGlyphTable, this, r);
    }
  }
}






nsresult
nsMathMLChar::PaintVertically(nsPresContext*      aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsFont&              aFont,
                              nsStyleContext*      aStyleContext,
                              nsGlyphTable*        aGlyphTable,
                              nsMathMLChar*        aChar,
                              nsRect&              aRect)
{
  nsresult rv = NS_OK;
  nsRect clipRect;
  nscoord dx, dy;

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  
  PRInt32 i;
  nsGlyphCode ch, chdata[4];
  nsBoundingMetrics bm, bmdata[4];
  nscoord stride = 0, offset[3], start[3], end[3];
  nscoord width = aRect.width;
  nsGlyphCode glue = aGlyphTable->GlueOf(aPresContext, aChar);
  for (i = 0; i < 4; i++) {
    switch (i) {
      case 0: ch = aGlyphTable->TopOf(aPresContext, aChar);    break;
      case 1: ch = aGlyphTable->MiddleOf(aPresContext, aChar); break;
      case 2: ch = aGlyphTable->BottomOf(aPresContext, aChar); break;
      case 3: ch = glue;                                       break;
    }
    
    if (!ch) ch = glue;
    if (!ch) {
      bm.Clear();  
    }
    else {
      rv = aGlyphTable->GetBoundingMetrics(aRenderingContext, aFont, ch, bm);
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
    bm = bmdata[i];
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
    
    if (ch) {
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
        aGlyphTable->DrawGlyph(aRenderingContext, aFont, ch, dx, dy, &clipRect);
      }
    }
  }

  
  
  if (!glue) { 
    
    
    
    
    
    
    nscoord lbearing, rbearing;
    PRInt32 first = 0, last = 2;
    if (chdata[1]) { 
      last = 1;
    }
    while (last <= 2) {
      if (chdata[last]) {
        lbearing = bmdata[last].leftBearing;
        rbearing = bmdata[last].rightBearing;
        if (chdata[first]) {
          if (lbearing < bmdata[first].leftBearing)
            lbearing = bmdata[first].leftBearing;
          if (rbearing > bmdata[first].rightBearing)
            rbearing = bmdata[first].rightBearing;
        }
      }
      else if (chdata[first]) {
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
  else { 
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

    for (i = 0; i < 2; i++) {
      PRInt32 count = 0;
      dy = offset[i];
      clipRect.SetRect(dx, end[i], width, start[i+1]-end[i]);
      clipRect.Inflate(edge, edge);
#ifdef SHOW_BORDERS
      
      aRenderingContext.SetColor(NS_RGB(255,0,0));
      aRenderingContext.DrawRect(clipRect);
#endif
      aRenderingContext.PushState();
      aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
      bm = bmdata[i];
      while (dy + bm.descent < start[i+1]) {
        if (count++ < 2) {
          stride = bm.descent;
          bm = bmdata[3]; 
          stride += bm.ascent;
        }
        
        NS_ASSERTION(1000 != count, "something is probably wrong somewhere");
        if (stride < onePixel || 1000 == count) {
          aRenderingContext.PopState();
          return NS_ERROR_UNEXPECTED;
        }
        dy += stride;
        aGlyphTable->DrawGlyph(aRenderingContext, aFont, glue, dx, dy);
      }
      aRenderingContext.PopState();
#ifdef SHOW_BORDERS
      
      nscoord height = bm.ascent + bm.descent;
      aRenderingContext.SetColor(NS_RGB(0,255,0));
      aRenderingContext.DrawRect(nsRect(dx, dy-bm.ascent, width, height));
#endif
    }
  }
  return NS_OK;
}


nsresult
nsMathMLChar::PaintHorizontally(nsPresContext*      aPresContext,
                                nsIRenderingContext& aRenderingContext,
                                nsFont&              aFont,
                                nsStyleContext*      aStyleContext,
                                nsGlyphTable*        aGlyphTable,
                                nsMathMLChar*        aChar,
                                nsRect&              aRect)
{
  nsresult rv = NS_OK;
  nsRect clipRect;
  nscoord dx, dy;

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  
  PRInt32 i;
  nsGlyphCode ch, chdata[4];
  nsBoundingMetrics bm, bmdata[4];
  nscoord stride = 0, offset[3], start[3], end[3];
  dy = aRect.y;
  nsGlyphCode glue = aGlyphTable->GlueOf(aPresContext, aChar);
  for (i = 0; i < 4; i++) {
    switch (i) {
      case 0: ch = aGlyphTable->LeftOf(aPresContext, aChar);   break;
      case 1: ch = aGlyphTable->MiddleOf(aPresContext, aChar); break;
      case 2: ch = aGlyphTable->RightOf(aPresContext, aChar);  break;
      case 3: ch = glue;                                       break;
    }
    
    if (!ch) ch = glue;
    if (!ch) {
      bm.Clear();  
    }
    else {
      rv = aGlyphTable->GetBoundingMetrics(aRenderingContext, aFont, ch, bm);
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
    bm = bmdata[i];
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
    
    if (ch) {
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
        aGlyphTable->DrawGlyph(aRenderingContext, aFont, ch, dx, dy, &clipRect);
      }
    }
  }

  
  
  if (!glue) { 
    
    
    
    
    
    nscoord ascent, descent;
    PRInt32 first = 0, last = 2;
    if (chdata[1]) { 
      last = 1;
    }
    while (last <= 2) {
      if (chdata[last]) {
        ascent = bmdata[last].ascent;
        descent = bmdata[last].descent;
        if (chdata[first]) {
          if (ascent > bmdata[first].ascent)
            ascent = bmdata[first].ascent;
          if (descent > bmdata[first].descent)
            descent = bmdata[first].descent;
        }
      }
      else if (chdata[first]) {
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
  else { 
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

    for (i = 0; i < 2; i++) {
      PRInt32 count = 0;
      dx = offset[i];
      clipRect.SetRect(end[i], aRect.y, start[i+1]-end[i], aRect.height);
      clipRect.Inflate(edge, edge);
#ifdef SHOW_BORDERS
      
      aRenderingContext.SetColor(NS_RGB(255,0,0));
      aRenderingContext.DrawRect(clipRect);
#endif
      aRenderingContext.PushState();
      aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
      bm = bmdata[i];
      while (dx + bm.rightBearing < start[i+1]) {
        if (count++ < 2) {
          stride = bm.rightBearing;
          bm = bmdata[3]; 
          stride -= bm.leftBearing;
        }
        
        NS_ASSERTION(1000 != count, "something is probably wrong somewhere");
        if (stride < onePixel || 1000 == count) {
          aRenderingContext.PopState();
          return NS_ERROR_UNEXPECTED;
        }
        dx += stride;
        aGlyphTable->DrawGlyph(aRenderingContext, aFont, glue, dx, dy);
      }
      aRenderingContext.PopState();
#ifdef SHOW_BORDERS
      
      nscoord width = bm.rightBearing - bm.leftBearing;
      aRenderingContext.SetColor(NS_RGB(0,255,0));
      aRenderingContext.DrawRect(nsRect(dx + bm.leftBearing, aRect.y, width, aRect.height));
#endif
    }
  }
  return NS_OK;
}
