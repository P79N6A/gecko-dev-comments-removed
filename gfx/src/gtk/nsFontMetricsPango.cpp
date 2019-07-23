





































#include <strings.h>
#include "nsFont.h"
#include "nsIDeviceContext.h"
#include "nsICharsetConverterManager.h"
#include "nsIPref.h"
#include "nsServiceManagerUtils.h"

#define PANGO_ENABLE_BACKEND
#define PANGO_ENABLE_ENGINE

#include "nsFontMetricsPango.h"
#include "nsRenderingContextGTK.h"
#include "nsDeviceContextGTK.h"
#include "nsFontConfigUtils.h"

#include "nsUnicharUtils.h"
#include "nsQuickSort.h"
#include "nsFontConfigUtils.h"

#include <fontconfig/fontconfig.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <freetype/tttables.h>

#include "mozilla-decoder.h"

#define FORCE_PR_LOG
#include "prlog.h"



static PRLogModuleInfo            *gPangoFontLog;
static int                         gNumInstances;







#define FONT_MAX_FONT_SCALE 2

static NS_DEFINE_CID(kCharsetConverterManagerCID,
                     NS_ICHARSETCONVERTERMANAGER_CID);

#ifdef DEBUG
#define DUMP_PRUNICHAR(ustr, ulen) for (PRUint32 llen=0;llen<ulen;llen++) \
                                      printf("0x%x ", ustr[llen]); \
                                   printf("\n");
#endif




#define MOZ_FT_ROUND(x) (((x) + 32) & ~63) // 63 = 2^6 - 1
#define MOZ_FT_TRUNC(x) ((x) >> 6)
#define CONVERT_DESIGN_UNITS_TO_PIXELS(v, s) \
        MOZ_FT_TRUNC(MOZ_FT_ROUND(FT_MulFix((v) , (s))))



static PangoLanguage *GetPangoLanguage(nsIAtom *aLangGroup);

static void   FreeGlobals    (void);

static PangoStyle  CalculateStyle  (PRUint8 aStyle);
static PangoWeight CalculateWeight (PRUint16 aWeight);

static nsresult    EnumFontsPango   (nsIAtom* aLangGroup, const char* aGeneric,
                                     PRUint32* aCount, PRUnichar*** aResult);
static int         CompareFontNames (const void* aArg1, const void* aArg2,
                                     void* aClosure);

nsFontMetricsPango::nsFontMetricsPango()
{
    if (!gPangoFontLog)
        gPangoFontLog = PR_NewLogModule("PangoFont");

    gNumInstances++;

    mPangoFontDesc = nsnull;
    mPangoContext = nsnull;
    mLTRPangoContext = nsnull;
    mRTLPangoContext = nsnull;
    mPangoAttrList = nsnull;
    mIsRTL = PR_FALSE;
    mPangoSpaceWidth = 0;

    static PRBool initialized = PR_FALSE;
    if (initialized)
        return;

    
    if (!mozilla_decoders_init())
        initialized = PR_TRUE;
}

nsFontMetricsPango::~nsFontMetricsPango()
{
    if (mDeviceContext)
        mDeviceContext->FontMetricsDeleted(this);

    if (mPangoFontDesc)
        pango_font_description_free(mPangoFontDesc);

    if (mLTRPangoContext)
        g_object_unref(mLTRPangoContext);

    if (mRTLPangoContext)
        g_object_unref(mRTLPangoContext);

    if (mPangoAttrList)
        pango_attr_list_unref(mPangoAttrList);

    

    if (--gNumInstances == 0)
        FreeGlobals();
}


NS_IMPL_ISUPPORTS1(nsFontMetricsPango, nsIFontMetrics)



NS_IMETHODIMP
nsFontMetricsPango::Init(const nsFont& aFont, nsIAtom* aLangGroup,
                         nsIDeviceContext *aContext)
{
    mFont = aFont;
    mLangGroup = aLangGroup;

    
    mDeviceContext = aContext;
    
    mPointSize = NSTwipsToFloatPoints(mFont.size);

    
    
    nscoord screenPixels = gdk_screen_height();
    mPointSize = PR_MIN(screenPixels * FONT_MAX_FONT_SCALE, mPointSize);

    
    mFont.EnumerateFamilies(nsFontMetricsPango::EnumFontCallback, this);

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

        PRInt32 minimumInt = 0;
        float minimum;
        nsresult res;
        res = prefService->GetIntPref(name.get(), &minimumInt);
        if (NS_FAILED(res))
            prefService->GetDefaultIntPref(name.get(), &minimumInt);

        if (minimumInt < 0)
            minimumInt = 0;

        minimum = minimumInt;

        
        
        minimum = NSTwipsToFloatPoints(NSFloatPixelsToTwips(minimum, mDeviceContext->DevUnitsToAppUnits()));
        if (mPointSize < minimum)
            mPointSize = minimum;
    }

    
    if (mPointSize < 1) {
#ifdef DEBUG
        printf("*** Warning: nsFontMetricsPango created with point size %f\n",
               mPointSize);
#endif
        mPointSize = 1;
    }

    nsresult rv = RealizeFont();
    if (NS_FAILED(rv))
        return rv;

    
    return CacheFontMetrics();
}

nsresult
nsFontMetricsPango::CacheFontMetrics(void)
{
    
    float f;
    float val;
    f = mDeviceContext->DevUnitsToAppUnits();

    mPangoAttrList = pango_attr_list_new();

    GList *items = pango_itemize(mPangoContext,
                                 "a", 0, 1, mPangoAttrList, NULL);

    if (!items)
        return NS_ERROR_FAILURE;

    guint nitems = g_list_length(items);
    if (nitems != 1)
        return NS_ERROR_FAILURE;

    PangoItem *item = (PangoItem *)items->data;
    PangoFcFont  *fcfont = PANGO_FC_FONT(item->analysis.font);
    if (!fcfont)
        return NS_ERROR_FAILURE;

    
    FT_Face face;
    face = pango_fc_font_lock_face(fcfont);
    if (!face)
    	return NS_ERROR_NOT_AVAILABLE;
    	
    TT_OS2 *os2;
    os2 = (TT_OS2 *) FT_Get_Sfnt_Table(face, ft_sfnt_os2);

    
    int size;
    if (FcPatternGetInteger(fcfont->font_pattern, FC_PIXEL_SIZE, 0, &size) !=
        FcResultMatch) {
        size = 12;
    }
    mEmHeight = PR_MAX(1, nscoord(size * f));

    
    val = MOZ_FT_TRUNC(face->size->metrics.ascender);
    mMaxAscent = NSToIntRound(val * f);

    
    val = -MOZ_FT_TRUNC(face->size->metrics.descender);
    mMaxDescent = NSToIntRound(val * f);

    nscoord lineHeight = mMaxAscent + mMaxDescent;

    
    if (lineHeight > mEmHeight)
        mLeading = lineHeight - mEmHeight;
    else
        mLeading = 0;

    
    mMaxHeight = lineHeight;

    
    mEmAscent = nscoord(mMaxAscent * mEmHeight / lineHeight);

    
    mEmDescent = mEmHeight - mEmAscent;

    
    val = MOZ_FT_TRUNC(face->size->metrics.max_advance);
    mMaxAdvance = NSToIntRound(val * f);
    
    
    mMaxStringLength = (PRInt32)floor(32767.0/val);
    mMaxStringLength = PR_MAX(1, mMaxStringLength);

    
    PangoLayout *layout = pango_layout_new(mPangoContext);
    pango_layout_set_text(layout, " ", 1);
    int pswidth, psheight;
    pango_layout_get_size(layout, &pswidth, &psheight);
    mPangoSpaceWidth = pswidth;
    g_object_unref(layout);

    
    nscoord tmpWidth;
    GetWidth(" ", 1, tmpWidth, NULL);
    mSpaceWidth = tmpWidth;

    
    
    
    
    GetWidth("x", 1, tmpWidth, NULL);
    mAveCharWidth = tmpWidth;

    
    if (pango_fc_font_has_char(fcfont, 'x')) {
        PangoRectangle rect;
        PangoGlyph glyph = pango_fc_font_get_glyph (fcfont, 'x');
        pango_font_get_glyph_extents (PANGO_FONT (fcfont), glyph, &rect, NULL);
        mXHeight = NSToIntRound(rect.height * f / PANGO_SCALE);
    }
    else {
        
        mXHeight = nscoord(((float)mMaxAscent) * 0.56 * f);
    }

    
    val = CONVERT_DESIGN_UNITS_TO_PIXELS(face->underline_position,
                                         face->size->metrics.y_scale);
    if (val) {
        mUnderlineOffset = NSToIntRound(val * f);
    }
    else {
        mUnderlineOffset =
            -NSToIntRound(PR_MAX(1, floor(0.1 *
                MOZ_FT_TRUNC(face->size->metrics.height) + 0.5)) * f);
    }

    
    val = CONVERT_DESIGN_UNITS_TO_PIXELS(face->underline_thickness,
                                         face->size->metrics.y_scale);
    if (val) {
        mUnderlineSize = nscoord(PR_MAX(f, NSToIntRound(val * f)));
    }
    else {
        mUnderlineSize =
            NSToIntRound(PR_MAX(1,
               floor(0.05 * MOZ_FT_TRUNC(face->size->metrics.height) + 0.5)) * f);
    }

    
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

    pango_fc_font_unlock_face(fcfont);

    



















    return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPango::Destroy()
{
    mDeviceContext = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPango::GetLangGroup(nsIAtom** aLangGroup)
{
    *aLangGroup = mLangGroup;
    NS_IF_ADDREF(*aLangGroup);

    return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsPango::GetFontHandle(nsFontHandle &aHandle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}



nsresult
nsFontMetricsPango::GetWidth(const char* aString, PRUint32 aLength,
                             nscoord& aWidth,
                             nsRenderingContextGTK *aContext)
{
    PangoLayout *layout = pango_layout_new(mPangoContext);

    pango_layout_set_text(layout, aString, aLength);

    if (mPangoSpaceWidth)
        FixupSpaceWidths(layout, aString);

    int width, height;

    pango_layout_get_size(layout, &width, &height);

    g_object_unref(layout);

    float f;
    f = mDeviceContext->DevUnitsToAppUnits();
    aWidth = NSToCoordRound(width * f / PANGO_SCALE);

    

    return NS_OK;
}

nsresult
nsFontMetricsPango::GetWidth(const PRUnichar* aString, PRUint32 aLength,
                             nscoord& aWidth, PRInt32 *aFontID,
                             nsRenderingContextGTK *aContext)
{
    nsresult rv = NS_OK;
    PangoLayout *layout = pango_layout_new(mPangoContext);

    gchar *text = g_utf16_to_utf8(aString, aLength,
                                  NULL, NULL, NULL);

    if (!text) {
        aWidth = 0;
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetWidth invalid unicode to follow");
        DUMP_PRUNICHAR(aString, aLength)
#endif
        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    gint width, height;

    pango_layout_set_text(layout, text, strlen(text));
    FixupSpaceWidths(layout, text);
    pango_layout_get_size(layout, &width, &height);

    float f;
    f = mDeviceContext->DevUnitsToAppUnits();
    aWidth = NSToCoordRound(width * f / PANGO_SCALE);

    

 loser:
    g_free(text);
    g_object_unref(layout);

    return rv;
}


nsresult
nsFontMetricsPango::GetTextDimensions(const PRUnichar* aString,
                                      PRUint32 aLength,
                                      nsTextDimensions& aDimensions, 
                                      PRInt32* aFontID,
                                      nsRenderingContextGTK *aContext)
{
    nsresult rv = NS_OK;

    PangoLayout *layout = pango_layout_new(mPangoContext);

    gchar *text = g_utf16_to_utf8(aString, aLength,
                                  NULL, NULL, NULL);

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetTextDimensions invalid unicode to follow");
        DUMP_PRUNICHAR(aString, aLength)
#endif
        aDimensions.width = 0;
        aDimensions.ascent = 0;
        aDimensions.descent = 0;

        rv = NS_ERROR_FAILURE;
        goto loser;
    }
        

    pango_layout_set_text(layout, text, strlen(text));
    FixupSpaceWidths(layout, text);

    
    PangoLayoutLine *line;
    if (pango_layout_get_line_count(layout) != 1) {
        printf("Warning: more than one line!\n");
    }
    line = pango_layout_get_line(layout, 0);

    PangoRectangle rect;
    pango_layout_line_get_extents(line, NULL, &rect);

    float P2T;
    P2T = mDeviceContext->DevUnitsToAppUnits();

    aDimensions.width = NSToCoordRound(rect.width * P2T / PANGO_SCALE);
    aDimensions.ascent = NSToCoordRound(PANGO_ASCENT(rect) * P2T / PANGO_SCALE);
    aDimensions.descent = NSToCoordRound(PANGO_DESCENT(rect) * P2T / PANGO_SCALE);

    
    

 loser:
    g_free(text);
    g_object_unref(layout);

    return rv;
}

nsresult
nsFontMetricsPango::GetTextDimensions(const char*         aString,
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

    return GetTextDimensionsInternal(aString, aLength, aAvailWidth, aBreaks,
                                     aNumBreaks, aDimensions, aNumCharsFit,
                                     aLastWordDimensions, aContext);

}

nsresult
nsFontMetricsPango::GetTextDimensions(const PRUnichar*    aString,
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
    nsresult rv = NS_OK;
    PRInt32 curBreak = 0;
    gchar *curChar;

    PRInt32 *utf8Breaks = new PRInt32[aNumBreaks];

    gchar *text = g_utf16_to_utf8(aString, (PRInt32)aLength,
                                  NULL, NULL, NULL);

    curChar = text;

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetWidth invalid unicode to follow");
        DUMP_PRUNICHAR(aString, (PRUint32)aLength)
#endif
        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    
    for (PRInt32 curOffset=0; curOffset < aLength;
         curOffset++, curChar = g_utf8_find_next_char(curChar, NULL)) {
        if (aBreaks[curBreak] == curOffset) {
            utf8Breaks[curBreak] = curChar - text;
            curBreak++;
        }

        if (NS_IS_HIGH_SURROGATE(aString[curOffset]))
            curOffset++;
    }

    
    utf8Breaks[curBreak] = curChar - text;

#if 0
    if (strlen(text) != aLength) {
        printf("Different lengths for utf16 %d and utf8 %d\n", aLength, strlen(text));
        DUMP_PRUNICHAR(aString, aLength)
        DUMP_PRUNICHAR(text, strlen(text))
        for (PRInt32 i = 0; i < aNumBreaks; ++i) {
            printf("  break %d utf16 %d utf8 %d\n", i, aBreaks[i], utf8Breaks[i]);
        }
    }
#endif

    
    
    curBreak = 0;
    rv = GetTextDimensionsInternal(text, strlen(text), aAvailWidth, utf8Breaks,
                                   aNumBreaks, aDimensions, aNumCharsFit,
                                   aLastWordDimensions, aContext);

    
    
    for (PRInt32 i = aNumBreaks - 1; i >= 0; --i) {
        if (utf8Breaks[i] == aNumCharsFit) {
            
            
            aNumCharsFit = aBreaks[i];
            break;
        }
    }

 loser:
    if (text)
        g_free(text);

    delete[] utf8Breaks;

    return rv;
}

nsresult
nsFontMetricsPango::DrawString(const char *aString, PRUint32 aLength,
                               nscoord aX, nscoord aY,
                               const nscoord* aSpacing,
                               nsRenderingContextGTK *aContext,
                               nsDrawingSurfaceGTK *aSurface)
{
    PangoLayout *layout = pango_layout_new(mPangoContext);

    pango_layout_set_text(layout, aString, aLength);
    FixupSpaceWidths(layout, aString);

    int x = aX;
    int y = aY;

    aContext->GetTranMatrix()->TransformCoord(&x, &y);

    PangoLayoutLine *line;
    if (pango_layout_get_line_count(layout) != 1) {
        printf("Warning: more than one line!\n");
    }
    line = pango_layout_get_line(layout, 0);

    aContext->UpdateGC();
    GdkGC *gc = aContext->GetGC();

    if (aSpacing && *aSpacing) {
        DrawStringSlowly(aString, NULL, aLength, aSurface->GetDrawable(),
                         gc, x, y, line, aSpacing);
    }
    else {
        gdk_draw_layout_line(aSurface->GetDrawable(), gc,
                             x, y,
                             line);
    }

    g_object_unref(gc);
    g_object_unref(layout);

    

    return NS_OK;
}

nsresult
nsFontMetricsPango::DrawString(const PRUnichar* aString, PRUint32 aLength,
                               nscoord aX, nscoord aY,
                               PRInt32 aFontID,
                               const nscoord* aSpacing,
                               nsRenderingContextGTK *aContext,
                               nsDrawingSurfaceGTK *aSurface)
{
    nsresult rv = NS_OK;
    int x = aX;
    int y = aY;

    aContext->UpdateGC();
    GdkGC *gc = aContext->GetGC();

    PangoLayout *layout = pango_layout_new(mPangoContext);

    gchar *text = g_utf16_to_utf8(aString, aLength,
                                  NULL, NULL, NULL);

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::DrawString invalid unicode to follow");
        DUMP_PRUNICHAR(aString, aLength)
#endif
        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    pango_layout_set_text(layout, text, strlen(text));
    FixupSpaceWidths(layout, text);

    aContext->GetTranMatrix()->TransformCoord(&x, &y);

    PangoLayoutLine *line;
    if (pango_layout_get_line_count(layout) != 1) {
        printf("Warning: more than one line!\n");
    }
    line = pango_layout_get_line(layout, 0);

    if (aSpacing && *aSpacing) {
        DrawStringSlowly(text, aString, aLength, aSurface->GetDrawable(),
                         gc, x, y, line, aSpacing);
    }
    else {
        gdk_draw_layout_line(aSurface->GetDrawable(), gc,
                             x, y,
                             line);
    }

 loser:

    g_free(text);
    g_object_unref(gc);
    g_object_unref(layout);

    

    return rv;
}

#ifdef MOZ_MATHML
nsresult
nsFontMetricsPango::GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                       nsBoundingMetrics &aBoundingMetrics,
                                       nsRenderingContextGTK *aContext)
{
    printf("GetBoundingMetrics (char *)\n");
    return NS_ERROR_FAILURE;
}

nsresult
nsFontMetricsPango::GetBoundingMetrics(const PRUnichar *aString,
                                       PRUint32 aLength,
                                       nsBoundingMetrics &aBoundingMetrics,
                                       PRInt32 *aFontID,
                                       nsRenderingContextGTK *aContext)
{
    nsresult rv = NS_OK;
    PangoLayout *layout = pango_layout_new(mPangoContext);

    gchar *text = g_utf16_to_utf8(aString, aLength,
                                  NULL, NULL, NULL);

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetBoundingMetrics invalid unicode to follow");
        DUMP_PRUNICHAR(aString, aLength)
#endif
        aBoundingMetrics.Clear();

        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    pango_layout_set_text(layout, text, -1);
    FixupSpaceWidths(layout, text);

    PangoLayoutLine *line;
    if (pango_layout_get_line_count(layout) != 1) {
        printf("Warning: more than one line!\n");
    }
    line = pango_layout_get_line(layout, 0);

    
    PangoRectangle ink, logical;
    pango_layout_line_get_extents(line, &ink, &logical);

    float P2T;
    P2T = mDeviceContext->DevUnitsToAppUnits();

    aBoundingMetrics.leftBearing  = NSToCoordRound(PANGO_LBEARING(ink) * P2T / PANGO_SCALE);
    aBoundingMetrics.rightBearing = NSToCoordRound(PANGO_RBEARING(ink) * P2T / PANGO_SCALE);
    aBoundingMetrics.ascent       = NSToCoordRound(PANGO_ASCENT(ink)   * P2T / PANGO_SCALE);
    aBoundingMetrics.descent      = NSToCoordRound(PANGO_DESCENT(ink)  * P2T / PANGO_SCALE);
    aBoundingMetrics.width        = NSToCoordRound(logical.width       * P2T / PANGO_SCALE);

 loser:
    g_free(text);
    g_object_unref(layout);

    return rv;
}

#endif 

GdkFont*
nsFontMetricsPango::GetCurrentGDKFont(void)
{
    return nsnull;
}

nsresult
nsFontMetricsPango::SetRightToLeftText(PRBool aIsRTL)
{
    if (aIsRTL) {
        if (!mRTLPangoContext) {
            mRTLPangoContext = gdk_pango_context_get();
            pango_context_set_base_dir(mRTLPangoContext, PANGO_DIRECTION_RTL);

            gdk_pango_context_set_colormap(mRTLPangoContext, gdk_rgb_get_cmap());
            pango_context_set_language(mRTLPangoContext, GetPangoLanguage(mLangGroup));
            pango_context_set_font_description(mRTLPangoContext, mPangoFontDesc);
        }
        mPangoContext = mRTLPangoContext;
    }
    else {
        mPangoContext = mLTRPangoContext;
    }

    mIsRTL = aIsRTL;
    return NS_OK;
}

PRBool
nsFontMetricsPango::GetRightToLeftText()
{
    return mIsRTL;
}

nsresult
nsFontMetricsPango::GetClusterInfo(const PRUnichar *aText,
                                   PRUint32 aLength,
                                   PRUint8 *aClusterStarts)
{
    nsresult rv = NS_OK;
    PangoLogAttr *attrs = NULL;
    gint n_attrs = 0;
    PangoLayout *layout = pango_layout_new(mPangoContext);
    
    
    gchar *text = g_utf16_to_utf8(aText, aLength, NULL, NULL, NULL);

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetWidth invalid unicode to follow");
        DUMP_PRUNICHAR(aText, aLength)
#endif
        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    
    pango_layout_set_text(layout, text, strlen(text));
    FixupSpaceWidths(layout, text);

    
    
    pango_layout_get_log_attrs(layout, &attrs, &n_attrs);

    for (PRUint32 pos = 0; pos < aLength; pos++) {
        if (NS_IS_HIGH_SURROGATE(aText[pos])) {
            aClusterStarts[pos] = 1;
            pos++;
        }
        else {
            aClusterStarts[pos] = attrs[pos].is_cursor_position;
        }
    }

 loser:
    if (attrs)
        g_free(attrs);
    if (text)
        g_free(text);
    if (layout)
        g_object_unref(layout);

    return rv;
}

PRInt32
nsFontMetricsPango::GetPosition(const PRUnichar *aText, PRUint32 aLength,
                                nsPoint aPt)
{
    int trailing = 0;
    int inx = 0;
    const gchar *curChar;
    PRInt32 retval = 0;

    float f = mDeviceContext->AppUnitsToDevUnits();
    
    PangoLayout *layout = pango_layout_new(mPangoContext);
    PRUint32 localX = (PRUint32)(aPt.x * PANGO_SCALE * f);
    PRUint32 localY = (PRUint32)(aPt.y * PANGO_SCALE * f);

    
    gchar *text = g_utf16_to_utf8(aText, aLength, NULL, NULL, NULL);

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetWidth invalid unicode to follow");
        DUMP_PRUNICHAR(aText, aLength)
#endif
        retval = -1;
        goto loser;
    }

    
    pango_layout_set_text(layout, text, strlen(text));
    FixupSpaceWidths(layout, text);
    
    pango_layout_xy_to_index(layout, localX, localY,
                             &inx, &trailing);

    
    curChar = text;

    for (PRUint32 curOffset=0; curOffset < aLength;
         curOffset++, curChar = g_utf8_find_next_char(curChar, NULL)) {

        
        if (curChar - text == inx) {
            retval = curOffset;
            break;
        }

        if (NS_IS_HIGH_SURROGATE(aText[curOffset]))
            curOffset++;
    }

    
    
    while (trailing) {
        retval++;
        
        
        if (retval < (PRInt32)aLength && NS_IS_HIGH_SURROGATE(aText[retval]))
            retval++;
        trailing--;
    }

 loser:
    if (text)
        g_free(text);
    if (layout)
        g_object_unref(layout);

    return retval;
}

nsresult
nsFontMetricsPango::GetRangeWidth(const PRUnichar *aText,
                                  PRUint32 aLength,
                                  PRUint32 aStart,
                                  PRUint32 aEnd,
                                  PRUint32 &aWidth)
{
    nsresult rv = NS_OK;
    PRUint32 utf8Start = 0;
    PRUint32 utf8End = 0;

    aWidth = 0;

    
    gchar *text = g_utf16_to_utf8(aText, aLength, NULL, NULL, NULL);
    gchar *curChar = text;

    if (!text) {
#ifdef DEBUG
        NS_WARNING("nsFontMetricsPango::GetWidth invalid unicode to follow");
        DUMP_PRUNICHAR(aText, aLength)
#endif
        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    
    for (PRUint32 curOffset = 0; curOffset < aLength;
         curOffset++, curChar = g_utf8_find_next_char(curChar, NULL)) {

        if (curOffset == aStart)
            utf8Start = curChar - text;

        if (curOffset == aEnd)
            utf8End = curChar - text;
        
        if (NS_IS_HIGH_SURROGATE(aText[curOffset]))
            curOffset++;
    }

    
    if (aLength == aEnd)
        utf8End = strlen(text);

    rv = GetRangeWidth(text, strlen(text), utf8Start, utf8End, aWidth);

 loser:
    if (text)
        g_free(text);

    return rv;
}

nsresult
nsFontMetricsPango::GetRangeWidth(const char *aText,
                                  PRUint32 aLength,
                                  PRUint32 aStart,
                                  PRUint32 aEnd,
                                  PRUint32 &aWidth)
{
    nsresult rv = NS_OK;
    int *ranges = NULL;
    int n_ranges = 0;
    float f;

    aWidth = 0;

    PangoLayout *layout = pango_layout_new(mPangoContext);

    if (!aText) {
        rv = NS_ERROR_FAILURE;
        goto loser;
    }

    pango_layout_set_text(layout, aText, aLength);
    FixupSpaceWidths(layout, aText);

    PangoLayoutLine *line;
    if (pango_layout_get_line_count(layout) != 1) {
        printf("Warning: more than one line!\n");
    }
    line = pango_layout_get_line(layout, 0);

    pango_layout_line_get_x_ranges(line, aStart, aEnd, &ranges, &n_ranges);

    aWidth = (ranges[((n_ranges - 1) * 2) + 1] - ranges[0]);

    f = mDeviceContext-> DevUnitsToAppUnits();
    aWidth = nscoord(aWidth * f / PANGO_SCALE);

 loser:
    if (ranges)
        g_free(ranges);
    if (layout)
        g_object_unref(layout);

    return rv;
}


PRUint32
nsFontMetricsPango::GetHints(void)
{
    return (NS_RENDERING_HINT_BIDI_REORDERING |
            NS_RENDERING_HINT_ARABIC_SHAPING | 
            NS_RENDERING_HINT_FAST_MEASURE |
            NS_RENDERING_HINT_REORDER_SPACED_TEXT |
            NS_RENDERING_HINT_TEXT_CLUSTERS);
}


nsresult
nsFontMetricsPango::FamilyExists(nsIDeviceContext *aDevice,
                                 const nsString &aName)
{
    
    NS_ConvertUTF16toUTF8 name(aName);

    nsresult rv = NS_ERROR_FAILURE;
    PangoContext *context = gdk_pango_context_get();
    PangoFontFamily **familyList;
    int n;

    pango_context_list_families(context, &familyList, &n);

    for (int i=0; i < n; i++) {
        const char *tmpname = pango_font_family_get_name(familyList[i]);
        if (!Compare(nsDependentCString(tmpname), name,
                     nsCaseInsensitiveCStringComparator())) {
            rv = NS_OK;
            break;
        }
    }

    g_free(familyList);
    g_object_unref(context);

    return rv;
}



nsresult
nsFontMetricsPango::RealizeFont(void)
{
    nsCString familyList;
    
    mPangoFontDesc = pango_font_description_new();

    
    
    for (int i=0; i < mFontList.Count(); ++i) {
        
        
        if (mFontIsGeneric[i])
            break;;

        nsCString *familyName = mFontList.CStringAt(i);
        familyList.Append(familyName->get());
        familyList.Append(',');
    }

    
    
    if (mGenericFont && !mFont.systemFont) {
        nsCString name;
        name += "font.name.";
        name += mGenericFont->get();
        name += ".";

        nsString langGroup;
        mLangGroup->ToString(langGroup);

        name.AppendWithConversion(langGroup);

        nsCOMPtr<nsIPref> pref;
        pref = do_GetService(NS_PREF_CONTRACTID);
        if (pref) {
            nsresult rv;
            nsXPIDLCString value;
            rv = pref->GetCharPref(name.get(), getter_Copies(value));

            
            
            if (NS_FFRECountHyphens(value) < 3) {
                nsCString tmpstr;
                tmpstr.Append(value);

                familyList.Append(tmpstr);
                familyList.Append(',');
            }
        }
    }

    
    if (mGenericFont && !mFont.systemFont) {
        familyList.Append(mGenericFont->get());
        familyList.Append(',');
    }

    
    pango_font_description_set_family(mPangoFontDesc,
                                      familyList.get());

    
    pango_font_description_set_size(mPangoFontDesc,
                                    (gint)(mPointSize * PANGO_SCALE));

    
    pango_font_description_set_style(mPangoFontDesc,
                                     CalculateStyle(mFont.style));

    
    pango_font_description_set_weight(mPangoFontDesc,
                                      CalculateWeight(mFont.weight));

    
    
    mLTRPangoContext = gdk_pango_context_get();
    mPangoContext = mLTRPangoContext;

    
    
    pango_context_set_base_dir(mPangoContext, PANGO_DIRECTION_LTR);

    
    gdk_pango_context_set_colormap(mPangoContext, gdk_rgb_get_cmap());

    
    pango_context_set_language(mPangoContext, GetPangoLanguage(mLangGroup));

    
    pango_context_set_font_description(mPangoContext, mPangoFontDesc);

    return NS_OK;
}


PRBool
nsFontMetricsPango::EnumFontCallback(const nsString &aFamily,
                                     PRBool aIsGeneric, void *aData)
{
    NS_ConvertUTF16toUTF8 name(aFamily);

    
    
    
    
    ToLowerCase(name);
    nsFontMetricsPango *metrics = (nsFontMetricsPango *)aData;
    metrics->mFontList.AppendCString(name);
    metrics->mFontIsGeneric.AppendElement((void *)aIsGeneric);
    if (aIsGeneric) {
        metrics->mGenericFont = 
            metrics->mFontList.CStringAt(metrics->mFontList.Count() - 1);
        return PR_FALSE; 
    }

    return PR_TRUE; 
}







void
nsFontMetricsPango::DrawStringSlowly(const gchar *aText,
                                     const PRUnichar *aOrigString,
                                     PRUint32 aLength,
                                     GdkDrawable *aDrawable,
                                     GdkGC *aGC, gint aX, gint aY,
                                     PangoLayoutLine *aLine,
                                     const nscoord *aSpacing)
{
    float app2dev;
    app2dev = mDeviceContext->AppUnitsToDevUnits();
    gint offset = 0;

    









    nscoord *utf8spacing = new nscoord[strlen(aText)];

    if (aOrigString) {
        const gchar *curChar = aText;
        bzero(utf8spacing, sizeof(nscoord) * strlen(aText));

        
        for (PRUint32 curOffset=0; curOffset < aLength;
             curOffset++, curChar = g_utf8_find_next_char(curChar, NULL)) {
            utf8spacing[curChar - aText] = aSpacing[curOffset];

            if (NS_IS_HIGH_SURROGATE(aOrigString[curOffset]))
                curOffset++;
        }
    }
    else {
        memcpy(utf8spacing, aSpacing, (sizeof(nscoord *) * aLength));
    }

    gint curRun = 0;

    for (GSList *tmpList = aLine->runs; tmpList && tmpList->data;
         tmpList = tmpList->next, curRun++) {
        PangoLayoutRun *layoutRun = (PangoLayoutRun *)tmpList->data;
        gint tmpOffset = 0;

        


        for (gint i=0; i < layoutRun->glyphs->num_glyphs; i++) {
            




            gint thisOffset = (gint)(utf8spacing[layoutRun->glyphs->log_clusters[i] + layoutRun->item->offset]
                                     * app2dev * PANGO_SCALE);
            layoutRun->glyphs->glyphs[i].geometry.width = thisOffset;
            tmpOffset += thisOffset;
        }

        
        offset += tmpOffset;
    }

    gdk_draw_layout_line(aDrawable, aGC, aX, aY, aLine);

    delete[] utf8spacing;
}

nsresult
nsFontMetricsPango::GetTextDimensionsInternal(const gchar*        aString,
                                              PRInt32             aLength,
                                              PRInt32             aAvailWidth,
                                              PRInt32*            aBreaks,
                                              PRInt32             aNumBreaks,
                                              nsTextDimensions&   aDimensions,
                                              PRInt32&            aNumCharsFit,
                                              nsTextDimensions&   aLastWordDimensions,
                                              nsRenderingContextGTK *aContext)
{
    NS_PRECONDITION(aBreaks[aNumBreaks - 1] == aLength, "invalid break array");

    
    
    PRInt32 prevBreakState_BreakIndex = -1; 
                                            
    nscoord prevBreakState_Width = 0; 

    
    GetMaxAscent(aLastWordDimensions.ascent);
    GetMaxDescent(aLastWordDimensions.descent);
    aLastWordDimensions.width = -1;
    aNumCharsFit = 0;

    
    nscoord width = 0;
    PRInt32 start = 0;
    nscoord aveCharWidth;
    GetAveCharWidth(aveCharWidth);

    while (start < aLength) {
        
        
        
        
        PRInt32 estimatedNumChars = 0;

        if (aveCharWidth > 0)
            estimatedNumChars = (aAvailWidth - width) / aveCharWidth;

        if (estimatedNumChars < 1)
            estimatedNumChars = 1;

        
        PRInt32 estimatedBreakOffset = start + estimatedNumChars;
        PRInt32 breakIndex;
        nscoord numChars;

        
        
        if (aLength <= estimatedBreakOffset) {
            
            numChars = aLength - start;
            breakIndex = aNumBreaks - 1;
        } 
        else {
            breakIndex = prevBreakState_BreakIndex;
            while (((breakIndex + 1) < aNumBreaks) &&
                   (aBreaks[breakIndex + 1] <= estimatedBreakOffset)) {
                ++breakIndex;
            }

            if (breakIndex == prevBreakState_BreakIndex) {
                ++breakIndex; 
                
            }

            numChars = aBreaks[breakIndex] - start;
        }

        
        nscoord twWidth = 0;
        if ((1 == numChars) && (aString[start] == ' '))
            GetSpaceWidth(twWidth);
        else if (numChars > 0)
            GetWidth(&aString[start], numChars, twWidth, aContext);

        
        PRBool  textFits = (twWidth + width) <= aAvailWidth;

        
        
        if (textFits) {
            aNumCharsFit += numChars;
            width += twWidth;
            start += numChars;

            
            
            prevBreakState_BreakIndex = breakIndex;
            prevBreakState_Width = width;
        }
        else {
            
            
            if (prevBreakState_BreakIndex > 0) {
                
                
                if (prevBreakState_BreakIndex == (breakIndex - 1)) {
                    aNumCharsFit = aBreaks[prevBreakState_BreakIndex];
                    width = prevBreakState_Width;
                    break;
                }
            }

            
            if (0 == breakIndex) {
                
                
                aNumCharsFit += numChars;
                width += twWidth;
                break;
            }

            
            
            width += twWidth;
            while ((breakIndex >= 1) && (width > aAvailWidth)) {
                twWidth = 0;
                start = aBreaks[breakIndex - 1];
                numChars = aBreaks[breakIndex] - start;

                if ((1 == numChars) && (aString[start] == ' '))
                    GetSpaceWidth(twWidth);
                else if (numChars > 0)
                    GetWidth(&aString[start], numChars, twWidth,
                             aContext);
                width -= twWidth;
                aNumCharsFit = start;
                breakIndex--;
            }
            break;
        }
    }

    aDimensions.width = width;
    GetMaxAscent(aDimensions.ascent);
    GetMaxDescent(aDimensions.descent);

    




    return NS_OK;
}

void
nsFontMetricsPango::FixupSpaceWidths (PangoLayout *aLayout,
                                      const char *aString)
{
    PangoLayoutLine *line = pango_layout_get_line(aLayout, 0);

    gint curRun = 0;

    for (GSList *tmpList = line->runs; tmpList && tmpList->data;
         tmpList = tmpList->next, curRun++) {
        PangoLayoutRun *layoutRun = (PangoLayoutRun *)tmpList->data;

        for (gint i=0; i < layoutRun->glyphs->num_glyphs; i++) {
            gint thisOffset = (gint)layoutRun->glyphs->log_clusters[i] + layoutRun->item->offset;
            if (aString[thisOffset] == ' ')
                layoutRun->glyphs->glyphs[i].geometry.width = mPangoSpaceWidth;
        }
    }
}


PangoLanguage *
GetPangoLanguage(nsIAtom *aLangGroup)
{
    
    nsCAutoString cname;
    aLangGroup->ToUTF8String(cname);

    
    
    const MozGtkLangGroup *langGroup;
    langGroup = NS_FindFCLangGroup(cname);

    
    
    
    
    
    if (!langGroup)
        return pango_language_from_string(cname.get());
    else if (langGroup->Lang) 
        return pango_language_from_string((char *) langGroup->Lang);

    return pango_language_from_string("en");
}


void
FreeGlobals(void)
{
}


PangoStyle
CalculateStyle(PRUint8 aStyle)
{
    switch(aStyle) {
    case NS_FONT_STYLE_ITALIC:
        return PANGO_STYLE_ITALIC;
        break;
    case NS_FONT_STYLE_OBLIQUE:
        return PANGO_STYLE_OBLIQUE;
        break;
    }

    return PANGO_STYLE_NORMAL;
}


PangoWeight
CalculateWeight (PRUint16 aWeight)
{
    







    PRInt32 baseWeight = (aWeight + 50) / 100;
    PRInt32 offset = aWeight - baseWeight * 100;

    
    if (baseWeight < 0)
        baseWeight = 0;
    if (baseWeight > 9)
        baseWeight = 9;

    
    static int fcWeightLookup[10] = {
        0, 0, 0, 0, 1, 1, 2, 3, 3, 4,
    };

    PRInt32 fcWeight = fcWeightLookup[baseWeight];

    



    fcWeight += offset;

    if (fcWeight < 0)
        fcWeight = 0;
    if (fcWeight > 4)
        fcWeight = 4;

    
    static int fcWeights[5] = {
        349,
        499,
        649,
        749,
        999
    };

    return (PangoWeight)fcWeights[fcWeight];
}


nsresult
EnumFontsPango(nsIAtom* aLangGroup, const char* aGeneric,
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

    if (!fs->nfont) {
        rv = NS_OK;
        goto end;
    }

    
    
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


int
CompareFontNames (const void* aArg1, const void* aArg2, void* aClosure)
{
    const PRUnichar* str1 = *((const PRUnichar**) aArg1);
    const PRUnichar* str2 = *((const PRUnichar**) aArg2);

    return nsCRT::strcmp(str1, str2);
}




nsFontEnumeratorPango::nsFontEnumeratorPango()
{
}

NS_IMPL_ISUPPORTS1(nsFontEnumeratorPango, nsIFontEnumerator)

NS_IMETHODIMP
nsFontEnumeratorPango::EnumerateAllFonts(PRUint32 *aCount,
                                         PRUnichar ***aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;
    NS_ENSURE_ARG_POINTER(aCount);
    *aCount = 0;

    return EnumFontsPango(nsnull, nsnull, aCount, aResult);
}

NS_IMETHODIMP
nsFontEnumeratorPango::EnumerateFonts(const char *aLangGroup,
                                      const char *aGeneric,
                                      PRUint32 *aCount,
                                      PRUnichar ***aResult)
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

    return EnumFontsPango(langGroup, generic, aCount, aResult);
}

NS_IMETHODIMP
nsFontEnumeratorPango::HaveFontFor(const char *aLangGroup,
                                   PRBool *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = PR_FALSE;
    NS_ENSURE_ARG_POINTER(aLangGroup);

    *aResult = PR_TRUE; 
    
    return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorPango::GetDefaultFont(const char *aLangGroup,
                                      const char *aGeneric,
                                      PRUnichar **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;

    
    

    return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorPango::UpdateFontList(PRBool *_retval)
{
    *_retval = PR_FALSE; 
    return NS_OK;
}
