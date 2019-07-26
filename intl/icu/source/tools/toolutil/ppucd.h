













#ifndef __PPUCD_H__
#define __PPUCD_H__

#include "unicode/utypes.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"

#include <stdio.h>


enum {
    
    PPUCD_NAME_ALIAS=UCHAR_STRING_LIMIT,
    PPUCD_CONDITIONAL_CASE_MAPPINGS,
    PPUCD_TURKIC_CASE_FOLDING
};

U_NAMESPACE_BEGIN

class U_TOOLUTIL_API PropertyNames {
public:
    virtual ~PropertyNames();
    virtual int32_t getPropertyEnum(const char *name) const;
    virtual int32_t getPropertyValueEnum(int32_t property, const char *name) const;
};

struct U_TOOLUTIL_API UniProps {
    UniProps();
    ~UniProps();

    int32_t getIntProp(int32_t prop) const { return intProps[prop-UCHAR_INT_START]; }

    UChar32 start, end;
    UBool binProps[UCHAR_BINARY_LIMIT];
    int32_t intProps[UCHAR_INT_LIMIT-UCHAR_INT_START];
    UVersionInfo age;
    UChar32 bmg;
    UChar32 scf, slc, stc, suc;
    int32_t digitValue;
    const char *numericValue;
    const char *name;
    const char *nameAlias;
    UnicodeString cf, lc, tc, uc;
    UnicodeSet scx;
};

class U_TOOLUTIL_API PreparsedUCD {
public:
    enum LineType {
        
        NO_LINE,
        
        EMPTY_LINE,

        
        UNICODE_VERSION_LINE,

        
        PROPERTY_LINE,
        
        BINARY_LINE,
        
        VALUE_LINE,

        
        DEFAULTS_LINE,
        
        BLOCK_LINE,
        
        CP_LINE,

        
        ALG_NAMES_RANGE_LINE,

        LINE_TYPE_COUNT
    };

    



    PreparsedUCD(const char *filename, UErrorCode &errorCode);

    
    ~PreparsedUCD();

    
    void setPropertyNames(const PropertyNames *pn) { pnames=pn; }

    



    LineType readLine(UErrorCode &errorCode);

    
    int32_t getLineNumber() const { return lineNumber; }

    
    const char *nextField();

    
    const UVersionInfo &getUnicodeVersion() const { return ucdVersion; }

    
    UBool lineHasPropertyValues() const { return DEFAULTS_LINE<=lineType && lineType<=CP_LINE; }

    






    const UniProps *getProps(UnicodeSet &newValues, UErrorCode &errorCode);

    





    UBool getRangeForAlgNames(UChar32 &start, UChar32 &end, UErrorCode &errorCode);

private:
    UBool isLineBufferAvailable(int32_t i) {
        return defaultLineIndex!=i && blockLineIndex!=i;
    }

    
    const char *firstField();

    UBool parseProperty(UniProps &props, const char *field, UnicodeSet &newValues,
                        UErrorCode &errorCode);
    UChar32 parseCodePoint(const char *s, UErrorCode &errorCode);
    UBool parseCodePointRange(const char *s, UChar32 &start, UChar32 &end, UErrorCode &errorCode);
    void parseString(const char *s, UnicodeString &uni, UErrorCode &errorCode);
    void parseScriptExtensions(const char *s, UnicodeSet &scx, UErrorCode &errorCode);

    static const int32_t kNumLineBuffers=3;

    PropertyNames *icuPnames;  
    const PropertyNames *pnames;  
    FILE *file;
    int32_t defaultLineIndex, blockLineIndex, lineIndex;
    int32_t lineNumber;
    LineType lineType;
    char *fieldLimit;
    char *lineLimit;

    UVersionInfo ucdVersion;
    UniProps defaultProps, blockProps, cpProps;
    
    
    char lines[kNumLineBuffers][4096];
};

U_NAMESPACE_END

#endif
