








#include "SkFontHost.h"
#include "SkDescriptor.h"
#include "SkString.h"
#include "SkStream.h"
#include <stdio.h>


#define SK_CAN_USE_MMAP 

#ifndef SK_FONTPATH
    #define SK_FONTPATH "the complete path for a font file"
#endif

struct FontFaceRec {
    const char* fFileName;    
    uint8_t     fFamilyIndex;
    SkBool8     fBold;
    SkBool8     fItalic;

    static const FontFaceRec& FindFace(const FontFaceRec rec[], int count,
                                       int isBold, int isItalic);
};

struct FontFamilyRec {
    const FontFaceRec*  fFaces;
    int                 fFaceCount;
};

const FontFaceRec& FontFaceRec::FindFace(const FontFaceRec rec[], int count,
                                         int isBold, int isItalic)
{
    SkASSERT(count > 0);
    
    int i;

    
    for (i = 0; i < count; i++) {
        if (rec[i].fBold == isBold && rec[i].fItalic == isItalic)
            return rec[i];
    }
    
    for (i = 0; i < count; i++) {
        if (rec[i].fBold == isBold)
            return rec[i];
    }
    
    for (i = 0; i < count; i++) {
        if (!rec[i].fBold && !rec[i].fItalic)
            return rec[i];
    }
    
    return rec[0];
}

enum {
    DEFAULT_FAMILY_INDEX,
    
    FAMILY_INDEX_COUNT
};

static const FontFaceRec gDefaultFaces[] = {
    { SK_FONTPATH, DEFAULT_FAMILY_INDEX, 0,  0 }
};


static const FontFamilyRec gFamilies[] = {
    { gDefaultFaces,   SK_ARRAY_COUNT(gDefaultFaces)  }
};

#define DEFAULT_FAMILY_INDEX            DEFAULT_FAMILY_INDEX
#define DEFAULT_FAMILY_FACE_INDEX       0





struct FontFamilyMatchRec {
    const char* fLCName;
    int         fFamilyIndex;
};







static const FontFamilyMatchRec gMatches[] = {
#if 0
    { "Ahem",               Ahem_FAMILY_INDEX },
    { "arial",              SANS_FAMILY_INDEX },
    { "courier",            MONO_FAMILY_INDEX },
    { "courier new",        MONO_FAMILY_INDEX },
    { "cursive",            SERIF_FAMILY_INDEX },
    { "fantasy",            SERIF_FAMILY_INDEX },
    { "georgia",            SERIF_FAMILY_INDEX },
    { "goudy",              SERIF_FAMILY_INDEX },
    { "helvetica",          SANS_FAMILY_INDEX },
    { "palatino",           SERIF_FAMILY_INDEX },
    { "tahoma",             SANS_FAMILY_INDEX },
    { "sans-serif",         SANS_FAMILY_INDEX },
    { "serif",              SERIF_FAMILY_INDEX },
    { "times",              SERIF_FAMILY_INDEX },
    { "times new roman",    SERIF_FAMILY_INDEX },
    { "verdana",            SANS_FAMILY_INDEX }
#endif
};



#include "SkTSearch.h"

static bool contains_only_ascii(const char s[])
{
    for (;;)
    {
        int c = *s++;
        if (c == 0)
            break;
        if ((c >> 7) != 0)
            return false;
    }
    return true;
}

#define TRACE_FONT_NAME(code)


const FontFamilyRec* find_family_rec(const char target[])
{
    int     index;

    
    
    

TRACE_FONT_NAME(printf("----------------- font request <%s>", target);)

    if (contains_only_ascii(target))
    {
        
        index = SkStrLCSearch(&gMatches[0].fLCName, SK_ARRAY_COUNT(gMatches),
                              target, sizeof(gMatches[0]));
        if (index >= 0)
        {
            TRACE_FONT_NAME(printf(" found %d\n", index);)
            return &gFamilies[gMatches[index].fFamilyIndex];
        }
    }

    

#if 0
    if (strstr(target, "sans") || strstr(target, "Sans"))
    {
        TRACE_FONT_NAME(printf(" found sans\n");)
        return &gFamilies[SANS_FAMILY_INDEX];
    }
    if (strstr(target, "serif") || strstr(target, "Serif"))
    {
        TRACE_FONT_NAME(printf(" found serif\n");)
        return &gFamilies[SERIF_FAMILY_INDEX];
    }
    if (strstr(target, "mono") || strstr(target, "Mono"))
    {
        TRACE_FONT_NAME(printf(" found mono\n");)
        return &gFamilies[MONO_FAMILY_INDEX];
    }
#endif

    TRACE_FONT_NAME(printf(" use default\n");)
    
    return &gFamilies[DEFAULT_FAMILY_INDEX];
}



static const FontFaceRec* get_default_face()
{
    return &gFamilies[DEFAULT_FAMILY_INDEX].fFaces[DEFAULT_FAMILY_FACE_INDEX];
}

static SkTypeface::Style get_style(const FontFaceRec& face) {
    int style = 0;
    if (face.fBold) {
        style |= SkTypeface::kBold;
    }
    if (face.fItalic) {
        style |= SkTypeface::kItalic;
    }
    return static_cast<SkTypeface::Style>(style);
}


static uint32_t get_id(const FontFaceRec& face) {
    uintptr_t id = reinterpret_cast<uintptr_t>(&face);
    return static_cast<uint32_t>(id);
}

class FontFaceRec_Typeface : public SkTypeface {
public:
    FontFaceRec_Typeface(const FontFaceRec& face) :
                         SkTypeface(get_style(face), get_id(face)),
                         fFace(face)
    {
    }

    
    const FontFaceRec& fFace;
};

static const FontFaceRec* get_typeface_rec(const SkTypeface* face)
{
    const FontFaceRec_Typeface* f = (FontFaceRec_Typeface*)face;
    return f ? &f->fFace : get_default_face();
}

static uint32_t ptr2uint32(const void* p)
{
    
    
    return (uint32_t)((char*)p - (char*)0);
}

SkTypeface* SkFontHost::CreateTypeface(const SkTypeface* familyFace,
                                       const char familyName[],
                                       const void* data, size_t bytelength,
                                       SkTypeface::Style style)
{
    const FontFamilyRec* family;
    
    if (familyFace)
        family = &gFamilies[
                    ((FontFaceRec_Typeface*)familyFace)->fFace.fFamilyIndex];
    else if (familyName)
        family = find_family_rec(familyName);
    else
        family = &gFamilies[DEFAULT_FAMILY_INDEX];

    const FontFaceRec& face = FontFaceRec::FindFace(family->fFaces,
                                            family->fFaceCount,
                                            (style & SkTypeface::kBold) != 0,
                                            (style & SkTypeface::kItalic) != 0);

    
    if (familyFace != NULL &&
            &((FontFaceRec_Typeface*)familyFace)->fFace == &face)
    {
        familyFace->ref();
        return (SkTypeface*)familyFace;
    }
    return SkNEW_ARGS(FontFaceRec_Typeface, (face));
}


SkAdvancedTypefaceMetrics* SkFontHost::GetAdvancedTypefaceMetrics(
        uint32_t fontID,
        SkAdvancedTypefaceMetrics::PerGlyphInfo perGlyphInfo) {
    sk_throw();  
    return NULL;
}

SkTypeface* SkFontHost::CreateTypefaceFromStream(SkStream* stream) {
    sk_throw();  
    return NULL;
}

SkTypeface* SkFontHost::CreateTypefaceFromFile(const char path[]) {
    sk_throw();  
    return NULL;
}

bool SkFontHost::ValidFontID(uint32_t fontID) {
    return get_id(*get_default_face()) == fontID;
}

SkStream* SkFontHost::OpenStream(uint32_t fontID) {
    sk_throw();  
    return NULL;
}

size_t SkFontHost::GetFileName(SkFontID fontID, char path[], size_t length,
                               int32_t* index) {
    SkDebugf("SkFontHost::GetFileName unimplemented\n");
    return 0;
}

void SkFontHost::Serialize(const SkTypeface* tface, SkWStream* stream) {
    const FontFaceRec* face = &((const FontFaceRec_Typeface*)tface)->fFace;
    stream->write(face, sizeof(face));
}

SkTypeface* SkFontHost::Deserialize(SkStream* stream) {
    const FontFaceRec* face;
    stream->read(&face, sizeof(face));
    return new FontFaceRec_Typeface(*face);
}

SkScalerContext* SkFontHost::CreateFallbackScalerContext(
                                                const SkScalerContext::Rec& rec)
{
    const FontFaceRec* face = get_default_face();

    SkAutoDescriptor    ad(sizeof(rec) + sizeof(face) +
                           SkDescriptor::ComputeOverhead(2));
    SkDescriptor*       desc = ad.getDesc();
    SkScalerContext::Rec* newRec;

    desc->init();
    newRec = reinterpret_cast<SkScalerContext::Rec*>(
        desc->addEntry(kRec_SkDescriptorTag, sizeof(rec), &rec));
    newRec->fFontID = get_id(*face);
    desc->computeChecksum();

    return SkFontHost::CreateScalerContext(desc);
}

size_t SkFontHost::ShouldPurgeFontCache(size_t sizeAllocatedSoFar)
{
    return 0;   
}

