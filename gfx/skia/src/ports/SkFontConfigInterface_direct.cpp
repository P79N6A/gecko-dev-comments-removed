








#include <string>
#include <unistd.h>
#include <fcntl.h>

#include <fontconfig/fontconfig.h>

#include "SkFontConfigInterface.h"
#include "SkStream.h"

class SkFontConfigInterfaceDirect : public SkFontConfigInterface {
public:
            SkFontConfigInterfaceDirect();
    virtual ~SkFontConfigInterfaceDirect();

    virtual bool matchFamilyName(const char familyName[],
                                 SkTypeface::Style requested,
                                 FontIdentity* outFontIdentifier,
                                 SkString* outFamilyName,
                                 SkTypeface::Style* outStyle) SK_OVERRIDE;
    virtual SkStream* openStream(const FontIdentity&) SK_OVERRIDE;

private:
    SkMutex mutex_;
};

SkFontConfigInterface* SkFontConfigInterface::GetSingletonDirectInterface() {
    static SkFontConfigInterface* gDirect;
    if (NULL == gDirect) {
        gDirect = new SkFontConfigInterfaceDirect;
    }
    return gDirect;
}

namespace {




enum FontEquivClass
{
    OTHER,
    SANS,
    SERIF,
    MONO,
    SYMBOL,
    PGOTHIC,
    GOTHIC,
    PMINCHO,
    MINCHO,
    SIMSUN,
    NSIMSUN,
    SIMHEI,
    PMINGLIU,
    MINGLIU,
    PMINGLIUHK,
    MINGLIUHK,
    CAMBRIA,
};



FontEquivClass GetFontEquivClass(const char* fontname)
{
    
    
    
    
    
    
    
    
    

    
    
    
    
    


    struct FontEquivMap {
        FontEquivClass clazz;
        const char name[40];
    };

    static const FontEquivMap kFontEquivMap[] = {
        { SANS, "Arial" },
        { SANS, "Arimo" },
        { SANS, "Liberation Sans" },

        { SERIF, "Times New Roman" },
        { SERIF, "Tinos" },
        { SERIF, "Liberation Serif" },

        { MONO, "Courier New" },
        { MONO, "Cousine" },
        { MONO, "Liberation Mono" },

        { SYMBOL, "Symbol" },
        { SYMBOL, "Symbol Neu" },

        
        { PGOTHIC, "MS PGothic" },
        { PGOTHIC, "\xef\xbc\xad\xef\xbc\xb3 \xef\xbc\xb0"
                   "\xe3\x82\xb4\xe3\x82\xb7\xe3\x83\x83\xe3\x82\xaf" },
        { PGOTHIC, "IPAPGothic" },
        { PGOTHIC, "MotoyaG04Gothic" },

        
        { GOTHIC, "MS Gothic" },
        { GOTHIC, "\xef\xbc\xad\xef\xbc\xb3 "
                  "\xe3\x82\xb4\xe3\x82\xb7\xe3\x83\x83\xe3\x82\xaf" },
        { GOTHIC, "IPAGothic" },
        { GOTHIC, "MotoyaG04GothicMono" },

        
        { PMINCHO, "MS PMincho" },
        { PMINCHO, "\xef\xbc\xad\xef\xbc\xb3 \xef\xbc\xb0"
                   "\xe6\x98\x8e\xe6\x9c\x9d"},
        { PMINCHO, "IPAPMincho" },
        { PMINCHO, "MotoyaG04Mincho" },

        
        { MINCHO, "MS Mincho" },
        { MINCHO, "\xef\xbc\xad\xef\xbc\xb3 \xe6\x98\x8e\xe6\x9c\x9d" },
        { MINCHO, "IPAMincho" },
        { MINCHO, "MotoyaG04MinchoMono" },

        
        { SIMSUN, "Simsun" },
        { SIMSUN, "\xe5\xae\x8b\xe4\xbd\x93" },
        { SIMSUN, "MSung GB18030" },
        { SIMSUN, "Song ASC" },

        
        { NSIMSUN, "NSimsun" },
        { NSIMSUN, "\xe6\x96\xb0\xe5\xae\x8b\xe4\xbd\x93" },
        { NSIMSUN, "MSung GB18030" },
        { NSIMSUN, "N Song ASC" },

        
        { SIMHEI, "Simhei" },
        { SIMHEI, "\xe9\xbb\x91\xe4\xbd\x93" },
        { SIMHEI, "MYingHeiGB18030" },
        { SIMHEI, "MYingHeiB5HK" },

        
        { PMINGLIU, "PMingLiU"},
        { PMINGLIU, "\xe6\x96\xb0\xe7\xb4\xb0\xe6\x98\x8e\xe9\xab\x94" },
        { PMINGLIU, "MSung B5HK"},

        
        { MINGLIU, "MingLiU"},
        { MINGLIU, "\xe7\xb4\xb0\xe6\x98\x8e\xe9\xab\x94" },
        { MINGLIU, "MSung B5HK"},

        
        { PMINGLIUHK, "PMingLiU_HKSCS"},
        { PMINGLIUHK, "\xe6\x96\xb0\xe7\xb4\xb0\xe6\x98\x8e\xe9\xab\x94_HKSCS" },
        { PMINGLIUHK, "MSung B5HK"},

        
        { MINGLIUHK, "MingLiU_HKSCS"},
        { MINGLIUHK, "\xe7\xb4\xb0\xe6\x98\x8e\xe9\xab\x94_HKSCS" },
        { MINGLIUHK, "MSung B5HK"},

        
        { CAMBRIA, "Cambria" },
        { CAMBRIA, "Caladea" },
    };

    static const size_t kFontCount =
        sizeof(kFontEquivMap)/sizeof(kFontEquivMap[0]);

    
    
    for (size_t i = 0; i < kFontCount; ++i) {
        if (strcasecmp(kFontEquivMap[i].name, fontname) == 0)
            return kFontEquivMap[i].clazz;
    }
    return OTHER;
}




bool IsMetricCompatibleReplacement(const char* font_a, const char* font_b)
{
    FontEquivClass class_a = GetFontEquivClass(font_a);
    FontEquivClass class_b = GetFontEquivClass(font_b);

    return class_a != OTHER && class_a == class_b;
}





bool IsFallbackFontAllowed(const std::string& family) {
  const char* family_cstr = family.c_str();
  return family.empty() ||
         strcasecmp(family_cstr, "sans") == 0 ||
         strcasecmp(family_cstr, "serif") == 0 ||
         strcasecmp(family_cstr, "monospace") == 0;
}


FcPattern* MatchFont(FcFontSet* font_set,
                     FcChar8* post_config_family,
                     const std::string& family) {
  
  
  FcPattern* match = NULL;
  for (int i = 0; i < font_set->nfont; ++i) {
    FcPattern* current = font_set->fonts[i];
    FcBool is_scalable;

    if (FcPatternGetBool(current, FC_SCALABLE, 0,
                         &is_scalable) != FcResultMatch ||
        !is_scalable) {
      continue;
    }

    
    FcChar8* c_filename;
    if (FcPatternGetString(current, FC_FILE, 0, &c_filename) != FcResultMatch)
      continue;

    if (access(reinterpret_cast<char*>(c_filename), R_OK) != 0)
      continue;

    match = current;
    break;
  }

  if (match && !IsFallbackFontAllowed(family)) {
    bool acceptable_substitute = false;
    for (int id = 0; id < 255; ++id) {
      FcChar8* post_match_family;
      if (FcPatternGetString(match, FC_FAMILY, id, &post_match_family) !=
          FcResultMatch)
        break;
      acceptable_substitute =
          (strcasecmp(reinterpret_cast<char*>(post_config_family),
                      reinterpret_cast<char*>(post_match_family)) == 0 ||
           
           
           
           
           
           strcasecmp(family.c_str(),
                      reinterpret_cast<char*>(post_match_family)) == 0) ||
           IsMetricCompatibleReplacement(family.c_str(),
               reinterpret_cast<char*>(post_match_family));
      if (acceptable_substitute)
        break;
    }
    if (!acceptable_substitute)
      return NULL;
  }

  return match;
}


SkTypeface::Style GetFontStyle(FcPattern* font) {
    int resulting_bold;
    if (FcPatternGetInteger(font, FC_WEIGHT, 0, &resulting_bold))
        resulting_bold = FC_WEIGHT_NORMAL;

    int resulting_italic;
    if (FcPatternGetInteger(font, FC_SLANT, 0, &resulting_italic))
        resulting_italic = FC_SLANT_ROMAN;

    
    
    
    
    FcValue matrix;
    const bool have_matrix = FcPatternGet(font, FC_MATRIX, 0, &matrix) == 0;

    
    
    FcValue embolden;
    const bool have_embolden = FcPatternGet(font, FC_EMBOLDEN, 0, &embolden) == 0;

    int styleBits = 0;
    if (resulting_bold > FC_WEIGHT_MEDIUM && !have_embolden) {
        styleBits |= SkTypeface::kBold;
    }
    if (resulting_italic > FC_SLANT_ROMAN && !have_matrix) {
        styleBits |= SkTypeface::kItalic;
    }

    return (SkTypeface::Style)styleBits;
}

}  



#define kMaxFontFamilyLength    2048

SkFontConfigInterfaceDirect::SkFontConfigInterfaceDirect() {
    FcInit();
}

SkFontConfigInterfaceDirect::~SkFontConfigInterfaceDirect() {
}

bool SkFontConfigInterfaceDirect::matchFamilyName(const char familyName[],
                                                  SkTypeface::Style style,
                                                  FontIdentity* outIdentity,
                                                  SkString* outFamilyName,
                                                  SkTypeface::Style* outStyle) {
    std::string familyStr(familyName ? familyName : "");
    if (familyStr.length() > kMaxFontFamilyLength) {
        return false;
    }

    SkAutoMutexAcquire ac(mutex_);

    FcPattern* pattern = FcPatternCreate();

    if (familyName) {
        FcPatternAddString(pattern, FC_FAMILY, (FcChar8*)familyName);
    }
    FcPatternAddInteger(pattern, FC_WEIGHT,
                        (style & SkTypeface::kBold) ? FC_WEIGHT_BOLD
                                                    : FC_WEIGHT_NORMAL);
    FcPatternAddInteger(pattern, FC_SLANT,
                        (style & SkTypeface::kItalic) ? FC_SLANT_ITALIC
                                                      : FC_SLANT_ROMAN);
    FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);

    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    FcChar8* post_config_family;
    FcPatternGetString(pattern, FC_FAMILY, 0, &post_config_family);

    FcResult result;
    FcFontSet* font_set = FcFontSort(0, pattern, 0, 0, &result);
    if (!font_set) {
        FcPatternDestroy(pattern);
        return false;
    }

    FcPattern* match = MatchFont(font_set, post_config_family, familyStr);
    if (!match) {
        FcPatternDestroy(pattern);
        FcFontSetDestroy(font_set);
        return false;
    }

    FcPatternDestroy(pattern);

    

    if (FcPatternGetString(match, FC_FAMILY, 0, &post_config_family) != FcResultMatch) {
        FcFontSetDestroy(font_set);
        return false;
    }

    FcChar8* c_filename;
    if (FcPatternGetString(match, FC_FILE, 0, &c_filename) != FcResultMatch) {
        FcFontSetDestroy(font_set);
        return false;
    }

    int face_index;
    if (FcPatternGetInteger(match, FC_INDEX, 0, &face_index) != FcResultMatch) {
        FcFontSetDestroy(font_set);
        return false;
    }

    FcFontSetDestroy(font_set);

    if (outIdentity) {
        outIdentity->fTTCIndex = face_index;
        outIdentity->fString.set((const char*)c_filename);
    }
    if (outFamilyName) {
        outFamilyName->set((const char*)post_config_family);
    }
    if (outStyle) {
        *outStyle = GetFontStyle(match);
    }
    return true;
}

SkStream* SkFontConfigInterfaceDirect::openStream(const FontIdentity& identity) {
    return SkStream::NewFromFile(identity.fString.c_str());
}
