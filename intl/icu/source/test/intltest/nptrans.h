















#ifndef NPTRANS_H
#define NPTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA
#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uniset.h"
#include "unicode/ures.h"
#include "unicode/translit.h"

#include "intltest.h"


#define ASCII_SPACE 0x0020

class NamePrepTransform {

private :
    Transliterator *mapping;
    UnicodeSet unassigned;
    UnicodeSet prohibited;
    UnicodeSet labelSeparatorSet;
    UResourceBundle *bundle;
    NamePrepTransform(UParseError& parseError, UErrorCode& status);


public :

    static NamePrepTransform* createInstance(UParseError& parseError, UErrorCode& status);

    virtual ~NamePrepTransform();


    inline UBool isProhibited(UChar32 ch);

    


    inline UClassID getDynamicClassID() const { return getStaticClassID(); }

    


    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    

















    int32_t map(const UChar* src, int32_t srcLength, 
                        UChar* dest, int32_t destCapacity, 
                        UBool allowUnassigned,
                        UParseError* parseError,
                        UErrorCode& status );

    














    int32_t process(const UChar* src, int32_t srcLength, 
                            UChar* dest, int32_t destCapacity, 
                            UBool allowUnassigned,
                            UParseError* parseError,
                            UErrorCode& status );

    




    UBool isLabelSeparator(UChar32 ch, UErrorCode& status);

    inline UBool isLDHChar(UChar32 ch);

private:
    



    static const char fgClassID;
};

inline UBool NamePrepTransform::isLDHChar(UChar32 ch){
    
    if(ch>0x007A){
        return FALSE;
    }
    
    if( (ch==0x002D) || 
        (0x0030 <= ch && ch <= 0x0039) ||
        (0x0041 <= ch && ch <= 0x005A) ||
        (0x0061 <= ch && ch <= 0x007A)
      ){
        return TRUE;
    }
    return FALSE;
}

#endif 
#else
class NamePrepTransform {
};
#endif 

#endif









