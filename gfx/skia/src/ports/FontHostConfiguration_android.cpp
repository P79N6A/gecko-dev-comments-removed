
















#include "FontHostConfiguration_android.h"
#include "SkString.h"
#include "SkTDArray.h"
#include <expat.h>
#include <sys/system_properties.h>

#define SYSTEM_FONTS_FILE "/system/etc/system_fonts.xml"
#define FALLBACK_FONTS_FILE "/system/etc/fallback_fonts.xml"
#define VENDOR_FONTS_FILE "/vendor/etc/fallback_fonts.xml"





#define NO_TAG 0
#define NAMESET_TAG 1
#define FILESET_TAG 2





struct FamilyData {
    FamilyData(XML_Parser *parserRef, SkTDArray<FontFamily*> &familiesRef) :
            parser(parserRef), families(familiesRef), currentTag(NO_TAG) {};

    XML_Parser *parser;                
    SkTDArray<FontFamily*> &families;  
    FontFamily *currentFamily;         
    int currentTag;                    
};





void textHandler(void *data, const char *s, int len) {
    FamilyData *familyData = (FamilyData*) data;
    
    if (familyData->currentFamily &&
            (familyData->currentTag == NAMESET_TAG || familyData->currentTag == FILESET_TAG)) {
        
        char *buff;
        buff = (char*) malloc((len + 1) * sizeof(char));
        strncpy(buff, s, len);
        buff[len] = '\0';
        switch (familyData->currentTag) {
        case NAMESET_TAG:
            *(familyData->currentFamily->fNames.append()) = buff;
            break;
        case FILESET_TAG:
            *(familyData->currentFamily->fFileNames.append()) = buff;
            break;
        default:
            
            break;
        }
    }
}





void startElementHandler(void *data, const char *tag, const char **atts) {
    FamilyData *familyData = (FamilyData*) data;
    int len = strlen(tag);
    if (strncmp(tag, "family", len)== 0) {
        familyData->currentFamily = new FontFamily();
        familyData->currentFamily->order = -1;
        
        
        for (int i = 0; atts[i] != NULL; i += 2) {
            const char* attribute = atts[i];
            const char* valueString = atts[i+1];
            int value;
            int len = sscanf(valueString, "%d", &value);
            if (len > 0) {
                familyData->currentFamily->order = value;
            }
        }
    } else if (len == 7 && strncmp(tag, "nameset", len)== 0) {
        familyData->currentTag = NAMESET_TAG;
    } else if (len == 7 && strncmp(tag, "fileset", len) == 0) {
        familyData->currentTag = FILESET_TAG;
    } else if ((strncmp(tag, "name", len) == 0 && familyData->currentTag == NAMESET_TAG) ||
            (strncmp(tag, "file", len) == 0 && familyData->currentTag == FILESET_TAG)) {
        
        XML_SetCharacterDataHandler(*familyData->parser, textHandler);
    }
}





void endElementHandler(void *data, const char *tag) {
    FamilyData *familyData = (FamilyData*) data;
    int len = strlen(tag);
    if (strncmp(tag, "family", len)== 0) {
        
        *familyData->families.append() = familyData->currentFamily;
        familyData->currentFamily = NULL;
    } else if (len == 7 && strncmp(tag, "nameset", len)== 0) {
        familyData->currentTag = NO_TAG;
    } else if (len == 7 && strncmp(tag, "fileset", len)== 0) {
        familyData->currentTag = NO_TAG;
    } else if ((strncmp(tag, "name", len) == 0 && familyData->currentTag == NAMESET_TAG) ||
            (strncmp(tag, "file", len) == 0 && familyData->currentTag == FILESET_TAG)) {
        
        XML_SetCharacterDataHandler(*familyData->parser, NULL);
    }
}




void getLocale(AndroidLocale &locale)
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
    strncpy(locale.language, propLang, 2);
    locale.language[2] = '\0';
    strncpy(locale.region, propRegn, 2);
    locale.region[2] = '\0';
}








FILE* openLocalizedFile(const char* origname) {
    FILE* file = 0;
    SkString basename;
    SkString filename;
    AndroidLocale locale;

    basename.set(origname);
    
    if (basename.endsWith(".xml")) {
        basename.resize(basename.size()-4);
    }
    getLocale(locale);
    
    filename.printf("%s-%s-%s.xml", basename.c_str(), locale.language, locale.region);
    file = fopen(filename.c_str(), "r");
    if (!file) {
        
        filename.printf("%s-%s.xml", basename.c_str(), locale.language);
        file = fopen(filename.c_str(), "r");

        if (!file) {
            
            file = fopen(origname, "r");
        }
    }
    return file;
}





void parseConfigFile(const char *filename, SkTDArray<FontFamily*> &families) {
    XML_Parser parser = XML_ParserCreate(NULL);
    FamilyData *familyData = new FamilyData(&parser, families);
    XML_SetUserData(parser, familyData);
    XML_SetElementHandler(parser, startElementHandler, endElementHandler);
    FILE *file = openLocalizedFile(filename);
    
    
    if (file == NULL) {
        return;
    }
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
}

void getSystemFontFamilies(SkTDArray<FontFamily*> &fontFamilies) {
    parseConfigFile(SYSTEM_FONTS_FILE, fontFamilies);
}

void getFallbackFontFamilies(SkTDArray<FontFamily*> &fallbackFonts) {
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





void getFontFamilies(SkTDArray<FontFamily*> &fontFamilies) {
    SkTDArray<FontFamily*> fallbackFonts;

    getSystemFontFamilies(fontFamilies);
    getFallbackFontFamilies(fallbackFonts);

    
    for (int i = 0; i < fallbackFonts.count(); ++i) {
        *fontFamilies.append() = fallbackFonts[i];
    }
}
