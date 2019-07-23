











































#include "nsISupportsUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIPref.h"
#include "nsFontMetricsXft.h"
#include "prenv.h"
#include "prprf.h"
#include "prlink.h"
#include "nsQuickSort.h"
#include "nsFont.h"
#include "nsIDeviceContext.h"
#include "nsRenderingContextGTK.h"
#include "nsDeviceContextGTK.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsITimelineService.h"
#include "nsICharsetConverterManager.h"
#include "nsICharRepresentable.h"
#include "nsIPersistentProperties2.h"
#include "nsCompressedCharMap.h"
#include "nsNetUtil.h"
#include "nsClassHashtable.h"
#include "nsAutoBuffer.h"
#include "nsFontConfigUtils.h"

#include <gdk/gdkx.h>
#include <freetype/tttables.h>
#include <freetype/freetype.h>

#define FORCE_PR_LOG
#include "prlog.h"






class nsAutoDrawSpecBuffer;

class nsFontXft {
public:
    nsFontXft(FcPattern *aPattern, FcPattern *aFontName);
    virtual ~nsFontXft() = 0;

    
    
    
    XftFont   *GetXftFont (void);
    virtual nsresult GetTextExtents32 (const FcChar32 *aString, PRUint32 aLen, 
                                       XGlyphInfo &aGlyphInfo);
    gint     GetWidth32               (const FcChar32 *aString, PRUint32 aLen);

#ifdef MOZ_MATHML
    nsresult GetBoundingMetrics32 (const FcChar32 *aString, 
                                   PRUint32 aLength,
                                   nsBoundingMetrics &aBoundingMetrics);
#endif 

    PRInt16    GetMaxAscent(void);
    PRInt16    GetMaxDescent(void);

    virtual PRBool     HasChar(PRUint32 aChar) = 0;
    virtual FT_UInt    CharToGlyphIndex(FcChar32 aChar);

    virtual nsresult   DrawStringSpec(FcChar32* aString, PRUint32 aLen,
                                      void *aData);
                                      
    
    XftFont   *mXftFont;
    FcPattern *mPattern;
    FcPattern *mFontName;
    FcCharSet *mCharset;
};

class nsFontXftInfo;


class nsFontXftUnicode : public nsFontXft {
public:
    nsFontXftUnicode(FcPattern *aPattern, FcPattern *aFontName)
      : nsFontXft(aPattern, aFontName)
    { }

    virtual ~nsFontXftUnicode();

    virtual PRBool     HasChar (PRUint32 aChar);
};












class nsFontXftCustom : public nsFontXft {
public:
    nsFontXftCustom(FcPattern* aPattern, 
                    FcPattern* aFontName, 
                    nsFontXftInfo* aFontInfo)
      : nsFontXft(aPattern, aFontName)
      , mFontInfo(aFontInfo)
      , mFT_Face(nsnull)
    { }

    virtual ~nsFontXftCustom();

    virtual PRBool   HasChar            (PRUint32 aChar);
    virtual FT_UInt  CharToGlyphIndex   (FcChar32 aChar);
    virtual nsresult GetTextExtents32   (const FcChar32 *aString, 
                                         PRUint32 aLen, XGlyphInfo &aGlyphInfo);
    virtual nsresult DrawStringSpec     (FcChar32* aString, PRUint32 aLen,
                                         void *aData);

private:
    nsFontXftInfo *mFontInfo; 

    
    
    FT_Face    mFT_Face;
    nsresult   SetFT_FaceCharmap (void);
};

enum nsXftFontType {
    eFontTypeUnicode,
    eFontTypeCustom,
    eFontTypeCustomWide
};



class nsFontXftInfo {
    public:
    nsFontXftInfo() : mCCMap(nsnull), mConverter(0), 
                      mFontType(eFontTypeUnicode) 
                      { }

    ~nsFontXftInfo() {
        if (mCCMap)
            FreeCCMap(mCCMap);
    }

    
    PRUint16*                   mCCMap;
    
    nsCOMPtr<nsIUnicodeEncoder> mConverter;
    
    nsXftFontType               mFontType;
    
    
    FT_Encoding                 mFT_Encoding;
};

struct DrawStringData {
    nscoord                x;
    nscoord                y;
    const nscoord         *spacing;
    nscoord                xOffset;
    nsRenderingContextGTK *context;
    XftDraw               *draw;
    XftColor               color;
    float                  p2t;
    nsAutoDrawSpecBuffer  *drawBuffer;
};

#ifdef MOZ_MATHML
struct BoundingMetricsData {
    nsBoundingMetrics *bm;
    PRBool firstTime;
};
#endif 

#define AUTO_BUFFER_SIZE 3000
typedef nsAutoBuffer<FcChar32, AUTO_BUFFER_SIZE> nsAutoFcChar32Buffer;

static int      CompareFontNames (const void* aArg1, const void* aArg2,
                                  void* aClosure);
static nsresult EnumFontsXft     (nsIAtom* aLangGroup, const char* aGeneric,
                                  PRUint32* aCount, PRUnichar*** aResult);

static        void ConvertCharToUCS4    (const char *aString,
                                         PRUint32 aLength,
                                         nsAutoFcChar32Buffer &aOutBuffer,
                                         PRUint32 *aOutLen);
static        void ConvertUnicharToUCS4 (const PRUnichar *aString,
                                         PRUint32 aLength,
                                         nsAutoFcChar32Buffer &aOutBuffer,
                                         PRUint32 *aOutLen);
static    nsresult ConvertUCS4ToCustom  (FcChar32 *aSrc, PRUint32 aSrcLen,
                                         PRUint32& aDestLen, 
                                         nsIUnicodeEncoder *aConverter, 
                                         PRBool aIsWide, 
                                         nsAutoFcChar32Buffer &Result);

#ifdef MOZ_WIDGET_GTK2
static void GdkRegionSetXftClip(GdkRegion *aGdkRegion, XftDraw *aDraw);
#endif





#define FONT_MAX_FONT_SCALE 2

#define UCS2_REPLACEMENT 0xFFFD

#define IS_NON_BMP(c) ((c) >> 16)
#define IS_NON_SURROGATE(c) ((c < 0xd800 || c > 0xdfff))


class nsAutoDrawSpecBuffer {
public:
    enum {BUFFER_LEN=1024};
    nsAutoDrawSpecBuffer(XftDraw *aDraw, XftColor *aColor) :
                         mDraw(aDraw), mColor(aColor), mSpecPos(0) {}

    ~nsAutoDrawSpecBuffer() {
        Flush();
    }

    void Flush();
    void Draw(nscoord x, nscoord y, XftFont *font, FT_UInt glyph);

private:
    XftDraw         *mDraw;
    XftColor        *mColor;
    PRUint32         mSpecPos;
    XftGlyphFontSpec mSpecBuffer[BUFFER_LEN];
};


PRLogModuleInfo *gXftFontLoad = nsnull;
static int gNumInstances = 0;

#undef DEBUG_XFT_MEMORY
#ifdef DEBUG_XFT_MEMORY

extern "C" {
extern void XftMemReport(void);
extern void FcMemReport(void);
}
#endif

static nsresult
EnumFontsXft(nsIAtom* aLangGroup, const char* aGeneric,
             PRUint32* aCount, PRUnichar*** aResult);
 
static NS_DEFINE_CID(kCharsetConverterManagerCID,
                     NS_ICHARSETCONVERTERMANAGER_CID);

static PRBool                      gInitialized = PR_FALSE;
static nsIPersistentProperties*    gFontEncodingProperties = nsnull;
static nsICharsetConverterManager* gCharsetManager = nsnull;

typedef nsClassHashtable<nsCharPtrHashKey, nsFontXftInfo> nsFontXftInfoHash; 
static nsFontXftInfoHash           gFontXftMaps;
#define INITIAL_FONT_MAP_SIZE      32

static nsresult       GetEncoding(const char* aFontName,
                                  char **aEncoding,
                                  nsXftFontType &aType,
                                  FT_Encoding &aFTEncoding);
static nsresult       GetConverter(const char* aEncoding,
                                   nsIUnicodeEncoder** aConverter);
static nsresult       FreeGlobals(void);
static nsFontXftInfo* GetFontXftInfo(FcPattern* aPattern);

nsFontMetricsXft::nsFontMetricsXft(): mMiniFont(nsnull)
{
    if (!gXftFontLoad)
        gXftFontLoad = PR_NewLogModule("XftFontLoad");

    ++gNumInstances;
}

nsFontMetricsXft::~nsFontMetricsXft()
{
    if (mDeviceContext)
        mDeviceContext->FontMetricsDeleted(this);

    if (mPattern)
        FcPatternDestroy(mPattern);

    for (PRInt32 i= mLoadedFonts.Count() - 1; i >= 0; --i) {
        nsFontXft *font = (nsFontXft *)mLoadedFonts.ElementAt(i);
        delete font;
    }

    if (mMiniFont)
        XftFontClose(GDK_DISPLAY(), mMiniFont);

    if (--gNumInstances == 0) {
        FreeGlobals();
#ifdef DEBUG_XFT_MEMORY
        XftMemReport();
        FcMemReport();
#endif
    }
}

NS_IMPL_ISUPPORTS1(nsFontMetricsXft, nsIFontMetrics)



NS_IMETHODIMP
nsFontMetricsXft::Init(const nsFont& aFont, nsIAtom* aLangGroup,
                       nsIDeviceContext *aContext)
{
    mFont = aFont;
    mLangGroup = aLangGroup;

    
    mDeviceContext = aContext;

    float app2dev = mDeviceContext->AppUnitsToDevUnits();
    mPixelSize = NSTwipsToFloatPixels(mFont.size, app2dev);

    
    
    nscoord screenPixels = gdk_screen_height();
    mPixelSize = PR_MIN(screenPixels * FONT_MAX_FONT_SCALE, mPixelSize);

    
    mFont.EnumerateFamilies(nsFontMetricsXft::EnumFontCallback, this);

    nsCOMPtr<nsIPref> prefService;
    prefService = do_GetService(NS_PREF_CONTRACTID);
    if (!prefService)
        return NS_ERROR_FAILURE;
        
    nsXPIDLCString value;
    const char* langGroup;
    mLangGroup->GetUTF8String(&langGroup);

    
    if (!mGenericFont) {
        nsCAutoString name("font.default.");
        name.Append(langGroup);
        prefService->CopyCharPref(name.get(), getter_Copies(value));

        if (value.get())
            mDefaultFont = value.get();
        else
            mDefaultFont = "serif";
        
        mGenericFont = &mDefaultFont;
    }

    
    if (mLangGroup) {
        nsCAutoString name("font.min-size.");

        if (mGenericFont->Equals("monospace"))
            name.Append("fixed");
        else
            name.Append("variable");

        name.Append(char('.'));
        name.Append(langGroup);

        PRInt32 minimum = 0;
        nsresult res;
        res = prefService->GetIntPref(name.get(), &minimum);
        if (NS_FAILED(res))
            prefService->GetDefaultIntPref(name.get(), &minimum);

        if (minimum < 0)
            minimum = 0;

        if (mPixelSize < minimum)
            mPixelSize = minimum;
    }

    
    if (mPixelSize < 1) {
#ifdef DEBUG
        printf("*** Warning: nsFontMetricsXft was passed a pixel size of %f\n",
               mPixelSize);
#endif
        mPixelSize = 1;
    }
    if (!gInitialized) {
        CallGetService(kCharsetConverterManagerCID, &gCharsetManager);
        if (!gCharsetManager) {
            FreeGlobals();
            return NS_ERROR_FAILURE;
        }

        if (!gFontXftMaps.IsInitialized() && 
            !gFontXftMaps.Init(INITIAL_FONT_MAP_SIZE)) {
            FreeGlobals();
            return NS_ERROR_OUT_OF_MEMORY;
        }

        gInitialized = PR_TRUE;
    }

    if (NS_FAILED(RealizeFont()))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsXft::Destroy()
{
    mDeviceContext = nsnull;

    return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsXft::GetLangGroup(nsIAtom** aLangGroup)
{
    *aLangGroup = mLangGroup;
    NS_IF_ADDREF(*aLangGroup);

    return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsXft::GetFontHandle(nsFontHandle &aHandle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}



nsresult
nsFontMetricsXft::GetWidth(const char* aString, PRUint32 aLength,
                           nscoord& aWidth,
                           nsRenderingContextGTK *aContext)
{
    NS_TIMELINE_MARK_FUNCTION("GetWidth");

    XftFont *font = mWesternFont->mXftFont;
    NS_ASSERTION(font, "FindFont returned a bad font");

    XGlyphInfo glyphInfo;

    
    XftTextExtents8(GDK_DISPLAY(), font, (FcChar8 *)aString,
                    aLength, &glyphInfo);

    float f;
    f = mDeviceContext->DevUnitsToAppUnits();
    aWidth = NSToCoordRound(glyphInfo.xOff * f);

    return NS_OK;
}

nsresult
nsFontMetricsXft::GetWidth(const PRUnichar* aString, PRUint32 aLength,
                           nscoord& aWidth, PRInt32 *aFontID,
                           nsRenderingContextGTK *aContext)
{
    NS_TIMELINE_MARK_FUNCTION("GetWidth");
    if (!aLength) {
        aWidth = 0;
        return NS_OK;
    }

    gint rawWidth = RawGetWidth(aString, aLength);

    float f;
    f = mDeviceContext->DevUnitsToAppUnits();
    aWidth = NSToCoordRound(rawWidth * f);

    if (aFontID)
        *aFontID = 0;

    return NS_OK;
}

nsresult
nsFontMetricsXft::GetTextDimensions(const PRUnichar* aString,
                                    PRUint32 aLength,
                                    nsTextDimensions& aDimensions, 
                                    PRInt32* aFontID,
                                    nsRenderingContextGTK *aContext)
{
    NS_TIMELINE_MARK_FUNCTION("GetTextDimensions");
    aDimensions.Clear();

    if (!aLength)
        return NS_OK;

    nsresult rv;
    rv = EnumerateGlyphs(aString, aLength,
                         &nsFontMetricsXft::TextDimensionsCallback,
                         &aDimensions);

    NS_ENSURE_SUCCESS(rv, rv);

    float P2T;
    P2T = mDeviceContext->DevUnitsToAppUnits();

    aDimensions.width = NSToCoordRound(aDimensions.width * P2T);
    aDimensions.ascent = NSToCoordRound(aDimensions.ascent * P2T);
    aDimensions.descent = NSToCoordRound(aDimensions.descent * P2T);

    if (nsnull != aFontID)
        *aFontID = 0;

    return NS_OK;
}

nsresult
nsFontMetricsXft::GetTextDimensions(const char*         aString,
                                    PRInt32             aLength,
                                    PRInt32             aAvailWidth,
                                    PRInt32*            aBreaks,
                                    PRInt32             aNumBreaks,
                                    nsTextDimensions&   aDimensions,
                                    PRInt32&            aNumCharsFit,
                                    nsTextDimensions&   aLastWordDimensions,
                                    PRInt32*            aFontID,
                                    nsRenderingContextGTK *aContext)
{
    NS_NOTREACHED("GetTextDimensions");
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFontMetricsXft::GetTextDimensions(const PRUnichar*    aString,
                                    PRInt32             aLength,
                                    PRInt32             aAvailWidth,
                                    PRInt32*            aBreaks,
                                    PRInt32             aNumBreaks,
                                    nsTextDimensions&   aDimensions,
                                    PRInt32&            aNumCharsFit,
                                    nsTextDimensions&   aLastWordDimensions,
                                    PRInt32*            aFontID,
                                    nsRenderingContextGTK *aContext)
{
    NS_NOTREACHED("GetTextDimensions");
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFontMetricsXft::DrawString(const char *aString, PRUint32 aLength,
                             nscoord aX, nscoord aY,
                             const nscoord* aSpacing,
                             nsRenderingContextGTK *aContext,
                             nsDrawingSurfaceGTK *aSurface)
{
    NS_TIMELINE_MARK_FUNCTION("DrawString");

    
    DrawStringData data;
    memset(&data, 0, sizeof(data));

    data.x = aX;
    data.y = aY;
    data.spacing = aSpacing;
    data.context = aContext;
    data.p2t = mDeviceContext->DevUnitsToAppUnits();

    PrepareToDraw(aContext, aSurface, &data.draw, data.color);

    nsAutoDrawSpecBuffer drawBuffer(data.draw, &data.color);
    data.drawBuffer = &drawBuffer;

    return EnumerateGlyphs(aString, aLength,
                           &nsFontMetricsXft::DrawStringCallback, &data);
}

nsresult
nsFontMetricsXft::DrawString(const PRUnichar* aString, PRUint32 aLength,
                             nscoord aX, nscoord aY,
                             PRInt32 aFontID,
                             const nscoord* aSpacing,
                             nsRenderingContextGTK *aContext,
                             nsDrawingSurfaceGTK *aSurface)
{
    NS_TIMELINE_MARK_FUNCTION("DrawString");

    
    DrawStringData data;
    memset(&data, 0, sizeof(data));

    data.x = aX;
    data.y = aY;
    data.spacing = aSpacing;
    data.context = aContext;
    data.p2t = mDeviceContext->DevUnitsToAppUnits();

    
    PrepareToDraw(aContext, aSurface, &data.draw, data.color);

    nsAutoDrawSpecBuffer drawBuffer(data.draw, &data.color);
    data.drawBuffer = &drawBuffer;

    return EnumerateGlyphs(aString, aLength,
                           &nsFontMetricsXft::DrawStringCallback, &data);
}

#ifdef MOZ_MATHML

nsresult
nsFontMetricsXft::GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                     nsBoundingMetrics &aBoundingMetrics,
                                     nsRenderingContextGTK *aContext)
{
    aBoundingMetrics.Clear(); 

    if (!aString || !aLength)
        return NS_ERROR_FAILURE;

    
    BoundingMetricsData data;
    data.bm = &aBoundingMetrics;
    
    
    data.firstTime = PR_TRUE; 

    nsresult rv;
    rv = EnumerateGlyphs(aString, aLength,
                         &nsFontMetricsXft::BoundingMetricsCallback, &data);
    NS_ENSURE_SUCCESS(rv, rv);

    float P2T;
    P2T = mDeviceContext->DevUnitsToAppUnits();

    aBoundingMetrics.leftBearing =
        NSToCoordRound(aBoundingMetrics.leftBearing * P2T);
    aBoundingMetrics.rightBearing =
        NSToCoordRound(aBoundingMetrics.rightBearing * P2T);
    aBoundingMetrics.width = NSToCoordRound(aBoundingMetrics.width * P2T);
    aBoundingMetrics.ascent = NSToCoordRound(aBoundingMetrics.ascent * P2T);
    aBoundingMetrics.descent = NSToCoordRound(aBoundingMetrics.descent * P2T);

    return NS_OK;
}

nsresult
nsFontMetricsXft::GetBoundingMetrics(const PRUnichar *aString,
                                     PRUint32 aLength,
                                     nsBoundingMetrics &aBoundingMetrics,
                                     PRInt32 *aFontID,
                                     nsRenderingContextGTK *aContext)
{
    aBoundingMetrics.Clear(); 

    if (!aString || !aLength)
        return NS_ERROR_FAILURE;

    
    BoundingMetricsData data;
    data.bm = &aBoundingMetrics;
    
    
    data.firstTime = PR_TRUE; 

    nsresult rv;
    rv = EnumerateGlyphs(aString, aLength,
                         &nsFontMetricsXft::BoundingMetricsCallback, &data);
    NS_ENSURE_SUCCESS(rv, rv);

    float P2T;
    P2T = mDeviceContext->DevUnitsToAppUnits();

    aBoundingMetrics.leftBearing =
        NSToCoordRound(aBoundingMetrics.leftBearing * P2T);
    aBoundingMetrics.rightBearing =
        NSToCoordRound(aBoundingMetrics.rightBearing * P2T);
    aBoundingMetrics.width = NSToCoordRound(aBoundingMetrics.width * P2T);
    aBoundingMetrics.ascent = NSToCoordRound(aBoundingMetrics.ascent * P2T);
    aBoundingMetrics.descent = NSToCoordRound(aBoundingMetrics.descent * P2T);

    if (nsnull != aFontID)
        *aFontID = 0;

    return NS_OK;
}

#endif 

GdkFont*
nsFontMetricsXft::GetCurrentGDKFont(void)
{
    return nsnull;
}

nsresult
nsFontMetricsXft::SetRightToLeftText(PRBool aIsRTL)
{
    return NS_OK;
}

PRBool
nsFontMetricsXft::GetRightToLeftText()
{
    return PR_FALSE;
}

nsresult
nsFontMetricsXft::GetClusterInfo(const PRUnichar *aText,
                                 PRUint32 aLength,
                                 PRUint8 *aClusterStarts)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

PRInt32
nsFontMetricsXft::GetPosition(const PRUnichar *aText,
                              PRUint32 aLength,
                              nsPoint aPt)
{
    return -1;
}

nsresult
nsFontMetricsXft::GetRangeWidth(const PRUnichar *aText,
                                PRUint32 aLength,
                                PRUint32 aStart,
                                PRUint32 aEnd,
                                PRUint32 &aWidth)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFontMetricsXft::GetRangeWidth(const char *aText,
                                PRUint32 aLength,
                                PRUint32 aStart,
                                PRUint32 aEnd,
                                PRUint32 &aWidth)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

PRUint32
nsFontMetricsXft::GetHints(void)
{
    return 0;
}

nsresult
nsFontMetricsXft::RealizeFont(void)
{
    
    
    mWesternFont = FindFont('a');
    if (!mWesternFont)
        return NS_ERROR_FAILURE;

    return CacheFontMetrics();
}




#define MOZ_FT_ROUND(x) (((x) + 32) & ~63) // 63 = 2^6 - 1
#define MOZ_FT_TRUNC(x) ((x) >> 6)
#define CONVERT_DESIGN_UNITS_TO_PIXELS(v, s) \
        MOZ_FT_TRUNC(MOZ_FT_ROUND(FT_MulFix((v) , (s))))

nsresult
nsFontMetricsXft::CacheFontMetrics(void)
{
    
    float f;
    float val;
    f = mDeviceContext->DevUnitsToAppUnits();
    
    
    XftFont *xftFont = mWesternFont->mXftFont;
    NS_ASSERTION(xftFont, "FindFont returned a bad font");

    FT_Face face = XftLockFace(xftFont);
    if (!face)
        return NS_ERROR_NOT_AVAILABLE;

    
    int size;
    if (FcPatternGetInteger(mWesternFont->mPattern, FC_PIXEL_SIZE, 0, &size) !=
        FcResultMatch) {
        size = 12;
    }
    mEmHeight = PR_MAX(1, nscoord(size * f));

    
    mMaxAscent = nscoord(xftFont->ascent * f);

    
    mMaxDescent = nscoord(xftFont->descent * f);

    nscoord lineHeight = mMaxAscent + mMaxDescent;

    
    if (lineHeight > mEmHeight)
        mLeading = lineHeight - mEmHeight;
    else
        mLeading = 0;

    
    mMaxHeight = lineHeight;

    
    mEmAscent = nscoord(mMaxAscent * mEmHeight / lineHeight);

    
    mEmDescent = mEmHeight - mEmAscent;

    
    mMaxAdvance = nscoord(xftFont->max_advance_width * f);
    
    
    mMaxStringLength = (PRInt32)floor(32767.0/xftFont->max_advance_width);
    mMaxStringLength = PR_MAX(1, mMaxStringLength);

    
    gint rawWidth;
    PRUnichar unispace(' ');
    rawWidth = RawGetWidth(&unispace, 1);
    mSpaceWidth = NSToCoordRound(rawWidth * f);

    
    PRUnichar xUnichar('x');
    rawWidth = RawGetWidth(&xUnichar, 1);
    mAveCharWidth = NSToCoordRound(rawWidth * f);

    
    if (FcCharSetHasChar(mWesternFont->mCharset, xUnichar)) {
        XGlyphInfo extents;
        XftTextExtents16(GDK_DISPLAY(), xftFont, &xUnichar, 1, &extents);
        mXHeight = extents.height;
    }
    else {
        
        mXHeight = nscoord(((float)mMaxAscent) * 0.56);
    }
    mXHeight = nscoord(mXHeight * f);

    
    val = CONVERT_DESIGN_UNITS_TO_PIXELS(face->underline_position,
                                         face->size->metrics.y_scale);
    if (val) {
        mUnderlineOffset = NSToIntRound(val * f);
    }
    else {
        mUnderlineOffset =
            -NSToIntRound(PR_MAX(1, floor(0.1 * xftFont->height + 0.5)) * f);
    }

    
    val = CONVERT_DESIGN_UNITS_TO_PIXELS(face->underline_thickness,
                                         face->size->metrics.y_scale);
    if (val) {
        mUnderlineSize = nscoord(PR_MAX(f, NSToIntRound(val * f)));
    }
    else {
        mUnderlineSize =
            NSToIntRound(PR_MAX(1, floor(0.05 * xftFont->height + 0.5)) * f);
    }

    TT_OS2 *os2 = (TT_OS2 *) FT_Get_Sfnt_Table(face, ft_sfnt_os2);

    
    if (os2 && os2->ySuperscriptYOffset) {
        val = CONVERT_DESIGN_UNITS_TO_PIXELS(os2->ySuperscriptYOffset,
                                             face->size->metrics.y_scale);
        mSuperscriptOffset = nscoord(PR_MAX(f, NSToIntRound(val * f)));
    }
    else {
        mSuperscriptOffset = mXHeight;
    }

    
    if (os2 && os2->ySubscriptYOffset) {
        val = CONVERT_DESIGN_UNITS_TO_PIXELS(os2->ySubscriptYOffset,
                                             face->size->metrics.y_scale);
        
        val = (val < 0) ? -val : val;
        mSubscriptOffset = nscoord(PR_MAX(f, NSToIntRound(val * f)));
    }
    else {
        mSubscriptOffset = mXHeight;
    }

    
    mStrikeoutOffset = NSToCoordRound(mXHeight / 2.0);

    
    mStrikeoutSize = mUnderlineSize;

    XftUnlockFace(xftFont);

    return NS_OK;
}

nsFontXft *
nsFontMetricsXft::FindFont(PRUint32 aChar)
{

    
    
    
    if (!mPattern) {
        SetupFCPattern();
        
        if (!mPattern)
            return nsnull;
    }

    
    if (mMatchType == eNoMatch) {
        
        
        
        DoMatch(PR_FALSE);
    }

    
    

    if (mLoadedFonts.Count() == 0) {
        
        
        return nsnull;
    }

    PRBool removeFirstFont = PR_FALSE;
    nsFontXft *font = (nsFontXft *)mLoadedFonts.ElementAt(0);
    if (font->HasChar(aChar)) {
        if (font->GetXftFont())
            return font;
        removeFirstFont = PR_TRUE;
    }

    
    

    if (mMatchType == eBestMatch)
        DoMatch(PR_TRUE);

    PRInt32 i = 1;
    if (removeFirstFont) {
        
        
        mLoadedFonts.RemoveElementAt(0);
        i = 0;
    }

    
    for (; i < mLoadedFonts.Count(); ++i) {
        nsFontXft *font = (nsFontXft *)mLoadedFonts.ElementAt(i);
        if (font->HasChar(aChar)) {
            if (font->GetXftFont())
                return font;
            
            
            
            
            mLoadedFonts.RemoveElementAt(i--);
        }
    }

    
    
    return nsnull;
}

void
nsFontMetricsXft::SetupFCPattern(void)
{
    if (PR_LOG_TEST(gXftFontLoad, PR_LOG_DEBUG)) {
        printf("[%p] setting up pattern with the following specification:\n",
               (void *)this);

        
        if (mFontList.Count() && !mFontIsGeneric[0]) {
            printf("\tadding non-generic families: ");
            for (int i=0; i < mFontList.Count(); ++i) {
                if (mFontIsGeneric[i])
                    break;

                nsCString *familyName = mFontList.CStringAt(i);
                printf("%s, ", familyName->get());
            }
            printf("\n");
        }

        
        const char *name;
        mLangGroup->GetUTF8String(&name);
        printf("\tlang group: %s\n", name);


    }

    mPattern = FcPatternCreate();
    if (!mPattern)
        return;

    if (gdk_rgb_get_cmap() != gdk_colormap_get_system())
        XftPatternAddBool(mPattern, XFT_RENDER, False);

    

    
    
    for (int i=0; i < mFontList.Count(); ++i) {
        
        
        if (mFontIsGeneric[i])
            break;;

        nsCString *familyName = mFontList.CStringAt(i);
        NS_AddFFRE(mPattern, familyName, PR_FALSE);
    }

    
    
    
    NS_AddLangGroup (mPattern, mLangGroup);

    
    
    if (mGenericFont && !mFont.systemFont) {
        NS_AddGenericFontFromPref(mGenericFont, mLangGroup, mPattern,
                                  gXftFontLoad);
    }

    
    if (mGenericFont && !mFont.systemFont)
        NS_AddFFRE(mPattern, mGenericFont, PR_FALSE);

    if (PR_LOG_TEST(gXftFontLoad, PR_LOG_DEBUG)) {
        
        if (mGenericFont && !mFont.systemFont) {
            printf("\tadding generic family: %s\n", mGenericFont->get());
        }

        
        printf("\tpixel,twip size: %f,%d\n", mPixelSize, mFont.size);

        
        printf("\tslant: ");
        switch(mFont.style) {
        case NS_FONT_STYLE_ITALIC:
            printf("italic\n");
            break;
        case NS_FONT_STYLE_OBLIQUE:
            printf("oblique\n");
            break;
        default:
            printf("roman\n");
            break;
        }

        
        printf("\tweight: (orig,calc) %d,%d\n",
               mFont.weight, NS_CalculateWeight(mFont.weight));

    }        

    
    
    
    
    FcPatternAddDouble(mPattern, FC_PIXEL_SIZE, mPixelSize + 0.000001);

    
    FcPatternAddInteger(mPattern, FC_SLANT,
                        NS_CalculateSlant(mFont.style));

    
    FcPatternAddInteger(mPattern, FC_WEIGHT,
                        NS_CalculateWeight(mFont.weight));

    
    FcConfigSubstitute(0, mPattern, FcMatchPattern);
    XftDefaultSubstitute(GDK_DISPLAY(), DefaultScreen(GDK_DISPLAY()),
                         mPattern);
}

void
nsFontMetricsXft::DoMatch(PRBool aMatchAll)
{
    FcFontSet *set = nsnull;
    
    FcResult   result;

    if (aMatchAll) {
        set = FcFontSort(0, mPattern, FcTrue, NULL, &result);
        if (!set || set->nfont == 1) {
            
            
            
            
            

            NS_WARNING("Detected buggy fontconfig, falling back to generic font");

            nsCAutoString genericFont;
            if (mGenericFont)
                genericFont.Assign(*mGenericFont);

            mFontList.Clear();
            mFontIsGeneric.Clear();

            mFontList.AppendCString(genericFont);
            mFontIsGeneric.AppendElement((void*) PR_TRUE);
            mGenericFont = mFontList.CStringAt(0);

            FcPatternDestroy(mPattern);
            SetupFCPattern();

            if (set)
                FcFontSetDestroy(set);

            set = FcFontSort(0, mPattern, FcTrue, NULL, &result);
        }
    }
    else {
        FcPattern* font = FcFontMatch(0, mPattern, &result);
        if (font) {
            set = FcFontSetCreate();
            FcFontSetAdd(set, font);
        }
    }

    
    if (!set) {
        goto loser;
    }

    if (PR_LOG_TEST(gXftFontLoad, PR_LOG_DEBUG)) {
        printf("matched the following (%d) fonts:\n", set->nfont);
    }

    
    
    
    for (int i=mLoadedFonts.Count(); i < set->nfont; ++i) {
        if (PR_LOG_TEST(gXftFontLoad, PR_LOG_DEBUG)) {
            char *name;
            FcPatternGetString(set->fonts[i], FC_FAMILY, 0, (FcChar8 **)&name);
            printf("\t%s\n", name);
        }

        nsFontXft *font;
        nsFontXftInfo *info;
        nsCOMPtr<nsIUnicodeEncoder> converter = 0;

        info = GetFontXftInfo(set->fonts[i]);
        if (info) {
            if (info->mFontType == eFontTypeUnicode)
                font = new nsFontXftUnicode(mPattern, set->fonts[i]);
            else
                font = new nsFontXftCustom(mPattern, set->fonts[i], info);
        }
        else {  
            font = new nsFontXftUnicode(mPattern, set->fonts[i]);
        }

        if (!font)
            goto loser;

        
        mLoadedFonts.AppendElement((void *)font);
    }

    
    FcFontSetDestroy(set);
    set = nsnull;

    
    if (aMatchAll)
        mMatchType = eAllMatching;
    else
        mMatchType = eBestMatch;
    return;

    
 loser:
    NS_WARNING("nsFontMetricsXft::DoMatch failed to match anything");

    if (set)
        FcFontSetDestroy(set);

    for (PRInt32 i = mLoadedFonts.Count() - 1; i >= 0; --i) {
        nsFontXft *font = (nsFontXft *)mLoadedFonts.ElementAt(i);
        mLoadedFonts.RemoveElementAt(i);
        delete font;
    }
}

gint
nsFontMetricsXft::RawGetWidth(const PRUnichar* aString, PRUint32 aLength)
{
    nscoord width = 0;
    nsresult rv;

    rv = EnumerateGlyphs(aString, aLength,
                         &nsFontMetricsXft::GetWidthCallback, &width);

    if (NS_FAILED(rv))
        width = 0;

    return width;
}

nsresult
nsFontMetricsXft::SetupMiniFont(void)
{
    
    if (mMiniFont)
        return NS_OK;

    FcPattern *pattern = nsnull;
    XftFont *font = nsnull;
    XftFont *xftFont = mWesternFont->mXftFont;
    NS_ASSERTION(xftFont, "FindFont returned a bad font");

    mMiniFontAscent = xftFont->ascent;
    mMiniFontDescent = xftFont->descent;

    pattern = FcPatternCreate();
    if (!pattern)
        return NS_ERROR_FAILURE;

    if (gdk_rgb_get_cmap() != gdk_colormap_get_system())
        XftPatternAddBool(mPattern, XFT_RENDER, False);

    FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *)"monospace");

    FcPatternAddInteger(pattern, FC_PIXEL_SIZE, int(0.5 * mPixelSize));

    FcPatternAddInteger(pattern, FC_WEIGHT,
                        NS_CalculateWeight(mFont.weight));

    FcConfigSubstitute(0, pattern, FcMatchPattern);
    XftDefaultSubstitute(GDK_DISPLAY(), DefaultScreen(GDK_DISPLAY()),
                         pattern);

    FcResult res;
    
    FcPattern *pat = FcFontMatch(0, pattern, &res);

    if (pat) {
        font = XftFontOpenPattern(GDK_DISPLAY(), pat);

        if (font) {
            mMiniFont = font;
            pat = nsnull; 
        }
        else {
            font = xftFont;
        }
    }

    
    
    for (int i=0; i < 16; ++i) {
        
        
        char c = i < 10 ? '0' + i : 'A' + i - 10;
        char str[2];
        str[0] = c;
        str[1] = '\0';

        XGlyphInfo extents;
        XftTextExtents8(GDK_DISPLAY(), font,
                        (FcChar8 *)str, 1, &extents);

        mMiniFontWidth = PR_MAX (mMiniFontWidth, extents.width);
        mMiniFontHeight = PR_MAX (mMiniFontHeight, extents.height);
    }

    if (!mMiniFont) {
        mMiniFontWidth /= 2;
        mMiniFontHeight /= 2;
    }

    mMiniFontPadding = PR_MAX(mMiniFontHeight / 10, 1);
    mMiniFontYOffset = ((mMiniFontAscent + mMiniFontDescent) -
                        (mMiniFontHeight * 2 + mMiniFontPadding * 5)) / 2;


    if (pat)
        FcPatternDestroy(pat);
    if (pattern)
        FcPatternDestroy(pattern);

    return NS_OK;
}

nsresult
nsFontMetricsXft::DrawUnknownGlyph(FcChar32   aChar,
                                   nscoord    aX,
                                   nscoord    aY,
                                   XftColor  *aColor,
                                   XftDraw   *aDraw)
{
    int width,height;
    int ndigit = (IS_NON_BMP(aChar)) ? 3 : 2;

    
    
    
    
    
    
    width = mMiniFontWidth * ndigit + mMiniFontPadding * (ndigit + 3);
    height = mMiniFontHeight * 2 + mMiniFontPadding * 5;

    
    
    
    
    XftDrawRect(aDraw, aColor,
                aX, aY - height,
                width, mMiniFontPadding);
    XftDrawRect(aDraw, aColor,
                aX, aY - mMiniFontPadding,
                width, mMiniFontPadding);

    
    XftDrawRect(aDraw, aColor,
                aX,
                aY - height + mMiniFontPadding,
                mMiniFontPadding, height - mMiniFontPadding * 2);
    XftDrawRect(aDraw, aColor,
                aX + width - mMiniFontPadding,
                aY - height + mMiniFontPadding,
                mMiniFontPadding, height - mMiniFontPadding * 2);

    
    
    if (!mMiniFont)
        return NS_OK;

    
    char buf[7];
    PR_snprintf (buf, sizeof(buf), "%0*X", ndigit * 2, aChar);

    
    XftDrawString8(aDraw, aColor, mMiniFont,
                   aX + mMiniFontPadding * 2,
                   aY - mMiniFontHeight - mMiniFontPadding * 3,
                   (FcChar8 *)&buf[0], 1);
    XftDrawString8(aDraw, aColor, mMiniFont,
                   aX + mMiniFontWidth + mMiniFontPadding * 3,
                   aY - mMiniFontHeight - mMiniFontPadding * 3,
                   (FcChar8 *)&buf[1], 1);

    if (ndigit == 2) {
        XftDrawString8(aDraw, aColor, mMiniFont,
                       aX + mMiniFontPadding * 2,
                       aY - mMiniFontPadding * 2,
                       (FcChar8 *)&buf[2], 1);
        XftDrawString8(aDraw, aColor, mMiniFont,
                       aX + mMiniFontWidth + mMiniFontPadding * 3,
                       aY - mMiniFontPadding * 2,
                       (FcChar8 *)&buf[3], 1);

        return NS_OK;
    }

    XftDrawString8(aDraw, aColor, mMiniFont,
                   aX + mMiniFontWidth * 2 + mMiniFontPadding * 4,
                   aY - mMiniFontHeight - mMiniFontPadding * 3,
                   (FcChar8 *)&buf[2], 1);
    XftDrawString8(aDraw, aColor, mMiniFont,
                   aX + mMiniFontPadding * 2,
                   aY - mMiniFontPadding * 2,
                   (FcChar8 *)&buf[3], 1);
    XftDrawString8(aDraw, aColor, mMiniFont,
                   aX + mMiniFontWidth + mMiniFontPadding * 3,
                   aY - mMiniFontPadding * 2,
                   (FcChar8 *)&buf[4], 1);
    XftDrawString8(aDraw, aColor, mMiniFont,
                   aX + mMiniFontWidth * 2 + mMiniFontPadding * 4,
                   aY - mMiniFontPadding * 2,
                   (FcChar8 *)&buf[5], 1);

    return NS_OK;
}

nsresult
nsFontMetricsXft::EnumerateXftGlyphs(const FcChar32 *aString, PRUint32 aLen,
                                     GlyphEnumeratorCallback aCallback,
                                     void *aCallbackData)
{
    nsFontXft* prevFont = nsnull;
    PRUint32 start = 0;
    nsresult rv = NS_OK;
    PRUint32 i = 0;

    for ( ; i < aLen; i ++) {
        nsFontXft *currFont = FindFont(aString[i]);

        
        
        
        
        
        
        if (currFont != prevFont || i - start > 512) {
            if (i > start) {
                rv = (this->*aCallback)(&aString[start], i - start, prevFont,
                                        aCallbackData);
                NS_ENSURE_SUCCESS(rv, rv);
            }
            prevFont = currFont;
            start = i;
        }
    }

    if (i > start)
        rv = (this->*aCallback)(&aString[start], i - start, prevFont,
                                aCallbackData);

    return rv;
}

nsresult
nsFontMetricsXft::EnumerateGlyphs(const PRUnichar *aString,
                                  PRUint32 aLen,
                                  GlyphEnumeratorCallback aCallback,
                                  void *aCallbackData)
{
    PRUint32 len;
    nsAutoFcChar32Buffer charBuffer;

    NS_ENSURE_TRUE(aLen, NS_OK); 

    ConvertUnicharToUCS4(aString, aLen, charBuffer, &len);
    if (!len)
        return NS_ERROR_OUT_OF_MEMORY;

    return EnumerateXftGlyphs(charBuffer.get(), len, aCallback, aCallbackData);
}

nsresult
nsFontMetricsXft::EnumerateGlyphs(const char *aString,
                                  PRUint32 aLen,
                                  GlyphEnumeratorCallback aCallback,
                                  void *aCallbackData)
{
    PRUint32 len;
    nsAutoFcChar32Buffer charBuffer;

    NS_ENSURE_TRUE(aLen, NS_OK); 

    
    ConvertCharToUCS4(aString, aLen, charBuffer, &len);
    if (!len)
        return NS_ERROR_OUT_OF_MEMORY;

    return EnumerateXftGlyphs(charBuffer.get(), len, aCallback, aCallbackData);
}

void
nsFontMetricsXft::PrepareToDraw(nsRenderingContextGTK *aContext,
                                nsDrawingSurfaceGTK *aSurface,
                                XftDraw **aDraw, XftColor &aColor)
{
    
    nscolor rccolor;

    aContext->GetColor(rccolor);

    aColor.pixel = gdk_rgb_xpixel_from_rgb(NS_TO_GDK_RGB(rccolor));
    aColor.color.red = (NS_GET_R(rccolor) << 8) | NS_GET_R(rccolor);
    aColor.color.green = (NS_GET_G(rccolor) << 8) | NS_GET_G(rccolor);
    aColor.color.blue = (NS_GET_B(rccolor) << 8) | NS_GET_B(rccolor);
    aColor.color.alpha = 0xffff;

    *aDraw = aSurface->GetXftDraw();

    nsCOMPtr<nsIRegion> lastRegion;
    nsCOMPtr<nsIRegion> clipRegion;

    aSurface->GetLastXftClip(getter_AddRefs(lastRegion));
    aContext->GetClipRegion(getter_AddRefs(clipRegion));

    
    if (!lastRegion || !clipRegion || !lastRegion->IsEqual(*clipRegion)) {
        aSurface->SetLastXftClip(clipRegion);

        GdkRegion *rgn = nsnull;
        clipRegion->GetNativeRegion((void *&)rgn);

#ifdef MOZ_WIDGET_GTK2
        GdkRegionSetXftClip(rgn, *aDraw);
#endif
    }
}

nsresult
nsFontMetricsXft::DrawStringCallback(const FcChar32 *aString, PRUint32 aLen,
                                     nsFontXft *aFont, void *aData)
{
    DrawStringData *data = (DrawStringData *)aData;

    
    
    if (!aFont) {
        SetupMiniFont();

        for (PRUint32 i = 0; i<aLen; i++) {
            
            
            const FcChar32 ch = aString[i];
            nscoord x = data->x + data->xOffset;
            nscoord y = data->y;

            
            data->context->GetTranMatrix()->TransformCoord(&x, &y);

            DrawUnknownGlyph(ch, x, y + mMiniFontYOffset, &data->color,
                             data->draw);

            if (data->spacing) {
                data->xOffset += *data->spacing;
                data->spacing += IS_NON_BMP(ch) ? 2 : 1;
            }
            else {
                data->xOffset +=
                    NSToCoordRound((mMiniFontWidth*(IS_NON_BMP(ch) ? 3 : 2) +
                                mMiniFontPadding*(IS_NON_BMP(ch) ? 6:5)) *
                            data->p2t);
            }
        }

        
        return NS_OK;
    }

    
    
    return aFont->DrawStringSpec(NS_CONST_CAST(FcChar32 *, aString), 
                                 aLen, data);
}

nsresult
nsFontMetricsXft::TextDimensionsCallback(const FcChar32 *aString, PRUint32 aLen,
                                         nsFontXft *aFont, void *aData)
{
    nsTextDimensions *dimensions = (nsTextDimensions *)aData;

    if (!aFont) {
        SetupMiniFont();
        for (PRUint32 i = 0; i<aLen; i++) {
            dimensions->width += 
                mMiniFontWidth * (IS_NON_BMP(aString[i]) ? 3 : 2) +
                mMiniFontPadding * (IS_NON_BMP(aString[i]) ? 6 : 5);
        }

        if (dimensions->ascent < mMiniFontAscent)
            dimensions->ascent = mMiniFontAscent;
        if (dimensions->descent < mMiniFontDescent)
            dimensions->descent = mMiniFontDescent;

        return NS_OK;
    }

    
    
    XGlyphInfo glyphInfo;
    nsresult rv = aFont->GetTextExtents32(aString, aLen, glyphInfo);
    NS_ENSURE_SUCCESS(rv, rv);

    dimensions->width += glyphInfo.xOff;

    nscoord tmpMaxAscent = aFont->GetMaxAscent();
    nscoord tmpMaxDescent = aFont->GetMaxDescent();

    if (dimensions->ascent < tmpMaxAscent)
        dimensions->ascent = tmpMaxAscent;
    if (dimensions->descent < tmpMaxDescent)
        dimensions->descent = tmpMaxDescent;

    return NS_OK;
}

nsresult
nsFontMetricsXft::GetWidthCallback(const FcChar32 *aString, PRUint32 aLen,
                                   nsFontXft *aFont, void *aData)
{
    nscoord *width = (nscoord*)aData;

    if (!aFont) {
        SetupMiniFont();
        for (PRUint32 i = 0; i < aLen; i++) {
            *width += mMiniFontWidth * (IS_NON_BMP(aString[i]) ? 3 : 2) + 
                      mMiniFontPadding * (IS_NON_BMP(aString[i]) ? 6 : 5);
        }
        return NS_OK;
    }

    *width += aFont->GetWidth32(aString, aLen);
    return NS_OK;
}

#ifdef MOZ_MATHML
nsresult
nsFontMetricsXft::BoundingMetricsCallback(const FcChar32 *aString, 
                                          PRUint32 aLen, nsFontXft *aFont, 
                                          void *aData)
{
    BoundingMetricsData *data = (BoundingMetricsData *)aData;
    nsBoundingMetrics bm;

    if (!aFont) {
        SetupMiniFont();

        
        for (PRUint32 i = 0; i < aLen; i++) {
            bm.width += mMiniFontWidth * (IS_NON_BMP(aString[i]) ? 3 : 2) +
                        mMiniFontPadding * (IS_NON_BMP(aString[i]) ? 6 : 5);
            bm.rightBearing += bm.width; 
        }
        bm.ascent = mMiniFontAscent;
        bm.descent = mMiniFontDescent;
    }
    else {
        nsresult rv;
        rv = aFont->GetBoundingMetrics32(aString, aLen, bm);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    if (data->firstTime) {  
        *(data->bm) = bm;
        data->firstTime = PR_FALSE;
    }
    else {
        *(data->bm) += bm;
    }

    return NS_OK;
}
#endif 


nsresult
nsFontMetricsXft::FamilyExists(nsIDeviceContext *aDevice,
                               const nsString &aName)
{
    
    NS_ConvertUTF16toUTF8 name(aName);

    FcFontSet *set = nsnull;
    FcObjectSet *os = nsnull;

    FcPattern *pat = FcPatternCreate();
    if (!pat)
        return NS_ERROR_FAILURE;

    nsresult rv = NS_ERROR_FAILURE;
    
    
    
    os = FcObjectSetBuild(FC_FAMILY, NULL);
    if (!os)
        goto end;

    set = FcFontList(0, pat, os);

    if (!set || !set->nfont)
        goto end;

    for (int i = 0; i < set->nfont; ++i) {
        const char *tmpname = NULL;
        if (FcPatternGetString(set->fonts[i], FC_FAMILY, 0,
                               (FcChar8 **)&tmpname) != FcResultMatch) {
            continue;
        }

        
        if (!Compare(nsDependentCString(tmpname), name,
                     nsCaseInsensitiveCStringComparator())) {
            rv = NS_OK;
            break;
        }
    }

 end:
    if (set)
        FcFontSetDestroy(set);
    if (os)
        FcObjectSetDestroy(os);

    FcPatternDestroy(pat);

    return rv;
}


PRBool
nsFontMetricsXft::EnumFontCallback(const nsString &aFamily, PRBool aIsGeneric,
                                   void *aData)
{
    NS_ConvertUTF16toUTF8 name(aFamily);

    
    
    
    
    ToLowerCase(name);
    nsFontMetricsXft *metrics = (nsFontMetricsXft *)aData;
    metrics->mFontList.AppendCString(name);
    metrics->mFontIsGeneric.AppendElement((void *)aIsGeneric);
    if (aIsGeneric) {
        metrics->mGenericFont = 
            metrics->mFontList.CStringAt(metrics->mFontList.Count() - 1);
        return PR_FALSE; 
    }

    return PR_TRUE; 
}



nsFontEnumeratorXft::nsFontEnumeratorXft()
{
}

NS_IMPL_ISUPPORTS1(nsFontEnumeratorXft, nsIFontEnumerator)

NS_IMETHODIMP
nsFontEnumeratorXft::EnumerateAllFonts(PRUint32 *aCount, PRUnichar ***aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;
    NS_ENSURE_ARG_POINTER(aCount);
    *aCount = 0;

    return EnumFontsXft(nsnull, nsnull, aCount, aResult);
}

NS_IMETHODIMP
nsFontEnumeratorXft::EnumerateFonts(const char *aLangGroup,
                                    const char *aGeneric,
                                    PRUint32 *aCount, PRUnichar ***aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;
    NS_ENSURE_ARG_POINTER(aCount);
    *aCount = 0;

    
    
    nsCOMPtr<nsIAtom> langGroup;
    if (aLangGroup && *aLangGroup)
      langGroup = do_GetAtom(aLangGroup);
    const char* generic = nsnull;
    if (aGeneric && *aGeneric)
      generic = aGeneric;

    return EnumFontsXft(langGroup, generic, aCount, aResult);
}

NS_IMETHODIMP
nsFontEnumeratorXft::HaveFontFor(const char *aLangGroup, PRBool *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = PR_FALSE;
    NS_ENSURE_ARG_POINTER(aLangGroup);

    *aResult = PR_TRUE; 
    
    return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorXft::GetDefaultFont(const char *aLangGroup, 
  const char *aGeneric, PRUnichar **aResult)
{
  
  

  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

#if 0
  
  
  

  FcResult res;
  FcPattern* match_pattern = NULL;
  if (aGeneric && *aGeneric)
    match_pattern = FcNameParse(aGeneric);
  else
    match_pattern = FcPatternCreate();

  if (!match_pattern)
    return NS_OK; 

  if (aLangGroup && *aLangGroup) {
    nsCOMPtr<nsIAtom> langGroup = do_GetAtom(aLangGroup);
    NS_AddLangGroup(match_pattern, langGroup);
  }

  FcConfigSubstitute(0, match_pattern, FcMatchPattern); 
  FcDefaultSubstitute(match_pattern);
  FcPattern* result_pattern = FcFontMatch(0, match_pattern, &res);
  if (result_pattern) {
    char *family;
    FcPatternGetString(result_pattern, FC_FAMILY, 0, (FcChar8 **)&family);
    PRUnichar* name = NS_STATIC_CAST(PRUnichar *,
                              nsMemory::Alloc ((strlen (family) + 1)
                                               * sizeof (PRUnichar)));
    if (name) {
      PRUnichar *r = name;
      for (char *f = family; *f; ++f)
        *r++ = *f;
      *r = '\0';

      *aResult = name;
    }

    FcPatternDestroy(result_pattern);
  }
  FcPatternDestroy(match_pattern);
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorXft::UpdateFontList(PRBool *_retval)
{
    *_retval = PR_FALSE; 
    return NS_OK;
}



nsFontXft::nsFontXft(FcPattern *aPattern, FcPattern *aFontName)
{
    
    mPattern = aPattern;
    mFontName = aFontName;
    
    FcPatternReference(aPattern);
    FcPatternReference(mFontName);

    mXftFont = nsnull;

    
    mCharset = nsnull;
    FcCharSet *charset = nsnull;

    
    FcPatternGetCharSet(aFontName, FC_CHARSET, 0, &charset);
    if (charset)
        mCharset = FcCharSetCopy(charset);
}

nsFontXft::~nsFontXft()
{
    if (mXftFont)
        XftFontClose(GDK_DISPLAY(), mXftFont);
    if (mCharset)
        FcCharSetDestroy(mCharset);
    if (mPattern)
        FcPatternDestroy(mPattern);
    if (mFontName)
        FcPatternDestroy(mFontName);
}

XftFont *
nsFontXft::GetXftFont(void)
{
    if (!mXftFont) {
        FcPattern *pat = FcFontRenderPrepare(0, mPattern, mFontName);
        if (!pat)
            return nsnull;

        
        
        
        
        
        
        
        
        
        
        if (FcGetVersion() < 20300)
            FcPatternDel(pat, FC_SPACING);

        mXftFont = XftFontOpenPattern(GDK_DISPLAY(), pat);
        if (!mXftFont)
            FcPatternDestroy(pat);
    }

    return mXftFont;
}


nsresult
nsFontXft::GetTextExtents32(const FcChar32 *aString, PRUint32 aLen, 
                            XGlyphInfo &aGlyphInfo)
{
    NS_PRECONDITION(mXftFont, "FindFont should not return bad fonts");

    
    XftTextExtents32(GDK_DISPLAY(), mXftFont,
                     NS_CONST_CAST(FcChar32*, aString), aLen, &aGlyphInfo);

    return NS_OK;
}

gint
nsFontXft::GetWidth32(const FcChar32 *aString, PRUint32 aLen)
{
    XGlyphInfo glyphInfo;
    GetTextExtents32(aString, aLen, glyphInfo);

    return glyphInfo.xOff;
}

#ifdef MOZ_MATHML
nsresult
nsFontXft::GetBoundingMetrics32(const FcChar32*    aString,
                                PRUint32           aLength,
                                nsBoundingMetrics& aBoundingMetrics)
{
    aBoundingMetrics.Clear ();

    if (aString && aLength) {
        XGlyphInfo glyphInfo;
        GetTextExtents32 (aString, aLength, glyphInfo);
        aBoundingMetrics.leftBearing = - glyphInfo.x;
        aBoundingMetrics.rightBearing = glyphInfo.width - glyphInfo.x;
        aBoundingMetrics.ascent = glyphInfo.y;
        aBoundingMetrics.descent = glyphInfo.height - glyphInfo.y;
        aBoundingMetrics.width = glyphInfo.xOff;
    }
    return NS_OK;
}
#endif 

PRInt16
nsFontXft::GetMaxAscent(void)
{
    NS_PRECONDITION(mXftFont, "FindFont should not return bad fonts");
    return mXftFont->ascent;
}

PRInt16
nsFontXft::GetMaxDescent(void)
{
    NS_PRECONDITION(mXftFont, "FindFont should not return bad fonts");
    return mXftFont->descent;
}

FT_UInt
nsFontXft::CharToGlyphIndex(FcChar32 aChar)
{
    return XftCharIndex(GDK_DISPLAY(), mXftFont, aChar);
}


nsresult
nsFontXft::DrawStringSpec(FcChar32 *aString, PRUint32 aLen, void *aData)
{
    NS_PRECONDITION(mXftFont, "FindFont should not return bad fonts");
    DrawStringData *data = (DrawStringData *)aData;

    FcChar32 *pstr = aString;
    const FcChar32 *end = aString + aLen;

    while(pstr < end) {
        nscoord x = data->x + data->xOffset;               
        nscoord y = data->y;                        
           
        data->context->GetTranMatrix()->TransformCoord(&x, &y);
                                                                 
        


                                                  

        FT_UInt glyph = CharToGlyphIndex(*pstr);
        data->drawBuffer->Draw(x, y, mXftFont, glyph);

        if (data->spacing) {
            data->xOffset += *data->spacing;
            data->spacing += IS_NON_BMP(*pstr) ? 2 : 1; 
        }
        else {
            XGlyphInfo info;                        
            XftGlyphExtents(GDK_DISPLAY(), mXftFont, &glyph, 1, &info);
            data->xOffset += NSToCoordRound(info.xOff * data->p2t);
        }

        ++pstr;
    }                                                          
    return NS_OK;
}


  
nsFontXftUnicode::~nsFontXftUnicode()
{
}

PRBool
nsFontXftUnicode::HasChar(PRUint32 aChar)
{
    return FcCharSetHasChar(mCharset, (FcChar32) aChar);
}



nsFontXftCustom::~nsFontXftCustom()
{
    if (mXftFont && mFT_Face)
       XftUnlockFace(mXftFont);
}



nsresult
nsFontXftCustom::GetTextExtents32(const FcChar32 *aString, PRUint32 aLen, 
                                  XGlyphInfo &aGlyphInfo)
{
    NS_PRECONDITION(mXftFont, "FindFont should not return bad fonts");

    nsAutoFcChar32Buffer buffer;
    nsresult rv;
    PRUint32 destLen = aLen;
    PRBool isWide = (mFontInfo->mFontType == eFontTypeCustomWide); 

    
    rv = ConvertUCS4ToCustom(NS_CONST_CAST(FcChar32 *, aString), 
                             aLen, destLen, mFontInfo->mConverter,
                             isWide, buffer);
    NS_ENSURE_SUCCESS(rv, rv);
      
    FcChar32 *str = buffer.get();

    
    if (isWide) { 
        XftTextExtents32(GDK_DISPLAY(), mXftFont, str, destLen, &aGlyphInfo);
        return NS_OK;
    }

    
    
    rv = SetFT_FaceCharmap();
    NS_ENSURE_SUCCESS(rv, rv);

    
    for (PRUint32 i = 0; i < destLen; i++) {
        str[i] = FT_Get_Char_Index (mFT_Face, FT_ULong(str[i]));
    }

    XftGlyphExtents(GDK_DISPLAY(), mXftFont, str, destLen, &aGlyphInfo);
        
    return NS_OK;
}

PRBool
nsFontXftCustom::HasChar(PRUint32 aChar)
{
    return (mFontInfo->mCCMap &&
            CCMAP_HAS_CHAR_EXT(mFontInfo->mCCMap, aChar)); 
}

FT_UInt
nsFontXftCustom::CharToGlyphIndex(FcChar32 aChar)
{
    if (mFontInfo->mFontType == eFontTypeCustomWide)
        return XftCharIndex(GDK_DISPLAY(), mXftFont, aChar);
    else
        return FT_Get_Char_Index(mFT_Face, aChar);
}



nsresult
nsFontXftCustom::DrawStringSpec(FcChar32* aString, PRUint32 aLen,
                                void* aData)
{
    NS_PRECONDITION(mXftFont, "FindFont should not return bad fonts");

    nsresult rv = NS_OK;
    nsAutoFcChar32Buffer buffer;
    PRUint32 destLen = aLen;
    PRBool isWide = (mFontInfo->mFontType == eFontTypeCustomWide); 

    rv = ConvertUCS4ToCustom(aString, aLen, destLen, mFontInfo->mConverter, 
                             isWide, buffer);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!isWide) {
        
        
        
        
        
        
        
        rv = SetFT_FaceCharmap();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    FcChar32 *str = buffer.get();

    return nsFontXft::DrawStringSpec(str, destLen, aData);
}

nsresult
nsFontXftCustom::SetFT_FaceCharmap(void)
{
    NS_PRECONDITION(mXftFont, "FindFont should not return bad fonts");

    if (mFT_Face)
        return NS_OK;

    mFT_Face = XftLockFace(mXftFont);

    NS_ENSURE_TRUE(mFT_Face != nsnull, NS_ERROR_UNEXPECTED);

    
    
        
    if (FT_Select_Charmap (mFT_Face, mFontInfo->mFT_Encoding))
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

void
nsAutoDrawSpecBuffer::Draw(nscoord x, nscoord y, XftFont *font, FT_UInt glyph)
{
    if (mSpecPos >= BUFFER_LEN-1)
        Flush();

    
    
    NS_ASSERTION(sizeof(short) == sizeof(mSpecBuffer[mSpecPos].x), "Unexpected coordinate type");
    if ((short)x != x || (short)y != y){
        NS_WARNING("ignoring coordinate overflow");
        return;
    }

    mSpecBuffer[mSpecPos].x = x;
    mSpecBuffer[mSpecPos].y = y;
    mSpecBuffer[mSpecPos].font = font;
    mSpecBuffer[mSpecPos].glyph = glyph;
    ++mSpecPos;
}

void
nsAutoDrawSpecBuffer::Flush()
{
    if (mSpecPos) {
        
        
        
        for (PRUint32 i = 0; i < mSpecPos; i++) {
            XftGlyphFontSpec *sp = &mSpecBuffer[i];
            XGlyphInfo info;
            XftGlyphExtents(GDK_DISPLAY(), sp->font, &sp->glyph, 1, &info);
            if (info.width && info.height) {
                
                
                XftDrawGlyphFontSpec(mDraw, mColor, mSpecBuffer+i, mSpecPos-i);
                break;
            }
        }
        mSpecPos = 0;
    }
}




int
CompareFontNames (const void* aArg1, const void* aArg2, void* aClosure)
{
    const PRUnichar* str1 = *((const PRUnichar**) aArg1);
    const PRUnichar* str2 = *((const PRUnichar**) aArg2);

    return nsCRT::strcmp(str1, str2);
}


nsresult
EnumFontsXft(nsIAtom* aLangGroup, const char* aGeneric,
             PRUint32* aCount, PRUnichar*** aResult)
{
    FcPattern   *pat = NULL;
    FcObjectSet *os  = NULL;
    FcFontSet   *fs  = NULL;
    nsresult     rv  = NS_ERROR_FAILURE;

    PRUnichar **array = NULL;
    PRUint32    narray = 0;
    PRInt32     serif = 0, sansSerif = 0, monospace = 0, nGenerics;

    *aCount = 0;
    *aResult = nsnull;

    pat = FcPatternCreate();
    if (!pat)
        goto end;

    os = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, NULL);
    if (!os)
        goto end;

    
    if (aLangGroup)
        NS_AddLangGroup(pat, aLangGroup);

    
    fs = FcFontList(0, pat, os);

    if (!fs)
        goto end;

    
    
    if (!aGeneric)
        serif = sansSerif = monospace = 1;
    else if (!strcmp(aGeneric, "serif"))
        serif = 1;
    else if (!strcmp(aGeneric, "sans-serif"))
        sansSerif = 1;
    else if (!strcmp(aGeneric, "monospace"))
        monospace = 1;
    else if (!strcmp(aGeneric, "cursive") || !strcmp(aGeneric, "fantasy"))
        serif = sansSerif =  1;
    else
        NS_NOTREACHED("unexpected generic family");
    nGenerics = serif + sansSerif + monospace;

    array = NS_STATIC_CAST(PRUnichar **,
               nsMemory::Alloc((fs->nfont + nGenerics) * sizeof(PRUnichar *)));
    if (!array)
        goto end;

    if (serif) {
        PRUnichar *name = ToNewUnicode(NS_LITERAL_STRING("serif"));
        if (!name)
            goto end;
        array[narray++] = name;
    }

    if (sansSerif) {
        PRUnichar *name = ToNewUnicode(NS_LITERAL_STRING("sans-serif"));
        if (!name)
            goto end;
        array[narray++] = name;
    }

    if (monospace) {
        PRUnichar *name = ToNewUnicode(NS_LITERAL_STRING("monospace"));
        if (!name)
            goto end;
        array[narray++] = name;
    }

    for (int i=0; i < fs->nfont; ++i) {
        char *family;

        
        if (FcPatternGetString (fs->fonts[i], FC_FAMILY, 0,
                                (FcChar8 **) &family) != FcResultMatch) {
            continue;
        }

        
        PRUnichar* name =  UTF8ToNewUnicode(nsDependentCString(family));

        if (!name)
            goto end;

        array[narray++] = name;
    }

    NS_QuickSort(array + nGenerics, narray - nGenerics, sizeof (PRUnichar*),
                 CompareFontNames, nsnull);

    *aCount = narray;
    if (narray)
        *aResult = array;
    else
        nsMemory::Free(array);

    rv = NS_OK;

 end:
    if (NS_FAILED(rv) && array) {
        while (narray)
            nsMemory::Free (array[--narray]);
        nsMemory::Free (array);
    }
    if (pat)
        FcPatternDestroy(pat);
    if (os)
        FcObjectSetDestroy(os);
    if (fs)
        FcFontSetDestroy(fs);

    return rv;
}


void
ConvertCharToUCS4(const char *aString, PRUint32 aLength, 
                  nsAutoFcChar32Buffer &aOutBuffer, PRUint32 *aOutLen)
{
    *aOutLen = 0;
    FcChar32 *outBuffer;

    if (!aOutBuffer.EnsureElemCapacity(aLength))
        return;
    outBuffer  = aOutBuffer.get();
    
    for (PRUint32 i = 0; i < aLength; ++i) {
        outBuffer[i] = PRUint8(aString[i]); 
    }

    *aOutLen = aLength;
}


  

void
ConvertUnicharToUCS4(const PRUnichar *aString, PRUint32 aLength,
                     nsAutoFcChar32Buffer &aOutBuffer, PRUint32 *aOutLen)
{
    *aOutLen = 0;
    FcChar32 *outBuffer;

    if (!aOutBuffer.EnsureElemCapacity(aLength))
        return;
    outBuffer  = aOutBuffer.get();

    PRUint32 outLen = 0;

    
    
    for (PRUint32 i = 0; i < aLength; ++i) {
        PRUnichar c = aString[i];

        
        if (IS_NON_SURROGATE(c)) {
            outBuffer[outLen] = c;
        }
        else if (NS_IS_HIGH_SURROGATE(aString[i])) {
            if (i + 1 < aLength && NS_IS_LOW_SURROGATE(aString[i+1])) {
                outBuffer[outLen] = SURROGATE_TO_UCS4(c, aString[i + 1]);
                ++i;
            }
            else { 
                outBuffer[outLen] = UCS2_REPLACEMENT;
            }
        }
        else if (NS_IS_LOW_SURROGATE(aString[i])) { 
            outBuffer[outLen] = UCS2_REPLACEMENT;
        }

        outLen++;
    }

    *aOutLen = outLen;
}

#ifdef MOZ_WIDGET_GTK2

void
GdkRegionSetXftClip(GdkRegion *aGdkRegion, XftDraw *aDraw)
{
    GdkRectangle  *rects   = nsnull;
    int            n_rects = 0;

    gdk_region_get_rectangles(aGdkRegion, &rects, &n_rects);

    XRectangle *xrects = g_new(XRectangle, n_rects);

    for (int i=0; i < n_rects; ++i) {
        xrects[i].x = CLAMP(rects[i].x, G_MINSHORT, G_MAXSHORT);
        xrects[i].y = CLAMP(rects[i].y, G_MINSHORT, G_MAXSHORT);
        xrects[i].width = CLAMP(rects[i].width, G_MINSHORT, G_MAXSHORT);
        xrects[i].height = CLAMP(rects[i].height, G_MINSHORT, G_MAXSHORT);
    }

    XftDrawSetClipRectangles(aDraw, 0, 0, xrects, n_rects);

    g_free(xrects);
    g_free(rects);
}
#endif 




nsresult
GetEncoding(const char *aFontName, char **aEncoding, nsXftFontType &aType,
            FT_Encoding &aFTEncoding)
{
  
    if ((!strcmp(aFontName, "Helvetica" )) ||
         (!strcmp(aFontName, "Times" )) ||
         (!strcmp(aFontName, "Times New Roman" )) ||
         (!strcmp(aFontName, "Courier New" )) ||
         (!strcmp(aFontName, "Courier" )) ||
         (!strcmp(aFontName, "Arial" )) ||
         (!strcmp(aFontName, "MS P Gothic" )) ||
        (!strcmp(aFontName, "Verdana" ))) {
        
        return NS_ERROR_NOT_AVAILABLE; 
    }

    nsCAutoString name;
    name.Assign(NS_LITERAL_CSTRING("encoding.") + 
                nsDependentCString(aFontName) + NS_LITERAL_CSTRING(".ttf"));

    name.StripWhitespace();
    ToLowerCase(name);

    
    if (!gFontEncodingProperties)
        NS_LoadPersistentPropertiesFromURISpec(&gFontEncodingProperties,
            NS_LITERAL_CSTRING("resource://gre/res/fonts/fontEncoding.properties"));

    nsAutoString encoding;
    *aEncoding = nsnull;
    if (gFontEncodingProperties) {
        nsresult rv = gFontEncodingProperties->GetStringProperty(name,
                                                                 encoding);
        if (NS_FAILED(rv)) 
            return NS_ERROR_NOT_AVAILABLE;  

        
        if (encoding.Length() > 5 && 
            StringEndsWith(encoding, NS_LITERAL_STRING(".wide"))) {
            aType = eFontTypeCustomWide;
            encoding.Truncate(encoding.Length() - 5);
        }
        else  {
            aType = eFontTypeCustom;

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
          
            nsAutoString ftCharMap; 
            nsresult rv = gFontEncodingProperties->GetStringProperty(
                          Substring(name, 0, name.Length() - 4) + 
                          NS_LITERAL_CSTRING(".ftcmap"), ftCharMap);
          
            if (NS_FAILED(rv)) 
                aFTEncoding = ft_encoding_none;
            else if (ftCharMap.LowerCaseEqualsLiteral("mac_roman"))
                aFTEncoding = ft_encoding_apple_roman;
            else if (ftCharMap.LowerCaseEqualsLiteral("unicode"))
                aFTEncoding = ft_encoding_unicode;
        }

        
        *aEncoding = ToNewCString(encoding);
        if (PR_LOG_TEST(gXftFontLoad, PR_LOG_DEBUG)) {
          printf("\t\tc> it's %s and encoding is %s\n",
                  aType==eFontTypeCustom ? "narrow" : "wide", *aEncoding);
        }

        return NS_OK;
    }

    return NS_ERROR_NOT_AVAILABLE;
}


static nsresult
GetConverter(const char* aEncoding, nsIUnicodeEncoder **aConverter)
{
    nsresult rv;

    if (!gCharsetManager) {
        CallGetService(kCharsetConverterManagerCID, &gCharsetManager);
        if (!gCharsetManager) {
            FreeGlobals();
            return NS_ERROR_FAILURE;
        }
    }

    
    
    
    rv = gCharsetManager->GetUnicodeEncoderRaw(aEncoding, aConverter);
    NS_ENSURE_SUCCESS(rv, rv);
    if (PR_LOG_TEST(gXftFontLoad, PR_LOG_DEBUG)) {
        printf("\t\tc> got the converter for %s \n", aEncoding);
    }

    return (*aConverter)->SetOutputErrorBehavior(
            (*aConverter)->kOnError_Replace, nsnull, '?');
}



nsresult
FreeGlobals(void)
{
    gInitialized = 0;

    NS_IF_RELEASE(gFontEncodingProperties);
    NS_IF_RELEASE(gCharsetManager);

    gFontXftMaps.Clear();

    return NS_OK;
}

nsFontXftInfo*
GetFontXftInfo(FcPattern* aPattern)
{
    const char* family;

    
    if (FcPatternGetString(aPattern, FC_FAMILY, 0, (FcChar8 **) &family) 
         != FcResultMatch) {
        return nsnull;
    }

    NS_ASSERTION(gFontXftMaps.IsInitialized(), "gFontXMaps should be init'd by now.");

    nsFontXftInfo* info;

    
    if (gFontXftMaps.Get(family, &info))
        return info;

    nsCOMPtr<nsIUnicodeEncoder> converter;
    nsXftFontType fontType =  eFontTypeUnicode; 
    nsXPIDLCString encoding;
    FT_Encoding ftEncoding = ft_encoding_unicode;
    PRUint16* ccmap = nsnull;

    
    
    
    
    
    
    
    
    
    if (NS_SUCCEEDED(GetEncoding(family, getter_Copies(encoding), 
                     fontType, ftEncoding))) {
        if (NS_SUCCEEDED(GetConverter(encoding.get(), 
                         getter_AddRefs(converter)))) {
            nsCOMPtr<nsICharRepresentable> mapper(do_QueryInterface(converter));
            if (PR_LOG_TEST(gXftFontLoad, PR_LOG_DEBUG)) {
               printf("\t\tc> got the converter and CMap :%s !!\n",
                      encoding.get());
            }

            if (mapper) {
                ccmap = MapperToCCMap(mapper);
            }
        }
    }

    
    

    info = new nsFontXftInfo; 
    if (!info) 
        return nsnull; 

    info->mCCMap =  ccmap;  
    info->mConverter = converter;
    info->mFontType = fontType;
    info->mFT_Encoding = ftEncoding;

    gFontXftMaps.Put(family, info); 

    return info;
}




nsresult
ConvertUCS4ToCustom(FcChar32 *aSrc,  PRUint32 aSrcLen,
                    PRUint32& aDestLen, nsIUnicodeEncoder *aConverter,
                    PRBool aIsWide, nsAutoFcChar32Buffer& aResult)
{
    nsresult rv = NS_OK;

    nsCOMPtr<nsIUnicodeEncoder> converter = aConverter;
    if (!converter )
        return NS_ERROR_UNEXPECTED;

    
    
    
    
    
    
    PRUnichar *utf16Src  = NS_REINTERPRET_CAST(PRUnichar *, aSrc);
    PRUnichar *utf16Ptr = utf16Src;
    for (PRUint32 i = 0; i < aSrcLen; ++i, ++utf16Ptr) {
        if (!IS_NON_BMP(aSrc[i])) {
            *utf16Ptr = PRUnichar(aSrc[i]);
        }
        else {
            *utf16Ptr = H_SURROGATE(aSrc[i]);
            *++utf16Ptr = L_SURROGATE(aSrc[i]);
        }
    }

    PRInt32 utf16SrcLen = utf16Ptr - utf16Src;
    PRInt32 medLen = utf16SrcLen;
    
    if (aIsWide &&
        NS_FAILED(aConverter->GetMaxLength(utf16Src, utf16SrcLen, &medLen))) {
        return NS_ERROR_UNEXPECTED;
    }

    nsAutoBuffer<char, AUTO_BUFFER_SIZE> medBuffer;
    if (!medBuffer.EnsureElemCapacity(medLen))
        return NS_ERROR_OUT_OF_MEMORY;
    char *med  = medBuffer.get();

    
    rv = converter->Convert(utf16Src, &utf16SrcLen, med, &medLen);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (aIsWide) {
#ifdef IS_LITTLE_ENDIAN
        
        char* pstr = med;
        while (pstr < med + medLen) {
            PRUint8 tmp = pstr[0];
            pstr[0] = pstr[1];
            pstr[1] = tmp;
            pstr += 2; 
        }
#endif
        
        ConvertUnicharToUCS4(NS_REINTERPRET_CAST(PRUnichar *, med),
                             medLen >> 1, aResult, &aDestLen);
        rv = aDestLen ? rv : NS_ERROR_OUT_OF_MEMORY;
    }
    else {
        
        ConvertCharToUCS4(med, medLen, aResult, &aDestLen);
        rv = aDestLen ? rv : NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}
