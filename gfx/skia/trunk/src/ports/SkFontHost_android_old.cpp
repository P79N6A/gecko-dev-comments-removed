








#include "SkFontDescriptor.h"
#include "SkFontHost.h"
#include "SkFontHost_FreeType_common.h"
#include "SkDescriptor.h"
#include "SkStream.h"
#include "SkPaint.h"
#include "SkString.h"
#include "SkStream.h"
#include "SkThread.h"
#include "SkTSearch.h"
#include <stdio.h>

#define FONT_CACHE_MEMORY_BUDGET    (768 * 1024)

#ifndef SK_FONT_FILE_PREFIX
    #define SK_FONT_FILE_PREFIX          "/fonts/"
#endif

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


static int32_t gUniqueFontID;


static SkMutex gFamilyMutex;
static FamilyRec* gFamilyHead;
static SkTDArray<NameFamilyPair> gNameList;

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
    
    SkASSERT(!"faces list is empty");
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
    SkASSERT(family->fFaces[face->style()] == face);
    family->fFaces[face->style()] = NULL;

    for (int i = 0; i < 4; i++) {
        if (family->fFaces[i] != NULL) {    
            return NULL;
        }
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
    NameFamilyPair* list = gNameList.begin();
    int             count = gNameList.count();

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

    NameFamilyPair* list = gNameList.begin();
    int             count = gNameList.count();

    int index = SkStrLCSearch(&list[0].fName, count, name, sizeof(list[0]));

    if (index < 0) {
        list = gNameList.insert(~index);
        list->construct(name, family);
    }
}

static void remove_from_names(FamilyRec* emptyFamily)
{
#ifdef SK_DEBUG
    for (int i = 0; i < 4; i++) {
        SkASSERT(emptyFamily->fFaces[i] == NULL);
    }
#endif

    SkTDArray<NameFamilyPair>& list = gNameList;

    
    for (int i = list.count() - 1; i >= 0; --i) {
        NameFamilyPair* pair = &list[i];
        if (pair->fFamily == emptyFamily) {
            pair->destruct();
            list.remove(i);
        }
    }
}



class FamilyTypeface : public SkTypeface_FreeType {
public:
    FamilyTypeface(Style style, bool sysFont, SkTypeface* familyMember,
                   bool isFixedWidth)
    : SkTypeface_FreeType(style, sk_atomic_inc(&gUniqueFontID) + 1, isFixedWidth) {
        fIsSysFont = sysFont;

        SkAutoMutexAcquire  ac(gFamilyMutex);

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
        SkAutoMutexAcquire  ac(gFamilyMutex);

        
        
        FamilyRec* family = remove_from_family(this);
        if (NULL != family) {
            remove_from_names(family);
            detach_and_delete_family(family);
        }
    }

    bool isSysFont() const { return fIsSysFont; }

    virtual const char* getUniqueString() const = 0;
    virtual const char* getFilePath() const = 0;

protected:
    virtual void onGetFontDescriptor(SkFontDescriptor*, bool*) const SK_OVERRIDE;

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

    
    virtual SkStream* onOpenStream(int* ttcIndex) const SK_OVERRIDE {
        *ttcIndex = 0;
        
        
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

    
    virtual SkStream* onOpenStream(int* ttcIndex) const SK_OVERRIDE {
        *ttcIndex = 0;
        SkStream* stream = SkNEW_ARGS(SkFILEStream, (fPath.c_str()));

        
        if (stream->getLength() <= 0) {
            SkDELETE(stream);
            stream = NULL;
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

    SkAutoTUnref<SkStream> stream(SkStream::NewFromFile(fullpath.c_str()));
    if (stream.get()) {
        return SkTypeface_FreeType::ScanFont(stream, 0, name, style, isFixedWidth);
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

static const char* gSansNames[] = {
    "sans-serif", "arial", "helvetica", "tahoma", "verdana", NULL
};

static const char* gSerifNames[] = {
    "serif", "times", "times new roman", "palatino", "georgia", "baskerville",
    "goudy", "fantasy", "cursive", "ITC Stone Serif", NULL
};

static const char* gMonoNames[] = {
    "monospace", "courier", "courier new", "monaco", NULL
};


static const char* gFBNames[] = { NULL };





static const FontInitRec gSystemFonts[] = {
    { "DroidSans.ttf",              gSansNames  },
    { "DroidSans-Bold.ttf",         NULL        },
    { "DroidSerif-Regular.ttf",     gSerifNames },
    { "DroidSerif-Bold.ttf",        NULL        },
    { "DroidSerif-Italic.ttf",      NULL        },
    { "DroidSerif-BoldItalic.ttf",  NULL        },
    { "DroidSansMono.ttf",          gMonoNames  },
    



    { "DroidSansArabic.ttf",        gFBNames    },
    { "DroidSansHebrew.ttf",        gFBNames    },
    { "DroidSansThai.ttf",          gFBNames    },
    { "MTLmr3m.ttf",                gFBNames    }, 
    { "MTLc3m.ttf",                 gFBNames    }, 
    { "DroidSansJapanese.ttf",      gFBNames    },
    { "DroidSansFallback.ttf",      gFBNames    }
};

#define DEFAULT_NAMES   gSansNames


static FamilyRec* gDefaultFamily;
static SkTypeface* gDefaultNormal;








static uint32_t gFallbackFonts[SK_ARRAY_COUNT(gSystemFonts)+1];




static void load_system_fonts() {
    
    if (NULL != gDefaultNormal) {
        return;
    }

    const FontInitRec* rec = gSystemFonts;
    SkTypeface* firstInFamily = NULL;
    int fallbackCount = 0;

    for (size_t i = 0; i < SK_ARRAY_COUNT(gSystemFonts); i++) {
        
        if (rec[i].fNames != NULL) {
            firstInFamily = NULL;
        }

        bool isFixedWidth;
        SkString name;
        SkTypeface::Style style;

        
        bool isExpected = (rec[i].fNames != gFBNames);
        if (!get_name_and_style(rec[i].fFileName, &name, &style,
                                &isFixedWidth, isExpected)) {
            continue;
        }

        SkTypeface* tf = SkNEW_ARGS(FileTypeface,
                                    (style,
                                     true,  
                                     firstInFamily, 
                                     rec[i].fFileName,
                                     isFixedWidth) 
                                    );

        if (rec[i].fNames != NULL) {
            
            if (rec[i].fNames == gFBNames) {
            
            
                gFallbackFonts[fallbackCount++] = tf->uniqueID();
            }

            firstInFamily = tf;
            FamilyRec* family = find_family(tf);
            const char* const* names = rec[i].fNames;

            
            if (names == DEFAULT_NAMES) {
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

void FamilyTypeface::onGetFontDescriptor(SkFontDescriptor* desc,
                                         bool* isLocalStream) const {
    {
        SkAutoMutexAcquire ac(gFamilyMutex);
        
        desc->setFontFileName(this->getUniqueString());
    }
    *isLocalStream = !this->isSysFont();
}



SkTypeface* SkFontHost::CreateTypeface(const SkTypeface* familyFace,
                                       const char familyName[],
                                       SkTypeface::Style style) {
    load_system_fonts();

    SkAutoMutexAcquire  ac(gFamilyMutex);

    
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

SkTypeface* SkAndroidNextLogicalTypeface(SkFontID currFontID, SkFontID origFontID,
                                         const SkPaintOptionsAndroid& options) {
    load_system_fonts();

    




    const uint32_t* list = gFallbackFonts;
    for (int i = 0; list[i] != 0; i++) {
        if (list[i] == currFontID) {
            return find_from_uniqueID(list[i+1]);
        }
    }
    return find_from_uniqueID(list[0]);
}



SkTypeface* SkFontHost::CreateTypefaceFromStream(SkStream* stream) {
    if (NULL == stream || stream->getLength() <= 0) {
        return NULL;
    }

    bool isFixedWidth;
    SkString name;
    SkTypeface::Style style;
    SkTypeface_FreeType::ScanFont(stream, 0, &name, &style, &isFixedWidth);

    if (!name.isEmpty()) {
        return SkNEW_ARGS(StreamTypeface, (style, false, NULL, stream, isFixedWidth));
    } else {
        return NULL;
    }
}

SkTypeface* SkFontHost::CreateTypefaceFromFile(const char path[]) {
    SkStream* stream = SkNEW_ARGS(SkFILEStream, (path));
    SkTypeface* face = SkFontHost::CreateTypefaceFromStream(stream);
    
    stream->unref();
    return face;
}

