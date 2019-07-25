
















#include "SkFontHost.h"
#include "SkGraphics.h"
#include "SkDescriptor.h"
#include "SkMMapStream.h"
#include "SkPaint.h"
#include "SkString.h"
#include "SkStream.h"
#include "SkThread.h"
#include "SkTSearch.h"
#include "SkTypeface_android.h"
#include "FontHostConfiguration_android.h"
#include <stdio.h>
#include <string.h>

#define FONT_CACHE_MEMORY_BUDGET    (768 * 1024)

#ifndef SK_FONT_FILE_PREFIX
    #define SK_FONT_FILE_PREFIX          "/fonts/"
#endif

bool find_name_and_attributes(SkStream* stream, SkString* name,
                              SkTypeface::Style* style, bool* isFixedWidth);

static void GetFullPathForSysFonts(SkString* full, const char name[]) {
    full->set(getenv("ANDROID_ROOT"));
    full->append(SK_FONT_FILE_PREFIX);
    full->append(name);
}



struct FamilyRec;






struct NameFamilyPair {
    const char* fName;      
    FamilyRec*  fFamily;    

    void construct(const char name[], FamilyRec* family) {
        fName = strdup(name);
        fFamily = family;   
    }

    void destruct() {
        free((char*)fName);
        
    }
};
typedef SkTDArray<NameFamilyPair> NameFamilyPairList;


static int32_t gUniqueFontID;


SK_DECLARE_STATIC_MUTEX(gFamilyHeadAndNameListMutex);
static FamilyRec* gFamilyHead;
static SkTDArray<NameFamilyPair> gFallbackFilenameList;

static NameFamilyPairList& GetNameList() {
    



    static NameFamilyPairList* gNameList;
    if (NULL == gNameList) {
        gNameList = SkNEW(NameFamilyPairList);
        
    }
    return *gNameList;
}

struct FamilyRec {
    FamilyRec*  fNext;
    SkTypeface* fFaces[4];

    FamilyRec()
    {
        fNext = gFamilyHead;
        memset(fFaces, 0, sizeof(fFaces));
        gFamilyHead = this;
    }
};

static SkTypeface* find_best_face(const FamilyRec* family,
                                  SkTypeface::Style style) {
    SkTypeface* const* faces = family->fFaces;

    if (faces[style] != NULL) { 
        return faces[style];
    }
    
    style = (SkTypeface::Style)(style ^ SkTypeface::kItalic);
    if (faces[style] != NULL) {
        return faces[style];
    }
    
    if (faces[SkTypeface::kNormal] != NULL) {
        return faces[SkTypeface::kNormal];
    }
    
    for (int i = 0; i < 4; i++) {
        if (faces[i] != NULL) {
            return faces[i];
        }
    }
    
    SkDEBUGFAIL("faces list is empty");
    return NULL;
}

static FamilyRec* find_family(const SkTypeface* member) {
    FamilyRec* curr = gFamilyHead;
    while (curr != NULL) {
        for (int i = 0; i < 4; i++) {
            if (curr->fFaces[i] == member) {
                return curr;
            }
        }
        curr = curr->fNext;
    }
    return NULL;
}




static SkTypeface* find_from_uniqueID(uint32_t uniqueID) {
    FamilyRec* curr = gFamilyHead;
    while (curr != NULL) {
        for (int i = 0; i < 4; i++) {
            SkTypeface* face = curr->fFaces[i];
            if (face != NULL && face->uniqueID() == uniqueID) {
                return face;
            }
        }
        curr = curr->fNext;
    }
    return NULL;
}




static FamilyRec* remove_from_family(const SkTypeface* face) {
    FamilyRec* family = find_family(face);
    if (family) {
        SkASSERT(family->fFaces[face->style()] == face);
        family->fFaces[face->style()] = NULL;

        for (int i = 0; i < 4; i++) {
            if (family->fFaces[i] != NULL) {    
                return NULL;
            }
        }
    } else {

    }
    return family;  
}


static void detach_and_delete_family(FamilyRec* family) {
    FamilyRec* curr = gFamilyHead;
    FamilyRec* prev = NULL;

    while (curr != NULL) {
        FamilyRec* next = curr->fNext;
        if (curr == family) {
            if (prev == NULL) {
                gFamilyHead = next;
            } else {
                prev->fNext = next;
            }
            SkDELETE(family);
            return;
        }
        prev = curr;
        curr = next;
    }
    SkASSERT(!"Yikes, couldn't find family in our list to remove/delete");
}


static SkTypeface* find_typeface(const char name[], SkTypeface::Style style) {
    NameFamilyPairList& namelist = GetNameList();
    NameFamilyPair* list = namelist.begin();
    int             count = namelist.count();

    int index = SkStrLCSearch(&list[0].fName, count, name, sizeof(list[0]));

    if (index >= 0) {
        return find_best_face(list[index].fFamily, style);
    }
    return NULL;
}


static SkTypeface* find_typeface(const SkTypeface* familyMember,
                                 SkTypeface::Style style) {
    const FamilyRec* family = find_family(familyMember);
    return family ? find_best_face(family, style) : NULL;
}


static void add_name(const char name[], FamilyRec* family) {
    SkAutoAsciiToLC tolc(name);
    name = tolc.lc();

    NameFamilyPairList& namelist = GetNameList();
    NameFamilyPair* list = namelist.begin();
    int             count = namelist.count();

    int index = SkStrLCSearch(&list[0].fName, count, name, sizeof(list[0]));

    if (index < 0) {
        list = namelist.insert(~index);
        list->construct(name, family);
    }
}


static void remove_from_names(FamilyRec* emptyFamily) {
#ifdef SK_DEBUG
    for (int i = 0; i < 4; i++) {
        SkASSERT(emptyFamily->fFaces[i] == NULL);
    }
#endif

    SkTDArray<NameFamilyPair>& list = GetNameList();

    
    for (int i = list.count() - 1; i >= 0; --i) {
        NameFamilyPair* pair = &list[i];
        if (pair->fFamily == emptyFamily) {
            pair->destruct();
            list.remove(i);
        }
    }
}



class FamilyTypeface : public SkTypeface {
public:
    FamilyTypeface(Style style, bool sysFont, SkTypeface* familyMember,
                   bool isFixedWidth)
    : SkTypeface(style, sk_atomic_inc(&gUniqueFontID) + 1, isFixedWidth) {
        fIsSysFont = sysFont;

        
        FamilyRec* rec = NULL;
        if (familyMember) {
            rec = find_family(familyMember);
            SkASSERT(rec);
        } else {
            rec = SkNEW(FamilyRec);
        }
        rec->fFaces[style] = this;
    }

    virtual ~FamilyTypeface() {
        SkAutoMutexAcquire  ac(gFamilyHeadAndNameListMutex);

        
        
        FamilyRec* family = remove_from_family(this);
        if (NULL != family) {
            remove_from_names(family);
            detach_and_delete_family(family);
        }
    }

    bool isSysFont() const { return fIsSysFont; }

    virtual SkStream* openStream() = 0;
    virtual const char* getUniqueString() const = 0;
    virtual const char* getFilePath() const = 0;

private:
    bool    fIsSysFont;

    typedef SkTypeface INHERITED;
};



class StreamTypeface : public FamilyTypeface {
public:
    StreamTypeface(Style style, bool sysFont, SkTypeface* familyMember,
                   SkStream* stream, bool isFixedWidth)
    : INHERITED(style, sysFont, familyMember, isFixedWidth) {
        SkASSERT(stream);
        stream->ref();
        fStream = stream;
    }
    virtual ~StreamTypeface() {
        fStream->unref();
    }

    
    virtual SkStream* openStream() {
        
        
        fStream->ref();
        
        fStream->rewind();
        return fStream;
    }
    virtual const char* getUniqueString() const { return NULL; }
    virtual const char* getFilePath() const { return NULL; }

private:
    SkStream* fStream;

    typedef FamilyTypeface INHERITED;
};

class FileTypeface : public FamilyTypeface {
public:
    FileTypeface(Style style, bool sysFont, SkTypeface* familyMember,
                 const char path[], bool isFixedWidth)
    : INHERITED(style, sysFont, familyMember, isFixedWidth) {
        SkString fullpath;

        if (sysFont) {
            GetFullPathForSysFonts(&fullpath, path);
            path = fullpath.c_str();
        }
        fPath.set(path);
    }

    
    virtual SkStream* openStream() {
        SkStream* stream = SkNEW_ARGS(SkMMAPStream, (fPath.c_str()));

        
        if (stream->getLength() <= 0) {
            SkDELETE(stream);
            
            stream = SkNEW_ARGS(SkFILEStream, (fPath.c_str()));
            if (stream->getLength() <= 0) {
                SkDELETE(stream);
                stream = NULL;
            }
        }
        return stream;
    }
    virtual const char* getUniqueString() const {
        const char* str = strrchr(fPath.c_str(), '/');
        if (str) {
            str += 1;   
        }
        return str;
    }
    virtual const char* getFilePath() const {
        return fPath.c_str();
    }

private:
    SkString fPath;

    typedef FamilyTypeface INHERITED;
};




static bool get_name_and_style(const char path[], SkString* name,
                               SkTypeface::Style* style,
                               bool* isFixedWidth, bool isExpected) {
    SkString        fullpath;
    GetFullPathForSysFonts(&fullpath, path);

    SkMMAPStream stream(fullpath.c_str());
    if (stream.getLength() > 0) {
        return find_name_and_attributes(&stream, name, style, isFixedWidth);
    }
    else {
        SkFILEStream stream(fullpath.c_str());
        if (stream.getLength() > 0) {
            return find_name_and_attributes(&stream, name, style, isFixedWidth);
        }
    }

    if (isExpected) {
        SkDebugf("---- failed to open <%s> as a font\n", fullpath.c_str());
    }
    return false;
}


struct FontInitRec {
    const char*         fFileName;
    const char* const*  fNames;     
};


static const char* gFBNames[] = { NULL };

static const struct {
    const char* fFileName;
    FallbackScripts fScript;
} gFBFileNames[] = {
    { "DroidNaskh-Regular.ttf", kArabic_FallbackScript },
    { "DroidSansEthiopic-Regular.ttf", kEthiopic_FallbackScript },
    { "DroidSansHebrew-Regular.ttf", kHebrewRegular_FallbackScript },
    { "DroidSansHebrew-Bold.ttf", kHebrewBold_FallbackScript },
    { "DroidSansThai.ttf", kThai_FallbackScript },
    { "DroidSansArmenian.ttf", kArmenian_FallbackScript },
    { "DroidSansGeorgian.ttf", kGeorgian_FallbackScript },
    { "Lohit-Devanagari.ttf", kDevanagari_FallbackScript },
    { "Lohit-Bengali.ttf", kBengali_FallbackScript },
    { "Lohit-Tamil.ttf", kTamil_FallbackScript },
};





static FontInitRec *gSystemFonts;
static size_t gNumSystemFonts = 0;


static FamilyRec* gDefaultFamily;
static SkTypeface* gDefaultNormal;
static char** gDefaultNames = NULL;
static uint32_t *gFallbackFonts;
static uint32_t *gFallbackScriptsMap;

static void dump_globals() {
    SkDebugf("gDefaultNormal=%p id=%u refCnt=%d", gDefaultNormal,
             gDefaultNormal ? gDefaultNormal->uniqueID() : 0,
             gDefaultNormal ? gDefaultNormal->getRefCnt() : 0);

    if (gDefaultFamily) {
        SkDebugf("gDefaultFamily=%p fFaces={%u,%u,%u,%u} refCnt={%d,%d,%d,%d}",
                 gDefaultFamily,
                 gDefaultFamily->fFaces[0] ? gDefaultFamily->fFaces[0]->uniqueID() : 0,
                 gDefaultFamily->fFaces[1] ? gDefaultFamily->fFaces[1]->uniqueID() : 0,
                 gDefaultFamily->fFaces[2] ? gDefaultFamily->fFaces[2]->uniqueID() : 0,
                 gDefaultFamily->fFaces[3] ? gDefaultFamily->fFaces[3]->uniqueID() : 0,
                 gDefaultFamily->fFaces[0] ? gDefaultFamily->fFaces[0]->getRefCnt() : 0,
                 gDefaultFamily->fFaces[1] ? gDefaultFamily->fFaces[1]->getRefCnt() : 0,
                 gDefaultFamily->fFaces[2] ? gDefaultFamily->fFaces[2]->getRefCnt() : 0,
                 gDefaultFamily->fFaces[3] ? gDefaultFamily->fFaces[3]->getRefCnt() : 0);
    } else {
        SkDebugf("gDefaultFamily=%p", gDefaultFamily);
    }

    SkDebugf("gNumSystemFonts=%d gSystemFonts=%p gFallbackFonts=%p",
             gNumSystemFonts, gSystemFonts, gFallbackFonts);

    for (size_t i = 0; i < gNumSystemFonts; ++i) {
        SkDebugf("gSystemFonts[%d] fileName=%s", i, gSystemFonts[i].fFileName);
        size_t namesIndex = 0;
        if (gSystemFonts[i].fNames)
            for (const char* fontName = gSystemFonts[i].fNames[namesIndex];
                    fontName != 0;
                    fontName = gSystemFonts[i].fNames[++namesIndex]) {
                SkDebugf("       name[%u]=%s", namesIndex, fontName);
            }
    }

    if (gFamilyHead) {
        FamilyRec* rec = gFamilyHead;
        int i=0;
        while (rec) {
            SkDebugf("gFamilyHead[%d]=%p fFaces={%u,%u,%u,%u} refCnt={%d,%d,%d,%d}",
                     i++, rec,
                     rec->fFaces[0] ? rec->fFaces[0]->uniqueID() : 0,
                     rec->fFaces[1] ? rec->fFaces[1]->uniqueID() : 0,
                     rec->fFaces[2] ? rec->fFaces[2]->uniqueID() : 0,
                     rec->fFaces[3] ? rec->fFaces[3]->uniqueID() : 0,
                     rec->fFaces[0] ? rec->fFaces[0]->getRefCnt() : 0,
                     rec->fFaces[1] ? rec->fFaces[1]->getRefCnt() : 0,
                     rec->fFaces[2] ? rec->fFaces[2]->getRefCnt() : 0,
                     rec->fFaces[3] ? rec->fFaces[3]->getRefCnt() : 0);
            rec = rec->fNext;
        }
    } else {
        SkDebugf("gFamilyHead=%p", gFamilyHead);
    }

}




static void load_font_info() {
    SkTDArray<FontFamily*> fontFamilies;
    getFontFamilies(fontFamilies);

    SkTDArray<FontInitRec> fontInfo;
    bool firstInFamily = false;
    for (int i = 0; i < fontFamilies.count(); ++i) {
        FontFamily *family = fontFamilies[i];
        firstInFamily = true;
        for (int j = 0; j < family->fFileNames.count(); ++j) {
            FontInitRec fontInfoRecord;
            fontInfoRecord.fFileName = family->fFileNames[j];
            if (j == 0) {
                if (family->fNames.count() == 0) {
                    
                    fontInfoRecord.fNames = (char **)gFBNames;
                } else {
                    SkTDArray<const char*> names = family->fNames;
                    const char **nameList = (const char**)
                            malloc((names.count() + 1) * sizeof(char*));
                    if (nameList == NULL) {
                        
                        break;
                    }
                    if (gDefaultNames == NULL) {
                        gDefaultNames = (char**) nameList;
                    }
                    for (int i = 0; i < names.count(); ++i) {
                        nameList[i] = names[i];
                    }
                    nameList[names.count()] = NULL;
                    fontInfoRecord.fNames = nameList;
                }
            } else {
                fontInfoRecord.fNames = NULL;
            }
            *fontInfo.append() = fontInfoRecord;
        }
    }
    gNumSystemFonts = fontInfo.count();
    gSystemFonts = (FontInitRec*) malloc(gNumSystemFonts * sizeof(FontInitRec));
    gFallbackFonts = (uint32_t*) malloc((gNumSystemFonts + 1) * sizeof(uint32_t));
    gFallbackScriptsMap = (uint32_t*) calloc(kFallbackScriptNumber, sizeof(uint32_t));
    if (gSystemFonts == NULL) {
        
        SkDEBUGFAIL("No system fonts were found");
        gNumSystemFonts = 0;
    }
    SkDEBUGF(("---- We have %d system fonts", gNumSystemFonts));
    for (size_t i = 0; i < gNumSystemFonts; ++i) {
        gSystemFonts[i].fFileName = fontInfo[i].fFileName;
        gSystemFonts[i].fNames = fontInfo[i].fNames;
        SkDEBUGF(("---- gSystemFonts[%d] fileName=%s", i, fontInfo[i].fFileName));
    }
    fontFamilies.deleteAll();
}







static void init_system_fonts() {
    
    if (gDefaultNormal) {
        return;
    }

    SkASSERT(gUniqueFontID == 0);

    load_font_info();

    const FontInitRec* rec = gSystemFonts;
    SkTypeface* firstInFamily = NULL;
    int fallbackCount = 0;

    for (size_t i = 0; i < gNumSystemFonts; i++) {
        
        if (rec[i].fNames != NULL) {
            firstInFamily = NULL;
        }

        bool isFixedWidth;
        SkString name;
        SkTypeface::Style style;

        
        bool isExpected = (rec[i].fNames != gFBNames);
        if (!get_name_and_style(rec[i].fFileName, &name, &style,
                                &isFixedWidth, isExpected)) {
            
            
            
            sk_atomic_inc(&gUniqueFontID);
            continue;
        }

        SkTypeface* tf = SkNEW_ARGS(FileTypeface,
                                    (style,
                                     true,  
                                     firstInFamily, 
                                     rec[i].fFileName,
                                     isFixedWidth) 
                                    );

        SkDEBUGF(("---- SkTypeface[%d] %s fontID %d\n",
                  i, rec[i].fFileName, tf->uniqueID()));

        FallbackScripts fallbackScript = SkGetFallbackScriptFromID(rec[i].fFileName);
        if (SkTypeface_ValidScript(fallbackScript)) {
            gFallbackScriptsMap[fallbackScript] = tf->uniqueID();
        }

        if (rec[i].fNames != NULL) {
            
            if (rec[i].fNames == gFBNames) {
                SkDEBUGF(("---- adding %s as fallback[%d] fontID %d\n",
                          rec[i].fFileName, fallbackCount, tf->uniqueID()));
                gFallbackFonts[fallbackCount++] = tf->uniqueID();
            }

            firstInFamily = tf;
            FamilyRec* family = find_family(tf);
            const char* const* names = rec[i].fNames;

            
            if (names == gDefaultNames) {
                gDefaultFamily = family;
            }
            
            while (*names) {
                add_name(*names, family);
                names += 1;
            }
        }
    }

    
    
    gDefaultNormal = find_best_face(gDefaultFamily, SkTypeface::kNormal);
    
    gFallbackFonts[fallbackCount] = 0;


}

static size_t find_uniqueID(const char* filename) {
    
    
    
    const FontInitRec* rec = gSystemFonts;
    for (size_t i = 0; i < gNumSystemFonts; i++) {
        if (strcmp(rec[i].fFileName, filename) == 0) {
            return i+1;
        }
    }
    return 0;
}

static void reload_fallback_fonts() {
    SkGraphics::PurgeFontCache();

    SkTDArray<FontFamily*> fallbackFamilies;
    getFallbackFontFamilies(fallbackFamilies);

    int fallbackCount = 0;
    for (int i = 0; i < fallbackFamilies.count(); ++i) {
        FontFamily *family = fallbackFamilies[i];

        for (int j = 0; j < family->fFileNames.count(); ++j) {
            if (family->fFileNames[j]) {

                
                bool isFixedWidth;
                SkString name;
                SkTypeface::Style style;
                if (!get_name_and_style(family->fFileNames[j], &name, &style,
                                        &isFixedWidth, false)) {
                    continue;
                }

                size_t uniqueID = find_uniqueID(family->fFileNames[j]);
                SkASSERT(uniqueID != 0);
                SkDEBUGF(("---- reload %s as fallback[%d] fontID %d oldFontID %d\n",
                          family->fFileNames[j], fallbackCount, uniqueID,
                          gFallbackFonts[fallbackCount]));

                gFallbackFonts[fallbackCount++] = uniqueID;
                break;  
            }
        }
    }
    
    gFallbackFonts[fallbackCount] = 0;
}

static void load_system_fonts() {
    static AndroidLocale prevLocale;
    AndroidLocale locale;

    getLocale(locale);

    if (!gDefaultNormal) {
        prevLocale = locale;
        init_system_fonts();
    } else if (strncmp(locale.language, prevLocale.language, 2) ||
            strncmp(locale.region, prevLocale.region, 2)) {
        prevLocale = locale;
        reload_fallback_fonts();
    }
}



void SkFontHost::Serialize(const SkTypeface* face, SkWStream* stream) {
    
    bool isCustomFont = !((FamilyTypeface*)face)->isSysFont();
    stream->writeBool(isCustomFont);

    if (isCustomFont) {
        SkStream* fontStream = ((FamilyTypeface*)face)->openStream();

        
        uint32_t len = fontStream->getLength();
        stream->write32(len);

        
        void* fontData = malloc(len);

        fontStream->read(fontData, len);
        stream->write(fontData, len);

        fontStream->unref();
        free(fontData);


    } else {
        const char* name = ((FamilyTypeface*)face)->getUniqueString();

        stream->write8((uint8_t)face->style());

        if (NULL == name || 0 == *name) {
            stream->writePackedUInt(0);

        } else {
            uint32_t len = strlen(name);
            stream->writePackedUInt(len);
            stream->write(name, len);

        }
    }
}

SkTypeface* SkFontHost::Deserialize(SkStream* stream) {
    {
        SkAutoMutexAcquire  ac(gFamilyHeadAndNameListMutex);
        load_system_fonts();
    }

    
    bool isCustomFont = stream->readBool();

    if (isCustomFont) {

        
        uint32_t len = stream->readU32();

        
        SkMemoryStream* fontStream = new SkMemoryStream(len);
        stream->read((void*)fontStream->getMemoryBase(), len);

        SkTypeface* face = CreateTypefaceFromStream(fontStream);

        fontStream->unref();


        return face;

    } else {
        int style = stream->readU8();

        int len = stream->readPackedUInt();
        if (len > 0) {
            SkString str;
            str.resize(len);
            stream->read(str.writable_str(), len);

            const FontInitRec* rec = gSystemFonts;
            for (size_t i = 0; i < gNumSystemFonts; i++) {
                if (strcmp(rec[i].fFileName, str.c_str()) == 0) {
                    
                    for (int j = i; j >= 0; --j) {
                        if (rec[j].fNames != NULL) {
                            return SkFontHost::CreateTypeface(NULL,
                                        rec[j].fNames[0],
                                        (SkTypeface::Style)style);
                        }
                    }
                }
            }
        }
    }
    return NULL;
}



SkTypeface* SkFontHost::CreateTypeface(const SkTypeface* familyFace,
                                       const char familyName[],
                                       SkTypeface::Style style) {
    SkAutoMutexAcquire  ac(gFamilyHeadAndNameListMutex);

    load_system_fonts();

    
    style = (SkTypeface::Style)(style & SkTypeface::kBoldItalic);

    SkTypeface* tf = NULL;

    if (NULL != familyFace) {
        tf = find_typeface(familyFace, style);
    } else if (NULL != familyName) {

        tf = find_typeface(familyName, style);
    }

    if (NULL == tf) {
        tf = find_best_face(gDefaultFamily, style);
    }

    
    tf->ref();
    return tf;
}

SkTypeface* SkCreateTypefaceForScript(FallbackScripts script) {
    if (!SkTypeface_ValidScript(script)) {
        return NULL;
    }

    SkAutoMutexAcquire  ac(gFamilyHeadAndNameListMutex);

    load_system_fonts();

    if (gFallbackScriptsMap[script] == 0) {
        return NULL;
    }

    SkTypeface* tf = find_from_uniqueID(gFallbackScriptsMap[script]);
    
    tf->ref();
    return tf;
}

const char* SkGetFallbackScriptID(FallbackScripts script) {
    for (int i = 0; i < sizeof(gFBFileNames) / sizeof(gFBFileNames[0]); i++) {
        if (gFBFileNames[i].fScript == script) {
            return gFBFileNames[i].fFileName;
        }
    }
    return NULL;
}

FallbackScripts SkGetFallbackScriptFromID(const char* fileName) {
    for (int i = 0; i < sizeof(gFBFileNames) / sizeof(gFBFileNames[0]); i++) {
        if (strcmp(gFBFileNames[i].fFileName, fileName) == 0) {
            return gFBFileNames[i].fScript;
        }
    }
    return kFallbackScriptNumber; 
}

SkStream* SkFontHost::OpenStream(uint32_t fontID) {
    SkAutoMutexAcquire  ac(gFamilyHeadAndNameListMutex);

    FamilyTypeface* tf = (FamilyTypeface*)find_from_uniqueID(fontID);
    SkStream* stream = tf ? tf->openStream() : NULL;

    if (stream && stream->getLength() == 0) {
        stream->unref();
        stream = NULL;
    }
    return stream;
}

size_t SkFontHost::GetFileName(SkFontID fontID, char path[], size_t length,
                               int32_t* index) {
    SkAutoMutexAcquire  ac(gFamilyHeadAndNameListMutex);

    FamilyTypeface* tf = (FamilyTypeface*)find_from_uniqueID(fontID);
    const char* src = tf ? tf->getFilePath() : NULL;

    if (src) {
        size_t size = strlen(src);
        if (path) {
            memcpy(path, src, SkMin32(size, length));
        }
        if (index) {
            *index = 0; 
        }
        return size;
    } else {
        return 0;
    }
}

SkFontID SkFontHost::NextLogicalFont(SkFontID currFontID, SkFontID origFontID) {
    SkAutoMutexAcquire  ac(gFamilyHeadAndNameListMutex);

    load_system_fonts();

    const SkTypeface* origTypeface = find_from_uniqueID(origFontID);
    const SkTypeface* currTypeface = find_from_uniqueID(currFontID);

    SkASSERT(origTypeface != 0);
    SkASSERT(currTypeface != 0);
    SkASSERT(gFallbackFonts);

    
    
    currFontID = find_typeface(currTypeface, SkTypeface::kNormal)->uniqueID();

    




    const uint32_t* list = gFallbackFonts;
    for (int i = 0; list[i] != 0; i++) {
        if (list[i] == currFontID) {
            if (list[i+1] == 0)
                return 0;
            const SkTypeface* nextTypeface = find_from_uniqueID(list[i+1]);
            return find_typeface(nextTypeface, origTypeface->style())->uniqueID();
        }
    }

    
    
    const SkTypeface* firstTypeface = find_from_uniqueID(list[0]);
    return find_typeface(firstTypeface, origTypeface->style())->uniqueID();
}



SkTypeface* SkFontHost::CreateTypefaceFromStream(SkStream* stream) {
    if (NULL == stream || stream->getLength() <= 0) {
        return NULL;
    }

    
    
    load_system_fonts();

    bool isFixedWidth;
    SkTypeface::Style style;

    if (find_name_and_attributes(stream, NULL, &style, &isFixedWidth)) {
        SkAutoMutexAcquire  ac(gFamilyHeadAndNameListMutex);
        return SkNEW_ARGS(StreamTypeface, (style, false, NULL, stream, isFixedWidth));
    } else {
        return NULL;
    }
}

SkTypeface* SkFontHost::CreateTypefaceFromFile(const char path[]) {
    SkStream* stream = SkNEW_ARGS(SkMMAPStream, (path));
    SkTypeface* face = SkFontHost::CreateTypefaceFromStream(stream);
    
    stream->unref();
    return face;
}
