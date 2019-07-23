










































#include "nsQuickSort.h" 
#include "nsFontMetricsBeOS.h" 
#include "nsIServiceManager.h" 
#include "nsICharsetConverterManager.h" 
#include "nsISaveAsCharset.h" 
#include "nsIPrefService.h" 
#include "nsIPrefBranch.h"
#include "nsCOMPtr.h" 
#include "nspr.h" 
#include "nsReadableUtils.h"
 
#include <UnicodeBlockObjects.h>

#undef USER_DEFINED 
#define USER_DEFINED "x-user-def" 
 
#undef NOISY_FONTS
#undef REALLY_NOISY_FONTS

nsFontMetricsBeOS::nsFontMetricsBeOS()
{
}

nsFontMetricsBeOS::~nsFontMetricsBeOS()
{
  if (mDeviceContext) 
  {
    
    mDeviceContext->FontMetricsDeleted(this);
    mDeviceContext = nsnull;
  }
}
 
NS_IMPL_ISUPPORTS1(nsFontMetricsBeOS, nsIFontMetrics) 
 

typedef struct nsFontEnumParamsBeOS {
  nsFontMetricsBeOS *metrics;
  nsStringArray family;
  nsVoidArray isgeneric;
} NS_FONT_ENUM_PARAMS_BEOS;


static PRBool FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  NS_FONT_ENUM_PARAMS_BEOS *param = (NS_FONT_ENUM_PARAMS_BEOS *) aData;
  param->family.AppendString(aFamily);
  param->isgeneric.AppendElement((void*) aGeneric);
  if (aGeneric)
    return PR_FALSE;

  return PR_TRUE;
}

NS_IMETHODIMP nsFontMetricsBeOS::Init(const nsFont& aFont, nsIAtom* aLangGroup,
  nsIDeviceContext* aContext)
{
  NS_ASSERTION(!(nsnull == aContext), "attempt to init fontmetrics with null device context");

  mLangGroup = aLangGroup;
  mDeviceContext = aContext;

  
  NS_FONT_ENUM_PARAMS_BEOS param;
  param.metrics = this;
  aFont.EnumerateFamilies(FontEnumCallback, &param);
 
  PRInt16  face = 0;

  mFont = aFont;

  float app2twip = aContext->DevUnitsToTwips();

  
  
  PRBool fontfound = PR_FALSE;
  PRBool isfixed = PR_FALSE;
  for (int i=0 ; i<param.family.Count() && !fontfound ; i++) 
  {
    nsString *fam = param.family.StringAt(i);
    PRBool isgeneric = ( param.isgeneric[i] ) ? PR_TRUE: PR_FALSE;
    NS_ConvertUTF16toUTF8 family(*fam);
    
    isfixed = family.Equals("monospace") || family.Equals("fixed");
    if (!isgeneric) 
    {
      
      if (count_font_styles((font_family)family.get()) <= 0) 
      {
        
        continue;
      }
      mFontHandle.SetFamilyAndStyle( (font_family)family.get(), NULL );
      fontfound = PR_TRUE;
      break;
    } 
    else 
    {
      
      
      
      const char *lang;
      aLangGroup->GetUTF8String( &lang );
      char prop[256];
      snprintf( prop, sizeof(prop), "%s.%s", family.get(), lang );

      
      nsXPIDLCString real_family;
      nsresult res;
      
      nsCOMPtr<nsIPrefService> prefs = do_GetService( NS_PREFSERVICE_CONTRACTID, &res );
      if (NS_SUCCEEDED(res)) 
      {
        nsCOMPtr<nsIPrefBranch> branch;
        prefs->GetBranch("font.name.", getter_AddRefs(branch));
        branch->GetCharPref(prop, getter_Copies(real_family));

        if (!real_family.IsEmpty() && real_family.Length() <= B_FONT_FAMILY_LENGTH  && count_font_styles((font_family)real_family.get()) > 0) 
        {
          mFontHandle.SetFamilyAndStyle( (font_family)real_family.get(), NULL );
          fontfound = PR_TRUE;
          break;        
        }
      } 
      
      if (isfixed)
        mFontHandle = BFont(be_fixed_font);
      else
        mFontHandle = BFont(be_plain_font);
      fontfound = PR_TRUE;
      break;
    }
  }

  
  if (!fontfound)
  {
    if (isfixed)
      mFontHandle = BFont(be_fixed_font);
    else
      mFontHandle = BFont(be_plain_font);
  } 
 
  if (aFont.style == NS_FONT_STYLE_ITALIC)
    face |= B_ITALIC_FACE;

  if ( aFont.weight > NS_FONT_WEIGHT_NORMAL )
  {
    mIsBold = PR_TRUE;
  	face |= B_BOLD_FACE;
  }
  else
    mIsBold = PR_FALSE;
        
  
  
  if ( aFont.decorations & NS_FONT_DECORATION_UNDERLINE )
  	face |= B_UNDERSCORE_FACE;
  	
  if ( aFont.decorations & NS_FONT_DECORATION_LINE_THROUGH )
  	face |= B_STRIKEOUT_FACE;

  mFontHandle.SetFace( face );
  
  if (aFont.style == NS_FONT_STYLE_ITALIC
    && !(mFontHandle.Face() & B_ITALIC_FACE)) 
    mFontHandle.SetShear(105.0);

  mFontHandle.SetSize(mFont.size/app2twip);
  mFontHandle.SetSpacing(B_FIXED_SPACING);

#ifdef NOISY_FONTS
#ifdef DEBUG
  fprintf(stderr, "looking for font %s (%d)", wildstring, aFont.size / app2twip);
#endif
#endif
  
  
  mFontWidthCache.Init(256);
  
  RealizeFont(aContext);

  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::Destroy()
{
  mDeviceContext = nsnull;
  return NS_OK;
}


void nsFontMetricsBeOS::RealizeFont(nsIDeviceContext* aContext)
{
  float f;
  f = aContext->DevUnitsToAppUnits();
  
  struct font_height height;
  mFontHandle.GetHeight( &height );
 
  struct font_height emHeight; 
  mFontHandle.GetHeight(&emHeight);
  
 
  int lineSpacing = nscoord(height.ascent + height.descent); 
  if (lineSpacing > (emHeight.ascent + emHeight.descent))
    mLeading = nscoord((lineSpacing - (emHeight.ascent + emHeight.descent)) * f); 
  else
    mLeading = 0; 

  mEmHeight = PR_MAX(1, nscoord((emHeight.ascent + emHeight.descent) * f)); 
  mEmAscent = nscoord(height.ascent * (emHeight.ascent + emHeight.descent) * f / lineSpacing); 
  mEmDescent = mEmHeight - mEmAscent; 

  mMaxHeight = nscoord((height.ascent + 
                        height.descent) * f) ; 
  mMaxAscent = nscoord(height.ascent * f) ;
  mMaxDescent = nscoord(height.descent * f);
  
  mMaxAdvance = nscoord((mFontHandle.BoundingBox().Width()+1) * f); 

  float rawWidth = mFontHandle.StringWidth("x"); 
  mAveCharWidth = NSToCoordRound(rawWidth * f); 

  
  mXHeight = NSToCoordRound((float) height.ascent* f * 0.56f); 

  rawWidth = mFontHandle.StringWidth(" "); 
  mSpaceWidth = NSToCoordRound(rawWidth * f); 
 
 
  mUnderlineOffset = -NSToIntRound(MAX (1, floor (0.1 * (height.ascent + height.descent + height.leading) + 0.5)) * f); 
  
  mUnderlineSize = NSToIntRound(MAX(1, floor (0.05 * (height.ascent + height.descent + height.leading) + 0.5)) * f); 
 
  mSuperscriptOffset = mXHeight; 
 
  mSubscriptOffset = mXHeight; 
 
   
  mStrikeoutOffset = NSToCoordRound(mXHeight / 2.0); 
  mStrikeoutSize = mUnderlineSize; 
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetXHeight(nscoord& aResult)
{
  aResult = mXHeight;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetSuperscriptOffset(nscoord& aResult)
{
  aResult = mSuperscriptOffset;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetSubscriptOffset(nscoord& aResult)
{
  aResult = mSubscriptOffset;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mStrikeoutOffset; 
  aSize = mStrikeoutSize; 


  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetUnderline(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mUnderlineOffset;
  aSize = mUnderlineSize;
  
  
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetHeight(nscoord &aHeight)
{ 
  aHeight = mMaxHeight; 
  return NS_OK; 
} 
 
NS_IMETHODIMP  nsFontMetricsBeOS::GetNormalLineHeight(nscoord &aHeight) 
{
  aHeight = mEmHeight + mLeading; 
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetLeading(nscoord &aLeading)
{
  aLeading = mLeading;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetEmHeight(nscoord &aHeight) 
{ 
  aHeight = mEmHeight; 
  return NS_OK; 
} 
 
NS_IMETHODIMP  nsFontMetricsBeOS::GetEmAscent(nscoord &aAscent) 
{ 
  aAscent = mEmAscent; 
  return NS_OK; 
} 
 
NS_IMETHODIMP  nsFontMetricsBeOS::GetEmDescent(nscoord &aDescent) 
{ 
  aDescent = mEmDescent; 
  return NS_OK; 
} 
 
NS_IMETHODIMP  nsFontMetricsBeOS::GetMaxHeight(nscoord &aHeight) 
{ 
  aHeight = mMaxHeight; 
  return NS_OK; 
} 
 
NS_IMETHODIMP  nsFontMetricsBeOS::GetMaxAscent(nscoord &aAscent)
{
  aAscent = mMaxAscent;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetMaxDescent(nscoord &aDescent)
{
  aDescent = mMaxDescent;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetMaxAdvance(nscoord &aAdvance)
{
  aAdvance = mMaxAdvance;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsBeOS::GetAveCharWidth(nscoord &aAveCharWidth)
{
  aAveCharWidth = mAveCharWidth;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetSpaceWidth(nscoord &aSpaceWidth) 
{ 
  aSpaceWidth = mSpaceWidth; 
  return NS_OK; 
} 

NS_IMETHODIMP  nsFontMetricsBeOS::GetLangGroup(nsIAtom** aLangGroup)
{
  if (!aLangGroup)
    return NS_ERROR_NULL_POINTER;

  *aLangGroup = mLangGroup;
  NS_IF_ADDREF(*aLangGroup);

  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsBeOS::GetFontHandle(nsFontHandle &aHandle)
{
  aHandle = (nsFontHandle)&mFontHandle;
  return NS_OK;
} 
 
nsresult 
nsFontMetricsBeOS::FamilyExists(const nsString& aName) 
{ 
  NS_ConvertUTF16toUTF8 family(aName);
  printf("exists? %s", (font_family)family.get()); 
  return  (count_font_styles((font_family)family.get()) > 0) ? NS_OK : NS_ERROR_FAILURE;
} 


inline uint32 utf8_char_len(uchar byte) 
{
	return (((0xE5000000 >> ((byte >> 3) & 0x1E)) & 3) + 1);
}




inline PRUint32 utf8_to_index(char *utf8char)
{
	PRUint32 ch = 0;
	switch (utf8_char_len(*utf8char) - 1) 
	{
		case 3: ch += *utf8char++; ch <<= 6;
		case 2: ch += *utf8char++; ch <<= 6;
		case 1: ch += *utf8char++; ch <<= 6;
		case 0: ch += *utf8char++;
	}
	return ch;
}

float  nsFontMetricsBeOS::GetStringWidth(char *utf8str, uint32 bytelen)
{
	float retwidth = 0;
	uint32 charlen = 1;
	
	for (uint32 i =0; i < bytelen && *utf8str  != '\0'; i += charlen)
	{
		float width = 0;
		
		charlen = ((0xE5000000 >> ((*utf8str >> 3) & 0x1E)) & 3) + 1;
		
		PRUint32 index = utf8_to_index(utf8str);
		if (!mFontWidthCache.Get(index, &width))
		{
			width = mFontHandle.StringWidth(utf8str, charlen);
			mFontWidthCache.Put(index, width);
		}
		retwidth +=  width;
		utf8str += charlen;
	}
	if (mIsBold && !(mFontHandle.Face() & B_BOLD_FACE))
		retwidth += 1.0;
	return retwidth;
}
 

 
nsFontEnumeratorBeOS::nsFontEnumeratorBeOS() 
{ 
} 
 
NS_IMPL_ISUPPORTS1(nsFontEnumeratorBeOS, nsIFontEnumerator)
 
static int 
CompareFontNames(const void* aArg1, const void* aArg2, void* aClosure) 
{ 
  const PRUnichar* str1 = *((const PRUnichar**) aArg1); 
  const PRUnichar* str2 = *((const PRUnichar**) aArg2); 
 
  
 
  return nsCRT::strcmp(str1, str2); 
} 

static int
FontMatchesGenericType(font_family family, uint32 flags, const char* aGeneric,
  const char* aLangGroup)
{
  
  
  if(aGeneric == nsnull || aLangGroup == nsnull)
    return 1;
  if (!strcmp(aLangGroup, "ja"))    
    return 1;
  if (strstr(aLangGroup, "zh"))
    return 1;
  if (!strcmp(aLangGroup, "ko"))
    return 1;
  if (!strcmp(aLangGroup, "th"))
    return 1;
  if (!strcmp(aLangGroup, "he"))
    return 1;
  if (!strcmp(aLangGroup, "ar"))
    return 1;
  if (strstr(aLangGroup, "user-def"))
    return 1;
  if (!strcmp(aLangGroup, "unicode"))
    return 1;

  if (strstr(aGeneric, "fantasy") 
  
#if 0
    && (strstr(family, "Baskerville") || 
        strstr(family, "Chicago") ||
        strstr(family, "Copprpl") ||
        strstr(family, "Embassy") ||
        strstr(family, "Europe") ||
        strstr(family, "Garmnd") ||
        strstr(family, "Impact") ||
        strstr(family, "ProFont") ||
        strstr(family, "VAGRounded"))
#endif      
    )
    return 1;
  
  
  if (!strcmp(aGeneric, "serif") && 
     (strstr(family, "Dutch") || strstr(family, "Times") || strstr(family, "Roman") ||
      strstr(family, "CentSchbook") || strstr(family, "Georgia") || strstr(family, "Baskerville") ||
      strstr(family, "Garmnd") || strstr(family, "Cyberbit") || strcmp(family, "Haru Tohaba") == 0))
    return 1;
  if (!strcmp(aGeneric, "sans-serif") && 
     (strstr(family, "Arial") || strstr(family, "Chianti") || strstr(family, "Helv") ||
      strstr(family, "Humnst") || strstr(family, "Swiss") || strstr(family, "Tahoma") ||
      strstr(family, "Sans") || strstr(family, "sans") || strstr(family, "Verdana") || 
      strstr(family, "Zurich") || strcmp(family, "Haru") == 0))
    return 1;
  if ((strstr(aGeneric, "monospace") || strstr(aGeneric, "-moz-fixed")) && 
    (flags & B_IS_FIXED || strstr(family, "Cour") || strstr(family, "Consol") ||
     strstr(family, "Fixed") || strstr(family, "Kurier") || strstr(family, "Lucida") ||
     strstr(family, "Mono") || strstr(family, "console") || strstr(family, "mono") ||
     strstr(family, "fixed")))
    return 1;
  if (strstr(aGeneric, "cursive") && 
    (strstr(family, "Cursiv") || strstr(family, "Kursiv") || strstr(family, "Script") ||
     strstr(family, "kursiv") || strstr(family, "Embassy") || strstr(family, "script") || 
     strstr(family, "Brush")))
    return 1;

  return 0;
}

static int MatchesLangGroup(font_family family,  const char* aLangGroup) 
{
  BFont font;
  font.SetFamilyAndStyle(family, NULL);
  unicode_block lang = font.Blocks();
  int match = 0;

  
  if ((strstr(aLangGroup, "user-def") || strstr(aLangGroup, "unicode")))
    return 1; 
  
  if ((strstr(aLangGroup, "baltic") || strstr(aLangGroup, "central-euro") || strstr(aLangGroup, "western")) && 
    lang.Includes(B_LATIN1_SUPPLEMENT_BLOCK))
    return 1;
  if (strstr(aLangGroup, "tr") && lang.Includes(B_LATIN_EXTENDED_A_BLOCK))
    return 1;
  if (strstr(aLangGroup, "el") && lang.Includes(B_BASIC_GREEK_BLOCK))
    return 1;
  if (strstr(aLangGroup, "cyrillic") && lang.Includes(B_CYRILLIC_BLOCK))
    return 1;
  if (strstr(aLangGroup, "he") && lang.Includes(B_BASIC_HEBREW_BLOCK))
    return 1;
  if (strstr(aLangGroup, "ar") && lang.Includes(B_BASIC_ARABIC_BLOCK))
    return 1;
  if (strstr(aLangGroup, "th") && lang.Includes(B_THAI_BLOCK))
    return 1;
  
  if ((strstr(aLangGroup, "ja") || strstr(aLangGroup, "ko") || strstr(aLangGroup, "zh") ) &&
    (lang.Includes(B_CJK_UNIFIED_IDEOGRAPHS_BLOCK) ||
     lang.Includes(B_CJK_MISCELLANEOUS_BLOCK) ||
     lang.Includes(B_ENCLOSED_CJK_LETTERS_AND_MONTHS_BLOCK) ||
     lang.Includes(B_CJK_COMPATIBILITY_BLOCK) ||
     lang.Includes(B_CJK_COMPATIBILITY_IDEOGRAPHS_BLOCK) ||
     lang.Includes(B_CJK_COMPATIBILITY_FORMS_BLOCK))) 
    match = 1;  
  
  if (strstr(aLangGroup, "ja") && (lang.Includes(B_HIRAGANA_BLOCK) || lang.Includes(B_KATAKANA_BLOCK) ))
    match = 1; 
  if (strstr(aLangGroup, "ko") && (lang.Includes(B_HANGUL_BLOCK)))
    match = 1;   
  if (strstr(aLangGroup, "zh") && (lang.Includes(B_HIGH_SURROGATES_BLOCK) || lang.Includes(B_LOW_SURROGATES_BLOCK) ))
    match = 1; 
   

 return match; 
}

static nsresult EnumFonts(const char * aLangGroup, const char* aGeneric, PRUint32* aCount, PRUnichar*** aResult) 
{ 
  int32 numFamilies = count_font_families(); 
 
  PRUnichar** array = (PRUnichar**) nsMemory::Alloc(numFamilies * sizeof(PRUnichar*)); 
  NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

  int j = 0;
  for(int32 i = 0; i < numFamilies; i++) 
  {
    font_family family; 
    uint32 flags; 
    if (get_font_family(i, &family, &flags) == B_OK) 
    {
      if (family && (!aLangGroup || MatchesLangGroup(family,  aLangGroup)))
      {
        if(FontMatchesGenericType(family, flags, aGeneric, aLangGroup))
        {
          if (!(array[j] = UTF8ToNewUnicode(nsDependentCString(family))))
            break; 
          ++j;
        }
      }
    }
  } 
  *aCount = j; 

  if (*aCount)
  {
    *aResult = array; 
    
    if (*aCount < numFamilies)
    {
      array = (PRUnichar**) nsMemory::Realloc(array, *aCount * sizeof(PRUnichar*));
      NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);
    }
    NS_QuickSort(array, j, sizeof(PRUnichar*), CompareFontNames, nsnull);
  }
  else 
  {
    nsMemory::Free(array); 
  }

  return NS_OK; 
} 
 
NS_IMETHODIMP 
nsFontEnumeratorBeOS::EnumerateAllFonts(PRUint32* aCount, PRUnichar*** aResult) 
{ 
  NS_ENSURE_ARG_POINTER(aResult); 
  *aResult = nsnull; 
  NS_ENSURE_ARG_POINTER(aCount); 
  *aCount = 0; 
 
  return EnumFonts(nsnull, nsnull, aCount, aResult); 
} 
 
NS_IMETHODIMP 
nsFontEnumeratorBeOS::EnumerateFonts(const char* aLangGroup, 
  const char* aGeneric, PRUint32* aCount, PRUnichar*** aResult) 
{ 
  NS_ENSURE_ARG_POINTER(aResult); 
  *aResult = nsnull; 
  NS_ENSURE_ARG_POINTER(aCount); 
  *aCount = 0; 

  
  
  const char* langGroup = nsnull;
  if (aLangGroup && *aLangGroup)
    langGroup = aLangGroup;
  const char* generic = nsnull;
  if (aGeneric && *aGeneric)
    generic = aGeneric;

  return EnumFonts(langGroup, generic, aCount, aResult); 
}

NS_IMETHODIMP
nsFontEnumeratorBeOS::HaveFontFor(const char* aLangGroup, PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aLangGroup); 
  NS_ENSURE_ARG_POINTER(aResult); 
  *aResult = PR_FALSE;

  int32 numFamilies = count_font_families(); 

  for(int32 i = 0; i < numFamilies; i++) 
  {
    font_family family; 
    uint32 flags; 
    if (get_font_family(i, &family, &flags) == B_OK) 
    {
      if (family && (!aLangGroup || MatchesLangGroup(family,  aLangGroup)))
      {
         *aResult = PR_TRUE;
         return NS_OK;
         
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorBeOS::GetDefaultFont(const char *aLangGroup, 
  const char *aGeneric, PRUnichar **aResult)
{
  
  

  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorBeOS::UpdateFontList(PRBool *updateFontList)
{
  *updateFontList = PR_FALSE; 
  return NS_OK;
}

