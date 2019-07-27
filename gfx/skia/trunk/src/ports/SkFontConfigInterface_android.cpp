







#include "SkFontConfigInterface.h"
#include "SkTypeface_android.h"

#include "SkFontConfigParser_android.h"
#include "SkFontConfigTypeface.h"
#include "SkFontHost_FreeType_common.h"
#include "SkFontMgr.h"
#include "SkGlyphCache.h"
#include "SkPaint.h"
#include "SkString.h"
#include "SkStream.h"
#include "SkThread.h"
#include "SkTypefaceCache.h"
#include "SkTArray.h"
#include "SkTDict.h"
#include "SkTSearch.h"

#include <stdio.h>
#include <string.h>

#ifndef SK_DEBUG_FONTS
    #define SK_DEBUG_FONTS 0
#endif

#if SK_DEBUG_FONTS
    #define DEBUG_FONT(args) SkDebugf args
#else
    #define DEBUG_FONT(args)
#endif




static const char* gTestMainConfigFile = NULL;
static const char* gTestFallbackConfigFile = NULL;
static const char* gTestFontFilePrefix = NULL;



typedef int32_t FontRecID;
#define INVALID_FONT_REC_ID -1

typedef int32_t FamilyRecID;
#define INVALID_FAMILY_REC_ID -1


struct FontRec {
    SkAutoTUnref<SkTypeface> fTypeface;
    SkString fFileName;
    SkTypeface::Style fStyle;
    bool fIsValid;
    FamilyRecID fFamilyRecID;
};

struct FamilyRec {
    FamilyRec() {
        memset(fFontRecID, INVALID_FONT_REC_ID, sizeof(fFontRecID));
    }

    static const int FONT_STYLE_COUNT = 4;
    FontRecID fFontRecID[FONT_STYLE_COUNT];
    bool fIsFallbackFont;
    SkString fFallbackName;
    SkPaintOptionsAndroid fPaintOptions;
};


typedef SkTDArray<FamilyRecID> FallbackFontList;

class SkFontConfigInterfaceAndroid : public SkFontConfigInterface {
public:
    SkFontConfigInterfaceAndroid(SkTDArray<FontFamily*>& fontFamilies);
    virtual ~SkFontConfigInterfaceAndroid();

    virtual bool matchFamilyName(const char familyName[],
                                 SkTypeface::Style requested,
                                 FontIdentity* outFontIdentifier,
                                 SkString* outFamilyName,
                                 SkTypeface::Style* outStyle) SK_OVERRIDE;
    virtual SkStream* openStream(const FontIdentity&) SK_OVERRIDE;

    
    virtual SkDataTable* getFamilyNames() SK_OVERRIDE;
    virtual bool matchFamilySet(const char inFamilyName[],
                                SkString* outFamilyName,
                                SkTArray<FontIdentity>*) SK_OVERRIDE;

    



    bool getFallbackFamilyNameForChar(SkUnichar uni, const char* lang, SkString* name);
    


    SkTypeface* nextLogicalTypeface(SkFontID currFontID, SkFontID origFontID,
                                    const SkPaintOptionsAndroid& options);
    SkTypeface* getTypefaceForGlyphID(uint16_t glyphID, const SkTypeface* origTypeface,
                                      const SkPaintOptionsAndroid& options,
                                      int* lowerBounds, int* upperBounds);

private:
    void addFallbackFamily(FamilyRecID fontRecID);
    SkTypeface* getTypefaceForFontRec(FontRecID fontRecID);
    FallbackFontList* getCurrentLocaleFallbackFontList();
    FallbackFontList* findFallbackFontList(const SkLanguage& lang, bool isOriginal = true);

    SkTArray<FontRec, true> fFonts;
    SkTArray<FamilyRec, true> fFontFamilies;
    SkTDict<FamilyRecID> fFamilyNameDict;
    FamilyRecID fDefaultFamilyRecID;

    
    SkTDict<FallbackFontList*> fFallbackFontDict;
    SkTDict<FallbackFontList*> fFallbackFontAliasDict;
    FallbackFontList fDefaultFallbackList;

    
    SkString fCachedLocale;
    FallbackFontList* fLocaleFallbackFontList;
};



SK_DECLARE_STATIC_MUTEX(gGetSingletonInterfaceMutex);
static SkFontConfigInterfaceAndroid* getSingletonInterface() {
    static SkFontConfigInterfaceAndroid* gFontConfigInterface;

    SkAutoMutexAcquire ac(gGetSingletonInterfaceMutex);
    if (NULL == gFontConfigInterface) {
        
        
        SkTDArray<FontFamily*> fontFamilies;
        if (!gTestMainConfigFile) {
            SkFontConfigParser::GetFontFamilies(fontFamilies);
        } else {
            SkFontConfigParser::GetTestFontFamilies(fontFamilies, gTestMainConfigFile,
                                                    gTestFallbackConfigFile);
        }

        gFontConfigInterface = new SkFontConfigInterfaceAndroid(fontFamilies);

        
        fontFamilies.deleteAll();
    }
    return gFontConfigInterface;
}

SkFontConfigInterface* SkFontConfigInterface::GetSingletonDirectInterface(SkBaseMutex*) {
    
    return getSingletonInterface();
}



static bool has_font(const SkTArray<FontRec, true>& array, const SkString& filename) {
    for (int i = 0; i < array.count(); i++) {
        if (array[i].fFileName == filename) {
            return true;
        }
    }
    return false;
}

#ifndef SK_FONT_FILE_PREFIX
    #define SK_FONT_FILE_PREFIX          "/fonts/"
#endif

static void get_path_for_sys_fonts(SkString* full, const SkString& name) {
    if (gTestFontFilePrefix) {
        full->set(gTestFontFilePrefix);
    } else {
        full->set(getenv("ANDROID_ROOT"));
        full->append(SK_FONT_FILE_PREFIX);
    }
    full->append(name);
}

static void insert_into_name_dict(SkTDict<FamilyRecID>& familyNameDict,
                                  const char* name, FamilyRecID familyRecID) {
    SkAutoAsciiToLC tolc(name);
    if (familyNameDict.find(tolc.lc())) {
        SkDebugf("---- system font attempting to use a the same name [%s] for"
                 "multiple families. skipping subsequent occurrences", tolc.lc());
    } else {
        familyNameDict.set(tolc.lc(), familyRecID);
    }
}



SkFontConfigInterfaceAndroid::SkFontConfigInterfaceAndroid(SkTDArray<FontFamily*>& fontFamilies) :
        fFonts(fontFamilies.count()),
        fFontFamilies(fontFamilies.count() / FamilyRec::FONT_STYLE_COUNT),
        fFamilyNameDict(1024),
        fDefaultFamilyRecID(INVALID_FAMILY_REC_ID),
        fFallbackFontDict(128),
        fFallbackFontAliasDict(128),
        fLocaleFallbackFontList(NULL) {

    for (int i = 0; i < fontFamilies.count(); ++i) {
        FontFamily* family = fontFamilies[i];

        
        
        FamilyRec* familyRec = NULL;
        FamilyRecID familyRecID = INVALID_FAMILY_REC_ID;

        for (int j = 0; j < family->fFontFiles.count(); ++j) {
            SkString filename;
            get_path_for_sys_fonts(&filename, family->fFontFiles[j].fFileName);

            if (has_font(fFonts, filename)) {
                SkDebugf("---- system font and fallback font files specify a duplicate "
                        "font %s, skipping the second occurrence", filename.c_str());
                continue;
            }

            FontRec& fontRec = fFonts.push_back();
            fontRec.fFileName = filename;
            fontRec.fStyle = SkTypeface::kNormal;
            fontRec.fIsValid = false;
            fontRec.fFamilyRecID = familyRecID;

            const FontRecID fontRecID = fFonts.count() - 1;

            SkAutoTUnref<SkStream> stream(SkStream::NewFromFile(filename.c_str()));
            if (stream.get() != NULL) {
                bool isFixedWidth;
                SkString name;
                fontRec.fIsValid = SkTypeface_FreeType::ScanFont(stream.get(), 0,
                                                                 &name, &fontRec.fStyle,
                                                                 &isFixedWidth);
            } else {
                if (!family->fIsFallbackFont) {
                    SkDebugf("---- failed to open <%s> as a font\n", filename.c_str());
                }
            }

            if (fontRec.fIsValid) {
                DEBUG_FONT(("---- SystemFonts[%d][%d] fallback=%d file=%s",
                           i, fFonts.count() - 1, family->fIsFallbackFont, filename.c_str()));
            } else {
                DEBUG_FONT(("---- SystemFonts[%d][%d] fallback=%d file=%s (INVALID)",
                           i, fFonts.count() - 1, family->fIsFallbackFont, filename.c_str()));
                continue;
            }

            
            
            if (familyRec == NULL) {
                familyRec = &fFontFamilies.push_back();
                familyRecID = fFontFamilies.count() - 1;
                fontRec.fFamilyRecID = familyRecID;

                familyRec->fIsFallbackFont = family->fIsFallbackFont;
                familyRec->fPaintOptions = family->fFontFiles[j].fPaintOptions;

            } else if (familyRec->fPaintOptions != family->fFontFiles[j].fPaintOptions) {
                SkDebugf("Every font file within a family must have identical"
                         "language and variant attributes");
                sk_throw();
            }

            
            if (INVALID_FONT_REC_ID != familyRec->fFontRecID[fontRec.fStyle]) {
                DEBUG_FONT(("Overwriting familyRec for style[%d] old,new:(%d,%d)",
                            fontRec.fStyle, familyRec->fFontRecID[fontRec.fStyle],
                            fontRecID));
            }
            familyRec->fFontRecID[fontRec.fStyle] = fontRecID;
        }

        if (familyRec) {
            if (familyRec->fIsFallbackFont) {
                
                
                addFallbackFamily(familyRecID);
            } else {
                
                const SkTArray<SkString>& names = family->fNames;
                if (names.empty()) {
                    SkDEBUGFAIL("ERROR: non-fallback font with no name");
                    continue;
                }

                for (int i = 0; i < names.count(); i++) {
                    insert_into_name_dict(fFamilyNameDict, names[i].c_str(), familyRecID);
                }
            }
        }
    }

    DEBUG_FONT(("---- We have %d system fonts", fFonts.count()));

    if (fFontFamilies.count() > 0) {
        fDefaultFamilyRecID = 0;
    }

    
    
    
    
    FallbackFontList* fallbackList;
    SkTDict<FallbackFontList*>::Iter iter(fFallbackFontDict);
    const char* fallbackLang = iter.next(&fallbackList);
    while(fallbackLang != NULL) {
        for (int i = 0; i < fDefaultFallbackList.count(); i++) {
            FamilyRecID familyRecID = fDefaultFallbackList[i];
            const SkString& fontLang = fFontFamilies[familyRecID].fPaintOptions.getLanguage().getTag();
            if (strcmp(fallbackLang, fontLang.c_str()) != 0) {
                fallbackList->push(familyRecID);
            }
        }
        
        fallbackLang = iter.next(&fallbackList);
    }
}

SkFontConfigInterfaceAndroid::~SkFontConfigInterfaceAndroid() {
    
    SkTDict<FallbackFontList*>::Iter iter(fFallbackFontDict);
    FallbackFontList* fallbackList;
    while(iter.next(&fallbackList) != NULL) {
        SkDELETE(fallbackList);
    }
}

void SkFontConfigInterfaceAndroid::addFallbackFamily(FamilyRecID familyRecID) {
    SkASSERT(familyRecID < fFontFamilies.count());
    FamilyRec& familyRec = fFontFamilies[familyRecID];
    SkASSERT(familyRec.fIsFallbackFont);

    
    
    
    
    familyRec.fFallbackName.printf("%.2x##fallback", familyRecID);
    insert_into_name_dict(fFamilyNameDict, familyRec.fFallbackName.c_str(), familyRecID);

    
    fDefaultFallbackList.push(familyRecID);

    
    const SkString& languageTag = familyRec.fPaintOptions.getLanguage().getTag();
    if (languageTag.isEmpty()) {
        return;
    }

    
    FallbackFontList* customList = NULL;
    if (!fFallbackFontDict.find(languageTag.c_str(), &customList)) {
        DEBUG_FONT(("----  Created fallback list for \"%s\"", languageTag.c_str()));
        customList = SkNEW(FallbackFontList);
        fFallbackFontDict.set(languageTag.c_str(), customList);
    }
    SkASSERT(customList != NULL);
    customList->push(familyRecID);
}


static FontRecID find_best_style(const FamilyRec& family, SkTypeface::Style style) {

    const FontRecID* fontRecIDs = family.fFontRecID;

    if (fontRecIDs[style] != INVALID_FONT_REC_ID) { 
        return fontRecIDs[style];
    }
    
    style = (SkTypeface::Style)(style ^ SkTypeface::kItalic);
    if (fontRecIDs[style] != INVALID_FONT_REC_ID) {
        return fontRecIDs[style];
    }
    
    if (fontRecIDs[SkTypeface::kNormal] != INVALID_FONT_REC_ID) {
        return fontRecIDs[SkTypeface::kNormal];
    }
    
    for (int i = 0; i < FamilyRec::FONT_STYLE_COUNT; i++) {
        if (fontRecIDs[i] != INVALID_FONT_REC_ID) {
            return fontRecIDs[i];
        }
    }
    
    SkDEBUGFAIL("No valid fonts exist for this family");
    return -1;
}

bool SkFontConfigInterfaceAndroid::matchFamilyName(const char familyName[],
                                                   SkTypeface::Style style,
                                                   FontIdentity* outFontIdentifier,
                                                   SkString* outFamilyName,
                                                   SkTypeface::Style* outStyle) {
    
    style = (SkTypeface::Style)(style & SkTypeface::kBoldItalic);

    bool exactNameMatch = false;

    FamilyRecID familyRecID = INVALID_FAMILY_REC_ID;
    if (NULL != familyName) {
        SkAutoAsciiToLC tolc(familyName);
        if (fFamilyNameDict.find(tolc.lc(), &familyRecID)) {
            exactNameMatch = true;
        }
    } else {
        familyRecID = fDefaultFamilyRecID;

    }

    
    
    
    if (INVALID_FAMILY_REC_ID == familyRecID) {
        return false;
    }

    FontRecID fontRecID = find_best_style(fFontFamilies[familyRecID], style);
    FontRec& fontRec = fFonts[fontRecID];

    if (NULL != outFontIdentifier) {
        outFontIdentifier->fID = fontRecID;
        outFontIdentifier->fTTCIndex = 0;
        outFontIdentifier->fString.set(fontRec.fFileName);

    }

    if (NULL != outFamilyName) {
        if (exactNameMatch) {
            outFamilyName->set(familyName);
        } else {
            
            const char* familyName = NULL;
            SkAssertResult(fFamilyNameDict.findKey(familyRecID, &familyName));
            SkASSERT(familyName);
            outFamilyName->set(familyName);
        }
    }

    if (NULL != outStyle) {
        *outStyle = fontRec.fStyle;
    }

    return true;
}

SkStream* SkFontConfigInterfaceAndroid::openStream(const FontIdentity& identity) {
    return SkStream::NewFromFile(identity.fString.c_str());
}

SkDataTable* SkFontConfigInterfaceAndroid::getFamilyNames() {
    SkTDArray<const char*> names;
    SkTDArray<size_t> sizes;

    SkTDict<FamilyRecID>::Iter iter(fFamilyNameDict);
    const char* familyName = iter.next(NULL);
    while(familyName != NULL) {
        *names.append() = familyName;
        *sizes.append() = strlen(familyName) + 1;

        
        familyName = iter.next(NULL);
    }

    return SkDataTable::NewCopyArrays((const void*const*)names.begin(),
                                      sizes.begin(), names.count());
}

bool SkFontConfigInterfaceAndroid::matchFamilySet(const char inFamilyName[],
                                                  SkString* outFamilyName,
                                                  SkTArray<FontIdentity>*) {
    return false;
}

static bool find_proc(SkTypeface* face, SkTypeface::Style style, void* ctx) {
    const FontRecID* fontRecID = (const FontRecID*)ctx;
    FontRecID currFontRecID = ((FontConfigTypeface*)face)->getIdentity().fID;
    return currFontRecID == *fontRecID;
}

SkTypeface* SkFontConfigInterfaceAndroid::getTypefaceForFontRec(FontRecID fontRecID) {
    FontRec& fontRec = fFonts[fontRecID];
    SkTypeface* face = fontRec.fTypeface.get();
    if (!face) {
        
        face = SkTypefaceCache::FindByProcAndRef(find_proc, &fontRecID);

        
        if (!face) {
            const char* familyName = NULL;
            SkAssertResult(fFamilyNameDict.findKey(fontRec.fFamilyRecID, &familyName));
            SkASSERT(familyName);
            face = SkTypeface::CreateFromName(familyName, fontRec.fStyle);
        }

        
        fontRec.fTypeface.reset(face);
    }
    SkASSERT(face);
    return face;
}

bool SkFontConfigInterfaceAndroid::getFallbackFamilyNameForChar(SkUnichar uni,
                                                                const char* lang,
                                                                SkString* name) {
    FallbackFontList* fallbackFontList = NULL;
    const SkString langTag(lang);
    if (langTag.isEmpty()) {
        fallbackFontList = this->getCurrentLocaleFallbackFontList();
    } else {
        fallbackFontList = this->findFallbackFontList(langTag);
    }

    for (int i = 0; i < fallbackFontList->count(); i++) {
        FamilyRecID familyRecID = fallbackFontList->getAt(i);

        
        int32_t acceptedVariants = SkPaintOptionsAndroid::kDefault_Variant |
                                   SkPaintOptionsAndroid::kElegant_Variant;
        if (!(fFontFamilies[familyRecID].fPaintOptions.getFontVariant() & acceptedVariants)) {
            continue;
        }

        FontRecID fontRecID = find_best_style(fFontFamilies[familyRecID], SkTypeface::kNormal);
        SkTypeface* face = this->getTypefaceForFontRec(fontRecID);

        SkPaint paint;
        paint.setTypeface(face);
        paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);

        uint16_t glyphID;
        paint.textToGlyphs(&uni, sizeof(uni), &glyphID);
        if (glyphID != 0) {
            name->set(fFontFamilies[familyRecID].fFallbackName);
            return true;
        }
    }
    return false;
}

FallbackFontList* SkFontConfigInterfaceAndroid::getCurrentLocaleFallbackFontList() {
    SkString locale = SkFontConfigParser::GetLocale();
    if (NULL == fLocaleFallbackFontList || locale != fCachedLocale) {
        fCachedLocale = locale;
        fLocaleFallbackFontList = this->findFallbackFontList(locale);
    }
    return fLocaleFallbackFontList;
}

FallbackFontList* SkFontConfigInterfaceAndroid::findFallbackFontList(const SkLanguage& lang,
                                                                     bool isOriginal) {
    const SkString& langTag = lang.getTag();
    if (langTag.isEmpty()) {
        return &fDefaultFallbackList;
    }

    FallbackFontList* fallbackFontList;
    if (fFallbackFontDict.find(langTag.c_str(), langTag.size(), &fallbackFontList) ||
        fFallbackFontAliasDict.find(langTag.c_str(), langTag.size(), &fallbackFontList)) {
        return fallbackFontList;
    }

    
    SkLanguage parent = lang.getParent();
    fallbackFontList = findFallbackFontList(parent, false);

    
    if (isOriginal) {
        DEBUG_FONT(("----  Created fallback list alias for \"%s\"", langTag.c_str()));
        fFallbackFontAliasDict.set(langTag.c_str(), fallbackFontList);
    }
    return fallbackFontList;
}

SkTypeface* SkFontConfigInterfaceAndroid::nextLogicalTypeface(SkFontID currFontID,
                                                              SkFontID origFontID,
                                                              const SkPaintOptionsAndroid& opts) {
    
    
    
    
    if (!opts.isUsingFontFallbacks()) {
        return NULL;
    }

    FallbackFontList* currentFallbackList = findFallbackFontList(opts.getLanguage());
    SkASSERT(currentFallbackList);

    SkTypeface::Style origStyle = SkTypeface::kNormal;
    const SkTypeface* origTypeface = SkTypefaceCache::FindByID(origFontID);
    if (NULL != origTypeface) {
        origStyle = origTypeface->style();
    }

    
    FontRecID currFontRecID = INVALID_FONT_REC_ID;
    const SkTypeface* currTypeface = SkTypefaceCache::FindByID(currFontID);
    
    
    if (NULL != currTypeface) {
        currFontRecID = ((FontConfigTypeface*)currTypeface)->getIdentity().fID;
        SkASSERT(INVALID_FONT_REC_ID != currFontRecID);
    }

    FamilyRecID currFamilyRecID = INVALID_FAMILY_REC_ID;
    if (INVALID_FONT_REC_ID != currFontRecID) {
        currFamilyRecID = fFonts[currFontRecID].fFamilyRecID;
    }

    
    int currFallbackFontIndex = currentFallbackList->find(currFamilyRecID);
    
    
    
    int nextFallbackFontIndex = currFallbackFontIndex + 1;

    if(nextFallbackFontIndex >= currentFallbackList->count()) {
        return NULL;
    }

    
    
    SkPaintOptionsAndroid::FontVariant variant = opts.getFontVariant();
    if (variant == SkPaintOptionsAndroid::kDefault_Variant) {
        variant = SkPaintOptionsAndroid::kCompact_Variant;
    }

    int32_t acceptedVariants = SkPaintOptionsAndroid::kDefault_Variant | variant;

    SkTypeface* nextLogicalTypeface = 0;
    while (nextFallbackFontIndex < currentFallbackList->count()) {
        FamilyRecID familyRecID = currentFallbackList->getAt(nextFallbackFontIndex);
        if ((fFontFamilies[familyRecID].fPaintOptions.getFontVariant() & acceptedVariants) != 0) {
            FontRecID matchedFont = find_best_style(fFontFamilies[familyRecID], origStyle);
            nextLogicalTypeface = this->getTypefaceForFontRec(matchedFont);
            break;
        }
        nextFallbackFontIndex++;
    }

    DEBUG_FONT(("---- nextLogicalFont: currFontID=%d, origFontID=%d, currRecID=%d, "
                "lang=%s, variant=%d, nextFallbackIndex[%d,%d] => nextLogicalTypeface=%d",
                currFontID, origFontID, currFontRecID, opts.getLanguage().getTag().c_str(),
                variant, nextFallbackFontIndex, currentFallbackList->getAt(nextFallbackFontIndex),
                (nextLogicalTypeface) ? nextLogicalTypeface->uniqueID() : 0));
    return SkSafeRef(nextLogicalTypeface);
}

SkTypeface* SkFontConfigInterfaceAndroid::getTypefaceForGlyphID(uint16_t glyphID,
                                                                const SkTypeface* origTypeface,
                                                                const SkPaintOptionsAndroid& opts,
                                                                int* lBounds, int* uBounds) {
    
    SkASSERT(opts.isUsingFontFallbacks());
    SkASSERT(origTypeface);

    SkTypeface* currentTypeface = NULL;
    int lowerBounds = 0; 
    int upperBounds = origTypeface->countGlyphs(); 

    
    if (glyphID < upperBounds) {
        currentTypeface = const_cast<SkTypeface*>(origTypeface);
    } else {
        FallbackFontList* currentFallbackList = findFallbackFontList(opts.getLanguage());
        SkASSERT(currentFallbackList);

        
        
        SkPaintOptionsAndroid::FontVariant variant = opts.getFontVariant();
        if (variant == SkPaintOptionsAndroid::kDefault_Variant) {
            variant = SkPaintOptionsAndroid::kCompact_Variant;
        }

        int32_t acceptedVariants = SkPaintOptionsAndroid::kDefault_Variant | variant;
        SkTypeface::Style origStyle = origTypeface->style();

        for (int x = 0; x < currentFallbackList->count(); ++x) {
            const FamilyRecID familyRecID = currentFallbackList->getAt(x);
            const SkPaintOptionsAndroid& familyOptions = fFontFamilies[familyRecID].fPaintOptions;
            if ((familyOptions.getFontVariant() & acceptedVariants) != 0) {
                FontRecID matchedFont = find_best_style(fFontFamilies[familyRecID], origStyle);
                currentTypeface = this->getTypefaceForFontRec(matchedFont);
                lowerBounds = upperBounds;
                upperBounds += currentTypeface->countGlyphs();
                if (glyphID < upperBounds) {
                    break;
                }
            }
        }
    }

    if (NULL != currentTypeface) {
        if (lBounds) {
            *lBounds = lowerBounds;
        }
        if (uBounds) {
            *uBounds = upperBounds;
        }
    }
    return currentTypeface;
}



bool SkGetFallbackFamilyNameForChar(SkUnichar uni, const char* lang, SkString* name) {
    SkFontConfigInterfaceAndroid* fontConfig = getSingletonInterface();
    return fontConfig->getFallbackFamilyNameForChar(uni, lang, name);
}

void SkUseTestFontConfigFile(const char* mainconf, const char* fallbackconf,
                             const char* fontsdir) {
    gTestMainConfigFile = mainconf;
    gTestFallbackConfigFile = fallbackconf;
    gTestFontFilePrefix = fontsdir;
    SkASSERT(gTestMainConfigFile);
    SkASSERT(gTestFallbackConfigFile);
    SkASSERT(gTestFontFilePrefix);
    SkDEBUGF(("Use Test Config File Main %s, Fallback %s, Font Dir %s",
              gTestMainConfigFile, gTestFallbackConfigFile, gTestFontFilePrefix));
}

SkTypeface* SkAndroidNextLogicalTypeface(SkFontID currFontID, SkFontID origFontID,
                                         const SkPaintOptionsAndroid& options) {
    SkFontConfigInterfaceAndroid* fontConfig = getSingletonInterface();
    return fontConfig->nextLogicalTypeface(currFontID, origFontID, options);

}

SkTypeface* SkGetTypefaceForGlyphID(uint16_t glyphID, const SkTypeface* origTypeface,
                                    const SkPaintOptionsAndroid& options,
                                    int* lowerBounds, int* upperBounds) {
    SkFontConfigInterfaceAndroid* fontConfig = getSingletonInterface();
    return fontConfig->getTypefaceForGlyphID(glyphID, origTypeface, options,
                                             lowerBounds, upperBounds);
}
