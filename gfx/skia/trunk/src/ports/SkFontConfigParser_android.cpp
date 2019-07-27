






#include "SkFontConfigParser_android.h"
#include "SkTDArray.h"
#include "SkTSearch.h"
#include "SkTypeface.h"

#include <expat.h>
#include <stdio.h>
#include <sys/system_properties.h>

#include <limits>

#define SYSTEM_FONTS_FILE "/system/etc/system_fonts.xml"
#define FALLBACK_FONTS_FILE "/system/etc/fallback_fonts.xml"
#define VENDOR_FONTS_FILE "/vendor/etc/fallback_fonts.xml"




#define NO_TAG 0
#define NAMESET_TAG 1
#define FILESET_TAG 2





struct FamilyData {
    FamilyData(XML_Parser *parserRef, SkTDArray<FontFamily*> &familiesRef) :
        parser(parserRef),
        families(familiesRef),
        currentFamily(NULL),
        currentFontInfo(NULL),
        currentTag(NO_TAG) {};

    XML_Parser *parser;                
    SkTDArray<FontFamily*> &families;  
    FontFamily *currentFamily;         
    FontFileInfo *currentFontInfo;     
    int currentTag;                    
};





static void textHandler(void *data, const char *s, int len) {
    FamilyData *familyData = (FamilyData*) data;
    
    if (familyData->currentFamily &&
            (familyData->currentTag == NAMESET_TAG || familyData->currentTag == FILESET_TAG)) {
        switch (familyData->currentTag) {
        case NAMESET_TAG: {
            SkAutoAsciiToLC tolc(s, len);
            familyData->currentFamily->fNames.push_back().set(tolc.lc(), len);
            break;
        }
        case FILESET_TAG:
            if (familyData->currentFontInfo) {
                familyData->currentFontInfo->fFileName.set(s, len);
            }
            break;
        default:
            
            break;
        }
    }
}


template <typename T> static bool parseNonNegativeInteger(const char* s, T* value) {
    SK_COMPILE_ASSERT(std::numeric_limits<T>::is_integer, T_must_be_integer);
    const T nMax = std::numeric_limits<T>::max() / 10;
    const T dMax = std::numeric_limits<T>::max() - (nMax * 10);
    T n = 0;
    for (; *s; ++s) {
        
        if (*s < '0' || '9' < *s) {
            return false;
        }
        int d = *s - '0';
        
        if (n > nMax || (n == nMax && d > dMax)) {
            return false;
        }
        n = (n * 10) + d;
    }
    *value = n;
    return true;
}





static void fontFileElementHandler(FamilyData *familyData, const char **attributes) {

    FontFileInfo& newFileInfo = familyData->currentFamily->fFontFiles.push_back();
    if (attributes) {
        int currentAttributeIndex = 0;
        while (attributes[currentAttributeIndex]) {
            const char* attributeName = attributes[currentAttributeIndex];
            const char* attributeValue = attributes[currentAttributeIndex+1];
            int nameLength = strlen(attributeName);
            int valueLength = strlen(attributeValue);
            if (strncmp(attributeName, "variant", nameLength) == 0) {
                if (strncmp(attributeValue, "elegant", valueLength) == 0) {
                    newFileInfo.fPaintOptions.setFontVariant(SkPaintOptionsAndroid::kElegant_Variant);
                } else if (strncmp(attributeValue, "compact", valueLength) == 0) {
                    newFileInfo.fPaintOptions.setFontVariant(SkPaintOptionsAndroid::kCompact_Variant);
                }
            } else if (strncmp(attributeName, "lang", nameLength) == 0) {
                newFileInfo.fPaintOptions.setLanguage(attributeValue);
            } else if (strncmp(attributeName, "index", nameLength) == 0) {
                int value;
                if (parseNonNegativeInteger(attributeValue, &value)) {
                    newFileInfo.fIndex = value;
                } else {
                    SkDebugf("---- SystemFonts index=%s (INVALID)", attributeValue);
                }
            }
            
            currentAttributeIndex += 2;
        }
    }
    familyData->currentFontInfo = &newFileInfo;
    XML_SetCharacterDataHandler(*familyData->parser, textHandler);
}





static void startElementHandler(void *data, const char *tag, const char **atts) {
    FamilyData *familyData = (FamilyData*) data;
    int len = strlen(tag);
    if (strncmp(tag, "family", len)== 0) {
        familyData->currentFamily = new FontFamily();
        familyData->currentFamily->order = -1;
        
        
        for (int i = 0; atts[i] != NULL; i += 2) {
            const char* valueString = atts[i+1];
            int value;
            int len = sscanf(valueString, "%d", &value);
            if (len > 0) {
                familyData->currentFamily->order = value;
            }
        }
    } else if (len == 7 && strncmp(tag, "nameset", len) == 0) {
        familyData->currentTag = NAMESET_TAG;
    } else if (len == 7 && strncmp(tag, "fileset", len) == 0) {
        familyData->currentTag = FILESET_TAG;
    } else if (strncmp(tag, "name", len) == 0 && familyData->currentTag == NAMESET_TAG) {
        
        XML_SetCharacterDataHandler(*familyData->parser, textHandler);
    } else if (strncmp(tag, "file", len) == 0 && familyData->currentTag == FILESET_TAG) {
        
        fontFileElementHandler(familyData, atts);
    }
}





static void endElementHandler(void *data, const char *tag) {
    FamilyData *familyData = (FamilyData*) data;
    int len = strlen(tag);
    if (strncmp(tag, "family", len)== 0) {
        
        *familyData->families.append() = familyData->currentFamily;
        familyData->currentFamily = NULL;
    } else if (len == 7 && strncmp(tag, "nameset", len) == 0) {
        familyData->currentTag = NO_TAG;
    } else if (len == 7 && strncmp(tag, "fileset", len) == 0) {
        familyData->currentTag = NO_TAG;
    } else if ((strncmp(tag, "name", len) == 0 && familyData->currentTag == NAMESET_TAG) ||
            (strncmp(tag, "file", len) == 0 && familyData->currentTag == FILESET_TAG)) {
        
        XML_SetCharacterDataHandler(*familyData->parser, NULL);
    }
}





static void parseConfigFile(const char *filename, SkTDArray<FontFamily*> &families) {

    FILE* file = NULL;

#if !defined(SK_BUILD_FOR_ANDROID_FRAMEWORK)
    
    
    char sdkVersion[PROP_VALUE_MAX];
    __system_property_get("ro.build.version.sdk", sdkVersion);
    const int sdkVersionInt = atoi(sdkVersion);

    if (0 != *sdkVersion && sdkVersionInt < 17) {
        SkString basename;
        SkString updatedFilename;
        SkString locale = SkFontConfigParser::GetLocale();

        basename.set(filename);
        
        if (basename.endsWith(".xml")) {
            basename.resize(basename.size()-4);
        }
        
        updatedFilename.printf("%s-%s.xml", basename.c_str(), locale.c_str());
        file = fopen(updatedFilename.c_str(), "r");
        if (!file) {
            
            updatedFilename.printf("%s-%.2s.xml", basename.c_str(), locale.c_str());
            file = fopen(updatedFilename.c_str(), "r");
        }
    }
#endif

    if (NULL == file) {
        file = fopen(filename, "r");
    }

    
    
    if (NULL == file) {
        return;
    }

    XML_Parser parser = XML_ParserCreate(NULL);
    FamilyData *familyData = new FamilyData(&parser, families);
    XML_SetUserData(parser, familyData);
    XML_SetElementHandler(parser, startElementHandler, endElementHandler);

    char buffer[512];
    bool done = false;
    while (!done) {
        fgets(buffer, sizeof(buffer), file);
        int len = strlen(buffer);
        if (feof(file) != 0) {
            done = true;
        }
        XML_Parse(parser, buffer, len, done);
    }
    XML_ParserFree(parser);
    fclose(file);
}

static void getSystemFontFamilies(SkTDArray<FontFamily*> &fontFamilies) {
    parseConfigFile(SYSTEM_FONTS_FILE, fontFamilies);
}

static void getFallbackFontFamilies(SkTDArray<FontFamily*> &fallbackFonts) {
    SkTDArray<FontFamily*> vendorFonts;
    parseConfigFile(FALLBACK_FONTS_FILE, fallbackFonts);
    parseConfigFile(VENDOR_FONTS_FILE, vendorFonts);

    
    
    int currentOrder = -1;
    for (int i = 0; i < vendorFonts.count(); ++i) {
        FontFamily* family = vendorFonts[i];
        int order = family->order;
        if (order < 0) {
            if (currentOrder < 0) {
                
                *fallbackFonts.append() = family;
            } else {
                
                
                *fallbackFonts.insert(currentOrder++) = family;
            }
        } else {
            
            
            *fallbackFonts.insert(order) = family;
            currentOrder = order + 1;
        }
    }
}





void SkFontConfigParser::GetFontFamilies(SkTDArray<FontFamily*> &fontFamilies) {

    getSystemFontFamilies(fontFamilies);

    
    SkTDArray<FontFamily*> fallbackFonts;
    getFallbackFontFamilies(fallbackFonts);
    for (int i = 0; i < fallbackFonts.count(); ++i) {
        fallbackFonts[i]->fIsFallbackFont = true;
        *fontFamilies.append() = fallbackFonts[i];
    }
}

void SkFontConfigParser::GetTestFontFamilies(SkTDArray<FontFamily*> &fontFamilies,
                                             const char* testMainConfigFile,
                                             const char* testFallbackConfigFile) {
    parseConfigFile(testMainConfigFile, fontFamilies);

    SkTDArray<FontFamily*> fallbackFonts;
    parseConfigFile(testFallbackConfigFile, fallbackFonts);

    
    for (int i = 0; i < fallbackFonts.count(); ++i) {
        fallbackFonts[i]->fIsFallbackFont = true;
        *fontFamilies.append() = fallbackFonts[i];
    }
}




SkString SkFontConfigParser::GetLocale()
{
    char propLang[PROP_VALUE_MAX], propRegn[PROP_VALUE_MAX];
    __system_property_get("persist.sys.language", propLang);
    __system_property_get("persist.sys.country", propRegn);

    if (*propLang == 0 && *propRegn == 0) {
        
        __system_property_get("ro.product.locale.language", propLang);
        __system_property_get("ro.product.locale.region", propRegn);
        if (*propLang == 0 && *propRegn == 0) {
            strcpy(propLang, "en");
            strcpy(propRegn, "US");
        }
    }

    SkString locale(6);
    char* localeCStr = locale.writable_str();

    strncpy(localeCStr, propLang, 2);
    localeCStr[2] = '-';
    strncpy(&localeCStr[3], propRegn, 2);
    localeCStr[5] = '\0';

    return locale;
}
