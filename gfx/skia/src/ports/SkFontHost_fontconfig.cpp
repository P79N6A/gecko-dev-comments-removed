

















#include <map>
#include <string>

#include <fontconfig/fontconfig.h>

#include "SkFontHost.h"
#include "SkStream.h"


SkTypeface::Style find_name_and_style(SkStream* stream, SkString* name);












static SkMutex global_fc_map_lock;
static std::map<std::string, unsigned> global_fc_map;
static std::map<unsigned, std::string> global_fc_map_inverted;
static std::map<uint32_t, SkTypeface *> global_fc_typefaces;
static unsigned global_fc_map_next_id = 0;


static const unsigned kFontCacheMemoryBudget = 2 * 1024 * 1024;  

static unsigned UniqueIdToFileId(unsigned uniqueid)
{
    return uniqueid >> 8;
}

static SkTypeface::Style UniqueIdToStyle(unsigned uniqueid)
{
    return static_cast<SkTypeface::Style>(uniqueid & 0xff);
}

static unsigned FileIdAndStyleToUniqueId(unsigned fileid,
                                         SkTypeface::Style style)
{
    SkASSERT((style & 0xff) == style);
    return (fileid << 8) | static_cast<int>(style);
}







static bool IsFallbackFontAllowed(const char* request)
{
    return strcmp(request, "Sans") == 0 ||
           strcmp(request, "Serif") == 0 ||
           strcmp(request, "Monospace") == 0;
}

class FontConfigTypeface : public SkTypeface {
public:
    FontConfigTypeface(Style style, uint32_t id)
        : SkTypeface(style, id)
    { }
};











static FcPattern* FontMatch(const char* type, FcType vtype, const void* value,
                            ...)
{
    va_list ap;
    va_start(ap, value);

    FcPattern* pattern = FcPatternCreate();
    const char* family_requested = NULL;

    for (;;) {
        FcValue fcvalue;
        fcvalue.type = vtype;
        switch (vtype) {
            case FcTypeString:
                fcvalue.u.s = (FcChar8*) value;
                break;
            case FcTypeInteger:
                fcvalue.u.i = (int)(intptr_t)value;
                break;
            default:
                SkASSERT(!"FontMatch unhandled type");
        }
        FcPatternAdd(pattern, type, fcvalue, 0);

        if (vtype == FcTypeString && strcmp(type, FC_FAMILY) == 0)
            family_requested = (const char*) value;

        type = va_arg(ap, const char *);
        if (!type)
            break;
        
        vtype = static_cast<FcType>(va_arg(ap, int));
        value = va_arg(ap, const void *);
    };
    va_end(ap);

    FcConfigSubstitute(0, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    FcChar8* post_config_family;
    FcPatternGetString(pattern, FC_FAMILY, 0, &post_config_family);

    FcResult result;
    FcPattern* match = FcFontMatch(0, pattern, &result);
    if (!match) {
        FcPatternDestroy(pattern);
        return NULL;
    }

    FcChar8* post_match_family;
    FcPatternGetString(match, FC_FAMILY, 0, &post_match_family);
    const bool family_names_match =
        !family_requested ?
        true :
        strcasecmp((char *)post_config_family, (char *)post_match_family) == 0;

    FcPatternDestroy(pattern);

    if (!family_names_match && !IsFallbackFontAllowed(family_requested)) {
        FcPatternDestroy(match);
        return NULL;
    }

    return match;
}





static unsigned FileIdFromFilename(const char* filename)
{
    SkAutoMutexAcquire ac(global_fc_map_lock);

    std::map<std::string, unsigned>::const_iterator i =
        global_fc_map.find(filename);
    if (i == global_fc_map.end()) {
        const unsigned fileid = global_fc_map_next_id++;
        global_fc_map[filename] = fileid;
        global_fc_map_inverted[fileid] = filename;
        return fileid;
    } else {
        return i->second;
    }
}


SkTypeface* SkFontHost::CreateTypeface(const SkTypeface* familyFace,
                                       const char familyName[],
                                       const void* data, size_t bytelength,
                                       SkTypeface::Style style)
{
    const char* resolved_family_name = NULL;
    FcPattern* face_match = NULL;

    {
        SkAutoMutexAcquire ac(global_fc_map_lock);
        FcInit();
    }

    if (familyFace) {
        
        
        
        SkAutoMutexAcquire ac(global_fc_map_lock);

        const unsigned fileid = UniqueIdToFileId(familyFace->uniqueID());
        std::map<unsigned, std::string>::const_iterator i =
            global_fc_map_inverted.find(fileid);
        if (i == global_fc_map_inverted.end())
            return NULL;

        FcInit();
        face_match = FontMatch(FC_FILE, FcTypeString, i->second.c_str(),
                               NULL);

        if (!face_match)
            return NULL;
        FcChar8* family;
        if (FcPatternGetString(face_match, FC_FAMILY, 0, &family)) {
            FcPatternDestroy(face_match);
            return NULL;
        }
        
        

        resolved_family_name = reinterpret_cast<char*>(family);
    } else if (familyName) {
        resolved_family_name = familyName;
    } else {
        return NULL;
    }

    
    SkASSERT(resolved_family_name);

    const int bold = style & SkTypeface::kBold ?
                     FC_WEIGHT_BOLD : FC_WEIGHT_NORMAL;
    const int italic = style & SkTypeface::kItalic ?
                       FC_SLANT_ITALIC : FC_SLANT_ROMAN;
    FcPattern* match = FontMatch(FC_FAMILY, FcTypeString, resolved_family_name,
                                 FC_WEIGHT, FcTypeInteger, bold,
                                 FC_SLANT, FcTypeInteger, italic,
                                 NULL);
    if (face_match)
        FcPatternDestroy(face_match);

    if (!match)
        return NULL;

    FcChar8* filename;
    if (FcPatternGetString(match, FC_FILE, 0, &filename) != FcResultMatch) {
        FcPatternDestroy(match);
        return NULL;
    }
    

    const unsigned fileid = FileIdFromFilename(reinterpret_cast<char*>(filename));
    const unsigned id = FileIdAndStyleToUniqueId(fileid, style);
    SkTypeface* typeface = SkNEW_ARGS(FontConfigTypeface, (style, id));
    FcPatternDestroy(match);

    {
        SkAutoMutexAcquire ac(global_fc_map_lock);
        global_fc_typefaces[id] = typeface;
    }

    return typeface;
}


SkTypeface* SkFontHost::CreateTypefaceFromStream(SkStream* stream)
{
    SkASSERT(!"SkFontHost::CreateTypefaceFromStream unimplemented");
    return NULL;
}


SkTypeface* SkFontHost::CreateTypefaceFromFile(const char path[])
{
    SkASSERT(!"SkFontHost::CreateTypefaceFromFile unimplemented");
    return NULL;
}


bool SkFontHost::ValidFontID(SkFontID uniqueID) {
    SkAutoMutexAcquire ac(global_fc_map_lock);
    return global_fc_typefaces.find(uniqueID) != global_fc_typefaces.end();
}


SkStream* SkFontHost::OpenStream(uint32_t id)
{
    SkAutoMutexAcquire ac(global_fc_map_lock);
    const unsigned fileid = UniqueIdToFileId(id);

    std::map<unsigned, std::string>::const_iterator i =
        global_fc_map_inverted.find(fileid);
    if (i == global_fc_map_inverted.end())
        return NULL;

    return SkNEW_ARGS(SkFILEStream, (i->second.c_str()));
}

size_t SkFontHost::GetFileName(SkFontID fontID, char path[], size_t length,
                               int32_t* index) {
    SkAutoMutexAcquire ac(global_fc_map_lock);
    const unsigned fileid = UniqueIdToFileId(fontID);

    std::map<unsigned, std::string>::const_iterator i =
    global_fc_map_inverted.find(fileid);
    if (i == global_fc_map_inverted.end()) {
        return 0;
    }

    const std::string& str = i->second;
    if (path) {
        memcpy(path, str.c_str(), SkMin32(str.size(), length));
    }
    if (index) {    
        *index = 0;
    }
    return str.size();
}

void SkFontHost::Serialize(const SkTypeface*, SkWStream*) {
    SkASSERT(!"SkFontHost::Serialize unimplemented");
}

SkTypeface* SkFontHost::Deserialize(SkStream* stream) {
    SkASSERT(!"SkFontHost::Deserialize unimplemented");
    return NULL;
}

SkFontID SkFontHost::NextLogicalFont(SkFontID currFontID, SkFontID origFontID) {
    
    return 0;
}



size_t SkFontHost::ShouldPurgeFontCache(size_t sizeAllocatedSoFar)
{
    if (sizeAllocatedSoFar > kFontCacheMemoryBudget)
        return sizeAllocatedSoFar - kFontCacheMemoryBudget;
    else
        return 0;   
}
