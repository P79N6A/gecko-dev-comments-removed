











































#define PANGO_ENABLE_BACKEND

#include "prtypes.h"
#include "prlink.h"
#include "gfxTypes.h"

#include "nsMathUtils.h"

#include "gfxContext.h"
#include "gfxPlatformGtk.h"
#include "gfxPangoFonts.h"
#include "gfxFontconfigUtils.h"

#include <freetype/tttables.h>

#include <cairo.h>
#include <cairo-ft.h>

#include <fontconfig/fcfreetype.h>
#include <pango/pango.h>
#include <pango/pangofc-fontmap.h>

#include <gdk/gdkscreen.h>

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

static PangoLanguage *GetPangoLanguage(const nsACString& aLangGroup);

static cairo_scaled_font_t *CreateScaledFont(FcPattern *aPattern);

 gfxPangoFontCache* gfxPangoFontCache::sPangoFontCache = nsnull;

static PangoFontMap *gPangoFontMap;








class gfxFcFont : public gfxFont {
public:
    virtual ~gfxFcFont ();
    static already_AddRefed<gfxFcFont> GetOrMakeFont(FcPattern *aPattern);

    virtual const gfxFont::Metrics& GetMetrics();

    virtual nsString GetUniqueName();

    
    virtual PRUint32 GetSpaceGlyph() {
        NS_ASSERTION(GetStyle()->size != 0,
                     "forgot to short-circuit a text run with zero-sized font?");
        GetMetrics();
        return mSpaceGlyph;
    }

    cairo_scaled_font_t *CairoScaledFont() { return mCairoFont; }
    void GetGlyphExtents(PRUint32 aGlyph, cairo_text_extents_t* aExtents);

protected:
    cairo_scaled_font_t *mCairoFont;

    PRUint32 mSpaceGlyph;
    Metrics mMetrics;
    PRPackedBool mHasMetrics;

    gfxFcFont(cairo_scaled_font_t *aCairoFont,
              gfxPangoFontEntry *aFontEntry, const gfxFontStyle *aFontStyle);

    virtual PRBool SetupCairoFont(gfxContext *aContext);

    
    static cairo_user_data_key_t sGfxFontKey;
};

class LockedFTFace {
public:
    LockedFTFace(gfxFcFont *aFont)
        : mGfxFont(aFont),
          mFace(cairo_ft_scaled_font_lock_face(aFont->CairoScaledFont()))
    {
    }

    ~LockedFTFace()
    {
        if (mFace) {
            cairo_ft_scaled_font_unlock_face(mGfxFont->CairoScaledFont());
        }
    }

    




    PRUint32 GetCharExtents(char aChar, cairo_text_extents_t* aExtents);

    void GetMetrics(gfxFont::Metrics* aMetrics, PRUint32* aSpaceGlyph);

private:
    nsRefPtr<gfxFcFont> mGfxFont;
    FT_Face mFace;
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

    gfxFcFont *mGfxFont;

    static gfxFcFont *GfxFont(gfxPangoFcFont *self)
    {
        if (!self->mGfxFont) {
            FcPattern *pattern = PANGO_FC_FONT(self)->font_pattern;
            self->mGfxFont = gfxFcFont::GetOrMakeFont(pattern).get();
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
gfx_pango_fc_font_init(gfxPangoFcFont *fontset)
{
}


static void
gfx_pango_fc_font_finalize(GObject *object)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(object);

    if (self->mGfxFont)
        self->mGfxFont->Release();

    G_OBJECT_CLASS(gfx_pango_fc_font_parent_class)->finalize(object);
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

#ifdef DEBUG
static PangoFontMetrics *
gfx_pango_fc_font_get_metrics(PangoFont *font, PangoLanguage *language)
{
    NS_WARNING("Using PangoFcFont::get_metrics");

    return PANGO_FONT_CLASS(gfx_pango_fc_font_parent_class)->
        get_metrics(font, language);
}
#endif

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

static void
gfx_pango_fc_font_class_init (gfxPangoFcFontClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    PangoFontClass *font_class = PANGO_FONT_CLASS (klass);
    PangoFcFontClass *fc_font_class = PANGO_FC_FONT_CLASS (klass);

    object_class->finalize = gfx_pango_fc_font_finalize;

#if 0
    
    
    font_class->get_coverage =
#endif
    font_class->get_glyph_extents = gfx_pango_fc_font_get_glyph_extents;
#ifdef DEBUG
    
    font_class->get_metrics = gfx_pango_fc_font_get_metrics;
#endif
    
    fc_font_class->lock_face = gfx_pango_fc_font_lock_face;
    fc_font_class->unlock_face = gfx_pango_fc_font_unlock_face;
}





static GQuark GetBaseFontQuark()
{
    
    
    
    static GQuark quark = g_quark_from_string("moz-base-font");
    return quark;
}

static PangoFont *
GetBaseFont(PangoContext *aContext)
{
    return static_cast<PangoFont*>
        (g_object_get_qdata(G_OBJECT(aContext), GetBaseFontQuark()));
}

static void
SetBaseFont(PangoContext *aContext, PangoFont *aBaseFont)
{
    g_object_ref(aBaseFont);
    g_object_set_qdata_full(G_OBJECT(aContext), GetBaseFontQuark(),
                            aBaseFont, g_object_unref);
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

    PangoFontMap *mFontMap;
    PangoContext *mContext;
    PangoFontDescription *mFontDesc;
    PangoLanguage *mLanguage;
    PangoFont *mBaseFont;
    PangoFontset *mFallbackFontset;

    static PangoFontset *
    NewFontset(PangoFontMap *aFontMap,
               PangoContext *aContext,
               const PangoFontDescription *aFontDesc,
               PangoLanguage *aLanguage)
    {
        gfxPangoFontset *fontset = static_cast<gfxPangoFontset *>
            (g_object_new(GFX_TYPE_PANGO_FONTSET, NULL));

        fontset->mFontMap = aFontMap;
        g_object_ref(aFontMap);

        fontset->mContext = aContext;
        g_object_ref(aContext);

        fontset->mFontDesc = pango_font_description_copy(aFontDesc);
        fontset->mLanguage = aLanguage;

        fontset->mBaseFont = GetBaseFont(aContext);
        if(fontset->mBaseFont)
            g_object_ref(fontset->mBaseFont);

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

    if (self->mContext)
        g_object_unref(self->mContext);
    if (self->mFontDesc)
        pango_font_description_free(self->mFontDesc);
    if (self->mBaseFont)
        g_object_unref(self->mBaseFont);
    if (self->mFontMap)
        g_object_unref(self->mFontMap);
    if (self->mFallbackFontset)
        g_object_unref(self->mFallbackFontset);

    G_OBJECT_CLASS(gfx_pango_fontset_parent_class)->finalize(object);
}

static PangoLanguage *
gfx_pango_fontset_get_language(PangoFontset *fontset)
{
    gfxPangoFontset *self = GFX_PANGO_FONTSET(fontset);
    return self->mLanguage;
}

struct ForeachExceptBaseData {
    PangoFont *mBaseFont;
    PangoFontset *mFontset;
    PangoFontsetForeachFunc mFunc;
    gpointer mData;
};

static gboolean
foreach_except_base_cb(PangoFontset *fontset, PangoFont *font, gpointer data)
{
    ForeachExceptBaseData *baseData =
        static_cast<ForeachExceptBaseData *>(data);
    
    
    return font != baseData->mBaseFont &&
        (*baseData->mFunc)(baseData->mFontset, font, baseData->mData);
}

static PangoFontset *
gfx_pango_font_map_load_fallback_fontset(PangoFontMap *fontmap,
                                         PangoContext *context,
                                         const PangoFontDescription *desc,
                                         PangoLanguage *language);

static PangoFontset *
EnsureFallbackFontset(gfxPangoFontset *self)
{
    if (!self->mFallbackFontset) {
        
        
        
        
        
        
        
        
        
        
        
        
        self->mFallbackFontset =
            gfx_pango_font_map_load_fallback_fontset(self->mFontMap,
                                                     self->mContext,
                                                     self->mFontDesc,
                                                     self->mLanguage);
    }
    return self->mFallbackFontset;
}

static void
gfx_pango_fontset_foreach(PangoFontset *fontset, PangoFontsetForeachFunc func,
                          gpointer data)
{
    gfxPangoFontset *self = GFX_PANGO_FONTSET(fontset);

    if (self->mBaseFont && (*func)(fontset, self->mBaseFont, data))
        return;

    
    PangoFontset *childFontset = EnsureFallbackFontset(self);
    ForeachExceptBaseData baseData = { self->mBaseFont, fontset, func, data };
    pango_fontset_foreach(childFontset, foreach_except_base_cb, &baseData);
}

static PangoFont *
gfx_pango_fontset_get_font(PangoFontset *fontset, guint wc)
{
    gfxPangoFontset *self = GFX_PANGO_FONTSET(fontset);

    PangoCoverageLevel baseLevel = PANGO_COVERAGE_NONE;
    if (self->mBaseFont) {
        
        PangoCoverage *coverage =
            pango_font_get_coverage(self->mBaseFont, self->mLanguage);
        if (coverage) {
            baseLevel = pango_coverage_get(coverage, wc);
            pango_coverage_unref(coverage);
        }
    }

    if (baseLevel != PANGO_COVERAGE_EXACT) {
        PangoFontset *childFontset = EnsureFallbackFontset(self);
        PangoFont *childFont = pango_fontset_get_font(childFontset, wc);
        if (!self->mBaseFont || childFont == self->mBaseFont)
            return childFont;

        if (childFont) {
            PangoCoverage *coverage =
                pango_font_get_coverage(childFont, self->mLanguage);
            if (coverage) {
                PangoCoverageLevel childLevel =
                    pango_coverage_get(coverage, wc);
                pango_coverage_unref(coverage);

                
                if (childLevel > baseLevel)
                    return childFont;
            }
            g_object_unref(childFont);
        }
    }

    g_object_ref(self->mBaseFont);
    return self->mBaseFont;
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
    PangoFont *baseFont = GetBaseFont(context);
    if (baseFont) {
        g_object_ref(baseFont);
        return baseFont;
    }

    return PANGO_FONT_MAP_CLASS(gfx_pango_font_map_parent_class)->
        load_font(fontmap, context, description);
}

static PangoFontset *
gfx_pango_font_map_load_fontset(PangoFontMap *fontmap, PangoContext *context,
                               const PangoFontDescription *desc,
                               PangoLanguage *language)
{
    return gfxPangoFontset::NewFontset(fontmap, context, desc, language);
}

static PangoFontset *
gfx_pango_font_map_load_fallback_fontset(PangoFontMap *fontmap,
                                         PangoContext *context,
                                         const PangoFontDescription *desc,
                                         PangoLanguage *language)
{
    return PANGO_FONT_MAP_CLASS(gfx_pango_font_map_parent_class)->
        load_fontset(fontmap, context, desc, language);
}

static void
gfx_pango_font_map_list_families(PangoFontMap *fontmap,
                                 PangoFontFamily ***families, int *n_families)
{
    return PANGO_FONT_MAP_CLASS(gfx_pango_font_map_parent_class)->
        list_families(fontmap, families, n_families);
}

static double
gfx_pango_font_map_get_resolution(PangoFcFontMap *fcfontmap,
                                  PangoContext *context)
{
    
    
    return gfxPlatformGtk::DPI();
}

static void
gfx_pango_font_map_context_substitute(PangoFcFontMap *fontmap,
                                      PangoContext *context,
                                      FcPattern *pattern)
{
    FcConfigSubstitute(NULL, pattern, FcMatchPattern);

    
    
    
    const cairo_font_options_t *options =
        gdk_screen_get_font_options(gdk_screen_get_default());

    cairo_ft_font_options_substitute(options, pattern);

    FcDefaultSubstitute(pattern);
}

static PangoFcFont *
gfx_pango_font_map_create_font(PangoFcFontMap *fontmap,
                               PangoContext *context,
                               const PangoFontDescription *desc,
                               FcPattern *pattern)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    PRBool newPattern = PR_FALSE;
    double size;
    if (FcPatternGetDouble(pattern, FC_PIXEL_SIZE, 0, &size) != FcResultMatch) {
        size = pango_font_description_get_size(desc) / FLOAT_PANGO_SCALE;
        pattern = FcPatternDuplicate(pattern);
        newPattern = PR_TRUE;
        FcPatternDel(pattern, FC_PIXEL_SIZE);
        FcPatternAddDouble(pattern, FC_PIXEL_SIZE, size);
    }

    PangoFcFont *font = PANGO_FC_FONT(g_object_new(GFX_TYPE_PANGO_FC_FONT,
                                                   "pattern", pattern, NULL));

    if (newPattern) {
        FcPatternDestroy(pattern);
    }
    return font;
}

static void
gfx_pango_font_map_class_init(gfxPangoFontMapClass *klass)
{
    
    

    PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (klass);
    fontmap_class->load_font = gfx_pango_font_map_load_font;
    fontmap_class->load_fontset = gfx_pango_font_map_load_fontset;
    fontmap_class->list_families = gfx_pango_font_map_list_families;
    

    PangoFcFontMapClass *fcfontmap_class = PANGO_FC_FONT_MAP_CLASS (klass);
    fcfontmap_class->get_resolution = gfx_pango_font_map_get_resolution;
    
    
    fcfontmap_class->context_substitute = gfx_pango_font_map_context_substitute;
    fcfontmap_class->create_font = gfx_pango_font_map_create_font;
}





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
FontCallback (const nsAString& fontName, const nsACString& genericName,
              void *closure)
{
    nsStringArray *sa = static_cast<nsStringArray*>(closure);

    
    if (genericName.Length() && FFRECountHyphens(fontName) >= 3)
        return PR_TRUE;

    if (sa->IndexOf(fontName) < 0) {
        sa->AppendString(fontName);
    }

    return PR_TRUE;
}

gfxPangoFontGroup::gfxPangoFontGroup (const nsAString& families,
                                      const gfxFontStyle *aStyle,
                                      gfxUserFontSet *aUserFontSet)
    : gfxFontGroup(families, aStyle, aUserFontSet),
      mBasePangoFont(nsnull), mAdjustedSize(0)
{
    mFonts.AppendElements(1);
}

gfxPangoFontGroup::~gfxPangoFontGroup()
{
    if (mBasePangoFont)
        g_object_unref(mBasePangoFont);
}

gfxFontGroup *
gfxPangoFontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxPangoFontGroup(mFamilies, aStyle, mUserFontSet);
}


void
gfxPangoFontGroup::GetFcFamilies(nsAString& aFcFamilies)
{
    nsStringArray familyArray;

    
    
    ForEachFontInternal(mFamilies, mStyle.langGroup, PR_TRUE, PR_FALSE,
                        FontCallback, &familyArray);

    if (familyArray.Count()) {
        int i = 0;
        while (1) {
            aFcFamilies.Append(*familyArray[i]);
            ++i;
            if (i >= familyArray.Count())
                break;
            aFcFamilies.Append(NS_LITERAL_STRING(","));
        }
    }
    else {
        
        
        
        
        
        aFcFamilies.Append(NS_LITERAL_STRING("sans-serif"));
    }
}

gfxFont *
gfxPangoFontGroup::GetFontAt(PRInt32 i) {
    NS_PRECONDITION(i == 0, "Only have one font");

    if (!mFonts[0]) {
        PangoFont *pangoFont = GetBasePangoFont();
        mFonts[0] = gfxPangoFcFont::GfxFont(GFX_PANGO_FC_FONT(pangoFont));
    }

    return mFonts[0];
}





cairo_user_data_key_t gfxFcFont::sGfxFontKey;

gfxFcFont::gfxFcFont(cairo_scaled_font_t *aCairoFont,
                     gfxPangoFontEntry *aFontEntry,
                     const gfxFontStyle *aFontStyle)
    : gfxFont(aFontEntry, aFontStyle),
      mCairoFont(aCairoFont),
      mHasMetrics(PR_FALSE)
{
    cairo_scaled_font_reference(mCairoFont);
    cairo_scaled_font_set_user_data(mCairoFont, &sGfxFontKey, this, NULL);
}

gfxFcFont::~gfxFcFont()
{
    cairo_scaled_font_set_user_data(mCairoFont, &sGfxFontKey, NULL, NULL);
    cairo_scaled_font_destroy(mCairoFont);
}

 void
gfxPangoFontGroup::Shutdown()
{
    gfxPangoFontCache::Shutdown();

    if (gPangoFontMap) {
        if (PANGO_IS_FC_FONT_MAP (gPangoFontMap)) {
            
            
            pango_fc_font_map_shutdown(PANGO_FC_FONT_MAP(gPangoFontMap));
        }
        g_object_unref(gPangoFontMap);
        gPangoFontMap = NULL;
    }
}

static PangoStyle
ThebesStyleToPangoStyle (const gfxFontStyle *fs)
{
    if (fs->style == FONT_STYLE_ITALIC)
        return PANGO_STYLE_ITALIC;
    if (fs->style == FONT_STYLE_OBLIQUE)
        return PANGO_STYLE_OBLIQUE;

    return PANGO_STYLE_NORMAL;
}

static PangoWeight
ThebesStyleToPangoWeight (const gfxFontStyle *fs)
{
    PRInt32 w = fs->weight;

    







    PRInt32 baseWeight = (w + 50) / 100;
    PRInt32 offset = w - baseWeight * 100;

    
    if (baseWeight < 0)
        baseWeight = 0;
    if (baseWeight > 9)
        baseWeight = 9;

    
    static const int fcWeightLookup[10] = {
        0, 0, 0, 0, 1, 1, 2, 3, 3, 4,
    };

    PRInt32 fcWeight = fcWeightLookup[baseWeight];

    



    fcWeight += offset;

    if (fcWeight < 0)
        fcWeight = 0;
    if (fcWeight > 4)
        fcWeight = 4;

    
    static const int fcWeights[5] = {
        349,
        449,
        649,
        749,
        900
    };

    return (PangoWeight)fcWeights[fcWeight];
}


static PangoFontDescription *
NewPangoFontDescription(const nsAString &aName, const gfxFontStyle *aFontStyle)
{
    PangoFontDescription *fontDesc = pango_font_description_new();

    pango_font_description_set_family(fontDesc,
                                      NS_ConvertUTF16toUTF8(aName).get());
    pango_font_description_set_absolute_size
        (fontDesc, moz_pango_units_from_double(aFontStyle->size));
    pango_font_description_set_style(fontDesc,
                                     ThebesStyleToPangoStyle(aFontStyle));
    pango_font_description_set_weight(fontDesc,
                                      ThebesStyleToPangoWeight(aFontStyle));
    return fontDesc;
}















already_AddRefed<gfxFcFont>
gfxFcFont::GetOrMakeFont(FcPattern *aPattern)
{
    cairo_scaled_font_t *cairoFont = CreateScaledFont(aPattern);

    nsRefPtr<gfxFcFont> font = static_cast<gfxFcFont*>
        (cairo_scaled_font_get_user_data(cairoFont, &sGfxFontKey));

    if (!font) {
        double size;
        if (FcPatternGetDouble(aPattern,
                               FC_PIXEL_SIZE, 0, &size) != FcResultMatch) {
            NS_NOTREACHED("No size on pattern");
            size = 0.0;
        }

        
        
        PRUint8 style = gfxFontconfigUtils::GetThebesStyle(aPattern);
        PRUint16 weight = gfxFontconfigUtils::GetThebesWeight(aPattern);

        
        
        
        NS_NAMED_LITERAL_CSTRING(langGroup, "x-unicode");
        gfxFontStyle fontStyle(style, weight, size, langGroup, 0.0,
                               PR_TRUE, PR_FALSE);

        FcChar8 *fc_file; 
        const char *file; 
        if (FcPatternGetString(aPattern,
                               FC_FILE, 0, &fc_file) == FcResultMatch) {
            file = reinterpret_cast<char*>(fc_file);
        } else {
            
            
            NS_NOTREACHED("Fonts without a file are not supported");
            static const char *noFile = "NO FILE";
            file = noFile;
        }
        int index;
        if (FcPatternGetInteger(aPattern, FC_INDEX, 0, &index)
            != FcResultMatch) {
            
            NS_NOTREACHED("No index in pattern");
            index = 0;
        }
        
        nsAutoString name;
        AppendUTF8toUTF16(file, name);
        if (index != 0) {
            name.AppendLiteral("/");
            name.AppendInt(index);
        }

        nsRefPtr<gfxPangoFontEntry> fe = new gfxPangoFontEntry(name);

        
        
        
        
        
        
        
        font = new gfxFcFont(cairoFont, fe, &fontStyle);
    }

    cairo_scaled_font_destroy(cairoFont);
    return font.forget();
}

static PangoContext *
GetPangoContext()
{
    PangoContext *context = pango_context_new();

    
    if (!gPangoFontMap) {
        gPangoFontMap = gfxPangoFontMap::NewFontMap();
    }
    pango_context_set_font_map(context, gPangoFontMap);

    return context;
}

static PangoFont*
LoadPangoFont(PangoContext *aPangoCtx, const PangoFontDescription *aPangoFontDesc)
{
    gfxPangoFontCache *cache = gfxPangoFontCache::GetPangoFontCache();
    if (!cache)
        return nsnull; 
    PangoFont* pangoFont = cache->Get(aPangoFontDesc);
    if (!pangoFont) {
        pangoFont = pango_context_load_font(aPangoCtx, aPangoFontDesc);
        if (pangoFont) {
            cache->Put(aPangoFontDesc, pangoFont);
        }
    }
    return pangoFont;
}


PangoFont *
gfxPangoFontGroup::GetBasePangoFont()
{
    if (mBasePangoFont)
        return mBasePangoFont;

    nsAutoString fcFamilies;
    GetFcFamilies(fcFamilies);
    PangoFontDescription *pangoFontDesc = 
        NewPangoFontDescription(fcFamilies, GetStyle());

    PangoContext *pangoCtx = GetPangoContext();

    if (!GetStyle()->langGroup.IsEmpty()) {
        PangoLanguage *lang = GetPangoLanguage(GetStyle()->langGroup);
        if (lang)
            pango_context_set_language(pangoCtx, lang);
    }

    PangoFont *pangoFont = LoadPangoFont(pangoCtx, pangoFontDesc);

    gfxFloat size = GetStyle()->size;
    if (size != 0.0 && GetStyle()->sizeAdjust != 0.0) {
        LockedFTFace
            face(gfxPangoFcFont::GfxFont(GFX_PANGO_FC_FONT(pangoFont)));
        
        cairo_text_extents_t extents;
        if (face.GetCharExtents('x', &extents) &&
            extents.y_bearing < 0.0) {
            gfxFloat aspect = -extents.y_bearing / size;
            size = GetStyle()->GetAdjustedSize(aspect);

            pango_font_description_set_absolute_size
                (pangoFontDesc, moz_pango_units_from_double(size));
            g_object_unref(pangoFont);
            pangoFont = LoadPangoFont(pangoCtx, pangoFontDesc);
        }
    }

    if (pangoFontDesc)
        pango_font_description_free(pangoFontDesc);
    if (pangoCtx)
        g_object_unref(pangoCtx);

    mBasePangoFont = pangoFont;
    mAdjustedSize = size;

    return pangoFont;
}

void
gfxFcFont::GetGlyphExtents(PRUint32 aGlyph, cairo_text_extents_t* aExtents)
{
    NS_PRECONDITION(aExtents != NULL, "aExtents must not be NULL");

    cairo_glyph_t glyphs[1];
    glyphs[0].index = aGlyph;
    glyphs[0].x = 0.0;
    glyphs[0].y = 0.0;
    
    
    
    
    
    cairo_scaled_font_glyph_extents(CairoScaledFont(), glyphs, 1, aExtents);
}

PRUint32
LockedFTFace::GetCharExtents(char aChar,
                             cairo_text_extents_t* aExtents)
{
    NS_PRECONDITION(aExtents != NULL, "aExtents must not be NULL");

    if (!mFace)
        return 0;

    
    
    
    
    
    
    
    
    
    FT_UInt gid = FcFreeTypeCharIndex(mFace, aChar); 
    if (gid) {
        mGfxFont->GetGlyphExtents(gid, aExtents);
    }

    return gid;
}




#define FLOAT_FROM_26_6(x) ((x) / 64.0)
#define FLOAT_FROM_16_16(x) ((x) / 65536.0)
#define ROUND_26_6_TO_INT(x) ((x) >= 0 ?  ((32 + (x)) >> 6) \
                                       : -((32 - (x)) >> 6))

static inline FT_Long
ScaleRoundDesignUnits(FT_Short aDesignMetric, FT_Fixed aScale)
{
    FT_Long fixed26dot6 = FT_MulFix(aDesignMetric, aScale);
    return ROUND_26_6_TO_INT(fixed26dot6);
}









static void
SnapLineToPixels(gfxFloat& aOffset, gfxFloat& aSize)
{
    gfxFloat snappedSize = PR_MAX(NS_floor(aSize + 0.5), 1.0);
    
    gfxFloat offset = aOffset - 0.5 * (aSize - snappedSize);
    
    aOffset = NS_floor(offset + 0.5);
    aSize = snappedSize;
}

void
LockedFTFace::GetMetrics(gfxFont::Metrics* aMetrics, PRUint32* aSpaceGlyph)
{
    NS_PRECONDITION(aMetrics != NULL, "aMetrics must not be NULL");
    NS_PRECONDITION(aSpaceGlyph != NULL, "aSpaceGlyph must not be NULL");

    if (NS_UNLIKELY(!mFace)) {
        
        
        aMetrics->emHeight = mGfxFont->GetStyle()->size;
        aMetrics->emAscent = 0.8 * aMetrics->emHeight;
        aMetrics->emDescent = 0.2 * aMetrics->emHeight;
        aMetrics->maxAscent = aMetrics->emAscent;
        aMetrics->maxDescent = aMetrics->maxDescent;
        aMetrics->maxHeight = aMetrics->emHeight;
        aMetrics->internalLeading = 0.0;
        aMetrics->externalLeading = 0.2 * aMetrics->emHeight;
        aSpaceGlyph = 0;
        aMetrics->spaceWidth = 0.5 * aMetrics->emHeight;
        aMetrics->maxAdvance = aMetrics->spaceWidth;
        aMetrics->aveCharWidth = aMetrics->spaceWidth;
        aMetrics->zeroOrAveCharWidth = aMetrics->spaceWidth;
        aMetrics->xHeight = 0.5 * aMetrics->emHeight;
        aMetrics->underlineSize = aMetrics->emHeight / 14.0;
        aMetrics->underlineOffset = -aMetrics->underlineSize;
        aMetrics->strikeoutOffset = 0.25 * aMetrics->emHeight;
        aMetrics->strikeoutSize = aMetrics->underlineSize;
        aMetrics->superscriptOffset = aMetrics->xHeight;
        aMetrics->subscriptOffset = aMetrics->xHeight;

        return;
    }

    const FT_Size_Metrics& ftMetrics = mFace->size->metrics;

    gfxFloat emHeight;
    
    gfxFloat yScale;
    if (FT_IS_SCALABLE(mFace)) {
        
        
        
        
        
        
        
        yScale = FLOAT_FROM_26_6(FLOAT_FROM_16_16(ftMetrics.y_scale));
        emHeight = mFace->units_per_EM * yScale;
    } else { 
        
        
        gfxFloat emUnit = mFace->units_per_EM;
        emHeight = ftMetrics.y_ppem;
        yScale = emHeight / emUnit;
    }

    TT_OS2 *os2 =
        static_cast<TT_OS2*>(FT_Get_Sfnt_Table(mFace, ft_sfnt_os2));

    aMetrics->maxAscent = FLOAT_FROM_26_6(ftMetrics.ascender);
    aMetrics->maxDescent = -FLOAT_FROM_26_6(ftMetrics.descender);
    aMetrics->maxAdvance = FLOAT_FROM_26_6(ftMetrics.max_advance);

    gfxFloat lineHeight;
    if (os2 && os2->sTypoAscender) {
        aMetrics->emAscent = os2->sTypoAscender * yScale;
        aMetrics->emDescent = -os2->sTypoDescender * yScale;
        FT_Short typoHeight =
            os2->sTypoAscender - os2->sTypoDescender + os2->sTypoLineGap;
        lineHeight = typoHeight * yScale;

        
        
        if (aMetrics->emAscent > aMetrics->maxAscent)
            aMetrics->maxAscent = aMetrics->emAscent;
        if (aMetrics->emDescent > aMetrics->maxDescent)
            aMetrics->maxDescent = aMetrics->emDescent;
    } else {
        aMetrics->emAscent = aMetrics->maxAscent;
        aMetrics->emDescent = aMetrics->maxDescent;
        lineHeight = FLOAT_FROM_26_6(ftMetrics.height);
    }

    cairo_text_extents_t extents;
    *aSpaceGlyph = GetCharExtents(' ', &extents);
    if (*aSpaceGlyph) {
        aMetrics->spaceWidth = extents.x_advance;
    } else {
        aMetrics->spaceWidth = aMetrics->maxAdvance; 
    }

    aMetrics->zeroOrAveCharWidth = 0.0;
    if (GetCharExtents('0', &extents)) {
        aMetrics->zeroOrAveCharWidth = extents.x_advance;
    }

    
    
    
    
    if (GetCharExtents('x', &extents) && extents.y_bearing < 0.0) {
        aMetrics->xHeight = -extents.y_bearing;
        aMetrics->aveCharWidth = extents.x_advance;
    } else {
        if (os2 && os2->sxHeight) {
            aMetrics->xHeight = os2->sxHeight * yScale;
        } else {
            
            
            
            aMetrics->xHeight = 0.5 * emHeight;
        }
        aMetrics->aveCharWidth = 0.0; 
    }
    
    
    if (os2 && os2->xAvgCharWidth) {
        
        
        gfxFloat avgCharWidth =
            ScaleRoundDesignUnits(os2->xAvgCharWidth, ftMetrics.x_scale);
        aMetrics->aveCharWidth =
            PR_MAX(aMetrics->aveCharWidth, avgCharWidth);
    }
    aMetrics->aveCharWidth =
        PR_MAX(aMetrics->aveCharWidth, aMetrics->zeroOrAveCharWidth);
    if (aMetrics->aveCharWidth == 0.0) {
        aMetrics->aveCharWidth = aMetrics->spaceWidth;
    }
    if (aMetrics->zeroOrAveCharWidth == 0.0) {
        aMetrics->zeroOrAveCharWidth = aMetrics->aveCharWidth;
    }
    
    aMetrics->maxAdvance =
        PR_MAX(aMetrics->maxAdvance, aMetrics->aveCharWidth);

    
    
    
    
    
    
    
    
    
    
    
    
    
    if (mFace->underline_position && mFace->underline_thickness) {
        aMetrics->underlineSize = mFace->underline_thickness * yScale;
        TT_Postscript *post = static_cast<TT_Postscript*>
            (FT_Get_Sfnt_Table(mFace, ft_sfnt_post));
        if (post && post->underlinePosition) {
            aMetrics->underlineOffset = post->underlinePosition * yScale;
        } else {
            aMetrics->underlineOffset = mFace->underline_position * yScale
                + 0.5 * aMetrics->underlineSize;
        }
    } else { 
        
        aMetrics->underlineSize = emHeight / 14.0;
        aMetrics->underlineOffset = -aMetrics->underlineSize;
    }

    if (os2 && os2->yStrikeoutSize && os2->yStrikeoutPosition) {
        aMetrics->strikeoutSize = os2->yStrikeoutSize * yScale;
        aMetrics->strikeoutOffset = os2->yStrikeoutPosition * yScale;
    } else { 
        aMetrics->strikeoutSize = aMetrics->underlineSize;
        
        aMetrics->strikeoutOffset = emHeight * 409.0 / 2048.0
            + 0.5 * aMetrics->strikeoutSize;
    }
    SnapLineToPixels(aMetrics->strikeoutOffset, aMetrics->strikeoutSize);

    if (os2 && os2->ySuperscriptYOffset) {
        gfxFloat val = ScaleRoundDesignUnits(os2->ySuperscriptYOffset,
                                             ftMetrics.y_scale);
        aMetrics->superscriptOffset = PR_MAX(1.0, val);
    } else {
        aMetrics->superscriptOffset = aMetrics->xHeight;
    }
    
    if (os2 && os2->ySubscriptYOffset) {
        gfxFloat val = ScaleRoundDesignUnits(os2->ySubscriptYOffset,
                                             ftMetrics.y_scale);
        
        val = fabs(val);
        aMetrics->subscriptOffset = PR_MAX(1.0, val);
    } else {
        aMetrics->subscriptOffset = aMetrics->xHeight;
    }

    aMetrics->maxHeight = aMetrics->maxAscent + aMetrics->maxDescent;

    
    
    
    
    
    
    aMetrics->emHeight = NS_floor(emHeight + 0.5);

    
    
    aMetrics->internalLeading =
        NS_floor(aMetrics->maxHeight - aMetrics->emHeight + 0.5);

    
    
    lineHeight = NS_floor(PR_MAX(lineHeight, aMetrics->maxHeight) + 0.5);
    aMetrics->externalLeading =
        lineHeight - aMetrics->internalLeading - aMetrics->emHeight;

    
    gfxFloat sum = aMetrics->emAscent + aMetrics->emDescent;
    aMetrics->emAscent = sum > 0.0 ?
        aMetrics->emAscent * aMetrics->emHeight / sum : 0.0;
    aMetrics->emDescent = aMetrics->emHeight - aMetrics->emAscent;
}

const gfxFont::Metrics&
gfxFcFont::GetMetrics()
{
    if (mHasMetrics)
        return mMetrics;

    if (NS_UNLIKELY(GetStyle()->size <= 0.0)) {
        new(&mMetrics) gfxFont::Metrics(); 
        mSpaceGlyph = 0;
    } else {
        LockedFTFace(this).GetMetrics(&mMetrics, &mSpaceGlyph);
    }

    SanitizeMetrics(&mMetrics, PR_FALSE);

#if 0
    
    

    fprintf (stderr, "Font: %s\n", NS_ConvertUTF16toUTF8(GetName()).get());
    fprintf (stderr, "    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics.emHeight, mMetrics.emAscent, mMetrics.emDescent);
    fprintf (stderr, "    maxAscent: %f maxDescent: %f\n", mMetrics.maxAscent, mMetrics.maxDescent);
    fprintf (stderr, "    internalLeading: %f externalLeading: %f\n", mMetrics.externalLeading, mMetrics.internalLeading);
    fprintf (stderr, "    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics.spaceWidth, mMetrics.aveCharWidth, mMetrics.xHeight);
    fprintf (stderr, "    uOff: %f uSize: %f stOff: %f stSize: %f suOff: %f suSize: %f\n", mMetrics.underlineOffset, mMetrics.underlineSize, mMetrics.strikeoutOffset, mMetrics.strikeoutSize, mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif

    mHasMetrics = PR_TRUE;
    return mMetrics;
}

nsString
gfxFcFont::GetUniqueName()
{
    return GetName();
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
    gfxTextRun *run = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!run)
        return nsnull;

    run->RecordSurrogates(aString);

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


static cairo_scaled_font_t *
CreateScaledFont(FcPattern *aPattern)
{
    cairo_font_face_t *face = cairo_ft_font_face_create_for_pattern(aPattern);
    double size;
    if (FcPatternGetDouble(aPattern,
                           FC_PIXEL_SIZE, 0, &size) != FcResultMatch) {
        NS_NOTREACHED("No size on pattern");
        size = 0.0;
    }
        
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

PRBool
gfxFcFont::SetupCairoFont(gfxContext *aContext)
{
    cairo_t *cr = aContext->GetCairo();

    
    
    
    
    
    
    cairo_scaled_font_t *cairoFont = CairoScaledFont();

    if (cairo_scaled_font_status(cairoFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    cairo_set_scaled_font(cr, cairoFont);
    return PR_TRUE;
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
    PangoFcFont *fcfont = PANGO_FC_FONT (pangofont);
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
            FT_UInt glyph = pango_fc_font_get_glyph (fcfont, ch);
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

    
    nsAutoString fcFamilies;
    GetFcFamilies(fcFamilies);
    PangoFontDescription *fontDesc =
        NewPangoFontDescription(fcFamilies, GetStyle());
    if (GetStyle()->sizeAdjust != 0.0) {
        gfxFloat size = GetAdjustedSize();
        pango_font_description_set_absolute_size
            (fontDesc, moz_pango_units_from_double(size));
    }

    pango_context_set_font_description(context, fontDesc);
    pango_font_description_free(fontDesc);

    PangoLanguage *lang = GetPangoLanguage(GetStyle()->langGroup);

    
    
    pango_context_set_language(context, lang);

    
    
    
    
    
    
    
    
    if (lang && !GetStyle()->systemFont) {
        SetBaseFont(context, GetBasePangoFont());
    }

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
GetPangoLanguage(const nsACString& aLangGroup)
{
    
    
    nsCAutoString lang;
    gfxFontconfigUtils::GetSampleLangForGroup(aLangGroup, &lang);

    if (lang.IsEmpty())
        return NULL;

    return pango_language_from_string(lang.get());
}

gfxPangoFontCache::gfxPangoFontCache()
{
    mPangoFonts.Init(500);
}

gfxPangoFontCache::~gfxPangoFontCache()
{
}

void
gfxPangoFontCache::Put(const PangoFontDescription *aFontDesc, PangoFont *aPangoFont)
{
    if (mPangoFonts.Count() > 5000)
        mPangoFonts.Clear();
    PRUint32 key = pango_font_description_hash(aFontDesc);
    gfxPangoFontWrapper *value = new gfxPangoFontWrapper(aPangoFont);
    if (!value)
        return;
    mPangoFonts.Put(key, value);
}

PangoFont*
gfxPangoFontCache::Get(const PangoFontDescription *aFontDesc)
{
    PRUint32 key = pango_font_description_hash(aFontDesc);
    gfxPangoFontWrapper *value;
    if (!mPangoFonts.Get(key, &value))
        return nsnull;
    PangoFont *font = value->Get();
    g_object_ref(font);
    return font;
}
