











































#define PANGO_ENABLE_BACKEND

#include "prtypes.h"
#include "prlink.h"
#include "gfxTypes.h"

#include "nsMathUtils.h"
#include "nsTArray.h"
#include "nsServiceManagerUtils.h"
#include "nsILanguageAtomService.h"

#include "gfxContext.h"
#include "gfxPlatformGtk.h"
#include "gfxPangoFonts.h"
#include "gfxFT2FontBase.h"
#include "gfxFT2Utils.h"
#include "gfxFontconfigUtils.h"
#include "gfxUserFontSet.h"

#include <freetype/tttables.h>

#include <cairo.h>
#include <cairo-ft.h>

#include <fontconfig/fcfreetype.h>
#include <pango/pango.h>
#include <pango/pangofc-fontmap.h>

#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdk.h>
#endif

#include <math.h>

#define FLOAT_PANGO_SCALE ((gfxFloat)PANGO_SCALE)

#ifndef PANGO_VERSION_CHECK
#define PANGO_VERSION_CHECK(x,y,z) 0
#endif
#ifndef PANGO_GLYPH_UNKNOWN_FLAG
#define PANGO_GLYPH_UNKNOWN_FLAG ((PangoGlyph)0x10000000)
#endif
#ifndef PANGO_GLYPH_EMPTY
#define PANGO_GLYPH_EMPTY           ((PangoGlyph)0)
#endif

#define IS_MISSING_GLYPH(g) ((g) & PANGO_GLYPH_UNKNOWN_FLAG)
#define IS_EMPTY_GLYPH(g) ((g) == PANGO_GLYPH_EMPTY)


int moz_pango_units_from_double(double d) {
    return NS_lround(d * FLOAT_PANGO_SCALE);
}

static PangoLanguage *GuessPangoLanguage(const nsACString& aLangGroup);

static cairo_scaled_font_t *CreateScaledFont(FcPattern *aPattern);

static PangoFontMap *gPangoFontMap;
static PangoFontMap *GetPangoFontMap();
static PRBool gUseFontMapProperty;

static FT_Library gFTLibrary;
static nsILanguageAtomService* gLangService;

NS_SPECIALIZE_TEMPLATE
class nsAutoRefTraits<PangoFont> : public gfxGObjectRefTraits<PangoFont> { };

NS_SPECIALIZE_TEMPLATE
class nsAutoRefTraits<PangoCoverage>
    : public nsPointerRefTraits<PangoCoverage> {
public:
    static void Release(PangoCoverage *aPtr) { pango_coverage_unref(aPtr); }
    static void AddRef(PangoCoverage *aPtr) { pango_coverage_ref(aPtr); }
};




#ifndef FC_FAMILYLANG
#define FC_FAMILYLANG "familylang"
#endif
#ifndef FC_FULLNAME
#define FC_FULLNAME "fullname"
#endif

static PRFuncPtr
FindFunctionSymbol(const char *name)
{
    PRLibrary *lib = nsnull;
    PRFuncPtr result = PR_FindFunctionSymbolAndLibrary(name, &lib);
    if (lib) {
        PR_UnloadLibrary(lib);
    }

    return result;
}





#define FONT_FACE_FAMILY_PREFIX "@font-face:"
































class gfxFcFontEntry : public gfxFontEntry {
public:
    const nsTArray< nsCountedRef<FcPattern> >& GetPatterns()
    {
        return mPatterns;
    }

protected:
    gfxFcFontEntry(const gfxProxyFontEntry &aProxyEntry)
        
        : gfxFontEntry(aProxyEntry.mFamily->Name())
    {
        mItalic = aProxyEntry.mItalic;
        mWeight = aProxyEntry.mWeight;
        mStretch = aProxyEntry.mStretch;
        mIsUserFont = PR_TRUE;
    }

    
    
    
    
    
    void AdjustPatternToCSS(FcPattern *aPattern);

    nsAutoTArray<nsCountedRef<FcPattern>,1> mPatterns;
};

void
gfxFcFontEntry::AdjustPatternToCSS(FcPattern *aPattern)
{
    int fontWeight = -1;
    FcPatternGetInteger(aPattern, FC_WEIGHT, 0, &fontWeight);
    int cssWeight = gfxFontconfigUtils::FcWeightForBaseWeight(mWeight / 100);
    if (cssWeight != fontWeight) {
        FcPatternDel(aPattern, FC_WEIGHT);
        FcPatternAddInteger(aPattern, FC_WEIGHT, cssWeight);
    }

    int fontSlant;
    FcResult res = FcPatternGetInteger(aPattern, FC_SLANT, 0, &fontSlant);
    
    
    if (res != FcResultMatch ||
        IsItalic() != (fontSlant != FC_SLANT_ROMAN)) {
        FcPatternDel(aPattern, FC_SLANT);
        FcPatternAddInteger(aPattern, FC_SLANT,
                            IsItalic() ? FC_SLANT_OBLIQUE : FC_SLANT_ROMAN);
    }

    
    
    
    FcChar8 *unused;
    if (FcPatternGetString(aPattern,
                           FC_FULLNAME, 0, &unused) == FcResultNoMatch) {
        nsCAutoString fullname;
        if (gfxFontconfigUtils::GetFullnameFromFamilyAndStyle(aPattern,
                                                              &fullname)) {
            FcPatternAddString(aPattern, FC_FULLNAME,
                               gfxFontconfigUtils::ToFcChar8(fullname));
        }
    }

    nsCAutoString family;
    family.Append(FONT_FACE_FAMILY_PREFIX);
    AppendUTF16toUTF8(Name(), family);

    FcPatternDel(aPattern, FC_FAMILY);
    FcPatternDel(aPattern, FC_FAMILYLANG);
    FcPatternAddString(aPattern, FC_FAMILY,
                       gfxFontconfigUtils::ToFcChar8(family));
}







class gfxLocalFcFontEntry : public gfxFcFontEntry {
public:
    gfxLocalFcFontEntry(const gfxProxyFontEntry &aProxyEntry,
                        const nsTArray< nsCountedRef<FcPattern> >& aPatterns)
        : gfxFcFontEntry(aProxyEntry)
    {
        if (!mPatterns.SetCapacity(aPatterns.Length()))
            return; 

        for (PRUint32 i = 0; i < aPatterns.Length(); ++i) {
            FcPattern *pattern = FcPatternDuplicate(aPatterns.ElementAt(i));
            if (!pattern)
                return; 

            AdjustPatternToCSS(pattern);

            mPatterns.AppendElement();
            mPatterns[i].own(pattern);
        }
    }
};







class gfxDownloadedFcFontEntry : public gfxFcFontEntry {
public:
    
    gfxDownloadedFcFontEntry(const gfxProxyFontEntry &aProxyEntry,
                             const PRUint8 *aData, FT_Face aFace)
        : gfxFcFontEntry(aProxyEntry), mFontData(aData), mFace(aFace)
    {
        NS_PRECONDITION(aFace != NULL, "aFace is NULL!");
        InitPattern();
    }

    virtual ~gfxDownloadedFcFontEntry();

    
    
    
    PangoCoverage *GetPangoCoverage();

protected:
    virtual void InitPattern();

    
    
    
    const PRUint8* mFontData;

    FT_Face mFace;

    
    
    
    nsAutoRef<PangoCoverage> mPangoCoverage;
};


static const char *kFontEntryFcProp = "-moz-font-entry";

static FcBool AddDownloadedFontEntry(FcPattern *aPattern,
                                     gfxDownloadedFcFontEntry *aFontEntry)
{
    FcValue value;
    value.type = FcTypeFTFace; 
    value.u.f = aFontEntry;

    return FcPatternAdd(aPattern, kFontEntryFcProp, value, FcFalse);
}

static FcBool DelDownloadedFontEntry(FcPattern *aPattern)
{
    return FcPatternDel(aPattern, kFontEntryFcProp);
}

static gfxDownloadedFcFontEntry *GetDownloadedFontEntry(FcPattern *aPattern)
{
    FcValue value;
    if (FcPatternGet(aPattern, kFontEntryFcProp, 0, &value) != FcResultMatch)
        return nsnull;

    if (value.type != FcTypeFTFace) {
        NS_NOTREACHED("Wrong type for -moz-font-entry font property");
        return nsnull;
    }

    return static_cast<gfxDownloadedFcFontEntry*>(value.u.f);
}

gfxDownloadedFcFontEntry::~gfxDownloadedFcFontEntry()
{
    if (mPatterns.Length() != 0) {
        
        
        NS_ASSERTION(mPatterns.Length() == 1,
                     "More than one pattern in gfxDownloadedFcFontEntry!");
        DelDownloadedFontEntry(mPatterns[0]);
        FcPatternDel(mPatterns[0], FC_FT_FACE);
    }
    FT_Done_Face(mFace);
    NS_Free((void*)mFontData);
}

typedef FcPattern* (*QueryFaceFunction)(const FT_Face face,
                                        const FcChar8 *file, int id,
                                        FcBlanks *blanks);

void
gfxDownloadedFcFontEntry::InitPattern()
{
    static QueryFaceFunction sQueryFacePtr =
        reinterpret_cast<QueryFaceFunction>
        (FindFunctionSymbol("FcFreeTypeQueryFace"));
    FcPattern *pattern;

    
    
    
    
    
    
    
    if (sQueryFacePtr) {
        
        
        
        
        
        
        
        
        pattern =
            (*sQueryFacePtr)(mFace, gfxFontconfigUtils::ToFcChar8(""), 0, NULL);
        if (!pattern)
            
            
            
            
            return;

        
        FcPatternDel(pattern, FC_FILE);
        FcPatternDel(pattern, FC_INDEX);

    } else {
        

        
        nsAutoRef<FcCharSet> charset(FcFreeTypeCharSet(mFace, NULL));
        
        
        if (!charset || FcCharSetCount(charset) == 0)
            return;

        pattern = FcPatternCreate();
        FcPatternAddCharSet(pattern, FC_CHARSET, charset);

        
        
        if (!(mFace->face_flags & FT_FACE_FLAG_SCALABLE)) {
            for (FT_Int i = 0; i < mFace->num_fixed_sizes; ++i) {
#if HAVE_FT_BITMAP_SIZE_Y_PPEM
                double size = FLOAT_FROM_26_6(mFace->available_sizes[i].y_ppem);
#else
                double size = mFace->available_sizes[i].height;
#endif
                FcPatternAddDouble (pattern, FC_PIXEL_SIZE, size);
            }

            
            
            FcPatternAddBool (pattern, FC_ANTIALIAS, FcFalse);
        }

        
        
        
        
        
        
        
        
        
    }

    AdjustPatternToCSS(pattern);

    FcPatternAddFTFace(pattern, FC_FT_FACE, mFace);
    AddDownloadedFontEntry(pattern, this);

    
    mPatterns.AppendElement();
    mPatterns[0].own(pattern);
}

static PangoCoverage *NewPangoCoverage(FcPattern *aFont)
{
    
    PangoCoverage *coverage = pango_coverage_new();

    FcCharSet *charset;
    if (FcPatternGetCharSet(aFont, FC_CHARSET, 0, &charset) != FcResultMatch)
        return coverage; 

    FcChar32 base;
    FcChar32 map[FC_CHARSET_MAP_SIZE];
    FcChar32 next;
    for (base = FcCharSetFirstPage(charset, map, &next);
         base != FC_CHARSET_DONE;
         base = FcCharSetNextPage(charset, map, &next)) {
        for (PRUint32 i = 0; i < FC_CHARSET_MAP_SIZE; ++i) {
            PRUint32 offset = 0;
            FcChar32 bitmap = map[i];
            for (; bitmap; bitmap >>= 1) {
                if (bitmap & 1) {
                    pango_coverage_set(coverage, base + offset,
                                       PANGO_COVERAGE_EXACT);
                }
                ++offset;
            }
            base += 32;
        }
    }
    return coverage;
}

PangoCoverage *
gfxDownloadedFcFontEntry::GetPangoCoverage()
{
    NS_ASSERTION(mPatterns.Length() != 0,
                 "Can't get coverage without a pattern!");
    if (!mPangoCoverage) {
        mPangoCoverage.own(NewPangoCoverage(mPatterns[0]));
    }
    return mPangoCoverage;
}








class gfxFcFont : public gfxFT2FontBase {
public:
    virtual ~gfxFcFont ();
    static already_AddRefed<gfxFcFont> GetOrMakeFont(FcPattern *aPattern);

protected:
    gfxFcFont(cairo_scaled_font_t *aCairoFont,
              gfxFontEntry *aFontEntry, const gfxFontStyle *aFontStyle);

    
    static cairo_user_data_key_t sGfxFontKey;
};












#define GFX_TYPE_PANGO_FC_FONT              (gfx_pango_fc_font_get_type())
#define GFX_PANGO_FC_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GFX_TYPE_PANGO_FC_FONT, gfxPangoFcFont))
#define GFX_IS_PANGO_FC_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GFX_TYPE_PANGO_FC_FONT))


GType gfx_pango_fc_font_get_type (void);

#define GFX_PANGO_FC_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GFX_TYPE_PANGO_FC_FONT, gfxPangoFcFontClass))
#define GFX_IS_PANGO_FC_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GFX_TYPE_PANGO_FC_FONT))
#define GFX_PANGO_FC_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GFX_TYPE_PANGO_FC_FONT, gfxPangoFcFontClass))


struct gfxPangoFcFont {
    PangoFcFont parent_instance;

    FcPattern *mRequestedPattern;
    PangoCoverage *mCoverage;
    gfxFcFont *mGfxFont;

    static nsReturnRef<PangoFont>
    NewFont(FcPattern *aRequestedPattern, FcPattern *aFontPattern)
    {
        
        
        
        
        
        
        
        
        
        
        
        
        
        gfxPangoFcFont *font = static_cast<gfxPangoFcFont*>
            (g_object_new(GFX_TYPE_PANGO_FC_FONT,
                          "pattern", aFontPattern, NULL));

        
        FcPatternReference(aRequestedPattern);
        font->mRequestedPattern = aRequestedPattern;

        
        
        
        PangoFontMap *fontmap = GetPangoFontMap();
        
        
        
        PangoFcFont *fc_font = &font->parent_instance;
        if (gUseFontMapProperty) {
            g_object_set(font, "fontmap", fontmap, NULL);
        } else {
            
            
            
            
            
            fc_font->fontmap = fontmap;
            g_object_ref(fc_font->fontmap);
        }

        return nsReturnRef<PangoFont>(PANGO_FONT(font));
    }

    static gfxFcFont *GfxFont(gfxPangoFcFont *self)
    {
        if (!self->mGfxFont) {
            PangoFcFont *fc_font = &self->parent_instance;

            if (NS_LIKELY(self->mRequestedPattern)) {
                
                nsAutoRef<FcPattern> renderPattern
                    (FcFontRenderPrepare(NULL, self->mRequestedPattern,
                                         fc_font->font_pattern));
                if (!renderPattern)
                    return nsnull;

                FcBool hinting = FcTrue;
                FcPatternGetBool(renderPattern, FC_HINTING, 0, &hinting);
                fc_font->is_hinted = hinting;

                
                
                FcMatrix *matrix;
                FcResult result = FcPatternGetMatrix(renderPattern,
                                                     FC_MATRIX, 0, &matrix);
                fc_font->is_transformed =
                    result == FcResultMatch &&
                    (matrix->xy != 0.0 || matrix->yx != 0.0 ||
                     matrix->xx != 1.0 || matrix->yy != 1.0);

                self->mGfxFont = gfxFcFont::GetOrMakeFont(renderPattern).get();
                if (self->mGfxFont) {
                    
                    FcPatternDestroy(self->mRequestedPattern);
                    self->mRequestedPattern = NULL;
                }

            } else {
                
                self->mGfxFont =
                    gfxFcFont::GetOrMakeFont(fc_font->font_pattern).get();
            }                
        }
        return self->mGfxFont;
    }

    static cairo_scaled_font_t *CairoFont(gfxPangoFcFont *self)
    {
        return gfxPangoFcFont::GfxFont(self)->CairoScaledFont();
    }
};

struct gfxPangoFcFontClass {
    PangoFcFontClass parent_class;
};

G_DEFINE_TYPE (gfxPangoFcFont, gfx_pango_fc_font, PANGO_TYPE_FC_FONT)

static void
gfx_pango_fc_font_init(gfxPangoFcFont *font)
{
}


static void
gfx_pango_fc_font_finalize(GObject *object)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(object);

    if (self->mRequestedPattern)
        FcPatternDestroy(self->mRequestedPattern);
    if (self->mCoverage)
        pango_coverage_unref(self->mCoverage);
    NS_IF_RELEASE(self->mGfxFont);

    G_OBJECT_CLASS(gfx_pango_fc_font_parent_class)->finalize(object);
}

static PangoCoverage *
gfx_pango_fc_font_get_coverage(PangoFont *font, PangoLanguage *lang)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);

    
    
    if (!self->mCoverage) {
        FcPattern *pattern = self->parent_instance.font_pattern;
        gfxDownloadedFcFontEntry *downloadedFontEntry =
            GetDownloadedFontEntry(pattern);
        
        
        
        if (!downloadedFontEntry) {
            self->mCoverage =
                PANGO_FONT_CLASS(gfx_pango_fc_font_parent_class)->
                get_coverage(font, lang);
        } else {
            self->mCoverage =
                pango_coverage_ref(downloadedFontEntry->GetPangoCoverage());
        }
    }

    return pango_coverage_ref(self->mCoverage);
}

static PangoFontDescription *
gfx_pango_fc_font_describe(PangoFont *font)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);
    PangoFcFont *fcFont = &self->parent_instance;
    PangoFontDescription *result =
        pango_font_description_copy(fcFont->description);

    gfxFcFont *gfxFont = gfxPangoFcFont::GfxFont(self);
    if (gfxFont) {
        double pixelsize = gfxFont->GetStyle()->size;
        double dpi = gfxPlatformGtk::GetPlatformDPI();
        gint size = moz_pango_units_from_double(pixelsize * dpi / 72.0);
        pango_font_description_set_size(result, size);
    }
    return result;
}

static PangoFontDescription *
gfx_pango_fc_font_describe_absolute(PangoFont *font)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);
    PangoFcFont *fcFont = &self->parent_instance;
    PangoFontDescription *result =
        pango_font_description_copy(fcFont->description);

    gfxFcFont *gfxFont = gfxPangoFcFont::GfxFont(self);
    if (gfxFont) {
        double size = gfxFont->GetStyle()->size * PANGO_SCALE;
        pango_font_description_set_absolute_size(result, size);
    }
    return result;
}

static void
gfx_pango_fc_font_get_glyph_extents(PangoFont *font, PangoGlyph glyph,
                                    PangoRectangle *ink_rect,
                                    PangoRectangle *logical_rect)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);
    gfxFcFont *gfxFont = gfxPangoFcFont::GfxFont(self);

    if (IS_MISSING_GLYPH(glyph)) {
        const gfxFont::Metrics& metrics = gfxFont->GetMetrics();

        PangoRectangle rect;
        rect.x = 0;
        rect.y = moz_pango_units_from_double(-metrics.maxAscent);
        rect.width = moz_pango_units_from_double(metrics.aveCharWidth);
        rect.height = moz_pango_units_from_double(metrics.maxHeight);
        if (ink_rect) {
            *ink_rect = rect;
        }
        if (logical_rect) {
            *logical_rect = rect;
        }
        return;
    }

    if (logical_rect) {
        
        
        
        
        
        const gfxFont::Metrics& metrics = gfxFont->GetMetrics();
        logical_rect->y = moz_pango_units_from_double(-metrics.maxAscent);
        logical_rect->height = moz_pango_units_from_double(metrics.maxHeight);
    }

    cairo_text_extents_t extents;
    if (IS_EMPTY_GLYPH(glyph)) {
        new (&extents) cairo_text_extents_t(); 
    } else {
        gfxFont->GetGlyphExtents(glyph, &extents);
    }

    if (ink_rect) {
        ink_rect->x = moz_pango_units_from_double(extents.x_bearing);
        ink_rect->y = moz_pango_units_from_double(extents.y_bearing);
        ink_rect->width = moz_pango_units_from_double(extents.width);
        ink_rect->height = moz_pango_units_from_double(extents.height);
    }
    if (logical_rect) {
        logical_rect->x = 0;
        logical_rect->width = moz_pango_units_from_double(extents.x_advance);
    }
}

static PangoFontMetrics *
gfx_pango_fc_font_get_metrics(PangoFont *font, PangoLanguage *language)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);

    
    PangoFontMetrics *result = pango_font_metrics_new();

    gfxFcFont *gfxFont = gfxPangoFcFont::GfxFont(self);
    if (gfxFont) {
        const gfxFont::Metrics& metrics = gfxFont->GetMetrics();

        result->ascent = moz_pango_units_from_double(metrics.maxAscent);
        result->descent = moz_pango_units_from_double(metrics.maxDescent);
        result->approximate_char_width =
            moz_pango_units_from_double(metrics.aveCharWidth);
        result->approximate_digit_width =
            moz_pango_units_from_double(metrics.zeroOrAveCharWidth);
        result->underline_position =
            moz_pango_units_from_double(metrics.underlineOffset);
        result->underline_thickness =
            moz_pango_units_from_double(metrics.underlineSize);
        result->strikethrough_position =
            moz_pango_units_from_double(metrics.strikeoutOffset);
        result->strikethrough_thickness =
            moz_pango_units_from_double(metrics.strikeoutSize);
    }
    return result;
}

static FT_Face
gfx_pango_fc_font_lock_face(PangoFcFont *font)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);
    return cairo_ft_scaled_font_lock_face(gfxPangoFcFont::CairoFont(self));
}

static void
gfx_pango_fc_font_unlock_face(PangoFcFont *font)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);
    cairo_ft_scaled_font_unlock_face(gfxPangoFcFont::CairoFont(self));
}

static guint
gfx_pango_fc_font_get_glyph(PangoFcFont *font, gunichar wc)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);
    gfxFcFont *gfxFont = gfxPangoFcFont::GfxFont(self);
    return gfxFont->GetGlyph(wc);
}

typedef int (*PangoVersionFunction)();

static void
gfx_pango_fc_font_class_init (gfxPangoFcFontClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    PangoFontClass *font_class = PANGO_FONT_CLASS (klass);
    PangoFcFontClass *fc_font_class = PANGO_FC_FONT_CLASS (klass);

    object_class->finalize = gfx_pango_fc_font_finalize;

    font_class->get_coverage = gfx_pango_fc_font_get_coverage;
    
    font_class->describe = gfx_pango_fc_font_describe;
    font_class->get_glyph_extents = gfx_pango_fc_font_get_glyph_extents;
    
    
    font_class->get_metrics = gfx_pango_fc_font_get_metrics;
    font_class->describe_absolute = gfx_pango_fc_font_describe_absolute;
    

    
    fc_font_class->lock_face = gfx_pango_fc_font_lock_face;
    fc_font_class->unlock_face = gfx_pango_fc_font_unlock_face;
    fc_font_class->get_glyph = gfx_pango_fc_font_get_glyph;

    
    
    
    
    
    
    
    PangoVersionFunction pango_version =
        reinterpret_cast<PangoVersionFunction>
        (FindFunctionSymbol("pango_version"));
    gUseFontMapProperty = pango_version && (*pango_version)() >= 12404;
}





static GQuark GetFontGroupQuark()
{
    
    
    
    static GQuark quark = g_quark_from_string("moz-font-group");
    return quark;
}

static void
gfxFontGroup_unref(gpointer data)
{
    gfxPangoFontGroup *fontGroup = static_cast<gfxPangoFontGroup*>(data);
    NS_RELEASE(fontGroup);
}

static void
SetFontGroup(PangoContext *aContext, gfxPangoFontGroup *aFontGroup)
{
    NS_ADDREF(aFontGroup);
    g_object_set_qdata_full(G_OBJECT(aContext), GetFontGroupQuark(),
                            aFontGroup, gfxFontGroup_unref);
}

static gfxPangoFontGroup *
GetFontGroup(PangoContext *aContext)
{
    return static_cast<gfxPangoFontGroup*>
        (g_object_get_qdata(G_OBJECT(aContext), GetFontGroupQuark()));
}








class gfxFcPangoFontSet {
public:
    THEBES_INLINE_DECL_REFCOUNTING(gfxFcPangoFontSet)
    
    explicit gfxFcPangoFontSet(FcPattern *aPattern,
                               gfxUserFontSet *aUserFontSet)
        : mSortPattern(aPattern), mUserFontSet(aUserFontSet),
          mFcFontSet(SortPreferredFonts()), mFcFontsTrimmed(0),
          mHaveFallbackFonts(PR_FALSE)
    {
    }

    
    
    PangoFont *GetFontAt(PRUint32 i)
    {
        if (i >= mFonts.Length() || !mFonts[i].mFont) { 
            
            FcPattern *fontPattern = GetFontPatternAt(i);
            if (!fontPattern)
                return NULL;

            mFonts[i].mFont =
                gfxPangoFcFont::NewFont(mSortPattern, fontPattern);
        }
        return mFonts[i].mFont;
    }

    FcPattern *GetFontPatternAt(PRUint32 i);

private:
    nsReturnRef<FcFontSet> SortPreferredFonts();
    nsReturnRef<FcFontSet> SortFallbackFonts();

    struct FontEntry {
        explicit FontEntry(FcPattern *aPattern) : mPattern(aPattern) {}
        nsCountedRef<FcPattern> mPattern;
        nsCountedRef<PangoFont> mFont;
    };

    struct LangSupportEntry {
        LangSupportEntry(FcChar8 *aLang, FcLangResult aSupport) :
            mLang(aLang), mBestSupport(aSupport) {}
        FcChar8 *mLang;
        FcLangResult mBestSupport;
    };

public:
    
    class LangComparator {
    public:
        PRBool Equals(const LangSupportEntry& a, const FcChar8 *b) const
        {
            return FcStrCmpIgnoreCase(a.mLang, b) == 0;
        }
    };

private:
    
    nsCountedRef<FcPattern> mSortPattern;
    
    nsRefPtr<gfxUserFontSet> mUserFontSet;
    
    
    nsTArray<FontEntry> mFonts;
    
    
    
    
    nsAutoRef<FcFontSet> mFcFontSet;
    
    nsAutoRef<FcCharSet> mCharSet;
    
    
    int mFcFontsTrimmed;
    
    
    PRPackedBool mHaveFallbackFonts;
};



static const nsTArray< nsCountedRef<FcPattern> >*
FindFontPatterns(gfxUserFontSet *mUserFontSet,
                const nsACString &aFamily, PRUint8 aStyle, PRUint16 aWeight)
{
    
    NS_ConvertUTF8toUTF16 utf16Family(aFamily);

    
    
    
    PRBool needsBold;

    gfxFontStyle style;
    style.style = aStyle;
    style.weight = aWeight;

    gfxFcFontEntry *fontEntry = static_cast<gfxFcFontEntry*>
        (mUserFontSet->FindFontEntry(utf16Family, style, needsBold));

    
    if (!fontEntry && aStyle != FONT_STYLE_NORMAL) {
        style.style = FONT_STYLE_NORMAL;
        fontEntry = static_cast<gfxFcFontEntry*>
            (mUserFontSet->FindFontEntry(utf16Family, style, needsBold));
    }

    if (!fontEntry)
        return NULL;

    return &fontEntry->GetPatterns();
}

typedef FcBool (*FcPatternRemoveFunction)(FcPattern *p, const char *object,
                                          int id);


static FcBool
moz_FcPatternRemove(FcPattern *p, const char *object, int id)
{
    static FcPatternRemoveFunction sFcPatternRemovePtr =
        reinterpret_cast<FcPatternRemoveFunction>
        (FindFunctionSymbol("FcPatternRemove"));

    if (!sFcPatternRemovePtr)
        return FcFalse;

    return (*sFcPatternRemovePtr)(p, object, id);
}



static PRBool
SlantIsAcceptable(FcPattern *aFont, int aRequestedSlant)
{
    
    if (aRequestedSlant == FC_SLANT_ITALIC)
        return PR_TRUE;

    int slant;
    FcResult result = FcPatternGetInteger(aFont, FC_SLANT, 0, &slant);
    
    
    if (result != FcResultMatch)
        return PR_TRUE;

    switch (aRequestedSlant) {
        case FC_SLANT_ROMAN:
            
            return slant == aRequestedSlant;
        case FC_SLANT_OBLIQUE:
            
            
            return slant != FC_SLANT_ITALIC;
    }

    return PR_TRUE;
}



static PRBool
SizeIsAcceptable(FcPattern *aFont, double aRequestedSize)
{
    double size;
    int v = 0;
    while (FcPatternGetDouble(aFont,
                              FC_PIXEL_SIZE, v, &size) == FcResultMatch) {
        ++v;
        if (5.0 * fabs(size - aRequestedSize) < aRequestedSize)
            return PR_TRUE;
    }

    
    return v == 0;
}



nsReturnRef<FcFontSet>
gfxFcPangoFontSet::SortPreferredFonts()
{
    gfxFontconfigUtils *utils = gfxFontconfigUtils::GetFontconfigUtils();
    if (!utils)
        return nsReturnRef<FcFontSet>();

    
    
    
    
    
    
    
    

    
    
    
    
    
    nsAutoTArray<LangSupportEntry,10> requiredLangs;
    for (int v = 0; ; ++v) {
        FcChar8 *lang;
        FcResult result = FcPatternGetString(mSortPattern, FC_LANG, v, &lang);
        if (result != FcResultMatch) {
            
            
            
            NS_ASSERTION(result != FcResultTypeMismatch,
                         "Expected a string for FC_LANG");
            break;
        }

        if (!requiredLangs.Contains(lang, LangComparator())) {
            FcLangResult bestLangSupport = utils->GetBestLangSupport(lang);
            if (bestLangSupport != FcLangDifferentLang) {
                requiredLangs.
                    AppendElement(LangSupportEntry(lang, bestLangSupport));
            }
        }
    }

    nsAutoRef<FcFontSet> fontSet(FcFontSetCreate());
    if (!fontSet)
        return fontSet.out();

    
    
    int requestedSlant = FC_SLANT_ROMAN;
    FcPatternGetInteger(mSortPattern, FC_SLANT, 0, &requestedSlant);
    double requestedSize = -1.0;
    FcPatternGetDouble(mSortPattern, FC_PIXEL_SIZE, 0, &requestedSize);

    nsTHashtable<gfxFontconfigUtils::DepFcStrEntry> existingFamilies;
    existingFamilies.Init(50);
    FcChar8 *family;
    for (int v = 0;
         FcPatternGetString(mSortPattern,
                            FC_FAMILY, v, &family) == FcResultMatch; ++v) {
        const nsTArray< nsCountedRef<FcPattern> > *familyFonts = nsnull;

        
        PRBool isUserFont = PR_FALSE;
        if (mUserFontSet) {
            

            nsDependentCString cFamily(gfxFontconfigUtils::ToCString(family));
            NS_NAMED_LITERAL_CSTRING(userPrefix, FONT_FACE_FAMILY_PREFIX);

            if (StringBeginsWith(cFamily, userPrefix)) {
                isUserFont = PR_TRUE;

                
                nsDependentCSubstring cssFamily(cFamily, userPrefix.Length());

                PRUint8 thebesStyle =
                    gfxFontconfigUtils::FcSlantToThebesStyle(requestedSlant);
                PRUint16 thebesWeight =
                    gfxFontconfigUtils::GetThebesWeight(mSortPattern);

                familyFonts = FindFontPatterns(mUserFontSet, cssFamily,
                                               thebesStyle, thebesWeight);
            }
        }

        if (!isUserFont) {
            familyFonts = &utils->GetFontsForFamily(family);
        }

        if (!familyFonts || familyFonts->Length() == 0) {
            
            
            
            
            
            
            
            
            
            if (v != 0 && moz_FcPatternRemove(mSortPattern, FC_FAMILY, v)) {
                --v;
            }
            continue;
        }

        
        
        
        gfxFontconfigUtils::DepFcStrEntry *entry =
            existingFamilies.PutEntry(family);
        if (entry) {
            if (entry->mKey) 
                continue;

            entry->mKey = family; 
        }

        for (PRUint32 f = 0; f < familyFonts->Length(); ++f) {
            FcPattern *font = familyFonts->ElementAt(f);

            
            
            if (!isUserFont && !SlantIsAcceptable(font, requestedSlant))
                continue;
            if (requestedSize != -1.0 && !SizeIsAcceptable(font, requestedSize))
                continue;

            for (PRUint32 r = 0; r < requiredLangs.Length(); ++r) {
                const LangSupportEntry& entry = requiredLangs[r];
                FcLangResult support =
                    gfxFontconfigUtils::GetLangSupport(font, entry.mLang);
                if (support <= entry.mBestSupport) { 
                    requiredLangs.RemoveElementAt(r);
                    --r;
                }
            }

            
            
            if (FcFontSetAdd(fontSet, font)) {
                FcPatternReference(font);
            }
        }
    }

    FcPattern *truncateMarker = NULL;
    for (PRUint32 r = 0; r < requiredLangs.Length(); ++r) {
        const nsTArray< nsCountedRef<FcPattern> >& langFonts =
            utils->GetFontsForLang(requiredLangs[r].mLang);

        PRBool haveLangFont = PR_FALSE;
        for (PRUint32 f = 0; f < langFonts.Length(); ++f) {
            FcPattern *font = langFonts[f];
            if (!SlantIsAcceptable(font, requestedSlant))
                continue;
            if (requestedSize != -1.0 && !SizeIsAcceptable(font, requestedSize))
                continue;

            haveLangFont = PR_TRUE;
            if (FcFontSetAdd(fontSet, font)) {
                FcPatternReference(font);
            }
        }

        if (!haveLangFont && langFonts.Length() > 0) {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            FcPattern *font = langFonts[0];
            if (FcFontSetAdd(fontSet, font)) {
                FcPatternReference(font);
                truncateMarker = font;
            }
            break;
        }
    }

    FcFontSet *sets[1] = { fontSet };
    FcResult result;
#ifdef SOLARIS
    
    
    fontSet.own(FcFontSetSort(FcConfigGetCurrent(), sets, 1, mSortPattern,
                              FcFalse, NULL, &result));
#else
    fontSet.own(FcFontSetSort(NULL, sets, 1, mSortPattern,
                              FcFalse, NULL, &result));
#endif

    if (truncateMarker != NULL && fontSet) {
        nsAutoRef<FcFontSet> truncatedSet(FcFontSetCreate());

        for (int f = 0; f < fontSet->nfont; ++f) {
            FcPattern *font = fontSet->fonts[f];
            if (font == truncateMarker)
                break;

            if (FcFontSetAdd(truncatedSet, font)) {
                FcPatternReference(font);
            }
        }

        fontSet.steal(truncatedSet);
    }

    return fontSet.out();
}

nsReturnRef<FcFontSet>
gfxFcPangoFontSet::SortFallbackFonts()
{
    
    
    
    
    
    
    
    
    FcResult result;
    return nsReturnRef<FcFontSet>(FcFontSort(NULL, mSortPattern,
                                             FcFalse, NULL, &result));
}


FcPattern *
gfxFcPangoFontSet::GetFontPatternAt(PRUint32 i)
{
    while (i >= mFonts.Length()) {
        while (!mFcFontSet) {
            if (mHaveFallbackFonts)
                return nsnull;

            mFcFontSet = SortFallbackFonts();
            mHaveFallbackFonts = PR_TRUE;
            mFcFontsTrimmed = 0;
            
        }

        while (mFcFontsTrimmed < mFcFontSet->nfont) {
            FcPattern *font = mFcFontSet->fonts[mFcFontsTrimmed];
            ++mFcFontsTrimmed;

            if (mFonts.Length() != 0) {
                
                
                
                
                FcCharSet *supportedChars = mCharSet;
                if (!supportedChars) {
                    FcPatternGetCharSet(mFonts[mFonts.Length() - 1].mPattern,
                                        FC_CHARSET, 0, &supportedChars);
                }

                if (supportedChars) {
                    FcCharSet *newChars = NULL;
                    FcPatternGetCharSet(font, FC_CHARSET, 0, &newChars);
                    if (newChars) {
                        if (FcCharSetIsSubset(newChars, supportedChars))
                            continue;

                        mCharSet.own(FcCharSetUnion(supportedChars, newChars));
                    } else if (!mCharSet) {
                        mCharSet.own(FcCharSetCopy(supportedChars));
                    }
                }
            }

            mFonts.AppendElement(font);
            if (mFonts.Length() >= i)
                break;
        }

        if (mFcFontsTrimmed == mFcFontSet->nfont) {
            
            mFcFontSet.reset();
        }
    }

    return mFonts[i].mPattern;
}





#define GFX_TYPE_PANGO_FONTSET              (gfx_pango_fontset_get_type())
#define GFX_PANGO_FONTSET(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GFX_TYPE_PANGO_FONTSET, gfxPangoFontset))
#define GFX_IS_PANGO_FONTSET(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GFX_TYPE_PANGO_FONTSET))


GType gfx_pango_fontset_get_type (void);

#define GFX_PANGO_FONTSET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GFX_TYPE_PANGO_FONTSET, gfxPangoFontsetClass))
#define GFX_IS_PANGO_FONTSET_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GFX_TYPE_PANGO_FONTSET))
#define GFX_PANGO_FONTSET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GFX_TYPE_PANGO_FONTSET, gfxPangoFontsetClass))


struct gfxPangoFontset {
    PangoFontset parent_instance;

    PangoLanguage *mLanguage;
    gfxFcPangoFontSet *mGfxFontSet;
    PangoFont *mBaseFont;
    gfxPangoFontGroup *mFontGroup;

    static PangoFontset *
    NewFontset(gfxPangoFontGroup *aFontGroup,
               PangoLanguage *aLanguage)
    {
        gfxPangoFontset *fontset = static_cast<gfxPangoFontset *>
            (g_object_new(GFX_TYPE_PANGO_FONTSET, NULL));

        fontset->mLanguage = aLanguage;

        
        if (aFontGroup->GetPangoLanguage() == aLanguage) {
            fontset->mGfxFontSet = aFontGroup->GetFontSet();
            NS_IF_ADDREF(fontset->mGfxFontSet);

        } else {
            
            
            
            fontset->mFontGroup = aFontGroup;
            NS_ADDREF(fontset->mFontGroup);

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (aFontGroup->GetPangoLanguage() &&
                !aFontGroup->GetStyle()->systemFont) {
                fontset->mBaseFont = aFontGroup->GetBasePangoFont();
                if (fontset->mBaseFont)
                    g_object_ref(fontset->mBaseFont);
            }
        }

        return PANGO_FONTSET(fontset);
    }
};

struct gfxPangoFontsetClass {
    PangoFontsetClass parent_class;
};

G_DEFINE_TYPE (gfxPangoFontset, gfx_pango_fontset, PANGO_TYPE_FONTSET)

static void
gfx_pango_fontset_init(gfxPangoFontset *fontset)
{
}

static void
gfx_pango_fontset_finalize(GObject *object)
{
    gfxPangoFontset *self = GFX_PANGO_FONTSET(object);

    if (self->mBaseFont)
        g_object_unref(self->mBaseFont);
    NS_IF_RELEASE(self->mGfxFontSet);
    NS_IF_RELEASE(self->mFontGroup);

    G_OBJECT_CLASS(gfx_pango_fontset_parent_class)->finalize(object);
}

static PangoLanguage *
gfx_pango_fontset_get_language(PangoFontset *fontset)
{
    gfxPangoFontset *self = GFX_PANGO_FONTSET(fontset);
    return self->mLanguage;
}

static gfxFcPangoFontSet *
GetGfxFontSet(gfxPangoFontset *self)
{
    if (!self->mGfxFontSet && self->mFontGroup) {
        self->mGfxFontSet = self->mFontGroup->GetFontSet(self->mLanguage);
        
        NS_RELEASE(self->mFontGroup);

        if (!self->mGfxFontSet)
            return nsnull;

        NS_ADDREF(self->mGfxFontSet);
    }
    return self->mGfxFontSet;
}

static void
gfx_pango_fontset_foreach(PangoFontset *fontset, PangoFontsetForeachFunc func,
                          gpointer data)
{
    gfxPangoFontset *self = GFX_PANGO_FONTSET(fontset);

    FcPattern *baseFontPattern = NULL;
    if (self->mBaseFont) {
        if ((*func)(fontset, self->mBaseFont, data))
            return;

        baseFontPattern = PANGO_FC_FONT(self->mBaseFont)->font_pattern;
    }        

    
    gfxFcPangoFontSet *gfxFontSet = GetGfxFontSet(self);
    if (!gfxFontSet)
        return;

    for (PRUint32 i = 0;
         FcPattern *pattern = gfxFontSet->GetFontPatternAt(i);
         ++i) {
        
        if (pattern == baseFontPattern) {
            continue;
        }
        PangoFont *font = gfxFontSet->GetFontAt(i);
        if (font) {
            if ((*func)(fontset, font, data))
                return;
        }
    }
}

static PRBool HasChar(FcPattern *aFont, FcChar32 wc)
{
    FcCharSet *charset = NULL;
    FcPatternGetCharSet(aFont, FC_CHARSET, 0, &charset);

    return charset && FcCharSetHasChar(charset, wc);
}

static PangoFont *
gfx_pango_fontset_get_font(PangoFontset *fontset, guint wc)
{
    gfxPangoFontset *self = GFX_PANGO_FONTSET(fontset);

    PangoFont *result = NULL;

    FcPattern *baseFontPattern = NULL;
    if (self->mBaseFont) {
        baseFontPattern = PANGO_FC_FONT(self->mBaseFont)->font_pattern;

        if (HasChar(baseFontPattern, wc)) {
            result = self->mBaseFont;
        }
    }

    if (!result) {
        
        gfxFcPangoFontSet *gfxFontSet = GetGfxFontSet(self);

        if (gfxFontSet) {
            for (PRUint32 i = 0;
                 FcPattern *pattern = gfxFontSet->GetFontPatternAt(i);
                 ++i) {
                
                if (pattern == baseFontPattern) {
                    continue;
                }

                if (HasChar(pattern, wc)) {
                    result = gfxFontSet->GetFontAt(i);
                    break;
                }
            }
        }

        if (!result) {
            
            if (self->mBaseFont) {
                result = self->mBaseFont;
            } else if (gfxFontSet) {
                result = gfxFontSet->GetFontAt(0);
            }
        }
    }

    if (!result)
        return NULL;

    g_object_ref(result);
    return result;
}

static void
gfx_pango_fontset_class_init (gfxPangoFontsetClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    PangoFontsetClass *fontset_class = PANGO_FONTSET_CLASS (klass);

    object_class->finalize = gfx_pango_fontset_finalize;
    
    
    fontset_class->get_font = gfx_pango_fontset_get_font;
    
    fontset_class->get_language = gfx_pango_fontset_get_language;
    fontset_class->foreach = gfx_pango_fontset_foreach;
}












#define GFX_TYPE_PANGO_FONT_MAP              (gfx_pango_font_map_get_type())
#define GFX_PANGO_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GFX_TYPE_PANGO_FONT_MAP, gfxPangoFontMap))
#define GFX_IS_PANGO_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GFX_TYPE_PANGO_FONT_MAP))

GType gfx_pango_font_map_get_type (void);

#define GFX_PANGO_FONT_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GFX_TYPE_PANGO_FONT_MAP, gfxPangoFontMapClass))
#define GFX_IS_PANGO_FONT_MAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GFX_TYPE_PANGO_FONT_MAP))
#define GFX_PANGO_FONT_MAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GFX_TYPE_PANGO_FONT_MAP, gfxPangoFontMapClass))



struct gfxPangoFontMap {
    PangoFcFontMap parent_instance;

    static PangoFontMap *
    NewFontMap()
    {
        gfxPangoFontMap *fontmap = static_cast<gfxPangoFontMap *>
            (g_object_new(GFX_TYPE_PANGO_FONT_MAP, NULL));

        return PANGO_FONT_MAP(fontmap);
    }
};

struct gfxPangoFontMapClass {
    PangoFcFontMapClass parent_class;
};

G_DEFINE_TYPE (gfxPangoFontMap, gfx_pango_font_map, PANGO_TYPE_FC_FONT_MAP)

static void
gfx_pango_font_map_init(gfxPangoFontMap *fontset)
{
}

static PangoFont *
gfx_pango_font_map_load_font(PangoFontMap *fontmap, PangoContext *context,
                             const PangoFontDescription *description)
{
    gfxPangoFontGroup *fontGroup = GetFontGroup(context);
    if (NS_UNLIKELY(!fontGroup)) {
        return PANGO_FONT_MAP_CLASS(gfx_pango_font_map_parent_class)->
            load_font(fontmap, context, description);
    }
        
    PangoFont *baseFont = fontGroup->GetBasePangoFont();
    if (NS_LIKELY(baseFont)) {
        g_object_ref(baseFont);
    }
    return baseFont;
}

static PangoFontset *
gfx_pango_font_map_load_fontset(PangoFontMap *fontmap, PangoContext *context,
                                const PangoFontDescription *desc,
                                PangoLanguage *language)
{
    gfxPangoFontGroup *fontGroup = GetFontGroup(context);
    if (NS_UNLIKELY(!fontGroup)) {
        return PANGO_FONT_MAP_CLASS(gfx_pango_font_map_parent_class)->
            load_fontset(fontmap, context, desc, language);
    }

    return gfxPangoFontset::NewFontset(fontGroup, language);
}

static double
gfx_pango_font_map_get_resolution(PangoFcFontMap *fcfontmap,
                                  PangoContext *context)
{
    
    
    return gfxPlatformGtk::GetPlatformDPI();
}

#ifdef MOZ_WIDGET_GTK2
static void ApplyGdkScreenFontOptions(FcPattern *aPattern);
#endif


static void
PrepareSortPattern(FcPattern *aPattern, double aFallbackSize,
                   double aSizeAdjustFactor, PRBool aIsPrinterFont)
{
    FcConfigSubstitute(NULL, aPattern, FcMatchPattern);

    
    
    
    
    
    
    
    
    
    
    
    
    if(aIsPrinterFont) {
       cairo_font_options_t *options = cairo_font_options_create();
       cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
       cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_GRAY);
       cairo_ft_font_options_substitute(options, aPattern);
       cairo_font_options_destroy(options);
    }
#ifdef MOZ_WIDGET_GTK2
    else {
       ApplyGdkScreenFontOptions(aPattern);
    }
#endif

    
    
    double size = aFallbackSize;
    if (FcPatternGetDouble(aPattern, FC_PIXEL_SIZE, 0, &size) != FcResultMatch
        || aSizeAdjustFactor != 1.0) {
        FcPatternDel(aPattern, FC_PIXEL_SIZE);
        FcPatternAddDouble(aPattern, FC_PIXEL_SIZE, size * aSizeAdjustFactor);
    }

    FcDefaultSubstitute(aPattern);
}

static void
gfx_pango_font_map_default_substitute(PangoFcFontMap *fontmap,
                                      FcPattern *pattern)
{
    
    
    PrepareSortPattern(pattern, 18.0, 1.0, FALSE);
}

static PangoFcFont *
gfx_pango_font_map_new_font(PangoFcFontMap *fontmap,
                            FcPattern *pattern)
{
    return PANGO_FC_FONT(g_object_new(GFX_TYPE_PANGO_FC_FONT,
                                      "pattern", pattern, NULL));
}

static void
gfx_pango_font_map_class_init(gfxPangoFontMapClass *klass)
{
    

    PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (klass);
    fontmap_class->load_font = gfx_pango_font_map_load_font;
    
    
    fontmap_class->load_fontset = gfx_pango_font_map_load_fontset;
    

    PangoFcFontMapClass *fcfontmap_class = PANGO_FC_FONT_MAP_CLASS (klass);
    fcfontmap_class->get_resolution = gfx_pango_font_map_get_resolution;
    
    

    
    
    
    
    
    
    fcfontmap_class->default_substitute = gfx_pango_font_map_default_substitute;
    fcfontmap_class->new_font = gfx_pango_font_map_new_font;
}





struct FamilyCallbackData {
    FamilyCallbackData(nsTArray<nsString> *aFcFamilyList,
                       gfxUserFontSet *aUserFontSet)
        : mFcFamilyList(aFcFamilyList), mUserFontSet(aUserFontSet)
    {
    }
    nsTArray<nsString> *mFcFamilyList;
    const gfxUserFontSet *mUserFontSet;
};

static int
FFRECountHyphens (const nsAString &aFFREName)
{
    int h = 0;
    PRInt32 hyphen = 0;
    while ((hyphen = aFFREName.FindChar('-', hyphen)) >= 0) {
        ++h;
        ++hyphen;
    }
    return h;
}

static PRBool
FamilyCallback (const nsAString& fontName, const nsACString& genericName,
                void *closure)
{
    FamilyCallbackData *data = static_cast<FamilyCallbackData*>(closure);
    nsTArray<nsString> *list = data->mFcFamilyList;

    
    if (genericName.Length() && FFRECountHyphens(fontName) >= 3)
        return PR_TRUE;

    if (!list->Contains(fontName)) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        const gfxUserFontSet *userFontSet = data->mUserFontSet;
        if (genericName.Length() == 0 &&
            userFontSet && userFontSet->HasFamily(fontName)) {
            nsAutoString userFontName =
                NS_LITERAL_STRING(FONT_FACE_FAMILY_PREFIX) + fontName;
            list->AppendElement(userFontName);
        }

        list->AppendElement(fontName);
    }

    return PR_TRUE;
}

gfxPangoFontGroup::gfxPangoFontGroup (const nsAString& families,
                                      const gfxFontStyle *aStyle,
                                      gfxUserFontSet *aUserFontSet)
    : gfxFontGroup(families, aStyle, aUserFontSet),
      mPangoLanguage(GuessPangoLanguage(aStyle->langGroup))
{
    mFonts.AppendElements(1);
}

gfxPangoFontGroup::~gfxPangoFontGroup()
{
}

gfxFontGroup *
gfxPangoFontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxPangoFontGroup(mFamilies, aStyle, mUserFontSet);
}


void
gfxPangoFontGroup::GetFcFamilies(nsTArray<nsString> *aFcFamilyList,
                                 const nsACString& aLangGroup)
{
    FamilyCallbackData data(aFcFamilyList, mUserFontSet);
    
    
    ForEachFontInternal(mFamilies, aLangGroup, PR_TRUE, PR_FALSE,
                        FamilyCallback, &data);
}

PangoFont *
gfxPangoFontGroup::GetBasePangoFont()
{
    return GetBaseFontSet()->GetFontAt(0);
}

gfxFont *
gfxPangoFontGroup::GetFontAt(PRInt32 i) {
    
    
    
    
    NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                 "Whoever was caching this font group should have "
                 "called UpdateFontList on it");

    NS_PRECONDITION(i == 0, "Only have one font");

    if (!mFonts[0]) {
        PangoFont *pangoFont = GetBasePangoFont();
        mFonts[0] = gfxPangoFcFont::GfxFont(GFX_PANGO_FC_FONT(pangoFont));
    }

    return mFonts[0];
}

void
gfxPangoFontGroup::UpdateFontList()
{
    if (!mUserFontSet)
        return;

    PRUint64 newGeneration = mUserFontSet->GetGeneration();
    if (newGeneration == mCurrGeneration)
        return;

    mFonts[0] = NULL;
    mFontSets.Clear();
    mCurrGeneration = newGeneration;
}

already_AddRefed<gfxFcPangoFontSet>
gfxPangoFontGroup::MakeFontSet(PangoLanguage *aLang, gfxFloat aSizeAdjustFactor,
                               nsAutoRef<FcPattern> *aMatchPattern)
{
    const char *lang = pango_language_to_string(aLang);

    const char *langGroup = nsnull;
    if (aLang != mPangoLanguage) {
        
        if (!gLangService) {
            CallGetService(NS_LANGUAGEATOMSERVICE_CONTRACTID, &gLangService);
        }
        if (gLangService) {
            nsIAtom *atom =
                gLangService->LookupLanguage(NS_ConvertUTF8toUTF16(lang));
            if (atom) {
                atom->GetUTF8String(&langGroup);
            }
        }
    }

    nsAutoTArray<nsString, 20> fcFamilyList;
    GetFcFamilies(&fcFamilyList,
                  langGroup ? nsDependentCString(langGroup) : mStyle.langGroup);

    

    
    nsAutoRef<FcPattern> pattern
        (gfxFontconfigUtils::NewPattern(fcFamilyList, mStyle, lang));

    PrepareSortPattern(pattern, mStyle.size, aSizeAdjustFactor, mStyle.printerFont);

    nsRefPtr<gfxFcPangoFontSet> fontset =
        new gfxFcPangoFontSet(pattern, mUserFontSet);

    if (aMatchPattern)
        aMatchPattern->steal(pattern);

    return fontset.forget();
}

gfxPangoFontGroup::
FontSetByLangEntry::FontSetByLangEntry(PangoLanguage *aLang,
                                       gfxFcPangoFontSet *aFontSet)
    : mLang(aLang), mFontSet(aFontSet)
{
}

gfxFcPangoFontSet *
gfxPangoFontGroup::GetFontSet(PangoLanguage *aLang)
{
    GetBaseFontSet(); 

    if (!aLang)
        return mFontSets[0].mFontSet;

    for (PRUint32 i = 0; i < mFontSets.Length(); ++i) {
        if (mFontSets[i].mLang == aLang)
            return mFontSets[i].mFontSet;
    }

    nsRefPtr<gfxFcPangoFontSet> fontSet =
        MakeFontSet(aLang, mSizeAdjustFactor);
    mFontSets.AppendElement(FontSetByLangEntry(aLang, fontSet));

    return fontSet;
}





cairo_user_data_key_t gfxFcFont::sGfxFontKey;

gfxFcFont::gfxFcFont(cairo_scaled_font_t *aCairoFont,
                     gfxFontEntry *aFontEntry,
                     const gfxFontStyle *aFontStyle)
    : gfxFT2FontBase(aCairoFont, aFontEntry, aFontStyle)
{
    cairo_scaled_font_set_user_data(mScaledFont, &sGfxFontKey, this, NULL);
}

gfxFcFont::~gfxFcFont()
{
    cairo_scaled_font_set_user_data(mScaledFont, &sGfxFontKey, NULL, NULL);
}

 void
gfxPangoFontGroup::Shutdown()
{
    if (gPangoFontMap) {
        if (PANGO_IS_FC_FONT_MAP (gPangoFontMap)) {
            
            
            
            pango_fc_font_map_shutdown(PANGO_FC_FONT_MAP(gPangoFontMap));
        }
        g_object_unref(gPangoFontMap);
        gPangoFontMap = NULL;
    }

    
    
    gFTLibrary = NULL;

    NS_IF_RELEASE(gLangService);
}

 gfxFontEntry *
gfxPangoFontGroup::NewFontEntry(const gfxProxyFontEntry &aProxyEntry,
                                const nsAString& aFullname)
{
    gfxFontconfigUtils *utils = gfxFontconfigUtils::GetFontconfigUtils();
    if (!utils)
        return nsnull;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsAutoRef<FcPattern> pattern(FcPatternCreate());
    if (!pattern)
        return nsnull;

    NS_ConvertUTF16toUTF8 fullname(aFullname);
    FcPatternAddString(pattern, FC_FULLNAME,
                       gfxFontconfigUtils::ToFcChar8(fullname));
    FcConfigSubstitute(NULL, pattern, FcMatchPattern);

    FcChar8 *name;
    for (int v = 0;
         FcPatternGetString(pattern, FC_FULLNAME, v, &name) == FcResultMatch;
         ++v) {
        const nsTArray< nsCountedRef<FcPattern> >& fonts =
            utils->GetFontsForFullname(name);

        if (fonts.Length() != 0)
            return new gfxLocalFcFontEntry(aProxyEntry, fonts);
    }

    return nsnull;
}

static FT_Library
GetFTLibrary()
{
    if (!gFTLibrary) {
        
        
        
        
        
        
        
        
        gfxFontStyle style;
        nsRefPtr<gfxPangoFontGroup> fontGroup =
            new gfxPangoFontGroup(NS_LITERAL_STRING("sans-serif"),
                                  &style, nsnull);

        gfxFcFont *font = static_cast<gfxFcFont*>(fontGroup->GetFontAt(0));
        if (!font)
            return NULL;

        gfxFT2LockedFace face(font);
        if (!face.get())
            return NULL;

        gFTLibrary = face.get()->glyph->library;
    }

    return gFTLibrary;
}

 gfxFontEntry *
gfxPangoFontGroup::NewFontEntry(const gfxProxyFontEntry &aProxyEntry,
                                const PRUint8 *aFontData, PRUint32 aLength)
{
    
    

    
    
    FT_Face face;
    FT_Error error =
        FT_New_Memory_Face(GetFTLibrary(), aFontData, aLength, 0, &face);
    if (error != 0) {
        NS_Free((void*)aFontData);
        return nsnull;
    }

    return new gfxDownloadedFcFontEntry(aProxyEntry, aFontData, face);
}


static double
GetPixelSize(FcPattern *aPattern)
{
    double size;
    if (FcPatternGetDouble(aPattern,
                           FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
        return size;

    NS_NOTREACHED("No size on pattern");
    return 0.0;
}















already_AddRefed<gfxFcFont>
gfxFcFont::GetOrMakeFont(FcPattern *aPattern)
{
    cairo_scaled_font_t *cairoFont = CreateScaledFont(aPattern);

    nsRefPtr<gfxFcFont> font = static_cast<gfxFcFont*>
        (cairo_scaled_font_get_user_data(cairoFont, &sGfxFontKey));

    if (!font) {
        gfxFloat size = GetPixelSize(aPattern);

        
        
        PRUint8 style = gfxFontconfigUtils::GetThebesStyle(aPattern);
        PRUint16 weight = gfxFontconfigUtils::GetThebesWeight(aPattern);

        
        
        
        NS_NAMED_LITERAL_CSTRING(langGroup, "x-unicode");
        
        gfxFontStyle fontStyle(style, weight, NS_FONT_STRETCH_NORMAL,
                               size, langGroup, 0.0,
                               PR_TRUE, PR_FALSE, PR_FALSE);

        nsRefPtr<gfxFontEntry> fe;
        FcChar8 *fc_file;
        if (FcPatternGetString(aPattern,
                               FC_FILE, 0, &fc_file) == FcResultMatch) {
            int index;
            if (FcPatternGetInteger(aPattern,
                                    FC_INDEX, 0, &index) != FcResultMatch) {
                
                NS_NOTREACHED("No index in pattern for font face from file");
                index = 0;
            }

            
            nsAutoString name;
            AppendUTF8toUTF16(gfxFontconfigUtils::ToCString(fc_file), name);
            if (index != 0) {
                name.AppendLiteral("/");
                name.AppendInt(index);
            }

            fe = new gfxFontEntry(name);
        } else {
            fe = GetDownloadedFontEntry(aPattern);
            if (!fe) {
                
                
                NS_NOTREACHED("Fonts without a file is not a web font!?");
                fe = new gfxFontEntry(nsString());
            }
        }

        
        
        
        
        
        
        
        font = new gfxFcFont(cairoFont, fe, &fontStyle);
    }

    cairo_scaled_font_destroy(cairoFont);
    return font.forget();
}

static PangoFontMap *
GetPangoFontMap()
{
    if (!gPangoFontMap) {
        gPangoFontMap = gfxPangoFontMap::NewFontMap();
    }
    return gPangoFontMap;
}

static PangoContext *
GetPangoContext()
{
    PangoContext *context = pango_context_new();
    pango_context_set_font_map(context, GetPangoFontMap());
    return context;
}

gfxFcPangoFontSet *
gfxPangoFontGroup::GetBaseFontSet()
{
    if (mFontSets.Length() > 0)
        return mFontSets[0].mFontSet;

    mSizeAdjustFactor = 1.0; 
    nsAutoRef<FcPattern> pattern;
    nsRefPtr<gfxFcPangoFontSet> fontSet =
        MakeFontSet(mPangoLanguage, mSizeAdjustFactor, &pattern);

    double size = GetPixelSize(pattern);
    if (size != 0.0 && mStyle.sizeAdjust != 0.0) {
        gfxFcFont *font =
            gfxPangoFcFont::GfxFont(GFX_PANGO_FC_FONT(fontSet->GetFontAt(0)));
        if (font) {
            const gfxFont::Metrics& metrics = font->GetMetrics();

            
            
            
            if (metrics.xHeight > 0.1 * metrics.emHeight) {
                mSizeAdjustFactor =
                    mStyle.sizeAdjust * metrics.emHeight / metrics.xHeight;

                size *= mSizeAdjustFactor;
                FcPatternDel(pattern, FC_PIXEL_SIZE);
                FcPatternAddDouble(pattern, FC_PIXEL_SIZE, size);

                fontSet = new gfxFcPangoFontSet(pattern, mUserFontSet);
            }
        }
    }

    PangoLanguage *pangoLang = mPangoLanguage;
    FcChar8 *fcLang;
    if (!pangoLang &&
        FcPatternGetString(pattern, FC_LANG, 0, &fcLang) == FcResultMatch) {
        pangoLang =
            pango_language_from_string(gfxFontconfigUtils::ToCString(fcLang));
    }

    mFontSets.AppendElement(FontSetByLangEntry(pangoLang, fontSet));

    return fontSet;
}
















static PRInt32 AppendDirectionalIndicatorUTF8(PRBool aIsRTL, nsACString& aString)
{
    static const PRUnichar overrides[2][2] =
      { { 0x202d, 0 }, { 0x202e, 0 }}; 
    AppendUTF16toUTF8(overrides[aIsRTL], aString);
    return 3; 
}

gfxTextRun *
gfxPangoFontGroup::MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                               const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "8bit should have been set");
    gfxTextRun *run = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!run)
        return nsnull;

    PRBool isRTL = run->IsRightToLeft();
    if ((aFlags & TEXT_IS_ASCII) && !isRTL) {
        
        const gchar *utf8Chars = reinterpret_cast<const gchar*>(aString);
        InitTextRun(run, utf8Chars, aLength, 0, PR_TRUE);
    } else {
        
        const char *chars = reinterpret_cast<const char*>(aString);
        NS_ConvertASCIItoUTF16 unicodeString(chars, aLength);
        nsCAutoString utf8;
        PRInt32 headerLen = AppendDirectionalIndicatorUTF8(isRTL, utf8);
        AppendUTF16toUTF8(unicodeString, utf8);
        InitTextRun(run, utf8.get(), utf8.Length(), headerLen, PR_TRUE);
    }
    run->FetchGlyphExtents(aParams->mContext);
    return run;
}

#if defined(ENABLE_FAST_PATH_8BIT)
PRBool
gfxPangoFontGroup::CanTakeFastPath(PRUint32 aFlags)
{
    
    
    
    PRBool speed = aFlags & gfxTextRunFactory::TEXT_OPTIMIZE_SPEED;
    PRBool isRTL = aFlags & gfxTextRunFactory::TEXT_IS_RTL;
    return speed && !isRTL && PANGO_IS_FC_FONT(GetBasePangoFont());
}
#endif

gfxTextRun *
gfxPangoFontGroup::MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                               const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
    gfxTextRun *run = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!run)
        return nsnull;

    nsCAutoString utf8;
    PRInt32 headerLen = AppendDirectionalIndicatorUTF8(run->IsRightToLeft(), utf8);
    AppendUTF16toUTF8(Substring(aString, aString + aLength), utf8);
    PRBool is8Bit = PR_FALSE;

#if defined(ENABLE_FAST_PATH_8BIT)
    if (CanTakeFastPath(aFlags)) {
        PRUint32 allBits = 0;
        PRUint32 i;
        for (i = 0; i < aLength; ++i) {
            allBits |= aString[i];
        }
        is8Bit = (allBits & 0xFF00) == 0;
    }
#endif
    InitTextRun(run, utf8.get(), utf8.Length(), headerLen, is8Bit);
    run->FetchGlyphExtents(aParams->mContext);
    return run;
}

void
gfxPangoFontGroup::InitTextRun(gfxTextRun *aTextRun, const gchar *aUTF8Text,
                               PRUint32 aUTF8Length, PRUint32 aUTF8HeaderLength,
                               PRBool aTake8BitPath)
{
#if defined(ENABLE_FAST_PATH_ALWAYS)
    CreateGlyphRunsFast(aTextRun, aUTF8Text + aUTF8HeaderLength, aUTF8Length - aUTF8HeaderLength);
#else
#if defined(ENABLE_FAST_PATH_8BIT)
    if (aTake8BitPath && CanTakeFastPath(aTextRun->GetFlags())) {
        nsresult rv = CreateGlyphRunsFast(aTextRun, aUTF8Text + aUTF8HeaderLength, aUTF8Length - aUTF8HeaderLength);
        if (NS_SUCCEEDED(rv))
            return;
    }
#endif

    CreateGlyphRunsItemizing(aTextRun, aUTF8Text, aUTF8Length, aUTF8HeaderLength);
#endif
}

static void ReleaseDownloadedFontEntry(void *data)
{
    gfxDownloadedFcFontEntry *downloadedFontEntry =
        static_cast<gfxDownloadedFcFontEntry*>(data);
    NS_RELEASE(downloadedFontEntry);
}


static cairo_scaled_font_t *
CreateScaledFont(FcPattern *aPattern)
{
    cairo_font_face_t *face = cairo_ft_font_face_create_for_pattern(aPattern);

    
    
    gfxDownloadedFcFontEntry *downloadedFontEntry =
        GetDownloadedFontEntry(aPattern);
    if (downloadedFontEntry &&
        cairo_font_face_status(face) == CAIRO_STATUS_SUCCESS) {
        static cairo_user_data_key_t sFontEntryKey;

        
        void *currentEntry =
            cairo_font_face_get_user_data(face, &sFontEntryKey);
        if (!currentEntry) {
            NS_ADDREF(downloadedFontEntry);
            cairo_font_face_set_user_data(face, &sFontEntryKey,
                                          downloadedFontEntry,
                                          ReleaseDownloadedFontEntry);
        } else {
            NS_ASSERTION(currentEntry == downloadedFontEntry,
                         "Unexpected cairo font face!");
        }
    }

    double size = GetPixelSize(aPattern);
        
    cairo_matrix_t fontMatrix;
    FcMatrix *fcMatrix;
    if (FcPatternGetMatrix(aPattern, FC_MATRIX, 0, &fcMatrix) == FcResultMatch)
        cairo_matrix_init(&fontMatrix, fcMatrix->xx, -fcMatrix->yx, -fcMatrix->xy, fcMatrix->yy, 0, 0);
    else
        cairo_matrix_init_identity(&fontMatrix);
    cairo_matrix_scale(&fontMatrix, size, size);

    
    
    
    cairo_matrix_t identityMatrix;
    cairo_matrix_init_identity(&identityMatrix);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    cairo_font_options_t *fontOptions = cairo_font_options_create();

    
    
    
    
    
    
    cairo_font_options_set_hint_metrics(fontOptions, CAIRO_HINT_METRICS_ON);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    FcBool hinting;
    if (FcPatternGetBool(aPattern, FC_HINTING, 0, &hinting) != FcResultMatch) {
        hinting = FcTrue;
    }
    cairo_hint_style_t hint_style;
    if (!hinting) {
        hint_style = CAIRO_HINT_STYLE_NONE;
    } else {
#ifdef FC_HINT_STYLE  
        int fc_hintstyle;
        if (FcPatternGetInteger(aPattern, FC_HINT_STYLE,
                                0, &fc_hintstyle        ) != FcResultMatch) {
            fc_hintstyle = FC_HINT_FULL;
        }
        switch (fc_hintstyle) {
            case FC_HINT_NONE:
                hint_style = CAIRO_HINT_STYLE_NONE;
                break;
            case FC_HINT_SLIGHT:
                hint_style = CAIRO_HINT_STYLE_SLIGHT;
                break;
            case FC_HINT_MEDIUM:
            default: 
                hint_style = CAIRO_HINT_STYLE_MEDIUM;
                break;
            case FC_HINT_FULL:
                hint_style = CAIRO_HINT_STYLE_FULL;
                break;
        }
#else 
        hint_style = CAIRO_HINT_STYLE_FULL;
#endif
    }
    cairo_font_options_set_hint_style(fontOptions, hint_style);

    int rgba;
    if (FcPatternGetInteger(aPattern,
                            FC_RGBA, 0, &rgba) != FcResultMatch) {
        rgba = FC_RGBA_UNKNOWN;
    }
    cairo_subpixel_order_t subpixel_order = CAIRO_SUBPIXEL_ORDER_DEFAULT;
    switch (rgba) {
        case FC_RGBA_UNKNOWN:
        case FC_RGBA_NONE:
        default:
            
            
            rgba = FC_RGBA_NONE;
            
            
            
        case FC_RGBA_RGB:
            subpixel_order = CAIRO_SUBPIXEL_ORDER_RGB;
            break;
        case FC_RGBA_BGR:
            subpixel_order = CAIRO_SUBPIXEL_ORDER_BGR;
            break;
        case FC_RGBA_VRGB:
            subpixel_order = CAIRO_SUBPIXEL_ORDER_VRGB;
            break;
        case FC_RGBA_VBGR:
            subpixel_order = CAIRO_SUBPIXEL_ORDER_VBGR;
            break;
    }
    cairo_font_options_set_subpixel_order(fontOptions, subpixel_order);

    FcBool fc_antialias;
    if (FcPatternGetBool(aPattern,
                         FC_ANTIALIAS, 0, &fc_antialias) != FcResultMatch) {
        fc_antialias = FcTrue;
    }
    cairo_antialias_t antialias;
    if (!fc_antialias) {
        antialias = CAIRO_ANTIALIAS_NONE;
    } else if (rgba == FC_RGBA_NONE) {
        antialias = CAIRO_ANTIALIAS_GRAY;
    } else {
        antialias = CAIRO_ANTIALIAS_SUBPIXEL;
    }
    cairo_font_options_set_antialias(fontOptions, antialias);

    cairo_scaled_font_t *scaledFont =
        cairo_scaled_font_create(face, &fontMatrix, &identityMatrix,
                                 fontOptions);

    cairo_font_options_destroy(fontOptions);
    cairo_font_face_destroy(face);

    NS_ASSERTION(cairo_scaled_font_status(scaledFont) == CAIRO_STATUS_SUCCESS,
                 "Failed to create scaled font");
    return scaledFont;
}

static void
SetupClusterBoundaries(gfxTextRun* aTextRun, const gchar *aUTF8, PRUint32 aUTF8Length,
                       PRUint32 aUTF16Offset, PangoAnalysis *aAnalysis)
{
    if (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) {
        
        
        
        
        return;
    }

    
    
    
    
    nsAutoTArray<PangoLogAttr,2000> buffer;
    if (!buffer.AppendElements(aUTF8Length + 1))
        return;

    pango_break(aUTF8, aUTF8Length, aAnalysis,
                buffer.Elements(), buffer.Length());

    const gchar *p = aUTF8;
    const gchar *end = aUTF8 + aUTF8Length;
    const PangoLogAttr *attr = buffer.Elements();
    gfxTextRun::CompressedGlyph g;
    while (p < end) {
        if (!attr->is_cursor_position) {
            aTextRun->SetGlyphs(aUTF16Offset, g.SetComplex(PR_FALSE, PR_TRUE, 0), nsnull);
        }
        ++aUTF16Offset;
        
        gunichar ch = g_utf8_get_char(p);
        NS_ASSERTION(ch != 0, "Shouldn't have NUL in pango_break");
        NS_ASSERTION(!IS_SURROGATE(ch), "Shouldn't have surrogates in UTF8");
        if (ch >= 0x10000) {
            
            aTextRun->SetGlyphs(aUTF16Offset, g.SetComplex(PR_FALSE, PR_FALSE, 0), nsnull);
            ++aUTF16Offset;
        }
        
        p = g_utf8_next_char(p);
        ++attr;
    }
}

static PRInt32
ConvertPangoToAppUnits(PRInt32 aCoordinate, PRUint32 aAppUnitsPerDevUnit)
{
    PRInt64 v = (PRInt64(aCoordinate)*aAppUnitsPerDevUnit + PANGO_SCALE/2)/PANGO_SCALE;
    return PRInt32(v);
}






 
static nsresult
SetGlyphsForCharacterGroup(const PangoGlyphInfo *aGlyphs, PRUint32 aGlyphCount,
                           gfxTextRun *aTextRun,
                           const gchar *aUTF8, PRUint32 aUTF8Length,
                           PRUint32 *aUTF16Offset,
                           PangoGlyphUnit aOverrideSpaceWidth)
{
    PRUint32 utf16Offset = *aUTF16Offset;
    PRUint32 textRunLength = aTextRun->GetLength();
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    const gfxTextRun::CompressedGlyph *charGlyphs = aTextRun->GetCharacterGlyphs();

    
    
    PangoGlyphUnit width = aGlyphs[0].geometry.width;
    if (aOverrideSpaceWidth && aUTF8[0] == ' ' &&
        (utf16Offset + 1 == textRunLength ||
         charGlyphs[utf16Offset].IsClusterStart())) {
        width = aOverrideSpaceWidth;
    }
    PRInt32 advance = ConvertPangoToAppUnits(width, appUnitsPerDevUnit);

    gfxTextRun::CompressedGlyph g;
    PRBool atClusterStart = aTextRun->IsClusterStart(utf16Offset);
    
    if (aGlyphCount == 1 && advance >= 0 && atClusterStart &&
        aGlyphs[0].geometry.x_offset == 0 &&
        aGlyphs[0].geometry.y_offset == 0 &&
        !IS_EMPTY_GLYPH(aGlyphs[0].glyph) &&
        gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
        gfxTextRun::CompressedGlyph::IsSimpleGlyphID(aGlyphs[0].glyph)) {
        aTextRun->SetSimpleGlyph(utf16Offset,
                                 g.SetSimpleGlyph(advance, aGlyphs[0].glyph));
    } else {
        nsAutoTArray<gfxTextRun::DetailedGlyph,10> detailedGlyphs;
        if (!detailedGlyphs.AppendElements(aGlyphCount))
            return NS_ERROR_OUT_OF_MEMORY;

        PRInt32 direction = aTextRun->IsRightToLeft() ? -1 : 1;
        PRUint32 pangoIndex = direction > 0 ? 0 : aGlyphCount - 1;
        PRUint32 detailedIndex = 0;
        for (PRUint32 i = 0; i < aGlyphCount; ++i) {
            const PangoGlyphInfo &glyph = aGlyphs[pangoIndex];
            pangoIndex += direction;
            
            
            if (IS_EMPTY_GLYPH(glyph.glyph))
                continue;

            gfxTextRun::DetailedGlyph *details = &detailedGlyphs[detailedIndex];
            ++detailedIndex;

            details->mGlyphID = glyph.glyph;
            NS_ASSERTION(details->mGlyphID == glyph.glyph,
                         "Seriously weird glyph ID detected!");
            details->mAdvance =
                ConvertPangoToAppUnits(glyph.geometry.width,
                                       appUnitsPerDevUnit);
            details->mXOffset =
                float(glyph.geometry.x_offset)*appUnitsPerDevUnit/PANGO_SCALE;
            details->mYOffset =
                float(glyph.geometry.y_offset)*appUnitsPerDevUnit/PANGO_SCALE;
        }
        g.SetComplex(atClusterStart, PR_TRUE, detailedIndex);
        aTextRun->SetGlyphs(utf16Offset, g, detailedGlyphs.Elements());
    }

    
    const gchar *p = aUTF8;
    const gchar *end = aUTF8 + aUTF8Length;
    while (1) {
        
        
        
        gunichar ch = g_utf8_get_char(p);
        NS_ASSERTION(!IS_SURROGATE(ch), "surrogates should not appear in UTF8");
        if (ch >= 0x10000) {
            
            ++utf16Offset;
        }
        NS_ASSERTION(!gfxFontGroup::IsInvalidChar(PRUnichar(ch)),
                     "Invalid character detected");
        ++utf16Offset;

        
        p = g_utf8_next_char(p);
        if (p >= end)
            break;

        if (utf16Offset >= textRunLength) {
            NS_ERROR("Someone has added too many glyphs!");
            return NS_ERROR_FAILURE;
        }

        g.SetComplex(aTextRun->IsClusterStart(utf16Offset), PR_FALSE, 0);
        aTextRun->SetGlyphs(utf16Offset, g, nsnull);
    }
    *aUTF16Offset = utf16Offset;
    return NS_OK;
}

nsresult
gfxPangoFontGroup::SetGlyphs(gfxTextRun *aTextRun,
                             const gchar *aUTF8, PRUint32 aUTF8Length,
                             PRUint32 *aUTF16Offset, PangoGlyphString *aGlyphs,
                             PangoGlyphUnit aOverrideSpaceWidth,
                             PRBool aAbortOnMissingGlyph)
{
    gint numGlyphs = aGlyphs->num_glyphs;
    PangoGlyphInfo *glyphs = aGlyphs->glyphs;
    const gint *logClusters = aGlyphs->log_clusters;
    
    
    

    
    
    
    
    nsAutoTArray<gint,2000> logGlyphs;
    if (!logGlyphs.AppendElements(aUTF8Length + 1))
        return NS_ERROR_OUT_OF_MEMORY;
    PRUint32 utf8Index = 0;
    for(; utf8Index < aUTF8Length; ++utf8Index)
        logGlyphs[utf8Index] = -1;
    logGlyphs[aUTF8Length] = numGlyphs;

    gint lastCluster = -1; 
    for (gint glyphIndex = 0; glyphIndex < numGlyphs; ++glyphIndex) {
        gint thisCluster = logClusters[glyphIndex];
        if (thisCluster != lastCluster) {
            lastCluster = thisCluster;
            NS_ASSERTION(0 <= thisCluster && thisCluster < gint(aUTF8Length),
                         "garbage from pango_shape - this is bad");
            logGlyphs[thisCluster] = glyphIndex;
        }
    }

    PRUint32 utf16Offset = *aUTF16Offset;
    PRUint32 textRunLength = aTextRun->GetLength();
    utf8Index = 0;
    
    gint nextGlyphClusterStart = logGlyphs[utf8Index];
    NS_ASSERTION(nextGlyphClusterStart >= 0, "No glyphs! - NUL in string?");
    while (utf8Index < aUTF8Length) {
        if (utf16Offset >= textRunLength) {
          NS_ERROR("Someone has added too many glyphs!");
          return NS_ERROR_FAILURE;
        }
        gint glyphClusterStart = nextGlyphClusterStart;
        
        PRUint32 clusterUTF8Start = utf8Index;
        
        NS_ASSERTION(aTextRun->GetCharacterGlyphs()->IsClusterStart(),
                     "Glyph cluster not aligned on character cluster.");
        do {
            ++utf8Index;
            nextGlyphClusterStart = logGlyphs[utf8Index];
        } while (nextGlyphClusterStart < 0);
        const gchar *clusterUTF8 = &aUTF8[clusterUTF8Start];
        PRUint32 clusterUTF8Length = utf8Index - clusterUTF8Start;

        PRBool haveMissingGlyph = PR_FALSE;
        gint glyphIndex = glyphClusterStart;

        
        do {
            if (IS_MISSING_GLYPH(glyphs[glyphIndex].glyph)) {
                
                
                
                haveMissingGlyph = PR_TRUE;
            }
            glyphIndex++;
        } while (glyphIndex < numGlyphs && 
                 logClusters[glyphIndex] == gint(clusterUTF8Start));

        if (haveMissingGlyph && aAbortOnMissingGlyph)
            return NS_ERROR_FAILURE;

        nsresult rv;
        if (haveMissingGlyph) {
            rv = SetMissingGlyphs(aTextRun, clusterUTF8, clusterUTF8Length,
                             &utf16Offset);
        } else {
            rv = SetGlyphsForCharacterGroup(&glyphs[glyphClusterStart],
                                            glyphIndex - glyphClusterStart,
                                            aTextRun,
                                            clusterUTF8, clusterUTF8Length,
                                            &utf16Offset, aOverrideSpaceWidth);
        }
        NS_ENSURE_SUCCESS(rv,rv);
    }
    *aUTF16Offset = utf16Offset;
    return NS_OK;
}

nsresult
gfxPangoFontGroup::SetMissingGlyphs(gfxTextRun *aTextRun,
                                    const gchar *aUTF8, PRUint32 aUTF8Length,
                                    PRUint32 *aUTF16Offset)
{
    PRUint32 utf16Offset = *aUTF16Offset;
    PRUint32 textRunLength = aTextRun->GetLength();
    for (PRUint32 index = 0; index < aUTF8Length;) {
        if (utf16Offset >= textRunLength) {
            NS_ERROR("Someone has added too many glyphs!");
            break;
        }
        gunichar ch = g_utf8_get_char(aUTF8 + index);
        aTextRun->SetMissingGlyph(utf16Offset, ch);

        ++utf16Offset;
        NS_ASSERTION(!IS_SURROGATE(ch), "surrogates should not appear in UTF8");
        if (ch >= 0x10000)
            ++utf16Offset;
        
        index = g_utf8_next_char(aUTF8 + index) - aUTF8;
    }

    *aUTF16Offset = utf16Offset;
    return NS_OK;
}

#if defined(ENABLE_FAST_PATH_8BIT) || defined(ENABLE_FAST_PATH_ALWAYS)
nsresult
gfxPangoFontGroup::CreateGlyphRunsFast(gfxTextRun *aTextRun,
                                       const gchar *aUTF8, PRUint32 aUTF8Length)
{
    const gchar *p = aUTF8;
    PangoFont *pangofont = GetBasePangoFont();
    gfxFcFont *gfxFont = gfxPangoFcFont::GfxFont(GFX_PANGO_FC_FONT(pangofont));
    PRUint32 utf16Offset = 0;
    gfxTextRun::CompressedGlyph g;
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();

    aTextRun->AddGlyphRun(gfxFont, 0);

    while (p < aUTF8 + aUTF8Length) {
        
        
        
        
        gunichar ch = g_utf8_get_char(p);
        p = g_utf8_next_char(p);
        
        if (ch == 0) {
            
            
            aTextRun->SetMissingGlyph(utf16Offset, 0);
        } else {
            NS_ASSERTION(!IsInvalidChar(ch), "Invalid char detected");
            FT_UInt glyph = gfxFont->GetGlyph(ch);
            if (!glyph)                  
                return NS_ERROR_FAILURE; 

            cairo_text_extents_t extents;
            gfxFont->GetGlyphExtents(glyph, &extents);

            PRInt32 advance = NS_lround(extents.x_advance * appUnitsPerDevUnit);
            if (advance >= 0 &&
                gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
                gfxTextRun::CompressedGlyph::IsSimpleGlyphID(glyph)) {
                aTextRun->SetSimpleGlyph(utf16Offset,
                                         g.SetSimpleGlyph(advance, glyph));
            } else {
                gfxTextRun::DetailedGlyph details;
                details.mGlyphID = glyph;
                NS_ASSERTION(details.mGlyphID == glyph,
                             "Seriously weird glyph ID detected!");
                details.mAdvance = advance;
                details.mXOffset = 0;
                details.mYOffset = 0;
                g.SetComplex(aTextRun->IsClusterStart(utf16Offset), PR_TRUE, 1);
                aTextRun->SetGlyphs(utf16Offset, g, &details);
            }

            NS_ASSERTION(!IS_SURROGATE(ch), "Surrogates shouldn't appear in UTF8");
            if (ch >= 0x10000) {
                
                ++utf16Offset;
            }
        }

        ++utf16Offset;
    }
    return NS_OK;
}
#endif

void 
gfxPangoFontGroup::CreateGlyphRunsItemizing(gfxTextRun *aTextRun,
                                            const gchar *aUTF8, PRUint32 aUTF8Length,
                                            PRUint32 aUTF8HeaderLen)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    PangoContext *context = GetPangoContext();
    
    
    pango_context_set_language(context, mPangoLanguage);
    SetFontGroup(context, this);

    PangoDirection dir = aTextRun->IsRightToLeft() ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;
    GList *items = pango_itemize_with_base_dir(context, dir, aUTF8, 0, aUTF8Length, nsnull, nsnull);

    PRUint32 utf16Offset = 0;
#ifdef DEBUG
    PRBool isRTL = aTextRun->IsRightToLeft();
#endif
    GList *pos = items;
    PangoGlyphString *glyphString = pango_glyph_string_new();
    if (!glyphString)
        goto out; 

    for (; pos && pos->data; pos = pos->next) {
        PangoItem *item = (PangoItem *)pos->data;
        NS_ASSERTION(isRTL == item->analysis.level % 2, "RTL assumption mismatch");

        PRUint32 offset = item->offset;
        PRUint32 length = item->length;
        if (offset < aUTF8HeaderLen) {
            if (offset + length <= aUTF8HeaderLen)
                continue;

            length -= aUTF8HeaderLen - offset;
            offset = aUTF8HeaderLen;
        }

        gfxFcFont *font =
            gfxPangoFcFont::GfxFont(GFX_PANGO_FC_FONT(item->analysis.font));

        nsresult rv = aTextRun->AddGlyphRun(font, utf16Offset);
        if (NS_FAILED(rv)) {
            NS_ERROR("AddGlyphRun Failed");
            goto out;
        }

        PRUint32 spaceWidth =
            moz_pango_units_from_double(font->GetMetrics().spaceWidth);

        const gchar *p = aUTF8 + offset;
        const gchar *end = p + length;
        while (p < end) {
            if (*p == 0) {
                aTextRun->SetMissingGlyph(utf16Offset, 0);
                ++p;
                ++utf16Offset;
                continue;
            }

            
            
            const gchar *text = p;
            do {
                ++p;
            } while(p < end && *p != 0);
            gint len = p - text;

            pango_shape(text, len, &item->analysis, glyphString);
            SetupClusterBoundaries(aTextRun, text, len, utf16Offset, &item->analysis);
            SetGlyphs(aTextRun, text, len, &utf16Offset, glyphString, spaceWidth, PR_FALSE);
        }
    }

out:
    if (glyphString)
        pango_glyph_string_free(glyphString);

    for (pos = items; pos; pos = pos->next)
        pango_item_free((PangoItem *)pos->data);

    if (items)
        g_list_free(items);

    g_object_unref(context);
}


PangoLanguage *
GuessPangoLanguage(const nsACString& aLangGroup)
{
    
    
    nsCAutoString lang;
    gfxFontconfigUtils::GetSampleLangForGroup(aLangGroup, &lang);

    if (lang.IsEmpty())
        return NULL;

    return pango_language_from_string(lang.get());
}

#ifdef MOZ_WIDGET_GTK2







#if MOZ_TREE_CAIRO


#undef cairo_ft_font_options_substitute



extern "C" {
NS_VISIBILITY_DEFAULT void
cairo_ft_font_options_substitute (const cairo_font_options_t *options,
                                  FcPattern                  *pattern);
}
#endif

static void
ApplyGdkScreenFontOptions(FcPattern *aPattern)
{
    const cairo_font_options_t *options =
        gdk_screen_get_font_options(gdk_screen_get_default());

    cairo_ft_font_options_substitute(options, aPattern);
}

#endif 
