




































#include <Script.h>
#include "nsDeviceContextMac.h"
#include "plhash.h"
#include "nsCRT.h"

#include "nsUnicodeFontMappingMac.h"
#include "nsUnicodeFontMappingCache.h"
#include "nsUnicodeMappingUtil.h"
#include "nsIUnicodeEncoder.h"
#include "nsCompressedCharMap.h"
#include "nsMacUnicodeFontInfo.h"

#include <UnicodeConverter.h>


#include "ignorable.x-ccmap"
DEFINE_X_CCMAP(gIgnorableCCMapExt, );


static UnicodeToTextInfo gConverters[32] = { 
       nsnull, nsnull, nsnull, nsnull, nsnull, nsnull, nsnull, nsnull,
       nsnull, nsnull, nsnull, nsnull, nsnull, nsnull, nsnull, nsnull,
       nsnull, nsnull, nsnull, nsnull, nsnull, nsnull, nsnull, nsnull,
       nsnull, nsnull, nsnull, nsnull, nsnull, nsnull, nsnull, nsnull
};

static UnicodeToTextInfo
GetConverterByScript(ScriptCode sc)
{
  
  
  
  if ((sc == smArabic) || (sc == smHebrew))
     return nsnull;
  NS_PRECONDITION(sc < 32, "illegal script id");
  if(sc >= 32)
    return nsnull;
  if (gConverters[sc] != nsnull) {
    return gConverters[sc];
  }
  OSStatus err = noErr;
    
  
  TextEncoding scriptEncoding;
  err = ::UpgradeScriptInfoToTextEncoding(sc, kTextLanguageDontCare, kTextRegionDontCare, nsnull, &scriptEncoding);
  if ( noErr == err ) 
 	  err = ::CreateUnicodeToTextInfoByEncoding(scriptEncoding, &gConverters[sc] );

  if (noErr != err) 
    gConverters[sc] = nsnull;
  return gConverters[sc];
}


class nsUnicodeFontMappingEntry
{
public:
    nsUnicodeFontMappingEntry(
        nsIUnicodeEncoder *aConverter, 
        PRUint16 *aCCMap, 
        short aFontNum,
        ScriptCode aScript)
    : mConverter(aConverter),
      mCCMap(aCCMap),
      mFontNum(aFontNum),
      mScript(aScript)
    {
        NS_ASSERTION(aConverter || aScript != BAD_SCRIPT, "internal error");
    }

    PRBool Convert(
        const PRUnichar *aString, 
        ByteCount aStringLength, 
        char *aBuffer, 
        ByteCount aBufferLength,
        ByteCount& oActualLength,
        ByteCount& oBytesRead,
        OptionBits opts)
    {
        if(mConverter)
        {
            oActualLength = aBufferLength;
            if(NS_SUCCEEDED(mConverter->Convert(aString, (PRInt32*) &aStringLength, aBuffer, 
                (PRInt32*) &oActualLength)) && oActualLength)
            {
                oBytesRead = 2 * aStringLength;
                return PR_TRUE;
            }
            return PR_FALSE;
        }

        UnicodeToTextInfo converter = GetConverterByScript(mScript);
        if(converter)
        {
            OSStatus err = ::ConvertFromUnicodeToText(converter, 2 * aStringLength,
                (ConstUniCharArrayPtr) aString,
                opts, 0, NULL, 0, NULL,
                aBufferLength, &oBytesRead, &oActualLength,
                (LogicalAddress) aBuffer);
    
            return (oActualLength > 0 ? PR_TRUE : PR_FALSE);
        }
        return PR_FALSE;
    }

    PRUint16* GetCCMap()
    {
        return mCCMap;
    }

    short GetFontNum()
    {
        return mFontNum; 
    }

private:
    nsCOMPtr<nsIUnicodeEncoder> mConverter;
    PRUint16                    *mCCMap; 
    short                       mFontNum;
    ScriptCode                  mScript;
};




static void FillVarBlockToScript( PRInt8 script, PRInt8 *aMap)
{
	if(BAD_SCRIPT == aMap[kBasicLatin - kUnicodeBlockFixedScriptMax])
		aMap[kBasicLatin - kUnicodeBlockFixedScriptMax] = script;
	if(BAD_SCRIPT == aMap[kOthers - kUnicodeBlockFixedScriptMax])
		aMap[kOthers - kUnicodeBlockFixedScriptMax] = script;
	switch( script )
	{
		case smRoman: 
		case smCentralEuroRoman: 
		case smVietnamese: 
			if(BAD_SCRIPT == aMap[kLatin - kUnicodeBlockFixedScriptMax])
				aMap[kLatin - kUnicodeBlockFixedScriptMax] = script;
			break;
		case smTradChinese:
			if(BAD_SCRIPT == aMap[kCJKMisc - kUnicodeBlockFixedScriptMax])
				aMap[kCJKMisc - kUnicodeBlockFixedScriptMax] = script;
			if(BAD_SCRIPT == aMap[kCJKIdeographs - kUnicodeBlockFixedScriptMax])
				aMap[kCJKIdeographs - kUnicodeBlockFixedScriptMax] = script;
			break;
		case smKorean:
		case smJapanese:
		case smSimpChinese:
			if(BAD_SCRIPT == aMap[kCJKMisc - kUnicodeBlockFixedScriptMax])
				aMap[kCJKMisc - kUnicodeBlockFixedScriptMax] = script;
			if(BAD_SCRIPT == aMap[kCJKIdeographs - kUnicodeBlockFixedScriptMax])
				aMap[kCJKIdeographs - kUnicodeBlockFixedScriptMax] = script;
			if(BAD_SCRIPT == aMap[kHiraganaKatakana - kUnicodeBlockFixedScriptMax])
				aMap[kHiraganaKatakana - kUnicodeBlockFixedScriptMax] = script;
			break;			
	};
}

struct MyFontEnumData {
    MyFontEnumData(nsIDeviceContext* aDC, nsUnicodeFontMappingMac* fontMapping)  : mContext(aDC) {
    	mFontMapping = fontMapping;
    };
    nsIDeviceContext* mContext;
	nsUnicodeFontMappingMac* mFontMapping;
};


PRBool nsUnicodeFontMappingMac::FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  MyFontEnumData* data = (MyFontEnumData*)aData;
  nsUnicodeMappingUtil * info = nsUnicodeMappingUtil::GetSingleton();
  NS_PRECONDITION(info != nsnull, "out of memory");
  if (aGeneric)
  {
  	if(nsnull == info)
  		return PR_FALSE;
    nsGenericFontNameType type = info->MapGenericFontNameType(aFamily);

	if( type != kUnknownGenericFontName) {
	    for(ScriptCode script = 0; script < 32 ; script++)
		{
			const nsString* fontName =  info->GenericFontNameForScript(script,type);
			if(nsnull != fontName) {
			    short fontNum;
				nsDeviceContextMac::GetMacFontNumber(*fontName, fontNum);

        if(0 != fontNum)
        {
            nsUnicodeFontMappingEntry* entry = new nsUnicodeFontMappingEntry(nsnull, nsnull, fontNum, script);
            if (entry)
                data->mFontMapping->mFontList.AppendElement(entry);

            if (BAD_FONT_NUM == data->mFontMapping->mScriptFallbackFontIDs[ script ])
              data->mFontMapping->mScriptFallbackFontIDs[ script ] = fontNum;
            
        }
			}
		}
	}
  }
  else
  {
    nsAutoString realFace;
    PRBool  aliased;
    data->mContext->GetLocalFontName(aFamily, realFace, aliased);
    if (aliased || (NS_OK == data->mContext->CheckFontExistence(realFace)))
    {
	    short fontNum;
		nsDeviceContextMac::GetMacFontNumber(realFace, fontNum);
		
		if(0 != fontNum) {
            nsCOMPtr<nsIUnicodeEncoder> converter;
            PRUint16 *ccmap = nsnull;

            
            nsMacUnicodeFontInfo::GetConverterAndCCMap(realFace, getter_AddRefs(converter), &ccmap);

            ScriptCode script = (converter ? BAD_SCRIPT : ::FontToScript(fontNum));

            nsUnicodeFontMappingEntry* entry = new nsUnicodeFontMappingEntry(converter, ccmap, fontNum, script);
            if(entry)
                data->mFontMapping->mFontList.AppendElement(entry);
            
            
            if(!converter)
            {    
                if(BAD_FONT_NUM == data->mFontMapping->mScriptFallbackFontIDs[ script ]) 
                    data->mFontMapping->mScriptFallbackFontIDs[ script ] = fontNum;
                FillVarBlockToScript( script, data->mFontMapping->mPrivBlockToScript);
            }
		}
	}
  }
  return PR_TRUE;
}

PRBool nsUnicodeFontMappingMac::ConvertUnicodeToGlyphs(short aFontNum, 
    const PRUnichar* aString, ByteCount aStringLength,
    char *aBuffer, ByteCount aBufferLength, ByteCount& oActualLength,
    ByteCount& oBytesRead, OptionBits opts)
{
    for(PRInt32 i = 0; i < mFontList.Count(); i++)
    {
        nsUnicodeFontMappingEntry* entry = (nsUnicodeFontMappingEntry*) mFontList[i];
        if(aFontNum == entry->GetFontNum())
            return entry->Convert(aString, aStringLength, aBuffer, aBufferLength,
                oActualLength, oBytesRead, opts);
    }
    return PR_FALSE;
}



nsUnicodeMappingUtil *nsUnicodeFontMappingMac::gUtil = nsnull;



void nsUnicodeFontMappingMac::InitByFontFamily(nsFont* aFont, nsIDeviceContext *aDeviceContext) 
{
    MyFontEnumData fontData(aDeviceContext, this);
    aFont->EnumerateFamilies(nsUnicodeFontMappingMac::FontEnumCallback, &fontData);
}


void nsUnicodeFontMappingMac::processOneLangRegion(const char* aLanguage, const char* aRegion )
{
	if ((! nsCRT::strcmp(aLanguage,"zh")) &&
	    ((! nsCRT::strcmp(aRegion,"TW")) || (! nsCRT::strcmp(aRegion,"HK"))))
	{
		FillVarBlockToScript(smTradChinese, mPrivBlockToScript);
	} 
	else if(! nsCRT::strcmp(aLanguage,"zh"))
	{
		FillVarBlockToScript(smSimpChinese, mPrivBlockToScript);
	} 
	else if(! nsCRT::strcmp(aLanguage,"ko"))
	{
		FillVarBlockToScript(smKorean, mPrivBlockToScript);
	}
	else if(! nsCRT::strcmp(aLanguage,"ja"))
	{
		FillVarBlockToScript(smJapanese, mPrivBlockToScript);
	}
}

PRBool nsUnicodeFontMappingMac::ScriptMapInitComplete()
{
	PRInt32 i;
	for(i = kUnicodeBlockFixedScriptMax ; i < kUnicodeBlockSize; i++) {
	   if(BAD_SCRIPT == mPrivBlockToScript[i - kUnicodeBlockFixedScriptMax])
	   		return PR_FALSE;
	}
	return PR_TRUE;
}


const PRUnichar kA = PRUnichar('A');
const PRUnichar kZ = PRUnichar('Z');
const PRUnichar ka = PRUnichar('a');
const PRUnichar kz = PRUnichar('z');
const PRUnichar kComma = PRUnichar(',');
const PRUnichar kUnderline = PRUnichar('_');
const PRUnichar kSpace = PRUnichar(' ');
const PRUnichar kNullCh       = PRUnichar('\0');


void nsUnicodeFontMappingMac::InitByLANG(const nsString& aLANG)
{
	
	if( ScriptMapInitComplete() )
		return;
  	const PRUnichar *p = aLANG.get();
  	PRUint32 len = aLANG.Length();
  	char language[3];
  	char region[3];
  	language[2] = region[2]= '\0';;
  	language[0]= language[1] = region[0]= region[1] = ' ';
	PRUint32 state = 0;
	
	for(PRUint32 i = 0; (state != -1) && (i < len); i++, p++)
  	{
  		switch (state) {
  			case 0:
  				if(( ka <= *p) && (*p <= kz )) {
  					language[state++] = (char)*p;
  				} else if( kSpace == *p) {
  					state = 0;
  				} else {
  					state = -1;
  				}
  				break;
   			case 1:
  				if(( ka <= *p) && (*p <= kz )) {
  					language[state++] = (char)*p;
  				} else {
  					state = -1;
  				}
  				break;
  			case 2:
  				if(kComma == *p) {
  					processOneLangRegion(language, region);
				  	return;
  				} else if(kUnderline == *p) {
  					state = 3;
  				} else {
  					state = -1;
  				}
   				break;
   			case 3:
   			case 4:
  				if(( kA <= *p) && (*p <= kZ )) {
  					region[state - 3] = (char)*p;
  					state++;
  				} else {
  					state = -1;
  				}
  				break;
  			case 5:
  				if(kComma == *p) {
 					processOneLangRegion(language, region);
				  	return;
  				} else {
  					state = -1;
  				}
   				break;
  		};
  	}
  	if((2 == state) || (5 == state)) {
 		processOneLangRegion(language, region);
  	}
}

void nsUnicodeFontMappingMac::InitByLangGroup(const nsString& aLangGroup)
{
	
	if( ScriptMapInitComplete() )
		return;
	if(aLangGroup.LowerCaseEqualsLiteral("x-western"))
 	{
		FillVarBlockToScript(smRoman, mPrivBlockToScript);		
 	} else if(aLangGroup.LowerCaseEqualsLiteral("zh-cn"))
 	{
		FillVarBlockToScript(smSimpChinese, mPrivBlockToScript);
 	} else if(aLangGroup.LowerCaseEqualsLiteral("ko"))
 	{
		FillVarBlockToScript(smKorean, mPrivBlockToScript);
 	} else if(aLangGroup.LowerCaseEqualsLiteral("zh-tw") ||
              aLangGroup.LowerCaseEqualsLiteral("zh-hk"))
 	{
		FillVarBlockToScript(smTradChinese, mPrivBlockToScript);
 	} else if(aLangGroup.LowerCaseEqualsLiteral("ja"))
 	{
		FillVarBlockToScript(smJapanese, mPrivBlockToScript);
 	}
}


void nsUnicodeFontMappingMac::InitDefaultScriptFonts()
{
	for(PRInt32 i = 0 ; i < smPseudoTotalScripts; i++)
	{
		
	   if(BAD_FONT_NUM == mScriptFallbackFontIDs[i])
   			mScriptFallbackFontIDs[i] = gUtil->ScriptFont(i);
    }
 	for(PRInt32 k = kUnicodeBlockFixedScriptMax ; k < kUnicodeBlockSize; k++)
	{
		
	    if(BAD_SCRIPT == mPrivBlockToScript[k - kUnicodeBlockFixedScriptMax])
	   		mPrivBlockToScript[k - kUnicodeBlockFixedScriptMax] = gUtil->BlockToScript((nsUnicodeBlock) k);
    }
}


nsUnicodeFontMappingMac::nsUnicodeFontMappingMac(
	nsFont* aFont, nsIDeviceContext *aDeviceContext, 
	const nsString& aLangGroup, const nsString& aLANG) 
{
  PRInt32 i;
  if (!gUtil)
    gUtil = nsUnicodeMappingUtil::GetSingleton();
	for(i = kUnicodeBlockFixedScriptMax ; i < kUnicodeBlockSize; i++)
	   mPrivBlockToScript[i - kUnicodeBlockFixedScriptMax] = BAD_SCRIPT;
	for(i = 0 ; i < smPseudoTotalScripts; i++)
	   mScriptFallbackFontIDs[i] = BAD_FONT_NUM;
	   
	InitByFontFamily(aFont, aDeviceContext);
	InitByLANG(aLANG);
	InitByLangGroup(aLangGroup);
	InitDefaultScriptFonts();
}


nsUnicodeFontMappingMac::~nsUnicodeFontMappingMac()
{
    for(PRInt32 i = 0; i < mFontList.Count(); i++)
    {
        nsUnicodeFontMappingEntry* entry = (nsUnicodeFontMappingEntry*) mFontList[i];
        delete entry;
    }
}


PRBool nsUnicodeFontMappingMac::Equals(const nsUnicodeFontMappingMac& aMap)
{
	PRUint32 i;
	if(&aMap != this) {
		for( i=0; i < smPseudoTotalScripts; i++)
		{
			if(mScriptFallbackFontIDs[i] != aMap.mScriptFallbackFontIDs[i])
				return PR_FALSE;
		}
		for( i=0; i < kUnicodeBlockVarScriptMax; i++)
		{
			if(mPrivBlockToScript[i] != aMap.mPrivBlockToScript[i])
				return PR_FALSE;
		}
	}
	return PR_TRUE;
}



short nsUnicodeFontMappingMac::GetFontID(PRUnichar aChar) {
    
    short firstSymbolicFont = BAD_FONT_NUM, firstNonSymbolicFont = BAD_FONT_NUM;
    PRInt32 firstSymbolicFontIndex = -1;
    
    
    if (CCMAP_HAS_CHAR_EXT(gIgnorableCCMapExt, aChar)) {
      return IGNORABLE_FONT_NUM;
    }

    
    
    for(PRInt32 i = 0; i < mFontList.Count(); i++)
    {
        nsUnicodeFontMappingEntry* entry = (nsUnicodeFontMappingEntry*) mFontList[i];
        PRUint16 *ccmap = entry->GetCCMap();
        if(ccmap && CCMAP_HAS_CHAR(ccmap, aChar))
        {
            firstSymbolicFontIndex = i;
            firstSymbolicFont = entry->GetFontNum();
            break;
        }
    }

    
	nsUnicodeBlock block = GetBlock(aChar);
	if(block < kUnicodeBlockFixedScriptMax) 
	{
		firstNonSymbolicFont = mScriptFallbackFontIDs[gUtil->BlockToScript(block)];
		
		
        if(firstSymbolicFont == BAD_FONT_NUM)
            return firstNonSymbolicFont;
        
        
        
        for(PRInt32 i = 0; i < mFontList.Count(); i++)
        {
            nsUnicodeFontMappingEntry* entry = (nsUnicodeFontMappingEntry*) mFontList[i];
            if(entry->GetFontNum() == firstNonSymbolicFont)
            {
                return (firstSymbolicFontIndex < i ? firstSymbolicFont : firstNonSymbolicFont);
            }
        }
        return firstNonSymbolicFont;
	}
	
    return (firstSymbolicFont != BAD_FONT_NUM ? firstSymbolicFont : 
		mScriptFallbackFontIDs[ mPrivBlockToScript[ block - kUnicodeBlockFixedScriptMax] ]);
}


static nsUnicodeBlock gU0xxxMap[32]=
{
 kBasicLatin, kLatin,    
 kLatin,      kLatin,    
 kOthers,     kOthers,   
 kOthers,     kGreek,    
 kCyrillic,   kCyrillic, 
 kArmenian,   kOthers,   
 kArabic,     kArabic,   
 kOthers,     kOthers,   
 kOthers,     kOthers,   
 kDevanagari, kBengali,  
 kGurmukhi,   kGujarati, 
 kOriya,      kTamil,    
 kTelugu,     kKannada,  
 kMalayalam,  kOthers ,  
 kThai,       kLao,      
 kTibetan,    kTibetan,  
};


static nsUnicodeBlock GetBlockU0XXX(PRUnichar aChar)
{
 nsUnicodeBlock res = gU0xxxMap[ (aChar >> 7) & 0x1F];
 if(res == kOthers) {
   if((0x0200 <= aChar) && ( aChar <= 0x024F ))           res =  kLatin;
   else if((0x0370 <= aChar) && ( aChar <= 0x037F ))      res =  kGreek;
   else if((0x0580 <= aChar) && ( aChar <= 0x058F ))      res =  kArmenian;
   else if((0x0590 <= aChar) && ( aChar <= 0x05ff ))      res =  kHebrew;
 } 
 return res;
}


static nsUnicodeBlock GetBlockU1XXX(PRUnichar aChar)
{
  switch( aChar & 0x0F00 )
  {
     case 0x0000: return kGeorgian;
     case 0x0100: return kHangul;
     case 0x0e00: return kLatin;
     case 0x0f00: return kGreek;
     default:   
     {
       if ((0x0200 <= aChar) && ( aChar <= 0x037c)) return kEthiopic;
       if ((0x0400 <= aChar) && ( aChar <= 0x0676)) return kCanadian;
       if ((0x0780 <= aChar) && ( aChar <= 0x07ff)) return kKhmer;
       return kOthers;
     }
  }
}


static nsUnicodeBlock GetBlockU2XXX(PRUnichar aChar)
{
  return kOthers;
}


static nsUnicodeBlock GetBlockU3XXX(PRUnichar aChar)
{
  if(aChar < 0x3040)        return kCJKMisc;
  else if(aChar < 0x3100)   return kHiraganaKatakana; 
  else if(aChar < 0x3130)   return kBopomofo; 
  else if(aChar < 0x3190)   return kHangul; 
  else if(aChar >= 0x3400)  return kCJKIdeographs; 
  else                      return kCJKMisc;
}



static nsUnicodeBlock GetBlockCJKIdeographs(PRUnichar aChar)
{
  return  kCJKIdeographs;
}


static nsUnicodeBlock GetBlockHangul(PRUnichar aChar)
{
  return  kHangul;
}


static nsUnicodeBlock GetBlockUAXXX(PRUnichar aChar)
{
  if(aChar < 0xAC00) return  kOthers;
  else               return  kHangul;
}


static nsUnicodeBlock GetBlockUDXXX(PRUnichar aChar)
{
  if(aChar < 0xD800) return  kHangul;
  else               return  kOthers;
}


static nsUnicodeBlock GetBlockUEXXX(PRUnichar aChar)
{
  return  kOthers;
}


static nsUnicodeBlock GetBlockUFXXX(PRUnichar aChar)
{
  
  if(aChar >= 0xff00) 
  {
    if(aChar < 0xff60)           return kCJKMisc;
    else if(aChar < 0xffA0)      return kHiraganaKatakana;
    else if(aChar < 0xffe0)      return kHangul;
    else                         return kOthers;    
  }

  
  if((0xf780 <= aChar) && (aChar <= 0xf7ff)) return kUserDefinedEncoding;
  else if(aChar < 0xf900)        return kOthers;
  else if(aChar < 0xfb00)        return kCJKIdeographs;
  else if(aChar < 0xfb10)        return kLatin;
  else if(aChar < 0xfb18)        return kArmenian;
  else if(aChar < 0xfb50)        return kHebrew;
  else if(aChar < 0xfe20)        return kArabic;
  else if(aChar < 0xfe70)        return kOthers;
  else                           return kArabic;
}


typedef nsUnicodeBlock (* getUnicodeBlock)(PRUnichar aChar);
static getUnicodeBlock gAllGetBlock[16] = 
{
  &GetBlockU0XXX,          
  &GetBlockU1XXX,          
  &GetBlockU2XXX,          
  &GetBlockU3XXX,          
  &GetBlockCJKIdeographs,  
  &GetBlockCJKIdeographs,  
  &GetBlockCJKIdeographs,  
  &GetBlockCJKIdeographs,  
  &GetBlockCJKIdeographs,  
  &GetBlockCJKIdeographs,  
  &GetBlockUAXXX,          
  &GetBlockHangul,         
  &GetBlockHangul,         
  &GetBlockUDXXX,          
  &GetBlockUEXXX,          
  &GetBlockUFXXX           
};


nsUnicodeBlock nsUnicodeFontMappingMac:: GetBlock(PRUnichar aChar)
{
   return (*(gAllGetBlock[(aChar >> 12)]))(aChar);
}
