




#include "mozilla/Logging.h"

#include "gfxFcPlatformFontList.h"
#include "gfxFont.h"
#include "gfxFontConstants.h"
#include "gfxFontFamilyList.h"
#include "gfxFT2Utils.h"
#include "gfxPlatform.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/TimeStamp.h"
#include "nsGkAtoms.h"
#include "nsILanguageAtomService.h"
#include "nsUnicodeProperties.h"
#include "nsUnicodeRange.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"

#include <fontconfig/fcfreetype.h>

#ifdef MOZ_WIDGET_GTK
#include <gdk/gdk.h>
#endif

using namespace mozilla;
using namespace mozilla::unicode;

#ifndef FC_POSTSCRIPT_NAME
#define FC_POSTSCRIPT_NAME  "postscriptname"      /* String */
#endif

#define PRINTING_FC_PROPERTY "gfx.printing"

#define LOG_FONTLIST(args) MOZ_LOG(gfxPlatform::GetLog(eGfxLog_fontlist), \
                               LogLevel::Debug, args)
#define LOG_FONTLIST_ENABLED() MOZ_LOG_TEST( \
                                   gfxPlatform::GetLog(eGfxLog_fontlist), \
                                   LogLevel::Debug)
#define LOG_CMAPDATA_ENABLED() MOZ_LOG_TEST( \
                                   gfxPlatform::GetLog(eGfxLog_cmapdata), \
                                   LogLevel::Debug)

static const FcChar8*
ToFcChar8Ptr(const char* aStr)
{
    return reinterpret_cast<const FcChar8*>(aStr);
}

static const char*
ToCharPtr(const FcChar8 *aStr)
{
    return reinterpret_cast<const char*>(aStr);
}

FT_Library gfxFcPlatformFontList::sCairoFTLibrary = nullptr;

static cairo_user_data_key_t sFcFontlistUserFontDataKey;



static uint32_t
FindCanonicalNameIndex(FcPattern* aFont, const char* aLangField)
{
    uint32_t n = 0, en = 0;
    FcChar8* lang;
    while (FcPatternGetString(aFont, aLangField, n, &lang) == FcResultMatch) {
        
        uint32_t len = strlen(ToCharPtr(lang));
        bool enPrefix = (strncmp(ToCharPtr(lang), "en", 2) == 0);
        if (enPrefix && (len == 2 || (len > 2 && aLangField[2] == '-'))) {
            en = n;
            break;
        }
        n++;
    }
    return en;
}

static void
GetFaceNames(FcPattern* aFont, const nsAString& aFamilyName,
             nsAString& aPostscriptName, nsAString& aFullname)
{
    
    FcChar8* psname;
    if (FcPatternGetString(aFont, FC_POSTSCRIPT_NAME, 0, &psname) == FcResultMatch) {
        AppendUTF8toUTF16(ToCharPtr(psname), aPostscriptName);
    }

    
    uint32_t en = FindCanonicalNameIndex(aFont, FC_FULLNAMELANG);
    FcChar8* fullname;
    if (FcPatternGetString(aFont, FC_FULLNAME, en, &fullname) == FcResultMatch) {
        AppendUTF8toUTF16(ToCharPtr(fullname), aFullname);
    }

    
    if (!aFullname.IsEmpty()) {
        return;
    }

    
    aFullname.Append(aFamilyName);

    
    en = FindCanonicalNameIndex(aFont, FC_STYLELANG);
    nsAutoString style;
    FcChar8* stylename = nullptr;
    FcPatternGetString(aFont, FC_STYLE, en, &stylename);
    if (stylename) {
        AppendUTF8toUTF16(ToCharPtr(stylename), style);
    }

    if (!style.IsEmpty() && !style.EqualsLiteral("Regular")) {
        aFullname.Append(' ');
        aFullname.Append(style);
    }
}

static uint16_t
MapFcWeight(int aFcWeight)
{
    if (aFcWeight <= (FC_WEIGHT_THIN + FC_WEIGHT_EXTRALIGHT) / 2) {
        return 100;
    } else if (aFcWeight <= (FC_WEIGHT_EXTRALIGHT + FC_WEIGHT_LIGHT) / 2) {
        return 200;
    } else if (aFcWeight <= (FC_WEIGHT_LIGHT + FC_WEIGHT_BOOK) / 2) {
        return 300;
    } else if (aFcWeight <= (FC_WEIGHT_REGULAR + FC_WEIGHT_MEDIUM) / 2) {
        
        return 400;
    } else if (aFcWeight <= (FC_WEIGHT_MEDIUM + FC_WEIGHT_DEMIBOLD) / 2) {
        return 500;
    } else if (aFcWeight <= (FC_WEIGHT_DEMIBOLD + FC_WEIGHT_BOLD) / 2) {
        return 600;
    } else if (aFcWeight <= (FC_WEIGHT_BOLD + FC_WEIGHT_EXTRABOLD) / 2) {
        return 700;
    } else if (aFcWeight <= (FC_WEIGHT_EXTRABOLD + FC_WEIGHT_BLACK) / 2) {
        return 800;
    } else if (aFcWeight <= FC_WEIGHT_BLACK) {
        return 900;
    }

    
    return 901;
}

static int16_t
MapFcWidth(int aFcWidth)
{
    if (aFcWidth <= (FC_WIDTH_ULTRACONDENSED + FC_WIDTH_EXTRACONDENSED) / 2) {
        return NS_FONT_STRETCH_ULTRA_CONDENSED;
    }
    if (aFcWidth <= (FC_WIDTH_EXTRACONDENSED + FC_WIDTH_CONDENSED) / 2) {
        return NS_FONT_STRETCH_EXTRA_CONDENSED;
    }
    if (aFcWidth <= (FC_WIDTH_CONDENSED + FC_WIDTH_SEMICONDENSED) / 2) {
        return NS_FONT_STRETCH_CONDENSED;
    }
    if (aFcWidth <= (FC_WIDTH_SEMICONDENSED + FC_WIDTH_NORMAL) / 2) {
        return NS_FONT_STRETCH_SEMI_CONDENSED;
    }
    if (aFcWidth <= (FC_WIDTH_NORMAL + FC_WIDTH_SEMIEXPANDED) / 2) {
        return NS_FONT_STRETCH_NORMAL;
    }
    if (aFcWidth <= (FC_WIDTH_SEMIEXPANDED + FC_WIDTH_EXPANDED) / 2) {
        return NS_FONT_STRETCH_SEMI_EXPANDED;
    }
    if (aFcWidth <= (FC_WIDTH_EXPANDED + FC_WIDTH_EXTRAEXPANDED) / 2) {
        return NS_FONT_STRETCH_EXPANDED;
    }
    if (aFcWidth <= (FC_WIDTH_EXTRAEXPANDED + FC_WIDTH_ULTRAEXPANDED) / 2) {
        return NS_FONT_STRETCH_EXTRA_EXPANDED;
    }
    return NS_FONT_STRETCH_ULTRA_EXPANDED;
}


struct MozLangGroupData {
    nsIAtom* const& mozLangGroup;
    const char *defaultLang;
};

const MozLangGroupData MozLangGroups[] = {
    { nsGkAtoms::x_western,      "en" },
    { nsGkAtoms::x_cyrillic,     "ru" },
    { nsGkAtoms::x_devanagari,   "hi" },
    { nsGkAtoms::x_tamil,        "ta" },
    { nsGkAtoms::x_armn,         "hy" },
    { nsGkAtoms::x_beng,         "bn" },
    { nsGkAtoms::x_cans,         "iu" },
    { nsGkAtoms::x_ethi,         "am" },
    { nsGkAtoms::x_geor,         "ka" },
    { nsGkAtoms::x_gujr,         "gu" },
    { nsGkAtoms::x_guru,         "pa" },
    { nsGkAtoms::x_khmr,         "km" },
    { nsGkAtoms::x_knda,         "kn" },
    { nsGkAtoms::x_mlym,         "ml" },
    { nsGkAtoms::x_orya,         "or" },
    { nsGkAtoms::x_sinh,         "si" },
    { nsGkAtoms::x_tamil,        "ta" },
    { nsGkAtoms::x_telu,         "te" },
    { nsGkAtoms::x_tibt,         "bo" },
    { nsGkAtoms::Unicode,        0    }
};

static void
GetSampleLangForGroup(nsIAtom* aLanguage, nsACString& aLangStr)
{
    aLangStr.Truncate();
    if (aLanguage) {
        
        const MozLangGroupData *mozLangGroup = nullptr;

        
        for (unsigned int i = 0; i < ArrayLength(MozLangGroups); ++i) {
            if (aLanguage == MozLangGroups[i].mozLangGroup) {
                mozLangGroup = &MozLangGroups[i];
                break;
            }
        }

        
        
        
        
        

        
        if (mozLangGroup) {
            if (mozLangGroup->defaultLang) {
                aLangStr.Assign(mozLangGroup->defaultLang);
            }
        } else {
            
            
            aLanguage->ToUTF8String(aLangStr);
        }
    }
}

gfxFontconfigFontEntry::gfxFontconfigFontEntry(const nsAString& aFaceName,
                                               FcPattern* aFontPattern)
        : gfxFontEntry(aFaceName), mFontPattern(aFontPattern),
          mFTFace(nullptr), mFTFaceInitialized(false),
          mAspect(0.0), mFontData(nullptr)
{
    
    int slant;
    if (FcPatternGetInteger(aFontPattern, FC_SLANT, 0, &slant) != FcResultMatch) {
        slant = FC_SLANT_ROMAN;
    }
    if (slant > 0) {
        mItalic = true;
    }

    
    int weight;
    if (FcPatternGetInteger(aFontPattern, FC_WEIGHT, 0, &weight) != FcResultMatch) {
        weight = FC_WEIGHT_REGULAR;
    }
    mWeight = MapFcWeight(weight);

    
    int width;
    if (FcPatternGetInteger(aFontPattern, FC_WIDTH, 0, &width) != FcResultMatch) {
        width = FC_WIDTH_NORMAL;
    }
    mStretch = MapFcWidth(width);
}

gfxFontconfigFontEntry::gfxFontconfigFontEntry(const nsAString& aFaceName,
                                               uint16_t aWeight,
                                               int16_t aStretch,
                                               bool aItalic,
                                               const uint8_t *aData,
                                               FT_Face aFace)
    : gfxFontEntry(aFaceName),
      mFTFace(aFace), mFTFaceInitialized(true),
      mAspect(0.0), mFontData(aData)
{
    mWeight = aWeight;
    mItalic = aItalic;
    mStretch = aStretch;
    mIsDataUserFont = true;

    
    
    
    
    
    
    
    
    
    
    mFontPattern = FcFreeTypeQueryFace(mFTFace, ToFcChar8Ptr(""), 0, nullptr);
    
    if (!mFontPattern) {
        mFontPattern = FcPatternCreate();
    }
    FcPatternDel(mFontPattern, FC_FILE);
    FcPatternDel(mFontPattern, FC_INDEX);

    
    
    FcPatternAddFTFace(mFontPattern, FC_FT_FACE, mFTFace);

    mUserFontData = new FTUserFontData(mFTFace, mFontData);
}

gfxFontconfigFontEntry::gfxFontconfigFontEntry(const nsAString& aFaceName,
                                               FcPattern* aFontPattern,
                                               uint16_t aWeight,
                                               int16_t aStretch,
                                               bool aItalic)
        : gfxFontEntry(aFaceName), mFontPattern(aFontPattern),
          mFTFace(nullptr), mFTFaceInitialized(false),
          mAspect(0.0), mFontData(nullptr)
{
    mWeight = aWeight;
    mItalic = aItalic;
    mStretch = aStretch;
    mIsLocalUserFont = true;
}

gfxFontconfigFontEntry::~gfxFontconfigFontEntry()
{
}

bool
gfxFontconfigFontEntry::SupportsLangGroup(nsIAtom *aLangGroup) const
{
    if (!aLangGroup || aLangGroup == nsGkAtoms::Unicode) {
        return true;
    }

    nsAutoCString fcLang;
    GetSampleLangForGroup(aLangGroup, fcLang);
    if (fcLang.IsEmpty()) {
        return true;
    }

    
    FcLangSet *langset;
    if (FcPatternGetLangSet(mFontPattern, FC_LANG, 0, &langset) != FcResultMatch) {
        return false;
    }

    if (FcLangSetHasLang(langset, (FcChar8 *)fcLang.get()) != FcLangDifferentLang) {
        return true;
    }

    return false;
}

nsresult
gfxFontconfigFontEntry::ReadCMAP(FontInfoData *aFontInfoData)
{
    
    if (mCharacterMap) {
        return NS_OK;
    }

    nsRefPtr<gfxCharacterMap> charmap;
    nsresult rv;
    bool symbolFont;

    if (aFontInfoData && (charmap = GetCMAPFromFontInfo(aFontInfoData,
                                                        mUVSOffset,
                                                        symbolFont))) {
        rv = NS_OK;
    } else {
        uint32_t kCMAP = TRUETYPE_TAG('c','m','a','p');
        charmap = new gfxCharacterMap();
        AutoTable cmapTable(this, kCMAP);

        if (cmapTable) {
            bool unicodeFont = false, symbolFont = false; 
            uint32_t cmapLen;
            const uint8_t* cmapData =
                reinterpret_cast<const uint8_t*>(hb_blob_get_data(cmapTable,
                                                                  &cmapLen));
            rv = gfxFontUtils::ReadCMAP(cmapData, cmapLen,
                                        *charmap, mUVSOffset,
                                        unicodeFont, symbolFont);
        } else {
            rv = NS_ERROR_NOT_AVAILABLE;
        }
    }

    mHasCmapTable = NS_SUCCEEDED(rv);
    if (mHasCmapTable) {
        gfxPlatformFontList *pfl = gfxPlatformFontList::PlatformFontList();
        mCharacterMap = pfl->FindCharMap(charmap);
    } else {
        
        mCharacterMap = new gfxCharacterMap();
    }

    LOG_FONTLIST(("(fontlist-cmap) name: %s, size: %d hash: %8.8x%s\n",
                  NS_ConvertUTF16toUTF8(mName).get(),
                  charmap->SizeOfIncludingThis(moz_malloc_size_of),
                  charmap->mHash, mCharacterMap == charmap ? " new" : ""));
    if (LOG_CMAPDATA_ENABLED()) {
        char prefix[256];
        sprintf(prefix, "(cmapdata) name: %.220s",
                NS_ConvertUTF16toUTF8(mName).get());
        charmap->Dump(prefix, eGfxLog_cmapdata);
    }

    return rv;
}

static bool
HasChar(FcPattern *aFont, FcChar32 aCh)
{
    FcCharSet *charset = nullptr;
    FcPatternGetCharSet(aFont, FC_CHARSET, 0, &charset);
    return charset && FcCharSetHasChar(charset, aCh);
}

bool
gfxFontconfigFontEntry::TestCharacterMap(uint32_t aCh)
{
    
    if (!mIsDataUserFont) {
        return HasChar(mFontPattern, aCh);
    }
    return gfxFontEntry::TestCharacterMap(aCh);
}

hb_blob_t*
gfxFontconfigFontEntry::GetFontTable(uint32_t aTableTag)
{
    
    if (mFontData) {
        return GetTableFromFontData(mFontData, aTableTag);
    }

    return gfxFontEntry::GetFontTable(aTableTag);
}

void
gfxFontconfigFontEntry::MaybeReleaseFTFace()
{
    
    if (mHBFace || mGrFace) {
        return;
    }
    
    if (!mIsDataUserFont) {
        if (mFTFace) {
            FT_Done_Face(mFTFace);
            mFTFace = nullptr;
        }
        mFTFaceInitialized = false;
    }
}

void
gfxFontconfigFontEntry::ForgetHBFace()
{
    gfxFontEntry::ForgetHBFace();
    MaybeReleaseFTFace();
}

void
gfxFontconfigFontEntry::ReleaseGrFace(gr_face* aFace)
{
    gfxFontEntry::ReleaseGrFace(aFace);
    MaybeReleaseFTFace();
}

double
gfxFontconfigFontEntry::GetAspect()
{
    if (mAspect == 0.0) {
        
        mAspect = 0.5;

        
        gfxFontStyle s;
        s.size = 100.0; 
        nsRefPtr<gfxFont> font = FindOrMakeFont(&s, false);
        if (font) {
            const gfxFont::Metrics& metrics =
                font->GetMetrics(gfxFont::eHorizontal);

            
            
            
            if (metrics.xHeight > 0.1 * metrics.emHeight) {
                mAspect = metrics.xHeight / metrics.emHeight;
            }
        }
    }
    return mAspect;
}

static void
PrepareFontOptions(FcPattern* aPattern,
                   cairo_font_options_t* aFontOptions)
{
    NS_ASSERTION(aFontOptions, "null font options passed to PrepareFontOptions");

    

    FcBool printing;
    if (FcPatternGetBool(aPattern, PRINTING_FC_PROPERTY, 0, &printing) !=
            FcResultMatch) {
        printing = FcFalse;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (printing) {
        cairo_font_options_set_hint_metrics(aFontOptions, CAIRO_HINT_METRICS_OFF);
    } else {
        cairo_font_options_set_hint_metrics(aFontOptions, CAIRO_HINT_METRICS_ON);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    FcBool hinting = FcFalse;
    if (FcPatternGetBool(aPattern, FC_HINTING, 0, &hinting) != FcResultMatch) {
        hinting = FcTrue;
    }

    cairo_hint_style_t hint_style;
    if (printing || !hinting) {
        hint_style = CAIRO_HINT_STYLE_NONE;
    } else {
        int fc_hintstyle;
        if (FcPatternGetInteger(aPattern, FC_HINT_STYLE,
                                0, &fc_hintstyle) != FcResultMatch) {
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
    }
    cairo_font_options_set_hint_style(aFontOptions, hint_style);

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
    cairo_font_options_set_subpixel_order(aFontOptions, subpixel_order);

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
    cairo_font_options_set_antialias(aFontOptions, antialias);
}

cairo_scaled_font_t*
gfxFontconfigFontEntry::CreateScaledFont(FcPattern* aRenderPattern,
                                         const gfxFontStyle *aStyle,
                                         bool aNeedsBold)
{
    if (aNeedsBold) {
        FcPatternAddBool(aRenderPattern, FC_EMBOLDEN, FcTrue);
    }

    
    bool needsOblique = !IsItalic() &&
            (aStyle->style & (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE)) &&
            aStyle->allowSyntheticStyle;

    if (needsOblique) {
        
        FcPatternDel(aRenderPattern, FC_EMBEDDED_BITMAP);
        FcPatternAddBool(aRenderPattern, FC_EMBEDDED_BITMAP, FcFalse);
    }

    cairo_font_face_t *face =
        cairo_ft_font_face_create_for_pattern(aRenderPattern);

    if (mFontData) {
        
        
        NS_ASSERTION(mFTFace, "FT_Face is null when setting user data");
        NS_ASSERTION(mUserFontData, "user font data is null when setting user data");
        cairo_font_face_set_user_data(face,
                                      &sFcFontlistUserFontDataKey,
                                      new FTUserFontDataRef(mUserFontData),
                                      FTUserFontDataRef::Destroy);
    }

    cairo_scaled_font_t *scaledFont = nullptr;

    cairo_matrix_t sizeMatrix;
    cairo_matrix_t identityMatrix;

    double adjustedSize = aStyle->size;
    if (aStyle->sizeAdjust >= 0.0) {
        adjustedSize = aStyle->GetAdjustedSize(GetAspect());
    }
    cairo_matrix_init_scale(&sizeMatrix, adjustedSize, adjustedSize);
    cairo_matrix_init_identity(&identityMatrix);

    if (needsOblique) {
        const double kSkewFactor = OBLIQUE_SKEW_FACTOR;

        cairo_matrix_t style;
        cairo_matrix_init(&style,
                          1,                
                          0,                
                          -1 * kSkewFactor,  
                          1,                
                          0,                
                          0);               
        cairo_matrix_multiply(&sizeMatrix, &sizeMatrix, &style);
    }

    cairo_font_options_t *fontOptions = cairo_font_options_create();
    PrepareFontOptions(aRenderPattern, fontOptions);

    scaledFont = cairo_scaled_font_create(face, &sizeMatrix,
                                          &identityMatrix, fontOptions);
    cairo_font_options_destroy(fontOptions);

    NS_ASSERTION(cairo_scaled_font_status(scaledFont) == CAIRO_STATUS_SUCCESS,
                 "Failed to make scaled font");

    cairo_font_face_destroy(face);

    return scaledFont;
}

#ifdef MOZ_WIDGET_GTK

static void ApplyGdkScreenFontOptions(FcPattern *aPattern);
#endif

static void
PreparePattern(FcPattern* aPattern, bool aIsPrinterFont)
{
    FcConfigSubstitute(nullptr, aPattern, FcMatchPattern);

    
    
    
    
    
    
    
    
    
    
    
    
    if(aIsPrinterFont) {
       cairo_font_options_t *options = cairo_font_options_create();
       cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
       cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_GRAY);
       cairo_ft_font_options_substitute(options, aPattern);
       cairo_font_options_destroy(options);
       FcPatternAddBool(aPattern, PRINTING_FC_PROPERTY, FcTrue);
    } else {
#ifdef MOZ_WIDGET_GTK
       ApplyGdkScreenFontOptions(aPattern);
#endif
    }

    FcDefaultSubstitute(aPattern);
}

gfxFont*
gfxFontconfigFontEntry::CreateFontInstance(const gfxFontStyle *aFontStyle,
                                           bool aNeedsBold)
{
    nsAutoRef<FcPattern> pattern(FcPatternCreate());
    FcPatternAddDouble(pattern, FC_PIXEL_SIZE, aFontStyle->size);

    PreparePattern(pattern, aFontStyle->printerFont);
    nsAutoRef<FcPattern> renderPattern
        (FcFontRenderPrepare(nullptr, pattern, mFontPattern));

    cairo_scaled_font_t* scaledFont =
        CreateScaledFont(renderPattern, aFontStyle, aNeedsBold);
    gfxFont* newFont =
        new gfxFontconfigFont(scaledFont, this, aFontStyle, aNeedsBold);
    cairo_scaled_font_destroy(scaledFont);

    return newFont;
}

nsresult
gfxFontconfigFontEntry::CopyFontTable(uint32_t aTableTag,
                                      FallibleTArray<uint8_t>& aBuffer)
{
    NS_ASSERTION(!mIsDataUserFont,
                 "data fonts should be reading tables directly from memory");

    if (!mFTFaceInitialized) {
        mFTFaceInitialized = true;
        FcChar8 *filename;
        if (FcPatternGetString(mFontPattern, FC_FILE, 0, &filename) != FcResultMatch) {
            return NS_ERROR_FAILURE;
        }
        int index;
        if (FcPatternGetInteger(mFontPattern, FC_INDEX, 0, &index) != FcResultMatch) {
            index = 0; 
        }
        if (FT_New_Face(gfxFcPlatformFontList::GetFTLibrary(),
                        (const char*)filename, index, &mFTFace) != 0) {
            return NS_ERROR_FAILURE;
        }
    }

    if (!mFTFace) {
        return NS_ERROR_NOT_AVAILABLE;
    }

    FT_ULong length = 0;
    if (FT_Load_Sfnt_Table(mFTFace, aTableTag, 0, nullptr, &length) != 0) {
        return NS_ERROR_NOT_AVAILABLE;
    }
    if (!aBuffer.SetLength(length, fallible)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    if (FT_Load_Sfnt_Table(mFTFace, aTableTag, 0, aBuffer.Elements(), &length) != 0) {
        aBuffer.Clear();
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

void
gfxFontconfigFontFamily::FindStyleVariations(FontInfoData *aFontInfoData)
{
    if (mHasStyles) {
        return;
    }

    
    uint32_t numFonts = mFontPatterns.Length();
    NS_ASSERTION(numFonts, "font family containing no faces!!");
    for (uint32_t i = 0; i < numFonts; i++) {
        FcPattern* face = mFontPatterns[i];

        
        nsAutoString psname, fullname;
        GetFaceNames(face, mName, psname, fullname);
        const nsAutoString& faceName = !psname.IsEmpty() ? psname : fullname;

        gfxFontconfigFontEntry *fontEntry =
            new gfxFontconfigFontEntry(faceName, face);
        AddFontEntry(fontEntry);

        if (LOG_FONTLIST_ENABLED()) {
            LOG_FONTLIST(("(fontlist) added (%s) to family (%s)"
                 " with style: %s weight: %d stretch: %d"
                 " psname: %s fullname: %s",
                 NS_ConvertUTF16toUTF8(fontEntry->Name()).get(),
                 NS_ConvertUTF16toUTF8(Name()).get(),
                 fontEntry->IsItalic() ? "italic" : "normal",
                 fontEntry->Weight(), fontEntry->Stretch(),
                 NS_ConvertUTF16toUTF8(psname).get(),
                 NS_ConvertUTF16toUTF8(fullname).get()));
        }
    }
    mFaceNamesInitialized = true;
    mFontPatterns.Clear();
    SetHasStyles(true);
}

void
gfxFontconfigFontFamily::AddFontPattern(FcPattern* aFontPattern)
{
    NS_ASSERTION(!mHasStyles,
                 "font patterns must not be added to already enumerated families");

    nsCountedRef<FcPattern> pattern(aFontPattern);
    mFontPatterns.AppendElement(pattern);
}

gfxFontconfigFont::gfxFontconfigFont(cairo_scaled_font_t *aScaledFont,
                                     gfxFontEntry *aFontEntry,
                                     const gfxFontStyle *aFontStyle,
                                     bool aNeedsBold) :
    gfxFT2FontBase(aScaledFont, aFontEntry, aFontStyle)
{
}

gfxFontconfigFont::~gfxFontconfigFont()
{
}

#ifdef USE_SKIA
already_AddRefed<mozilla::gfx::GlyphRenderingOptions>
gfxFontconfigFont::GetGlyphRenderingOptions(const TextRunDrawParams* aRunParams)
{
  cairo_scaled_font_t *scaled_font = CairoScaledFont();
  cairo_font_options_t *options = cairo_font_options_create();
  cairo_scaled_font_get_font_options(scaled_font, options);
  cairo_hint_style_t hint_style = cairo_font_options_get_hint_style(options);
  cairo_font_options_destroy(options);

  mozilla::gfx::FontHinting hinting;

  switch (hint_style) {
    case CAIRO_HINT_STYLE_NONE:
      hinting = mozilla::gfx::FontHinting::NONE;
      break;
    case CAIRO_HINT_STYLE_SLIGHT:
      hinting = mozilla::gfx::FontHinting::LIGHT;
      break;
    case CAIRO_HINT_STYLE_FULL:
      hinting = mozilla::gfx::FontHinting::FULL;
      break;
    default:
      hinting = mozilla::gfx::FontHinting::NORMAL;
      break;
  }

  
  return mozilla::gfx::Factory::CreateCairoGlyphRenderingOptions(hinting, false);
}
#endif

gfxFcPlatformFontList::gfxFcPlatformFontList()
    : mLocalNames(64)
    , mGenericMappings(32)
    , mFcSubstituteCache(64)
    , mLastConfig(nullptr)
{
    
    int rescanInterval = FcConfigGetRescanInterval(nullptr);
    if (rescanInterval) {
        mLastConfig = FcConfigGetCurrent();
        mCheckFontUpdatesTimer = do_CreateInstance("@mozilla.org/timer;1");
        if (mCheckFontUpdatesTimer) {
            mCheckFontUpdatesTimer->
                InitWithFuncCallback(CheckFontUpdates, this,
                                     (rescanInterval + 1) * 1000,
                                     nsITimer::TYPE_REPEATING_SLACK);
        } else {
            NS_WARNING("Failure to create font updates timer");
        }
    }

#ifdef MOZ_BUNDLED_FONTS
    mBundledFontsInitialized = false;
#endif
}

gfxFcPlatformFontList::~gfxFcPlatformFontList()
{
    if (mCheckFontUpdatesTimer) {
        mCheckFontUpdatesTimer->Cancel();
        mCheckFontUpdatesTimer = nullptr;
    }
}

void
gfxFcPlatformFontList::AddFontSetFamilies(FcFontSet* aFontSet)
{
    
    
    
    
    
    

    if (!aFontSet) {
        NS_WARNING("AddFontSetFamilies called with a null font set.");
        return;
    }

    FcChar8* lastFamilyName = (FcChar8*)"";
    gfxFontFamily* fontFamily = nullptr;
    nsAutoString familyName;
    for (int f = 0; f < aFontSet->nfont; f++) {
        FcPattern* font = aFontSet->fonts[f];

        
        FcBool scalable;
        if (FcPatternGetBool(font, FC_SCALABLE, 0, &scalable) != FcResultMatch ||
            !scalable) {
            continue;
        }

        
        uint32_t cIndex = FindCanonicalNameIndex(font, FC_FAMILYLANG);
        FcChar8* canonical = nullptr;
        FcPatternGetString(font, FC_FAMILY, cIndex, &canonical);
        if (!canonical) {
            continue;
        }

        
        if (FcStrCmp(canonical, lastFamilyName) != 0) {
            lastFamilyName = canonical;

            
            familyName.Truncate();
            AppendUTF8toUTF16(ToCharPtr(canonical), familyName);
            nsAutoString keyName(familyName);
            ToLowerCase(keyName);

            fontFamily = mFontFamilies.GetWeak(keyName);
            if (!fontFamily) {
                fontFamily = new gfxFontconfigFontFamily(familyName);
                mFontFamilies.Put(keyName, fontFamily);
            }

            
            
            
            FcChar8* otherName;
            int n = (cIndex == 0 ? 1 : 0);
            while (FcPatternGetString(font, FC_FAMILY, n, &otherName) == FcResultMatch) {
                NS_ConvertUTF8toUTF16 otherFamilyName(ToCharPtr(otherName));
                AddOtherFamilyName(fontFamily, otherFamilyName);
                n++;
                if (n == int(cIndex)) {
                    n++; 
                }
            }
        }

        NS_ASSERTION(fontFamily, "font must belong to a font family");
        gfxFontconfigFontFamily* fcFamily =
            static_cast<gfxFontconfigFontFamily*>(fontFamily);
        fcFamily->AddFontPattern(font);

        
        nsAutoString psname, fullname;
        GetFaceNames(font, familyName, psname, fullname);
        if (!psname.IsEmpty()) {
            ToLowerCase(psname);
            mLocalNames.Put(psname, font);
        }
        if (!fullname.IsEmpty()) {
            ToLowerCase(fullname);
            mLocalNames.Put(fullname, font);
        }
    }
}

nsresult
gfxFcPlatformFontList::InitFontList()
{
    mLastConfig = FcConfigGetCurrent();

    
    gfxPlatformFontList::InitFontList();

    mLocalNames.Clear();
    mGenericMappings.Clear();
    mFcSubstituteCache.Clear();

    
    FcFontSet* systemFonts = FcConfigGetFonts(nullptr, FcSetSystem);
    AddFontSetFamilies(systemFonts);

#ifdef MOZ_BUNDLED_FONTS
    ActivateBundledFonts();
    FcFontSet* appFonts = FcConfigGetFonts(nullptr, FcSetApplication);
    AddFontSetFamilies(appFonts);
#endif

    mOtherFamilyNamesInitialized = true;

    return NS_OK;
}




static void
GetSystemFontList(nsTArray<nsString>& aListOfFonts, nsIAtom *aLangGroup)
{
    aListOfFonts.Clear();

    nsAutoRef<FcPattern> pat(FcPatternCreate());
    if (!pat) {
        return;
    }

    nsAutoRef<FcObjectSet> os(FcObjectSetBuild(FC_FAMILY, nullptr));
    if (!os) {
        return;
    }

    
    nsAutoCString fcLang;
    GetSampleLangForGroup(aLangGroup, fcLang);
    if (!fcLang.IsEmpty()) {
        FcPatternAddString(pat, FC_LANG, ToFcChar8Ptr(fcLang.get()));
    }

    
    FcPatternAddBool(pat, FC_SCALABLE, FcTrue);

    nsAutoRef<FcFontSet> fs(FcFontList(nullptr, pat, os));
    if (!fs) {
        return;
    }

    for (int i = 0; i < fs->nfont; i++) {
        char *family;

        if (FcPatternGetString(fs->fonts[i], FC_FAMILY, 0,
                               (FcChar8 **) &family) != FcResultMatch)
        {
            continue;
        }

        
        nsAutoString strFamily;
        AppendUTF8toUTF16(family, strFamily);
        if (aListOfFonts.Contains(strFamily)) {
            continue;
        }

        aListOfFonts.AppendElement(strFamily);
    }

    aListOfFonts.Sort();
}

void
gfxFcPlatformFontList::GetFontList(nsIAtom *aLangGroup,
                                   const nsACString& aGenericFamily,
                                   nsTArray<nsString>& aListOfFonts)
{
    
    GetSystemFontList(aListOfFonts, aLangGroup);

    
    
    
    
    bool serif = false, sansSerif = false, monospace = false;
    if (aGenericFamily.IsEmpty())
        serif = sansSerif = monospace = true;
    else if (aGenericFamily.LowerCaseEqualsLiteral("serif"))
        serif = true;
    else if (aGenericFamily.LowerCaseEqualsLiteral("sans-serif"))
        sansSerif = true;
    else if (aGenericFamily.LowerCaseEqualsLiteral("monospace"))
        monospace = true;
    else if (aGenericFamily.LowerCaseEqualsLiteral("cursive") ||
             aGenericFamily.LowerCaseEqualsLiteral("fantasy"))
        serif = sansSerif = true;
    else
        NS_NOTREACHED("unexpected CSS generic font family");

    
    
    
    if (monospace)
        aListOfFonts.InsertElementAt(0, NS_LITERAL_STRING("monospace"));
    if (sansSerif)
        aListOfFonts.InsertElementAt(0, NS_LITERAL_STRING("sans-serif"));
    if (serif)
        aListOfFonts.InsertElementAt(0, NS_LITERAL_STRING("serif"));
}

gfxFontFamily*
gfxFcPlatformFontList::GetDefaultFont(const gfxFontStyle* aStyle)
{
    
    
    return FindGenericFamily(NS_LITERAL_STRING("-moz-default"),
                             aStyle->language);
}

gfxFontEntry*
gfxFcPlatformFontList::LookupLocalFont(const nsAString& aFontName,
                                       uint16_t aWeight,
                                       int16_t aStretch,
                                       bool aItalic)
{
    nsAutoString keyName(aFontName);
    ToLowerCase(keyName);

    
    FcPattern* fontPattern = mLocalNames.Get(keyName);
    if (!fontPattern) {
        return nullptr;
    }

    return new gfxFontconfigFontEntry(aFontName,
                                      fontPattern,
                                      aWeight, aStretch, aItalic);
}

gfxFontEntry*
gfxFcPlatformFontList::MakePlatformFont(const nsAString& aFontName,
                                        uint16_t aWeight,
                                        int16_t aStretch,
                                        bool aItalic,
                                        const uint8_t* aFontData,
                                        uint32_t aLength)
{
    FT_Face face;
    FT_Error error =
        FT_New_Memory_Face(gfxFcPlatformFontList::GetFTLibrary(),
                           aFontData, aLength, 0, &face);
    if (error != FT_Err_Ok) {
        NS_Free((void*)aFontData);
        return nullptr;
    }
    if (FT_Err_Ok != FT_Select_Charmap(face, FT_ENCODING_UNICODE)) {
        FT_Done_Face(face);
        NS_Free((void*)aFontData);
        return nullptr;
    }

    return new gfxFontconfigFontEntry(aFontName, aWeight, aStretch, aItalic,
                                      aFontData, face);
}

gfxFontFamily*
gfxFcPlatformFontList::FindFamily(const nsAString& aFamily,
                                  nsIAtom* aLanguage,
                                  bool aUseSystemFonts)
{
    nsAutoString familyName(aFamily);
    ToLowerCase(familyName);

    
    bool isDeprecatedGeneric = false;
    if (familyName.EqualsLiteral("sans") ||
        familyName.EqualsLiteral("sans serif")) {
        familyName.AssignLiteral("sans-serif");
        isDeprecatedGeneric = true;
    } else if (familyName.EqualsLiteral("mono")) {
        familyName.AssignLiteral("monospace");
        isDeprecatedGeneric = true;
    }

    
    if (isDeprecatedGeneric ||
        mozilla::FontFamilyName::Convert(familyName).IsGeneric()) {
        return FindGenericFamily(familyName, aLanguage);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    NS_ConvertUTF16toUTF8 familyToFind(familyName);
    gfxFontFamily* cached = mFcSubstituteCache.GetWeak(familyToFind);
    if (cached) {
        return cached;
    }

    const FcChar8* kSentinelName = ToFcChar8Ptr("-moz-sentinel");
    FcChar8* sentinelFirstFamily = nullptr;
    nsAutoRef<FcPattern> sentinelSubst(FcPatternCreate());
    FcPatternAddString(sentinelSubst, FC_FAMILY, kSentinelName);
    FcConfigSubstitute(nullptr, sentinelSubst, FcMatchPattern);
    FcPatternGetString(sentinelSubst, FC_FAMILY, 0, &sentinelFirstFamily);

    
    nsAutoRef<FcPattern> fontWithSentinel(FcPatternCreate());
    FcPatternAddString(fontWithSentinel, FC_FAMILY,
                       ToFcChar8Ptr(familyToFind.get()));
    FcPatternAddString(fontWithSentinel, FC_FAMILY, kSentinelName);
    FcConfigSubstitute(nullptr, fontWithSentinel, FcMatchPattern);

    
    FcChar8* substName = nullptr;
    for (int i = 0;
         FcPatternGetString(fontWithSentinel, FC_FAMILY,
                            i, &substName) == FcResultMatch;
         i++)
    {
        NS_ConvertUTF8toUTF16 subst(ToCharPtr(substName));
        if (sentinelFirstFamily &&
            FcStrCmp(substName, sentinelFirstFamily) == 0) {
            break;
        }
        gfxFontFamily* foundFamily = gfxPlatformFontList::FindFamily(subst);
        if (foundFamily) {
            
            
            mFcSubstituteCache.Put(familyToFind, foundFamily);
            return foundFamily;
        }
    }

    return nullptr;
}

bool
gfxFcPlatformFontList::GetStandardFamilyName(const nsAString& aFontName,
                                             nsAString& aFamilyName)
{
    
    
    if (aFontName.EqualsLiteral("serif") ||
        aFontName.EqualsLiteral("sans-serif") ||
        aFontName.EqualsLiteral("monospace")) {
        aFamilyName.Assign(aFontName);
        return true;
    }

    gfxFontFamily *family = FindFamily(aFontName);
    if (family) {
        family->LocalizedName(aFamilyName);
        return true;
    }

    return false;
}

 FT_Library
gfxFcPlatformFontList::GetFTLibrary()
{
    if (!sCairoFTLibrary) {
        
        
        
        
        
        
        
        
        

        bool needsBold;
        gfxFontStyle style;
        gfxPlatformFontList* pfl = gfxPlatformFontList::PlatformFontList();
        gfxFontFamily* family = pfl->GetDefaultFont(&style);
        NS_ASSERTION(family, "couldn't find a default font family");
        gfxFontEntry* fe = family->FindFontForStyle(style, needsBold);
        if (!fe) {
            return nullptr;
        }
        nsRefPtr<gfxFont> font = fe->FindOrMakeFont(&style, false);
        if (!font) {
            return nullptr;
        }

        gfxFT2FontBase* ft2Font = reinterpret_cast<gfxFT2FontBase*>(font.get());
        gfxFT2LockedFace face(ft2Font);
        if (!face.get()) {
            return nullptr;
        }

        sCairoFTLibrary = face.get()->glyph->library;
    }

    return sCairoFTLibrary;
}

gfxFontFamily*
gfxFcPlatformFontList::FindGenericFamily(const nsAString& aGeneric,
                                         nsIAtom* aLanguage)
{
    
    NS_ConvertUTF16toUTF8 generic(aGeneric);

    nsAutoCString fcLang;
    GetSampleLangForGroup(aLanguage, fcLang);

    nsAutoCString genericLang(generic);
    genericLang.Append(fcLang);

    
    gfxFontFamily *genericFamily = mGenericMappings.GetWeak(genericLang);
    if (genericFamily) {
        return genericFamily;
    }

    
    nsAutoRef<FcPattern> genericPattern(FcPatternCreate());
    FcPatternAddString(genericPattern, FC_FAMILY,
                       ToFcChar8Ptr(generic.get()));

    
    FcPatternAddBool(genericPattern, FC_SCALABLE, FcTrue);

    
    if (!fcLang.IsEmpty()) {
        FcPatternAddString(genericPattern, FC_LANG,
                           ToFcChar8Ptr(fcLang.get()));
    }

    
    FcConfigSubstitute(nullptr, genericPattern, FcMatchPattern);
    FcDefaultSubstitute(genericPattern);

    
    FcResult result;
    nsAutoRef<FcFontSet> faces(FcFontSort(nullptr, genericPattern, FcFalse,
                                          nullptr, &result));

    
    for (int i = 0; i < faces->nfont; i++) {
        FcPattern* font = faces->fonts[i];
        FcChar8* mappedGeneric = nullptr;

        
        FcBool scalable;
        if (FcPatternGetBool(font, FC_SCALABLE, 0, &scalable) != FcResultMatch ||
            !scalable) {
            continue;
        }

        FcPatternGetString(font, FC_FAMILY, 0, &mappedGeneric);
        if (mappedGeneric) {
            NS_ConvertUTF8toUTF16 mappedGenericName(ToCharPtr(mappedGeneric));
            genericFamily = gfxPlatformFontList::FindFamily(mappedGenericName);
            if (genericFamily) {
                
                mGenericMappings.Put(genericLang, genericFamily);
                break;
            }
        }
    }

    return genericFamily;
}

 void
gfxFcPlatformFontList::CheckFontUpdates(nsITimer *aTimer, void *aThis)
{
    
    FcInitBringUptoDate();

    
    gfxFcPlatformFontList *pfl = static_cast<gfxFcPlatformFontList*>(aThis);
    FcConfig* current = FcConfigGetCurrent();
    if (current != pfl->GetLastConfig()) {
        pfl->UpdateFontList();
        pfl->ForceGlobalReflow();
    }
}

#ifdef MOZ_BUNDLED_FONTS
void
gfxFcPlatformFontList::ActivateBundledFonts()
{
    if (!mBundledFontsInitialized) {
        mBundledFontsInitialized = true;
        nsCOMPtr<nsIFile> localDir;
        nsresult rv = NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(localDir));
        if (NS_FAILED(rv)) {
            return;
        }
        if (NS_FAILED(localDir->Append(NS_LITERAL_STRING("fonts")))) {
            return;
        }
        bool isDir;
        if (NS_FAILED(localDir->IsDirectory(&isDir)) || !isDir) {
            return;
        }
        if (NS_FAILED(localDir->GetNativePath(mBundledFontsPath))) {
            return;
        }
    }
    if (!mBundledFontsPath.IsEmpty()) {
        FcConfigAppFontAddDir(nullptr, ToFcChar8Ptr(mBundledFontsPath.get()));
    }
}
#endif

#ifdef MOZ_WIDGET_GTK







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


