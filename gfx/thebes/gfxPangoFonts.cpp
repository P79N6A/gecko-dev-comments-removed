











































#define PANGO_ENABLE_BACKEND
#define PANGO_ENABLE_ENGINE

#include "prtypes.h"
#include "prlink.h"
#include "gfxTypes.h"

#include "nsTArray.h"

#include "gfxContext.h"
#ifdef MOZ_WIDGET_GTK2
#include "gfxPlatformGtk.h"
#endif
#ifdef MOZ_WIDGET_QT
#include "gfxQtPlatform.h"
#endif
#include "gfxPangoFonts.h"
#include "gfxFT2FontBase.h"
#include "gfxFT2Utils.h"
#include "harfbuzz/hb-unicode.h"
#include "harfbuzz/hb-ot-tag.h"
#include "gfxHarfBuzzShaper.h"
#include "gfxUnicodeProperties.h"
#include "gfxFontconfigUtils.h"
#include "gfxUserFontSet.h"
#include "gfxAtoms.h"

#include <cairo.h>
#include <cairo-ft.h>

#include <fontconfig/fcfreetype.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <pango/pango-modules.h>
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

#define PRINTING_FC_PROPERTY "gfx.printing"

class gfxPangoFcFont;


int moz_pango_units_from_double(double d) {
    return NS_lround(d * FLOAT_PANGO_SCALE);
}

static PangoLanguage *GuessPangoLanguage(nsIAtom *aLanguage);

static cairo_scaled_font_t *
CreateScaledFont(FcPattern *aPattern, cairo_font_face_t *aFace);
static void SetMissingGlyphs(gfxTextRun *aTextRun, const gchar *aUTF8,
                             PRUint32 aUTF8Length, PRUint32 *aUTF16Offset);

static PangoFontMap *gPangoFontMap;
static PangoFontMap *GetPangoFontMap();
static PRBool gUseFontMapProperty;

static FT_Library gFTLibrary;

template <class T>
class gfxGObjectRefTraits : public nsPointerRefTraits<T> {
public:
    static void Release(T *aPtr) { g_object_unref(aPtr); }
    static void AddRef(T *aPtr) { g_object_ref(aPtr); }
};

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








class gfxFcFontEntry : public gfxFontEntry {
public:
    
    
    
    
    
    
    const nsTArray< nsCountedRef<FcPattern> >& GetPatterns()
    {
        return mPatterns;
    }

    PRBool ShouldUseHarfBuzz(PRInt32 aRunScript);
    void SkipHarfBuzz() { mSkipHarfBuzz = PR_TRUE; }

    static gfxFcFontEntry *LookupFontEntry(cairo_font_face_t *aFace)
    {
        return static_cast<gfxFcFontEntry*>
            (cairo_font_face_get_user_data(aFace, &sFontEntryKey));
    }

    
    
    virtual nsString FamilyName() const;

protected:
    gfxFcFontEntry(const nsAString& aName)
        : gfxFontEntry(aName),
          mSkipHarfBuzz(PR_FALSE), mSkipGraphiteCheck(PR_FALSE)
    {
    }

    
    
    nsAutoTArray<nsCountedRef<FcPattern>,1> mPatterns;
    PRPackedBool mSkipHarfBuzz;
    PRPackedBool mSkipGraphiteCheck;

    static cairo_user_data_key_t sFontEntryKey;
};

cairo_user_data_key_t gfxFcFontEntry::sFontEntryKey;

nsString
gfxFcFontEntry::FamilyName() const
{
    if (mIsUserFont) {
        
        
        return gfxFontEntry::FamilyName();
    }
    FcChar8 *familyname;
    if (!mPatterns.IsEmpty() &&
        FcPatternGetString(mPatterns[0],
                           FC_FAMILY, 0, &familyname) == FcResultMatch) {
        return NS_ConvertUTF8toUTF16((const char*)familyname);
    }
    return gfxFontEntry::FamilyName();
}

PRBool
gfxFcFontEntry::ShouldUseHarfBuzz(PRInt32 aRunScript) {
    if (mSkipHarfBuzz ||
        !gfxPlatform::GetPlatform()->UseHarfBuzzForScript(aRunScript))
    {
        return PR_FALSE;
    }

    if (mSkipGraphiteCheck) {
        return PR_TRUE;
    }

    
    
    FcChar8 *capability;
    
    if (mPatterns.IsEmpty() ||
        FcPatternGetString(mPatterns[0],
                           FC_CAPABILITY, 0, &capability) == FcResultNoMatch ||
        !FcStrStr(capability, gfxFontconfigUtils::ToFcChar8("ttable:Silf")))
    {
        mSkipGraphiteCheck = PR_TRUE;
        return PR_TRUE;
    }

    
    hb_script_t script =
        aRunScript <= HB_SCRIPT_INHERITED ? HB_SCRIPT_LATIN
        : static_cast<hb_script_t>(aRunScript);

    
    
    const FcChar8 otCapTemplate[] = "otlayout:XXXX";
    FcChar8 otCap[NS_ARRAY_LENGTH(otCapTemplate)];
    memcpy(otCap, otCapTemplate, NS_ARRAY_LENGTH(otCapTemplate));
    
    const PRUint32 scriptOffset = NS_ARRAY_LENGTH(otCapTemplate) - 5;

    for (const hb_tag_t *scriptTags = hb_ot_tags_from_script(script);
         hb_tag_t scriptTag = *scriptTags;
         scriptTags++) {
        if (scriptTag == HB_TAG('D','F','L','T')) { 
            continue;
        }

        
        otCap[scriptOffset + 0] = scriptTag >> 24;
        otCap[scriptOffset + 1] = scriptTag >> 16;
        otCap[scriptOffset + 2] = scriptTag >> 8;
        otCap[scriptOffset + 3] = scriptTag;
        if (FcStrStr(capability, otCap)) {
            return PR_TRUE;
        }
    }

    return PR_FALSE; 
}











class gfxSystemFcFontEntry : public gfxFcFontEntry {
public:
    
    
    gfxSystemFcFontEntry(cairo_font_face_t *aFontFace,
                         FcPattern *aFontPattern,
                         const nsAString& aName)
        : gfxFcFontEntry(aName), mFontFace(aFontFace)
    {
        cairo_font_face_reference(mFontFace);
        cairo_font_face_set_user_data(mFontFace, &sFontEntryKey, this, NULL);
        mPatterns.AppendElement();
        
        
        mPatterns[0] = aFontPattern;
    }

    ~gfxSystemFcFontEntry()
    {
        cairo_font_face_set_user_data(mFontFace, &sFontEntryKey, NULL, NULL);
        cairo_font_face_destroy(mFontFace);
    }
private:
    cairo_font_face_t *mFontFace;
};





#define FONT_FACE_FAMILY_PREFIX "@font-face:"
































class gfxUserFcFontEntry : public gfxFcFontEntry {
protected:
    gfxUserFcFontEntry(const gfxProxyFontEntry &aProxyEntry)
        
        : gfxFcFontEntry(aProxyEntry.mFamily->Name())
    {
        mItalic = aProxyEntry.mItalic;
        mWeight = aProxyEntry.mWeight;
        mStretch = aProxyEntry.mStretch;
        mIsUserFont = PR_TRUE;
    }

    
    
    
    
    
    void AdjustPatternToCSS(FcPattern *aPattern);
};

void
gfxUserFcFontEntry::AdjustPatternToCSS(FcPattern *aPattern)
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












class gfxLocalFcFontEntry : public gfxUserFcFontEntry {
public:
    gfxLocalFcFontEntry(const gfxProxyFontEntry &aProxyEntry,
                        const nsTArray< nsCountedRef<FcPattern> >& aPatterns)
        : gfxUserFcFontEntry(aProxyEntry)
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
        mIsLocalUserFont = PR_TRUE;
    }
};










class gfxDownloadedFcFontEntry : public gfxUserFcFontEntry {
public:
    
    gfxDownloadedFcFontEntry(const gfxProxyFontEntry &aProxyEntry,
                             const PRUint8 *aData, FT_Face aFace)
        : gfxUserFcFontEntry(aProxyEntry), mFontData(aData), mFace(aFace)
    {
        NS_PRECONDITION(aFace != NULL, "aFace is NULL!");
        InitPattern();
    }

    virtual ~gfxDownloadedFcFontEntry();

    
    PRBool SetCairoFace(cairo_font_face_t *aFace);

    
    
    
    PangoCoverage *GetPangoCoverage();

protected:
    void InitPattern();

    
    
    
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

static void ReleaseDownloadedFontEntry(void *data)
{
    gfxDownloadedFcFontEntry *downloadedFontEntry =
        static_cast<gfxDownloadedFcFontEntry*>(data);
    NS_RELEASE(downloadedFontEntry);
}

PRBool gfxDownloadedFcFontEntry::SetCairoFace(cairo_font_face_t *aFace)
{
    if (CAIRO_STATUS_SUCCESS !=
        cairo_font_face_set_user_data(aFace, &sFontEntryKey, this,
                                      ReleaseDownloadedFontEntry))
        return PR_FALSE;

    
    NS_ADDREF(this);
    return PR_TRUE;
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
    virtual ~gfxFcFont();
    static already_AddRefed<gfxFcFont>
    GetOrMakeFont(FcPattern *aRequestedPattern, FcPattern *aFontPattern);

    
    PangoFont *GetPangoFont() {
        if (!mPangoFont) {
            MakePangoFont();
        }
        return mPangoFont;
    }

protected:
    virtual PRBool InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength,
                               PRInt32 aRunScript,
                               PRBool aPreferPlatformShaping);

    PRBool InitGlyphRunWithPango(gfxTextRun *aTextRun,
                                 const PRUnichar *aString,
                                 PRUint32 aRunStart, PRUint32 aRunLength,
                                 PangoScript aScript);

private:
    static already_AddRefed<gfxFcFont> GetOrMakeFont(FcPattern *aPattern);
    gfxFcFont(cairo_scaled_font_t *aCairoFont, gfxFcFontEntry *aFontEntry,
              const gfxFontStyle *aFontStyle);

    void MakePangoFont();

    PangoFont *mPangoFont;

    
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

    PangoCoverage *mCoverage;
    gfxFcFont *mGfxFont;

    
    
    
    
    
    
    
    static nsReturnRef<PangoFont>
    NewFont(gfxFcFont *aGfxFont, FcPattern *aFontPattern);

    gfxFcFont *GfxFont() { return mGfxFont; }

    cairo_scaled_font_t *CairoFont()
    {
        return GfxFont()->CairoScaledFont();
    }

private:
    void SetFontMap();
};

struct gfxPangoFcFontClass {
    PangoFcFontClass parent_class;
};

G_DEFINE_TYPE (gfxPangoFcFont, gfx_pango_fc_font, PANGO_TYPE_FC_FONT)

 nsReturnRef<PangoFont>
gfxPangoFcFont::NewFont(gfxFcFont *aGfxFont, FcPattern *aFontPattern)
{
    
    gfxPangoFcFont *font = static_cast<gfxPangoFcFont*>
        (g_object_new(GFX_TYPE_PANGO_FC_FONT, "pattern", aFontPattern, NULL));

    font->mGfxFont = aGfxFont;
    font->SetFontMap();

    PangoFcFont *fc_font = &font->parent_instance;
    cairo_scaled_font_t *scaled_font = aGfxFont->CairoScaledFont();
    
    
    
    
    
    cairo_font_options_t *options = cairo_font_options_create();
    cairo_scaled_font_get_font_options(scaled_font, options);
    cairo_hint_style_t hint_style = cairo_font_options_get_hint_style(options);
    cairo_font_options_destroy(options);
    fc_font->is_hinted = hint_style != CAIRO_HINT_STYLE_NONE;

    
    
    cairo_matrix_t matrix;
    cairo_scaled_font_get_font_matrix(scaled_font, &matrix);
    fc_font->is_transformed = (matrix.xy != 0.0 || matrix.yx != 0.0 ||
                               matrix.xx != matrix.yy);

    return nsReturnRef<PangoFont>(PANGO_FONT(font));
}

void
gfxPangoFcFont::SetFontMap()
{
    
    
    
    PangoFontMap *fontmap = GetPangoFontMap();
    
    
    
    PangoFcFont *fc_font = &parent_instance;
    if (gUseFontMapProperty) {
        g_object_set(this, "fontmap", fontmap, NULL);
    } else {
        
        
        
        
        
        fc_font->fontmap = fontmap;
        g_object_ref(fc_font->fontmap);
    }
}

static void
gfx_pango_fc_font_init(gfxPangoFcFont *font)
{
}

static void
gfx_pango_fc_font_finalize(GObject *object)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(object);

    if (self->mCoverage)
        pango_coverage_unref(self->mCoverage);

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

static PRInt32
GetDPI()
{
#if defined(MOZ_WIDGET_GTK2)
    return gfxPlatformGtk::GetDPI();
#elif defined(MOZ_WIDGET_QT)
    return gfxQtPlatform::GetDPI();
#else
    return 96;
#endif
}

static PangoFontDescription *
gfx_pango_fc_font_describe(PangoFont *font)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);
    PangoFcFont *fcFont = &self->parent_instance;
    PangoFontDescription *result =
        pango_font_description_copy(fcFont->description);

    gfxFcFont *gfxFont = self->GfxFont();
    if (gfxFont) {
        double pixelsize = gfxFont->GetStyle()->size;
        double dpi = GetDPI();
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

    gfxFcFont *gfxFont = self->GfxFont();
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
    gfxFcFont *gfxFont = self->GfxFont();

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

    gfxFcFont *gfxFont = self->GfxFont();
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
    return cairo_ft_scaled_font_lock_face(self->CairoFont());
}

static void
gfx_pango_fc_font_unlock_face(PangoFcFont *font)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);
    cairo_ft_scaled_font_unlock_face(self->CairoFont());
}

static guint
gfx_pango_fc_font_get_glyph(PangoFcFont *font, gunichar wc)
{
    gfxPangoFcFont *self = GFX_PANGO_FC_FONT(font);
    gfxFcFont *gfxFont = self->GfxFont();
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








class gfxFcFontSet {
public:
    NS_INLINE_DECL_REFCOUNTING(gfxFcFontSet)
    
    explicit gfxFcFontSet(FcPattern *aPattern,
                               gfxUserFontSet *aUserFontSet)
        : mSortPattern(aPattern), mUserFontSet(aUserFontSet),
          mFcFontsTrimmed(0),
          mHaveFallbackFonts(PR_FALSE)
    {
        PRBool waitForUserFont;
        mFcFontSet = SortPreferredFonts(waitForUserFont);
        mWaitingForUserFont = waitForUserFont;
    }

    
    
    gfxFcFont *GetFontAt(PRUint32 i)
    {
        if (i >= mFonts.Length() || !mFonts[i].mFont) { 
            
            FcPattern *fontPattern = GetFontPatternAt(i);
            if (!fontPattern)
                return NULL;

            mFonts[i].mFont =
                gfxFcFont::GetOrMakeFont(mSortPattern, fontPattern);
        }
        return mFonts[i].mFont;
    }

    FcPattern *GetFontPatternAt(PRUint32 i);

    PRBool WaitingForUserFont() const {
        return mWaitingForUserFont;
    }

private:
    nsReturnRef<FcFontSet> SortPreferredFonts(PRBool& aWaitForUserFont);
    nsReturnRef<FcFontSet> SortFallbackFonts();

    struct FontEntry {
        explicit FontEntry(FcPattern *aPattern) : mPattern(aPattern) {}
        nsCountedRef<FcPattern> mPattern;
        nsRefPtr<gfxFcFont> mFont;
        nsCountedRef<PangoFont> mPangoFont;
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
    
    
    PRPackedBool mWaitingForUserFont;
};



static const nsTArray< nsCountedRef<FcPattern> >*
FindFontPatterns(gfxUserFontSet *mUserFontSet,
                const nsACString &aFamily, PRUint8 aStyle, PRUint16 aWeight,
                PRBool& aFoundFamily, PRBool& aWaitForUserFont)
{
    
    NS_ConvertUTF8toUTF16 utf16Family(aFamily);

    
    
    
    PRBool needsBold;

    gfxFontStyle style;
    style.style = aStyle;
    style.weight = aWeight;

    gfxUserFcFontEntry *fontEntry = static_cast<gfxUserFcFontEntry*>
        (mUserFontSet->FindFontEntry(utf16Family, style, aFoundFamily,
                                     needsBold, aWaitForUserFont));

    
    if (!fontEntry && aStyle != FONT_STYLE_NORMAL) {
        style.style = FONT_STYLE_NORMAL;
        fontEntry = static_cast<gfxUserFcFontEntry*>
            (mUserFontSet->FindFontEntry(utf16Family, style, aFoundFamily,
                                         needsBold, aWaitForUserFont));
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
gfxFcFontSet::SortPreferredFonts(PRBool &aWaitForUserFont)
{
    aWaitForUserFont = PR_FALSE;

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

                PRBool foundFamily, waitForUserFont;
                familyFonts = FindFontPatterns(mUserFontSet, cssFamily,
                                               thebesStyle, thebesWeight,
                                               foundFamily, waitForUserFont);
                if (waitForUserFont) {
                    aWaitForUserFont = PR_TRUE;
                }
                NS_ASSERTION(foundFamily,
                             "expected to find a user font, but it's missing!");
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
gfxFcFontSet::SortFallbackFonts()
{
    
    
    
    
    
    
    
    
    FcResult result;
    return nsReturnRef<FcFontSet>(FcFontSort(NULL, mSortPattern,
                                             FcFalse, NULL, &result));
}


FcPattern *
gfxFcFontSet::GetFontPatternAt(PRUint32 i)
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

static PRBool HasChar(FcPattern *aFont, FcChar32 wc)
{
    FcCharSet *charset = NULL;
    FcPatternGetCharSet(aFont, FC_CHARSET, 0, &charset);

    return charset && FcCharSetHasChar(charset, wc);
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

static PangoFcFont *
gfx_pango_font_map_new_font(PangoFcFontMap *fontmap,
                            FcPattern *pattern)
{
    
    
    
    
    
    
    
    return NULL;
}

static void
gfx_pango_font_map_class_init(gfxPangoFontMapClass *klass)
{
    

    
    
    
    
    
    
    

    PangoFcFontMapClass *fcfontmap_class = PANGO_FC_FONT_MAP_CLASS (klass);
    
    
    
    fcfontmap_class->new_font = gfx_pango_font_map_new_font;
    
    
    
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
       FcPatternAddBool(aPattern, PRINTING_FC_PROPERTY, FcTrue);
    } else {
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
       cairo_font_options_t *options = cairo_font_options_create();
       cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
       cairo_ft_font_options_substitute(options, aPattern);
       cairo_font_options_destroy(options);
#endif
#ifdef MOZ_WIDGET_GTK2
       ApplyGdkScreenFontOptions(aPattern);
#endif
    }

    
    
    double size = aFallbackSize;
    if (FcPatternGetDouble(aPattern, FC_PIXEL_SIZE, 0, &size) != FcResultMatch
        || aSizeAdjustFactor != 1.0) {
        FcPatternDel(aPattern, FC_PIXEL_SIZE);
        FcPatternAddDouble(aPattern, FC_PIXEL_SIZE, size * aSizeAdjustFactor);
    }

    FcDefaultSubstitute(aPattern);
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
        } else {
            list->AppendElement(fontName);
        }
    }

    return PR_TRUE;
}

gfxPangoFontGroup::gfxPangoFontGroup (const nsAString& families,
                                      const gfxFontStyle *aStyle,
                                      gfxUserFontSet *aUserFontSet)
    : gfxFontGroup(families, aStyle, aUserFontSet),
      mPangoLanguage(GuessPangoLanguage(aStyle->language))
{
    
    
    if (mPangoLanguage) {
        mStyle.language = do_GetAtom(pango_language_to_string(mPangoLanguage));
    }

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
                                 nsIAtom *aLanguage)
{
    FamilyCallbackData data(aFcFamilyList, mUserFontSet);
    
    
    ForEachFontInternal(mFamilies, aLanguage, PR_TRUE, PR_FALSE,
                        FamilyCallback, &data);
}

gfxFcFont *
gfxPangoFontGroup::GetBaseFont()
{
    if (!mFonts[0]) {
        mFonts[0] = GetBaseFontSet()->GetFontAt(0);
    }

    return static_cast<gfxFcFont*>(mFonts[0].get());
}

gfxFont *
gfxPangoFontGroup::GetFontAt(PRInt32 i) {
    
    
    
    
    NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                 "Whoever was caching this font group should have "
                 "called UpdateFontList on it");

    NS_PRECONDITION(i == 0, "Only have one font");

    return GetBaseFont();
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
    mUnderlineOffset = UNDERLINE_OFFSET_NOT_SET;
    mCurrGeneration = newGeneration;
    mSkipDrawing = PR_FALSE;
}

already_AddRefed<gfxFcFontSet>
gfxPangoFontGroup::MakeFontSet(PangoLanguage *aLang, gfxFloat aSizeAdjustFactor,
                               nsAutoRef<FcPattern> *aMatchPattern)
{
    const char *lang = pango_language_to_string(aLang);

    nsRefPtr <nsIAtom> langGroup;
    if (aLang != mPangoLanguage) {
        
        langGroup = do_GetAtom(lang);
    }

    nsAutoTArray<nsString, 20> fcFamilyList;
    GetFcFamilies(&fcFamilyList,
                  langGroup ? langGroup.get() : mStyle.language.get());

    

    
    nsAutoRef<FcPattern> pattern
        (gfxFontconfigUtils::NewPattern(fcFamilyList, mStyle, lang));

    PrepareSortPattern(pattern, mStyle.size, aSizeAdjustFactor, mStyle.printerFont);

    nsRefPtr<gfxFcFontSet> fontset =
        new gfxFcFontSet(pattern, mUserFontSet);

    mSkipDrawing = fontset->WaitingForUserFont();

    if (aMatchPattern)
        aMatchPattern->steal(pattern);

    return fontset.forget();
}

gfxPangoFontGroup::
FontSetByLangEntry::FontSetByLangEntry(PangoLanguage *aLang,
                                       gfxFcFontSet *aFontSet)
    : mLang(aLang), mFontSet(aFontSet)
{
}

gfxFcFontSet *
gfxPangoFontGroup::GetFontSet(PangoLanguage *aLang)
{
    GetBaseFontSet(); 

    if (!aLang)
        return mFontSets[0].mFontSet;

    for (PRUint32 i = 0; i < mFontSets.Length(); ++i) {
        if (mFontSets[i].mLang == aLang)
            return mFontSets[i].mFontSet;
    }

    nsRefPtr<gfxFcFontSet> fontSet =
        MakeFontSet(aLang, mSizeAdjustFactor);
    mFontSets.AppendElement(FontSetByLangEntry(aLang, fontSet));

    return fontSet;
}

already_AddRefed<gfxFont>
gfxPangoFontGroup::FindFontForChar(PRUint32 aCh, PRUint32 aPrevCh,
                                   PRInt32 aRunScript,
                                   gfxFont *aPrevMatchedFont,
                                   PRUint8 *aMatchType)
{
    if (aPrevMatchedFont) {
        PRUint8 category = gfxUnicodeProperties::GetGeneralCategory(aCh);
        
        
        
        
        if ((category == HB_CATEGORY_CONTROL ||
             category == HB_CATEGORY_FORMAT  ||
             gfxFontUtils::IsVarSelector(aCh))) {
            return nsRefPtr<gfxFont>(aPrevMatchedFont).forget();
        }

        
        
        
        
        
        if (aCh == ' ' ||
            (gfxFontUtils::IsJoinCauser(aPrevCh) &&
             static_cast<gfxFcFont*>(aPrevMatchedFont)->GetGlyph(aCh))) {
            return nsRefPtr<gfxFont>(aPrevMatchedFont).forget();
        }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    gfxFcFontSet *fontSet = GetBaseFontSet();
    PRUint32 nextFont = 0;
    FcPattern *basePattern = NULL;
    if (!mStyle.systemFont && mPangoLanguage) {
        basePattern = fontSet->GetFontPatternAt(0);
        if (HasChar(basePattern, aCh)) {
            *aMatchType = gfxTextRange::kFontGroup;
            return nsRefPtr<gfxFont>(GetBaseFont()).forget();
        }

        nextFont = 1;
    }

    
    const PangoScript script = static_cast<PangoScript>(aRunScript);
    
    
    PangoLanguage *scriptLang;
    if ((!basePattern ||
         !pango_language_includes_script(mPangoLanguage, script)) &&
        (scriptLang = pango_script_get_sample_language(script))) {
        fontSet = GetFontSet(scriptLang);
        nextFont = 0;
    }

    for (PRUint32 i = nextFont;
         FcPattern *pattern = fontSet->GetFontPatternAt(i);
         ++i) {
        if (pattern == basePattern) {
            continue; 
        }

        if (HasChar(pattern, aCh)) {
            *aMatchType = gfxTextRange::kFontGroup;
            return nsRefPtr<gfxFont>(fontSet->GetFontAt(i)).forget();
        }
    }

    return nsnull;
}





cairo_user_data_key_t gfxFcFont::sGfxFontKey;

gfxFcFont::gfxFcFont(cairo_scaled_font_t *aCairoFont,
                     gfxFcFontEntry *aFontEntry,
                     const gfxFontStyle *aFontStyle)
    : gfxFT2FontBase(aCairoFont, aFontEntry, aFontStyle),
      mPangoFont()
{
    cairo_scaled_font_set_user_data(mScaledFont, &sGfxFontKey, this, NULL);
}





static void
PangoFontToggleNotify(gpointer data, GObject* object, gboolean is_last_ref)
{
    gfxFcFont *font = static_cast<gfxFcFont*>(data);
    if (is_last_ref) { 
        NS_RELEASE(font);
    } else {
        NS_ADDREF(font);
    }
}

void
gfxFcFont::MakePangoFont()
{
    
    gfxFcFontEntry *fe = static_cast<gfxFcFontEntry*>(mFontEntry.get());
    nsAutoRef<PangoFont> pangoFont
        (gfxPangoFcFont::NewFont(this, fe->GetPatterns()[0]));
    mPangoFont = pangoFont;
    g_object_add_toggle_ref(G_OBJECT(mPangoFont), PangoFontToggleNotify, this);
    
    
    NS_ADDREF(this);
}

gfxFcFont::~gfxFcFont()
{
    cairo_scaled_font_set_user_data(mScaledFont, &sGfxFontKey, NULL, NULL);
    if (mPangoFont) {
        g_object_remove_toggle_ref(G_OBJECT(mPangoFont),
                                   PangoFontToggleNotify, this);
    }
}

PRBool
gfxFcFont::InitTextRun(gfxContext *aContext,
                       gfxTextRun *aTextRun,
                       const PRUnichar *aString,
                       PRUint32 aRunStart,
                       PRUint32 aRunLength,
                       PRInt32 aRunScript,
                       PRBool aPreferPlatformShaping)
{
    gfxFcFontEntry *fontEntry = static_cast<gfxFcFontEntry*>(GetFontEntry());

    if (fontEntry->ShouldUseHarfBuzz(aRunScript)) {
        if (!mHarfBuzzShaper) {
            gfxFT2LockedFace face(this);
            mHarfBuzzShaper = new gfxHarfBuzzShaper(this);
            
            mFUnitsConvFactor = face.XScale();
        }
        if (mHarfBuzzShaper->
            InitTextRun(aContext, aTextRun, aString,
                        aRunStart, aRunLength, aRunScript)) {
            return PR_TRUE;
        }

        
        fontEntry->SkipHarfBuzz();
        mHarfBuzzShaper = nsnull;
    }

    const PangoScript script = static_cast<PangoScript>(aRunScript);
    PRBool ok = InitGlyphRunWithPango(aTextRun,
                                      aString, aRunStart, aRunLength, script);

    NS_WARN_IF_FALSE(ok, "shaper failed, expect scrambled or missing text");
    return ok;
}

 void
gfxPangoFontGroup::Shutdown()
{
    if (gPangoFontMap) {
        g_object_unref(gPangoFontMap);
        gPangoFontMap = NULL;
    }

    
    
    gFTLibrary = NULL;
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

 FT_Library
gfxPangoFontGroup::GetFTLibrary()
{
    if (!gFTLibrary) {
        
        
        
        
        
        
        
        
        gfxFontStyle style;
        nsRefPtr<gfxPangoFontGroup> fontGroup =
            new gfxPangoFontGroup(NS_LITERAL_STRING("sans-serif"),
                                  &style, nsnull);

        gfxFcFont *font = fontGroup->GetBaseFont();
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
gfxFcFont::GetOrMakeFont(FcPattern *aRequestedPattern, FcPattern *aFontPattern)
{
    nsAutoRef<FcPattern> renderPattern
        (FcFontRenderPrepare(NULL, aRequestedPattern, aFontPattern));
    cairo_font_face_t *face =
        cairo_ft_font_face_create_for_pattern(renderPattern);

    
    nsRefPtr<gfxFcFontEntry> fe = gfxFcFontEntry::LookupFontEntry(face);
    if (!fe) {
        gfxDownloadedFcFontEntry *downloadedFontEntry =
            GetDownloadedFontEntry(aFontPattern);
        if (downloadedFontEntry) {
            
            fe = downloadedFontEntry;
            if (cairo_font_face_status(face) == CAIRO_STATUS_SUCCESS) {
                
                
                
                if (!downloadedFontEntry->SetCairoFace(face)) {
                    
                    cairo_font_face_destroy(face);
                    face = cairo_ft_font_face_create_for_pattern(aRequestedPattern);
                    fe = gfxFcFontEntry::LookupFontEntry(face);
                }
            }
        }
        if (!fe) {
            
            nsAutoString name;
            FcChar8 *fc_file;
            if (FcPatternGetString(renderPattern,
                                   FC_FILE, 0, &fc_file) == FcResultMatch) {
                int index;
                if (FcPatternGetInteger(renderPattern,
                                        FC_INDEX, 0, &index) != FcResultMatch) {
                    
                    index = 0;
                }

                AppendUTF8toUTF16(gfxFontconfigUtils::ToCString(fc_file), name);
                if (index != 0) {
                    name.AppendLiteral("/");
                    name.AppendInt(index);
                }
            }

            fe = new gfxSystemFcFontEntry(face, aFontPattern, name);
        }
    }

    cairo_scaled_font_t *cairoFont = CreateScaledFont(renderPattern, face);

    nsRefPtr<gfxFcFont> font = static_cast<gfxFcFont*>
        (cairo_scaled_font_get_user_data(cairoFont, &sGfxFontKey));

    if (!font) {
        gfxFloat size = GetPixelSize(renderPattern);

        
        
        PRUint8 style = gfxFontconfigUtils::GetThebesStyle(renderPattern);
        PRUint16 weight = gfxFontconfigUtils::GetThebesWeight(renderPattern);

        
        
        
        nsIAtom *language = gfxAtoms::en; 
        
        gfxFontStyle fontStyle(style, weight, NS_FONT_STRETCH_NORMAL,
                               size, language, 0.0,
                               PR_TRUE, PR_FALSE,
                               NS_LITERAL_STRING(""),
                               NS_LITERAL_STRING(""));

        
        
        
        
        
        
        
        font = new gfxFcFont(cairoFont, fe, &fontStyle);
    }

    cairo_scaled_font_destroy(cairoFont);
    cairo_font_face_destroy(face);
    return font.forget();
}

static PangoFontMap *
GetPangoFontMap()
{
    if (!gPangoFontMap) {
        
        
        gPangoFontMap = pango_cairo_font_map_get_default();

        if (PANGO_IS_FC_FONT_MAP(gPangoFontMap)) {
            g_object_ref(gPangoFontMap);
        } else {
            
            
            
            
            gPangoFontMap = gfxPangoFontMap::NewFontMap();
        }
    }
    return gPangoFontMap;
}

gfxFcFontSet *
gfxPangoFontGroup::GetBaseFontSet()
{
    if (mFontSets.Length() > 0)
        return mFontSets[0].mFontSet;

    mSizeAdjustFactor = 1.0; 
    nsAutoRef<FcPattern> pattern;
    nsRefPtr<gfxFcFontSet> fontSet =
        MakeFontSet(mPangoLanguage, mSizeAdjustFactor, &pattern);

    double size = GetPixelSize(pattern);
    if (size != 0.0 && mStyle.sizeAdjust != 0.0) {
        gfxFcFont *font = fontSet->GetFontAt(0);
        if (font) {
            const gfxFont::Metrics& metrics = font->GetMetrics();

            
            
            
            if (metrics.xHeight > 0.1 * metrics.emHeight) {
                mSizeAdjustFactor =
                    mStyle.sizeAdjust * metrics.emHeight / metrics.xHeight;

                size *= mSizeAdjustFactor;
                FcPatternDel(pattern, FC_PIXEL_SIZE);
                FcPatternAddDouble(pattern, FC_PIXEL_SIZE, size);

                fontSet = new gfxFcFontSet(pattern, mUserFontSet);
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












static cairo_scaled_font_t *
CreateScaledFont(FcPattern *aPattern, cairo_font_face_t *aFace)
{
    double size = GetPixelSize(aPattern);
        
    cairo_matrix_t fontMatrix;
    FcMatrix *fcMatrix;
    if (FcPatternGetMatrix(aPattern, FC_MATRIX, 0, &fcMatrix) == FcResultMatch)
        cairo_matrix_init(&fontMatrix, fcMatrix->xx, -fcMatrix->yx, -fcMatrix->xy, fcMatrix->yy, 0, 0);
    else
        cairo_matrix_init_identity(&fontMatrix);
    cairo_matrix_scale(&fontMatrix, size, size);

    FcBool printing;
    if (FcPatternGetBool(aPattern, PRINTING_FC_PROPERTY, 0, &printing) != FcResultMatch) {
        printing = FcFalse;
    }

    
    
    
    cairo_matrix_t identityMatrix;
    cairo_matrix_init_identity(&identityMatrix);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    cairo_font_options_t *fontOptions = cairo_font_options_create();

    
    
    
    
    
    
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    cairo_font_options_set_hint_metrics(fontOptions, CAIRO_HINT_METRICS_OFF);
#else
    if (printing) {
        cairo_font_options_set_hint_metrics(fontOptions, CAIRO_HINT_METRICS_OFF);
    } else {
        cairo_font_options_set_hint_metrics(fontOptions, CAIRO_HINT_METRICS_ON);
    }
#endif

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    FcBool hinting = FcFalse;
#ifndef MOZ_GFX_OPTIMIZE_MOBILE
    if (FcPatternGetBool(aPattern, FC_HINTING, 0, &hinting) != FcResultMatch) {
        hinting = FcTrue;
    }
#endif
    cairo_hint_style_t hint_style;
    if (printing || !hinting) {
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
        cairo_scaled_font_create(aFace, &fontMatrix, &identityMatrix,
                                 fontOptions);

    cairo_font_options_destroy(fontOptions);

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

static nsresult
SetGlyphs(gfxTextRun *aTextRun, const gchar *aUTF8, PRUint32 aUTF8Length,
          PRUint32 *aUTF16Offset, PangoGlyphString *aGlyphs,
          PangoGlyphUnit aOverrideSpaceWidth)
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

        nsresult rv;
        if (haveMissingGlyph) {
            SetMissingGlyphs(aTextRun, clusterUTF8, clusterUTF8Length,
                             &utf16Offset);
        } else {
            rv = SetGlyphsForCharacterGroup(&glyphs[glyphClusterStart],
                                            glyphIndex - glyphClusterStart,
                                            aTextRun,
                                            clusterUTF8, clusterUTF8Length,
                                            &utf16Offset, aOverrideSpaceWidth);
            NS_ENSURE_SUCCESS(rv,rv);
        }
    }
    *aUTF16Offset = utf16Offset;
    return NS_OK;
}

static void
SetMissingGlyphs(gfxTextRun *aTextRun, const gchar *aUTF8,
                 PRUint32 aUTF8Length, PRUint32 *aUTF16Offset)
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
}

static void
InitGlyphRunWithPangoAnalysis(gfxTextRun *aTextRun,
                              const gchar *aUTF8, PRUint32 aUTF8Length,
                              PRUint32 *aUTF16Offset,
                              PangoAnalysis *aAnalysis,
                              PangoGlyphUnit aOverrideSpaceWidth)
{
    PRUint32 utf16Offset = *aUTF16Offset;
    PangoGlyphString *glyphString = pango_glyph_string_new();

    const gchar *p = aUTF8;
    const gchar *end = p + aUTF8Length;
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

        pango_shape(text, len, aAnalysis, glyphString);
        SetupClusterBoundaries(aTextRun, text, len, utf16Offset, aAnalysis);
        SetGlyphs(aTextRun, text, len, &utf16Offset, glyphString,
                  aOverrideSpaceWidth);
    }

    pango_glyph_string_free(glyphString);
    *aUTF16Offset = utf16Offset;
}




typedef union {
    PangoAnalysis pango;
    
    
    struct {
        PangoEngineShape *shape_engine;
        PangoEngineLang  *lang_engine;
        PangoFont *font;
        guint8 level;
        guint8 gravity; 
        guint8 flags;
        guint8 script; 
        PangoLanguage *language;
        GSList *extra_attrs;
    } local;
} PangoAnalysisUnion;

PRBool
gfxFcFont::InitGlyphRunWithPango(gfxTextRun *aTextRun,
                                 const PRUnichar *aString,
                                 PRUint32 aRunStart, PRUint32 aRunLength,
                                 PangoScript aScript)
{
    NS_ConvertUTF16toUTF8 utf8(aString + aRunStart, aRunLength);

    PangoFont *font = GetPangoFont();
    gfxPangoFontGroup *fontGroup =
        static_cast<gfxPangoFontGroup*>(aTextRun->GetFontGroup());

    hb_language_t languageOverride = NULL;
    if (fontGroup->GetStyle()->languageOverride) {
        languageOverride =
            hb_ot_tag_to_language(fontGroup->GetStyle()->languageOverride);
    } else if (GetFontEntry()->mLanguageOverride) {
        languageOverride =
            hb_ot_tag_to_language(GetFontEntry()->mLanguageOverride);
    }

    PangoLanguage *language;
    if (languageOverride) {
        language =
            pango_language_from_string(hb_language_to_string(languageOverride));
    } else {
        language = fontGroup->GetPangoLanguage();
        
        
        
        
        
        
        const PangoScript script = static_cast<PangoScript>(aScript);
        PangoLanguage *scriptLang;
        if ((!language ||
             !pango_language_includes_script(language, script)) &&
            (scriptLang = pango_script_get_sample_language(script))) {
            language = scriptLang;
        }
    }

    static GQuark engineLangId =
        g_quark_from_static_string(PANGO_ENGINE_TYPE_LANG);
    static GQuark renderNoneId =
        g_quark_from_static_string(PANGO_RENDER_TYPE_NONE);
    PangoMap *langMap = pango_find_map(language, engineLangId, renderNoneId);

    static GQuark engineShapeId =
        g_quark_from_static_string(PANGO_ENGINE_TYPE_SHAPE);
    static GQuark renderFcId =
        g_quark_from_static_string(PANGO_RENDER_TYPE_FC);
    PangoMap *shapeMap = pango_find_map(language, engineShapeId, renderFcId);
    if (!shapeMap) {
        return PR_FALSE;
    }

    
    PangoEngineShape *shapeEngine =
        PANGO_ENGINE_SHAPE(pango_map_get_engine(shapeMap, aScript));
    if (!shapeEngine) {
        return PR_FALSE;
    }

    PangoEngineShapeClass *shapeClass = static_cast<PangoEngineShapeClass*>
        (g_type_class_peek(PANGO_TYPE_ENGINE_SHAPE));

    
    
    
    
    
    
    
    
    if (!shapeClass ||
        PANGO_ENGINE_SHAPE_GET_CLASS(shapeEngine)->covers != shapeClass->covers)
    {
        GSList *exact_engines;
        GSList *fallback_engines;
        pango_map_get_engines(shapeMap, aScript,
                              &exact_engines, &fallback_engines);

        GSList *engines = g_slist_concat(exact_engines, fallback_engines);
        for (GSList *link = engines; link; link = link->next) {
            PangoEngineShape *engine = PANGO_ENGINE_SHAPE(link->data);
            PangoCoverageLevel (*covers)(PangoEngineShape*, PangoFont*,
                                         PangoLanguage*, gunichar) =
                PANGO_ENGINE_SHAPE_GET_CLASS(shapeEngine)->covers;

            if ((shapeClass && covers == shapeClass->covers) ||
                covers(engine, font, language, ' ') != PANGO_COVERAGE_NONE)
            {
                shapeEngine = engine;
                break;
            }
        }
        g_slist_free(engines); 
    }

    PangoAnalysisUnion analysis;
    memset(&analysis, 0, sizeof(analysis));

    
    analysis.local.shape_engine = shapeEngine;
    
    analysis.local.lang_engine =
        PANGO_ENGINE_LANG(pango_map_get_engine(langMap, aScript));

    analysis.local.font = font;
    analysis.local.level = aTextRun->IsRightToLeft() ? 1 : 0;
    
    
    
    
    
    
    
    
#if 0
    analysis.local.gravity = PANGO_GRAVITY_SOUTH;
    analysis.local.flags = 0;
#endif
    
    analysis.local.script = aScript;

    analysis.local.language = language;
    
    analysis.local.extra_attrs = NULL;

    PangoGlyphUnit spaceWidth =
        moz_pango_units_from_double(GetMetrics().spaceWidth);

    PRUint32 utf16Offset = aRunStart;
    InitGlyphRunWithPangoAnalysis(aTextRun, utf8.get(), utf8.Length(),
                                  &utf16Offset, &analysis.pango, spaceWidth);
    return PR_TRUE;
}


PangoLanguage *
GuessPangoLanguage(nsIAtom *aLanguage)
{
    if (!aLanguage)
        return NULL;

    
    
    nsCAutoString lang;
    gfxFontconfigUtils::GetSampleLangForGroup(aLanguage, &lang);
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
