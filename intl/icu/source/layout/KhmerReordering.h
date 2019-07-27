







#ifndef __KHMERREORDERING_H
#define __KHMERREORDERING_H






#include "LETypes.h"
#include "OpenTypeTables.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;







































struct KhmerClassTable    
{
    enum CharClassValues  
                          
    {
        CC_RESERVED             =  0,
        CC_CONSONANT            =  1, 
        CC_CONSONANT2           =  2, 
        CC_CONSONANT3           =  3, 
        CC_ZERO_WIDTH_NJ_MARK   =  4, 
        CC_CONSONANT_SHIFTER    =  5, 
        CC_ROBAT                =  6, 
        CC_COENG                =  7, 
        CC_DEPENDENT_VOWEL      =  8, 
        CC_SIGN_ABOVE           =  9,
        CC_SIGN_AFTER           = 10,
        CC_ZERO_WIDTH_J_MARK    = 11, 
        CC_COUNT                = 12  
    };

    enum CharClassFlags
    {
        CF_CLASS_MASK    = 0x0000FFFF,

        CF_CONSONANT     = 0x01000000,  
        CF_SPLIT_VOWEL   = 0x02000000,  
        CF_DOTTED_CIRCLE = 0x04000000,  
        CF_COENG         = 0x08000000,  
        CF_SHIFTER       = 0x10000000,  
        CF_ABOVE_VOWEL   = 0x20000000,  

        
        CF_POS_BEFORE    = 0x00080000,
        CF_POS_BELOW     = 0x00040000,
        CF_POS_ABOVE     = 0x00020000,
        CF_POS_AFTER     = 0x00010000,
        CF_POS_MASK      = 0x000f0000
    };

    typedef le_uint32 CharClass;

    typedef le_int32 ScriptFlags;

    LEUnicode firstChar;   
    LEUnicode lastChar;    
    const CharClass *classTable;

    CharClass getCharClass(LEUnicode ch) const;

    static const KhmerClassTable *getKhmerClassTable();
};


class KhmerReordering  {
public:
    static le_int32 reorder(const LEUnicode *theChars, le_int32 charCount, le_int32 scriptCode,
        LEUnicode *outChars, LEGlyphStorage &glyphStorage);

    static const FeatureMap *getFeatureMap(le_int32 &count);

private:
    
    KhmerReordering();

    static le_int32 findSyllable(const KhmerClassTable *classTable, const LEUnicode *chars, le_int32 prev, le_int32 charCount);

};


U_NAMESPACE_END
#endif
