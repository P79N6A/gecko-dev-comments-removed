




































 
#include "nsMacUnicodeFontInfo.h"
#include "nsCRT.h"
#include "prmem.h"
#include <Fonts.h>


#include "nsICharRepresentable.h"
#include "nsCompressedCharMap.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsDependentString.h"
#include "nsLiteralString.h"
#include "nsDeviceContextMac.h"
#include "nsICharsetConverterManager.h"
#include "nsIPersistentProperties2.h"
#include "nsNetUtil.h"
#include "nsHashtable.h"
#include <ATSTypes.h>
#include <SFNTTypes.h>
#include <SFNTLayoutTypes.h>



#ifdef TRACK_INIT_PERFORMANCE
#include <DriverServices.h>
#endif

class nsFontCleanupObserver : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsFontCleanupObserver() { }
  virtual ~nsFontCleanupObserver() {}
};

NS_IMPL_ISUPPORTS1(nsFontCleanupObserver, nsIObserver)

NS_IMETHODIMP nsFontCleanupObserver::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  if (! nsCRT::strcmp(NS_XPCOM_SHUTDOWN_OBSERVER_ID,aTopic))
  {
    nsMacUnicodeFontInfo::FreeGlobals();
  }
  return NS_OK;
}

static nsIPersistentProperties* gFontEncodingProperties = nsnull;
static nsICharsetConverterManager* gCharsetManager = nsnull;
static nsObjectHashtable* gFontMaps = nsnull;
static nsFontCleanupObserver *gFontCleanupObserver = nsnull;
static PRUint16* gCCMap = nsnull;


#ifdef IS_BIG_ENDIAN 
# undef GET_SHORT
# define GET_SHORT(p) (*((PRUint16*)p))
# undef GET_LONG
# define GET_LONG(p)  (*((PRUint32*)p))
#else
# ifdef IS_LITTLE_ENDIAN 
#  undef GET_SHORT
#  define GET_SHORT(p) (((p)[0] << 8) | (p)[1])
#  undef GET_LONG
#  define GET_LONG(p) (((p)[0] << 24) | ((p)[1] << 16) | ((p)[2] << 8) | (p)[3])
# endif
#endif


enum {
  kFMOpenTypeFontTechnology     = FOUR_CHAR_CODE('OTTO')
};


enum {
  headFontTableTag = FOUR_CHAR_CODE('head'),
  locaFontTableTag = FOUR_CHAR_CODE('loca')
};

#define ADD_GLYPH(a,b) SET_REPRESENTABLE(a,b)
#define FONT_HAS_GLYPH(a,b) IS_REPRESENTABLE(a,b)

#undef SET_SPACE
#define SET_SPACE(c) ADD_GLYPH(spaces, c)
#undef SHOULD_BE_SPACE
#define SHOULD_BE_SPACE(c) FONT_HAS_GLYPH(spaces, c)

static PRInt8
GetIndexToLocFormat(FMFont aFont)
{
  PRUint16 indexToLocFormat;
  ByteCount len = 0;
  OSStatus err = ::FMGetFontTable(aFont, headFontTableTag, 50, 2, &indexToLocFormat, nsnull);
  if (err != noErr) 
    return -1;
    
  if (!indexToLocFormat) 
    return 0;
    
  return 1;
}

static PRUint8*
GetSpaces(FMFont aFont, PRUint32* aMaxGlyph)
{
  PRInt8 isLong = GetIndexToLocFormat(aFont);
  if (isLong < 0) 
    return nsnull;

  ByteCount len = 0;
  OSStatus err = ::FMGetFontTable(aFont, locaFontTableTag, 0, 0,  NULL, &len);
  
  if ((err != noErr) || (!len))
    return nsnull;
  
  PRUint8* buf = (PRUint8*) nsMemory::Alloc(len);
  NS_ASSERTION(buf, "cannot read 'loca' table because out of memory");
  if (!buf) 
    return nsnull;
  
  ByteCount newLen = 0;
  err = ::FMGetFontTable(aFont, locaFontTableTag, 0, len, buf, &newLen);
  NS_ASSERTION((newLen == len), "cannot read 'loca' table from the font");
  
  if (newLen != len) 
  {
    nsMemory::Free(buf);
    return nsnull;
  }

  if (isLong) 
  {
    PRUint32 longLen = ((len / 4) - 1);
    *aMaxGlyph = longLen;
    PRUint32* longBuf = (PRUint32*) buf;
    for (PRUint32 i = 0; i < longLen; i++) 
    {
      if (longBuf[i] == longBuf[i+1]) 
        buf[i] = 1;
      else 
        buf[i] = 0;
    }
  }
  else 
  {
    PRUint32 shortLen = ((len / 2) - 1);
    *aMaxGlyph = shortLen;
    PRUint16* shortBuf = (PRUint16*) buf;
    for (PRUint16 i = 0; i < shortLen; i++) 
    {
      if (shortBuf[i] == shortBuf[i+1]) 
        buf[i] = 1;
      else 
        buf[i] = 0;
    }
  }

  return buf;
}

static int spacesInitialized = 0;
static PRUint32 spaces[2048];
static void InitSpace()
{
  if (!spacesInitialized) 
  {
    spacesInitialized = 1;
    SET_SPACE(0x0020);
    SET_SPACE(0x00A0);
    for (PRUint16 c = 0x2000; c <= 0x200B; c++) 
      SET_SPACE(c);
    SET_SPACE(0x3000);
  }
}

static void HandleFormat4(PRUint16* aEntry, PRUint8* aEnd, 
                            PRUint8* aIsSpace,  PRUint32 aMaxGlyph,
                            PRUint32* aFontInfo)
{
  
  PRUint8* end = aEnd;  
  PRUint16* s = aEntry;
  PRUint16 segCount = CFSwapInt16BigToHost(s[3]) / 2;
  PRUint16* endCode = &s[7];
  PRUint16* startCode = endCode + segCount + 1;
  PRUint16* idDelta = startCode + segCount;
  PRUint16* idRangeOffset = idDelta + segCount;
  PRUint16* glyphIdArray = idRangeOffset + segCount;

  PRUint16 i;
  InitSpace();
  
  for (i = 0; i < segCount; i++) 
  {
    if (idRangeOffset[i]) 
    {
      PRUint16 startC = CFSwapInt16BigToHost(startCode[i]);
      PRUint16 endC = CFSwapInt16BigToHost(endCode[i]);
      for (PRUint32 c = startC; c <= endC; c++) 
      {
        PRUint16* g = (CFSwapInt16BigToHost(idRangeOffset[i])/2 + (c - startC) + &idRangeOffset[i]);
        if ((PRUint8*) g < end) 
        {
          if (*g) 
          {
            PRUint16 glyph = CFSwapInt16BigToHost(idDelta[i]) + *g;
            if (glyph < aMaxGlyph) 
            {
              if (aIsSpace && aIsSpace[glyph]) 
              {
                if (SHOULD_BE_SPACE(c)) 
                  ADD_GLYPH(aFontInfo, c);
              }
              else 
              {
                ADD_GLYPH(aFontInfo, c);
              }
            }
          }
        }
        else 
        {
          
        }
      }
    }
    else 
    {
      PRUint16 endC = CFSwapInt16BigToHost(endCode[i]);
      for (PRUint32 c = CFSwapInt16BigToHost(startCode[i]); c <= endC; c++) 
      {
        PRUint16 glyph = CFSwapInt16BigToHost(idDelta[i]) + c;
        if (glyph < aMaxGlyph) 
        {
          if (aIsSpace && aIsSpace[glyph]) 
          {
            if (SHOULD_BE_SPACE(c)) 
              ADD_GLYPH(aFontInfo, c);
          }
          else 
          {
            ADD_GLYPH(aFontInfo, c);
          }
        }
      }
    }
  }
}
static PRBool FillFontInfoFromCMAP(FMFont aFont, PRUint32 *aFontInfo, FourCharCode aFontFormat)
{
  ByteCount len;
  OSErr err = ::FMGetFontTable(aFont, cmapFontTableTag, 0, 0, NULL, &len);
  if((err!=noErr) || (!len))
    return PR_FALSE;

  PRUint8* buf = (PRUint8*) nsMemory::Alloc(len);
  NS_ASSERTION(buf, "cannot read cmap because out of memory");
  if (!buf) 
    return PR_FALSE;

  ByteCount newLen;
  err = ::FMGetFontTable(aFont, cmapFontTableTag, 0, len,  buf, &newLen);
  NS_ASSERTION(newLen == len, "cannot read cmap from the font");
  if (newLen != len) 
  {
    nsMemory::Free(buf);
    return PR_FALSE;
  }
  
  PRUint8* p = buf + sizeof(PRUint16); 
  PRUint16 n = GET_SHORT(p); 
  p += sizeof(PRUint16); 

  PRUint16 i;
  PRUint32 offset;
  PRUint32 platformUnicodeOffset = 0;
  
  
  
  
  for (i = 0; i < n; i++) 
  {
    PRUint16 platformID = GET_SHORT(p); 
    p += sizeof(PRUint16); 
    PRUint16 encodingID = GET_SHORT(p); 
    p += sizeof(PRUint16); 
    offset = GET_LONG(p);  
    p += sizeof(PRUint32); 
#ifdef DEBUG_TRUE_TYPE
    printf("p=%d e=%d offset=%x\n", platformID, encodingID, offset);
#endif
    if (platformID == kFontMicrosoftPlatform) 
    { 
      if (encodingID == kFontMicrosoftStandardScript) 
      { 
        
        
        break;  
      } 
#if 0
      
      else if (encodingID == kFontMicrosoftSymbolScript) 
      { 
        NS_ASSERTION(false, "cannot handle symbol font");
        nsMemory::Free(buf);
        return PR_FALSE;
      } 
#endif
    } 
    else {
      if (platformID == kFontUnicodePlatform) 
      { 
        platformUnicodeOffset = offset;
      }
    }
  } 
  
  NS_ASSERTION((i != n) || ( 0 != platformUnicodeOffset), "do not know the TrueType encoding");
  if ((i == n) && ( 0 == platformUnicodeOffset)) 
  {  
    nsMemory::Free(buf);
    return PR_FALSE;
  }

  
  
  if(platformUnicodeOffset)
    offset = platformUnicodeOffset;

  p = buf + offset;
  PRUint16 format = GET_SHORT(p);
  NS_ASSERTION((kSFNTLookupSegmentArray == format), "hit some unknown format");
  switch(format) {
    case kSFNTLookupSegmentArray: 
    {
      PRUint32 maxGlyph;
      PRUint8* isSpace = GetSpaces(aFont, &maxGlyph);
      
      
      
      
      
      
      
      

      HandleFormat4((PRUint16*) (buf + offset), buf+len, isSpace, maxGlyph, aFontInfo);

      if (isSpace)
        nsMemory::Free(isSpace);
      nsMemory::Free(buf);                                
      return PR_TRUE;
    }
    break;
    default:
    {
      nsMemory::Free(buf);
      return PR_FALSE;
    }
    break;    
  }
  
}



static PRUint16* InitGlobalCCMap()
{
  PRUint32 info[2048];
  memset(info, 0, sizeof(info));

#ifdef TRACK_INIT_PERFORMANCE
  AbsoluteTime startTime;
  AbsoluteTime endTime;
  startTime = UpTime();
#endif  
  
  FMFontFamilyIterator aFontIterator;
  OSStatus status = 0;
  FMFont aFont; 
  FMFontFamily aFontFamily;
  status = ::FMCreateFontFamilyIterator(NULL, NULL, kFMDefaultOptions,
                                        &aFontIterator);
  while (status == noErr)
  {
    FourCharCode aFormat;
    status = ::FMGetNextFontFamily(&aFontIterator, &aFontFamily);
    OSStatus status2;
    FMFontStyle aStyle;
    status2 = ::FMGetFontFromFontFamilyInstance(aFontFamily, 0, &aFont, &aStyle);
    NS_ASSERTION(status2 == noErr, "cannot get font from family");
    if (status2 == noErr)
    {
      status2 = ::FMGetFontFormat(aFont, &aFormat);
#ifdef DEBUG_TRUE_TYPE
      OSStatus status3 = ::FMGetFontFormat(aFont, &aFormat);
      const char *four = (const char*) &aFormat;
      Str255 familyName;
      status3 = ::FMGetFontFamilyName(aFontFamily, familyName);
      familyName[familyName[0]+1] = '\0';
      printf("%s format = %c%c%c%c\n", familyName+1, *four, *(four+1), *(four+2), *(four+3));
#endif
     if ((status2 == noErr) && 
         ((kFMTrueTypeFontTechnology == aFormat) ||
          (kFMOpenTypeFontTechnology == aFormat)))
     {
       PRBool ret = FillFontInfoFromCMAP(aFont, info, aFormat);
     }
    }
  }
  
  status = ::FMDisposeFontFamilyIterator(&aFontIterator);  
  
  PRUint16* map = MapToCCMap(info);
  NS_ASSERTION(map, "cannot create the compressed map");

  
  gFontCleanupObserver = new nsFontCleanupObserver();
  NS_ASSERTION(gFontCleanupObserver, "failed to create observer");
  if (gFontCleanupObserver) {
    
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService(do_GetService("@mozilla.org/observer-service;1", &rv));
    if (NS_SUCCEEDED(rv)) {
      rv = observerService->AddObserver(gFontCleanupObserver, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
    }    
  }
    
#ifdef TRACK_INIT_PERFORMANCE
  endTime = UpTime();
  Nanoseconds diff = ::AbsoluteToNanoseconds(SubAbsoluteFromAbsolute(endTime, startTime));
  printf("nsMacUnicodeFontInfo::InitGolbal take %d %d nanosecond\n", diff.hi, diff.lo);
#endif

  return map;
}


static nsresult
GetEncoding(const nsCString& aFontName, nsACString& aValue)
{
  nsresult rv;
  
  if (! gFontEncodingProperties) {
    
    if (aFontName.EqualsLiteral("Lucida Grande") ||
        aFontName.EqualsLiteral("Charcoal") ||
        aFontName.EqualsLiteral("Chicago") ||
        aFontName.EqualsLiteral("Capitals") ||
        aFontName.EqualsLiteral("Gadget") ||
        aFontName.EqualsLiteral("Sand") ||
        aFontName.EqualsLiteral("Techno") ||
        aFontName.EqualsLiteral("Textile") ||
        aFontName.EqualsLiteral("Geneva") )
      return NS_ERROR_NOT_AVAILABLE; 

    
    rv = NS_LoadPersistentPropertiesFromURISpec(&gFontEncodingProperties,
         NS_LITERAL_CSTRING("resource://gre/res/fonts/fontEncoding.properties"));
    if (NS_FAILED(rv))
      return rv;
  }

  nsCAutoString name(NS_LITERAL_CSTRING("encoding.") +
                     aFontName +
                     NS_LITERAL_CSTRING(".ttf"));
  name.StripWhitespace();
  ToLowerCase(name);

  nsAutoString value;
  rv = gFontEncodingProperties->GetStringProperty(name, value);
  if (NS_SUCCEEDED(rv))
    LossyCopyUTF16toASCII(value, aValue);
  return rv;
}




static nsresult
GetConverter(const nsCString& aFontName, nsIUnicodeEncoder** aConverter)
{
  *aConverter = nsnull;

  nsCAutoString value;
  nsresult rv = GetEncoding(aFontName, value);
  if (NS_FAILED(rv)) return rv;
  
  if (!gCharsetManager)
  {
    rv = CallGetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &gCharsetManager);
    if(NS_FAILED(rv)) return rv;
  }
  
  rv = gCharsetManager->GetUnicodeEncoderRaw(value.get(), aConverter);
  if (NS_FAILED(rv)) return rv;

  nsIUnicodeEncoder* tmp = *aConverter;
  return tmp->SetOutputErrorBehavior(tmp->kOnError_Replace, nsnull, '?');
}



static PRUint16*
GetCCMapThroughConverter(nsIUnicodeEncoder *converter)
{
  
  nsCOMPtr<nsICharRepresentable> mapper(do_QueryInterface(converter));
  return (mapper ? MapperToCCMap(mapper) : nsnull);
}

static PRBool PR_CALLBACK
HashtableFreeCCMap(nsHashKey *aKey, void *aData, void *closure)
{
    PRUint16* ccmap = (PRUint16*)aData;
    FreeCCMap(ccmap);
    return PR_TRUE;
}




nsresult
nsMacUnicodeFontInfo::GetConverterAndCCMap(const nsString& aFontName, nsIUnicodeEncoder** aConverter,
    PRUint16** aCCMap)
{
    if(NS_SUCCEEDED(GetConverter(NS_ConvertUTF16toUTF8(aFontName), aConverter)) && *aConverter)
    {
        
        if(!gFontMaps)
        {
            gFontMaps = new nsObjectHashtable(nsnull, nsnull, HashtableFreeCCMap, nsnull);
            if(!gFontMaps)
            {
                *aConverter = nsnull;
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }

        
        nsStringKey hashKey(aFontName);
        *aCCMap = (PRUint16*) gFontMaps->Get(&hashKey);
        if(!*aCCMap)
        {
            
            *aCCMap = GetCCMapThroughConverter(*aConverter);
            if(!*aCCMap)
            {
                *aConverter = nsnull;
                return NS_ERROR_FAILURE;
            }
            gFontMaps->Put(&hashKey, *aCCMap);
        }
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}



PRBool nsMacUnicodeFontInfo::HasGlyphFor(PRUnichar aChar)
{
  if (0xfffd == aChar)
    return PR_FALSE;

  if (!gCCMap) 
    gCCMap = InitGlobalCCMap();

  NS_ASSERTION(gCCMap, "cannot init global ccmap");
    
  if (gCCMap)
    return CCMAP_HAS_CHAR(gCCMap, aChar);

  return PR_FALSE;
}

void nsMacUnicodeFontInfo::FreeGlobals()
{    
  NS_IF_RELEASE(gFontEncodingProperties);
  NS_IF_RELEASE(gCharsetManager);

  delete gFontMaps;
  if (gCCMap)
    FreeCCMap(gCCMap);  
}
